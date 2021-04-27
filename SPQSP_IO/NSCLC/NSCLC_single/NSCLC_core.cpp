#include "NSCLC_core.h"

//#include "NSCLC/SP_QSP_NSCLC/core/Param.h"
#include "NSCLC/SP_QSP_NSCLC/core/GlobalUtilities.h"
#include "InitialCondition.h"

#include <algorithm>    // std::max

extern FileOutputHub output_hub;

extern RNG rng;

//extern SP_QSP_IO::SP_QSP_NSCLC::Param params;

namespace SP_QSP_IO {
namespace SP_QSP_NSCLC {
extern Param params;
};
};
static auto& params = SP_QSP_IO::SP_QSP_NSCLC::params;

extern InitialCondition ic;
extern std::string initialCellFileName_core;
extern std::string initialCellFileName_margin;

typedef SP_QSP_IO::Coord Coord;

NSCLC_Core::NSCLC_Core()
: _tumor(params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_X),
	params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_Y),
	params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_Z))
, _lymph()
{


}

NSCLC_Core::~NSCLC_Core()
{
}

/*! Setup QSP module.
*/
bool NSCLC_Core::setup_qsp(QSP_IO::Param& p){
	_lymph.setup_param(p);
	params.update_from_qsp();
	return _lymph.get_initial_success();
}
/*! initialize compartments: randomly populate voxels
	This function is called only when creating the model for the first time,
	and not reading from saved state. Objects created in this function should
	be already in the serialization and can be loaded directly.
*/
void NSCLC_Core::initializeSimulation(void){

	// rule to randomly populate voxel during initlaization or grid shifting
	_tumor.set_allow_shift(ic.getVal(IC_GRID_SHIFT));
	_tumor._voxel_ic.setup(ic.getVal(IC_DENSITY_CSC),
		ic.getVal(IC_X_SIZE),
		ic.getVal(IC_Y_SIZE),
		ic.getVal(IC_Z_SIZE),
		ic.getVal(IC_X_MIN),
		ic.getVal(IC_Y_MIN),
		ic.getVal(IC_Z_MIN));

	//t cell sources
	{
		std::vector<Coord> c_tumor;
		unsigned int nr_source_tumor;
		_tumor.for_each_grid_coord(true, true, true, [&](Coord&c){
			c_tumor.push_back(c);
		});
		nr_source_tumor = int(c_tumor.size()*ic.getVal(IC_TUMOR_VAS_FOLD)
			* params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_REC_PORT_PROB));
		rng.shuffle_first_k(c_tumor, nr_source_tumor);
		for (size_t i = 0; i < nr_source_tumor; i++)
		{
			_tumor.add_lymphocyte_source(c_tumor[i]);
		}
		std::cout << "core nr sources: tumor: " << nr_source_tumor << std::endl;
	}

	std::string s;
	_tumor.initCompartment(s);
}

void NSCLC_Core::timeSlice(const long slice){
	
	const double dt = params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_SEC_PER_TIME_SLICE);
	const double t0 = slice * dt;
	// std::cout << "RNG check (" << slice << ") START : " << rng.get_unif_01() << std::endl;

	/* update cancer number and blood concentration */
	auto& qsp_var = _lymph.get_var_exchange();
	double lymphCC = qsp_var[SP_QSP_IO::SP_QSP_NSCLC::LymphCentral::QSPEX_TUM_C];

	/* if QSP halted, skip*/
	std::cout << "lymph CC: " << lymphCC << std::endl;
	double abm_min_cc = params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_C1_MIN);
	if (lymphCC > abm_min_cc)
	{
		_tumor.update_abm_with_qsp(qsp_var);

		//std::cout << "nivo: " << qsp_var[3] << std::endl;

		/*
		for (auto& v : qsp_var)
		{
		std::cout << v << ", ";
		}
		std::cout << std::endl;
		*/

		/* ABM time step */
		_tumor.timeSlice(slice);
		//std::cout << "RNG check (" << slice << ") MARGI : " << rng.get_unif_01() << std::endl;

		/* update QSP variables */
		auto& abm_var_0 = _tumor.get_var_exchange();

		size_t abm_var_len = abm_var_0.size();
		auto abm_var = std::vector<double>(abm_var_len, 0);

		double w = params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_WEIGHT_QSP);


		double tumCC= abm_var_0[SP_QSP_IO::SP_QSP_NSCLC::Tumor::TUMEX_CC];

		double abm_scaler = (1 - w) / w * lymphCC / (tumCC+ abm_min_cc );

		std::cout << "scalor:\n" <<  abm_scaler<< std::endl;

		for (size_t i = 0; i < abm_var_len; i++)
		{
			abm_var[i] = abm_var_0[i] * abm_scaler;
		}

		_lymph.update_qsp_var(abm_var);

	}

	/* QSP time step */
	_lymph.time_step(t0, dt);
	//std::cout << "RNG check (" << slice << ") QSP: " << rng.get_unif_01() << std::endl;
	return;

}

void NSCLC_Core::write_stats_header(void) const {

	auto& statsStream= output_hub.getStatsFstream();
	statsStream<< _tumor.get_stats().writeHeader();
	return;
}

void NSCLC_Core::write_stats_slice(unsigned long slice)const{
	{
		auto& statsStream = output_hub.getStatsFstream();
		statsStream << _tumor.get_stats().writeSlice(slice);
		statsStream.flush();
	}
	return;
}


void NSCLC_Core::write_QSP(unsigned long slice, bool header)const{
	auto& stream = output_hub.get_lymph_blood_QSP_stream();
	if (header){
		stream << "time" << _lymph.getQSPHeaders() << std::endl;
	}
	else{
		stream << slice << _lymph << std::endl;
	}
	return;
}

void NSCLC_Core::writeOde(unsigned long slice){
	_tumor.printCellOdeToFile(slice);
}


void NSCLC_Core::briefStats(unsigned long slice){
	std::cout << "Time: " << slice << std::endl;
	{
		const auto& stats = _tumor.get_stats();
		std::cout << "Core: " << "nrCell: " << _tumor.getNrCell()
			<< ", CD8: " << stats.getTCell()
			<< ", Treg: " << stats.getTreg()
			<< ", Cancer cell:" << stats.getCancerCell() << std::endl;
	}
}

/*! Print grid info to file.
    \param [in] slice
	\param [in] option: 1. only cellular scale; 2. only molecular scale; 3. both scales
*/
void NSCLC_Core::writeGrids(unsigned long slice, unsigned int option){
	if (option == 1 || option == 3)
	{
		{
			std::ofstream& snap = output_hub.getNewGridToSnapshotStream(slice, "cell_");
			snap << _tumor.compartment_cells_to_string();
			snap.close();
		}
		
	}
	if (option == 2 || option == 3)
	{
		{
			std::ofstream&  snap = output_hub.getNewGridToSnapshotStream(slice, "grid_core_");
			snap << _tumor.printGridToFile();
			snap.close();

		}
	}
}

