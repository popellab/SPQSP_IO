#include "NSCLC_core.h"

//#include "NSCLC/SP_QSP_NSCLC/core/Param.h"
#include "NSCLC/SP_QSP_NSCLC/core/GlobalUtilities.h"
#include "InitialCondition.h"

#include <algorithm>    // std::max
#include <math.h> // pow

extern FileOutputHub output_hub;

extern RNG rng;

//extern SP_QSP_IO::Param params;

namespace SP_QSP_IO {
	namespace SP_QSP_NSCLC {
		extern Param params;
	}
};
static auto& params = SP_QSP_IO::SP_QSP_NSCLC::params;

extern InitialCondition ic;
extern std::string initialCellFileName_core;
extern std::string initialCellFileName_margin;

typedef SP_QSP_IO::Coord Coord;

NSCLC_Core::NSCLC_Core()
: _tumor_core(params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_X),
	params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_Y),
	params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_Z))
, _tumor_margin(params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_X),
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
	_tumor_margin.set_allow_shift(ic.getVal(IC_MARGIN_GRID_SHIFT));
	_tumor_margin.set_shift_th(ic.getVal(IC_MARGIN_GRID_SHIFT_TH));
	_tumor_margin._voxel_ic.setup(ic.getVal(IC_MARGIN_STATIONARY),
		ic.getVal(IC_DENSITY_MARGIN_CANCER),
		params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_X),
		params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_TUMOR_Y),
		ic.getVal(IC_MARGIN_CANCER_BOUNDARY));

	_tumor_core.set_allow_shift(ic.getVal(IC_CORE_GRID_SHIFT));
	_tumor_core._voxel_ic.setup(ic.getVal(IC_CORE_STATIONARY),
			ic.getVal(IC_DENSITY_CORE_CANCER),
			0, 0, 0);


	//t cell sources
	// margin:
	{
		std::vector<Coord> c_normal, c_tumor;
		unsigned int nr_source_normal, nr_source_tumor;
		_tumor_margin.for_each_grid_coord(true, true, true, [&](Coord&c){
			if (c.z > ic.getVal(IC_MARGIN_CANCER_BOUNDARY)){
				c_normal.push_back(c);
			}
			else{
				c_tumor.push_back(c);
			}
		});
		nr_source_normal = int(c_normal.size()*ic.getVal(IC_MARGIN_NORMAL_VAS_FOLD)
			* params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_REC_PORT_PROB));
		rng.shuffle_first_k(c_normal, nr_source_normal);
		nr_source_tumor = int(c_tumor.size()*ic.getVal(IC_MARGIN_TUMOR_VAS_FOLD)
			* params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_REC_PORT_PROB));
		rng.shuffle_first_k(c_tumor, nr_source_tumor);

		for (size_t i = 0; i < nr_source_normal; i++)
		{
			_tumor_margin.add_lymphocyte_source(c_normal[i]);
		}
		for (size_t i = 0; i < nr_source_tumor; i++)
		{
			_tumor_margin.add_lymphocyte_source(c_tumor[i]);
		}
		std::cout << "margin nr sources: tumor: " << nr_source_tumor 
			<< ", normal: " << nr_source_normal << std::endl;
	}
	// core:
	{
		std::vector<Coord> c_tumor;
		unsigned int nr_source_tumor;
		_tumor_margin.for_each_grid_coord(true, true, true, [&](Coord&c){
			c_tumor.push_back(c);
		});
		nr_source_tumor = int(c_tumor.size()*ic.getVal(IC_CORE_TUMOR_VAS_FOLD)
			* params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_REC_PORT_PROB));
		rng.shuffle_first_k(c_tumor, nr_source_tumor);
		for (size_t i = 0; i < nr_source_tumor; i++)
		{
			_tumor_core.add_lymphocyte_source(c_tumor[i]);
		}
		std::cout << "core nr sources: tumor: " << nr_source_tumor << std::endl;
	}

	std::string s;
	_tumor_core.initCompartment(s);
	_tumor_margin.initCompartment(s);
}

/*! initialize compartments: create initial cells from file input specifications
	This function is called only when creating the model for the first time,
	and not reading from saved state. Objects created in this function should
	be already in the serialization and can be loaded directly.
*/
void NSCLC_Core::initializeSimulation(std::string core, std::string margin){

	_tumor_core.set_allow_shift(false);
	_tumor_margin.set_allow_shift(false);

	_tumor_core.initCompartment(core);
	_tumor_margin.initCompartment(margin);
}

void NSCLC_Core::timeSlice(const long slice){
	
	const double dt = params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_SEC_PER_TIME_SLICE);
	const double t0 = slice * dt;
	// std::cout << "RNG check (" << slice << ") START : " << rng.get_unif_01() << std::endl;

	/* update cancer number and blood concentration */
	auto& qsp_var = _lymph.get_var_exchange();
	double lymphCC = qsp_var[SP_QSP_IO::SP_QSP_NSCLC::LymphCentral::QSPEX_TUM_C];

	/* if QSP halted, skip*/
	//std::cout << "lymph CC: " << lymphCC << std::endl;
	//double abm_min_cc = params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_C1_MIN);
	//if (lymphCC > abm_min_cc)
	{
		_tumor_core.update_abm_with_qsp(qsp_var);
		_tumor_margin.update_abm_with_qsp(qsp_var);

		//std::cout << "nivo: " << qsp_var[3] << std::endl;

		/* ABM time step */
		_tumor_core.timeSlice(slice);
		//std::cout << "RNG check (" << slice << ") CORE: " << rng.get_unif_01() << std::endl;
		_tumor_margin.timeSlice(slice);
		//std::cout << "RNG check (" << slice << ") MARGI : " << rng.get_unif_01() << std::endl;

		/* update QSP variables */
		auto& abm_var_0 = _tumor_core.get_var_exchange();
		auto& abm_var_1 = _tumor_margin.get_var_exchange();

		size_t abm_var_len = abm_var_0.size();
		auto abm_var = std::vector<double>(abm_var_len, 0);

		double w = params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_WEIGHT_QSP);

		bool use_resect = params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_QSP_RESECTION);
		bool post_resect = slice > params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_RESECT_TIME_STEP);

		double fraction_margin = use_resect && post_resect
			? ic.getVal(IC_FRACTION_MARGIN_RES)
			: ic.getVal(IC_FRACTION_MARGIN);

		double fraction_core = 1 - fraction_margin;

		double tumCC_core = abm_var_0[SP_QSP_IO::SP_QSP_NSCLC::Tumor::TUMEX_CC];
		double tumCC_margin = abm_var_1[SP_QSP_IO::SP_QSP_NSCLC::Tumor::TUMEX_CC];
 
		/* Scaling using cancer cell number as reference
		double abm_scaler_core = (1 - w) / w * lymphCC / (tumCC_core + abm_min_cc )* fraction_core;
		double abm_scaler_margin = (1 - w) / w * lymphCC / (tumCC_margin  + abm_min_cc) * fraction_margin;
		*/

		/* Scaling using tumor volume as reference*/
		double window_vol = _tumor_core.getGridSize() * 1E-18 * 
			pow(params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_VOXEL_SIZE), 3);
		double vol_tum = _lymph.calc_tumor_vol(w);
		//std::cout << "window size (m^3): " << window_vol << std::endl;
		//std::cout << "tumor size (m^3): " << vol_tum << std::endl;

		double abm_scaler_core = (1 - w) * fraction_core * vol_tum / window_vol;
		double abm_scaler_margin = (1 - w) * fraction_margin * vol_tum / window_vol;

		/*
			*/
		std::cout << "scalor: ( f = " << fraction_margin << ", " << fraction_core <<  ")\n" 
			<< "core:" << abm_scaler_core 
			<< "\nmargin: " << abm_scaler_margin << std::endl << std::endl;

		for (size_t i = 0; i < abm_var_len; i++)
		{
			abm_var[i] = abm_var_0[i] * abm_scaler_core
				+ abm_var_1[i] * abm_scaler_margin;
		}
		int idx = SP_QSP_IO::SP_QSP_NSCLC::Tumor::TUMEX_CC_T_KILL;
		std::cout << "total: " << abm_var[idx] << ", core: " << abm_var_0[idx]
			<< ", margin: " << abm_var_1[idx] << std::endl;

		_lymph.update_qsp_var(abm_var);

	}

	/* QSP time step */
	_lymph.time_step(t0, dt);
	//std::cout << "RNG check (" << slice << ") QSP: " << rng.get_unif_01() << std::endl;
	return;

}

void NSCLC_Core::write_stats_header(void) const {

	auto& statsStream_core = output_hub.getStatsFstream(0);
	statsStream_core  << _tumor_core.get_stats().writeHeader();
	auto& statsStream_margin = output_hub.getStatsFstream(1);
	statsStream_margin  << _tumor_margin.get_stats().writeHeader();
	return;
}

void NSCLC_Core::write_stats_slice(unsigned long slice)const{
	{
		auto& statsStream = output_hub.getStatsFstream(0);
		statsStream << _tumor_core.get_stats().writeSlice(slice);
		statsStream.flush();
	}
	{
		auto& statsStream = output_hub.getStatsFstream(1);
		statsStream << _tumor_margin.get_stats().writeSlice(slice);
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
	_tumor_core.printCellOdeToFile(slice);
}


void NSCLC_Core::briefStats(unsigned long slice){
	std::cout << "Time: " << slice << std::endl;
	{
		const auto& stats = _tumor_core.get_stats();
		std::cout << "Core: " << "nrCell: " << _tumor_core.getNrCell()
			<< ", CD8: " << stats.getTCell()
			<< ", Treg: " << stats.getTreg()
			<< ", Cancer cell:" << stats.getCancerCell() << std::endl;
		std::cout << "Nivo: " << _lymph.get_nivo_nM() << " nM; "
			<< "IL2: " << _tumor_core.get_chem_grid().get_average_concentration(SP_QSP_IO::SP_QSP_NSCLC::CHEM_IL_2) << " ng/mL; "
			<< "IFNg: " << _tumor_core.get_chem_grid().get_average_concentration(SP_QSP_IO::SP_QSP_NSCLC::CHEM_IFN) << " ng/mL" << std::endl;
	}
	{
		const auto& stats = _tumor_margin.get_stats();
		std::cout << "Margin: " << "nrCell: " << _tumor_margin.getNrCell()
			<< ", CD8: " << stats.getTCell()
			<< ", Treg: " << stats.getTreg()
			<< ", Cancer cell:" << stats.getCancerCell() << std::endl;
		std::cout << "Nivo: " << _lymph.get_nivo_nM() << " nM; "
			<< "IL2: " << _tumor_margin.get_chem_grid().get_average_concentration(SP_QSP_IO::SP_QSP_NSCLC::CHEM_IL_2) << " ng/mL; "
			<< "IFNg: " << _tumor_margin.get_chem_grid().get_average_concentration(SP_QSP_IO::SP_QSP_NSCLC::CHEM_IFN) << " ng/mL" << std::endl;
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
			std::ofstream& snap = output_hub.getNewGridToSnapshotStream(slice, "cell_core_");
			snap << _tumor_core.compartment_cells_to_string();
			snap.close();
		}
		{
			std::ofstream& snap = output_hub.getNewGridToSnapshotStream(slice, "cell_margin_");
			snap << _tumor_margin.compartment_cells_to_string();
			snap.close();
		}
	}
	if (option == 2 || option == 3)
	{
		{
			std::ofstream&  snap = output_hub.getNewGridToSnapshotStream(slice, "grid_core_");
			snap << _tumor_core.printGridToFile();
			snap.close();

		}
		{
			std::ofstream&  snap = output_hub.getNewGridToSnapshotStream(slice, "grid_margin_");
			snap << _tumor_margin.printGridToFile();
			snap.close();
		}
	}
}

