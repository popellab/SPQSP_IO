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
static auto& params = NSCLC::params;

extern InitialCondition ic;
extern std::string initialCellFileName_core;
extern std::string initialCellFileName_margin;

typedef SP_QSP_IO::Coord Coord;

NSCLC_Core::NSCLC_Core()
: _ROI_core()
, _ROI_margin()
, _lymph()
{
}

NSCLC_Core::~NSCLC_Core()
{
	for (auto& ptumor : _ROI_core) {
		delete ptumor;
	}
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

	for (int i = 0; i < ic.getVal(IC_NUM_ROI_core); i++)
	{
		auto pTumor = new NSCLC::Tumor(params.getVal(NSCLC::PARAM_TUMOR_X),
			params.getVal(NSCLC::PARAM_TUMOR_Y),
			params.getVal(NSCLC::PARAM_TUMOR_Z));
		_ROI_core.push_back(pTumor);
	}

	for (int i = 0; i < ic.getVal(IC_NUM_ROI_margin); i++)
	{
		auto pTumor = new NSCLC::Tumor(params.getVal(NSCLC::PARAM_TUMOR_X),
			params.getVal(NSCLC::PARAM_TUMOR_Y),
			params.getVal(NSCLC::PARAM_TUMOR_Z));
		_ROI_margin.push_back(pTumor);
	}

	std::string s;
	// rule to randomly populate voxel during initlaization or grid shifting
	for (auto& ptumor : _ROI_core) {
		auto& tumor = *ptumor;
		tumor.set_allow_shift(ic.getVal(IC_CORE_GRID_SHIFT));
		tumor._voxel_ic.setup(ic.getVal(IC_CORE_STATIONARY),
			ic.getVal(IC_DENSITY_CORE_CANCER),
			0, 0, 0);

		tumor.initCompartment(s);
		//std::cout << "core populated" << std::endl;
	}

	for (auto& ptumor : _ROI_margin) {

		auto& tumor = *ptumor;
		tumor.set_allow_shift(ic.getVal(IC_MARGIN_GRID_SHIFT));
		tumor._voxel_ic.setup(ic.getVal(IC_MARGIN_STATIONARY),
			ic.getVal(IC_DENSITY_MARGIN_CANCER),
			params.getVal(NSCLC::PARAM_TUMOR_X),
			params.getVal(NSCLC::PARAM_TUMOR_Y),
			ic.getVal(IC_MARGIN_CANCER_BOUNDARY));

		tumor.initCompartment(s);
		//std::cout << "margin populated" << std::endl;
	}

	//t cell sources
	// core:
	for (auto& ptumor : _ROI_core) {
		auto& tumor = *ptumor;
		std::vector<Coord> c_tumor;
		//std::cout << "num_source: " << c_tumor.size() << std::endl;
		unsigned int nr_source_tumor;
		tumor.for_each_grid_coord(true, true, true, [&](Coord&c){
			c_tumor.push_back(c);
		});
		//std::cout << "num_source: " << c_tumor.size() << std::endl;
		nr_source_tumor = int(c_tumor.size()*ic.getVal(IC_CORE_TUMOR_VAS_FOLD)
			* params.getVal(NSCLC::PARAM_REC_PORT_PROB));
		rng.shuffle_first_k(c_tumor, nr_source_tumor);
		for (size_t i = 0; i < nr_source_tumor; i++)
		{
			tumor.add_lymphocyte_source(c_tumor[i]);
		}
		//std::cout << "core nr sources: tumor: " << nr_source_tumor << std::endl;
	}

	// margin:
	for (auto& ptumor : _ROI_margin) {
		auto& tumor = *ptumor;
		std::vector<Coord> c_normal, c_tumor;
		unsigned int nr_source_normal, nr_source_tumor;
		tumor.for_each_grid_coord(true, true, true, [&](Coord&c){
			if (c.z > ic.getVal(IC_MARGIN_CANCER_BOUNDARY)){
				c_normal.push_back(c);
			}
			else{
				c_tumor.push_back(c);
			}
		});
		nr_source_normal = int(c_normal.size()*ic.getVal(IC_MARGIN_NORMAL_VAS_FOLD)
			* params.getVal(NSCLC::PARAM_REC_PORT_PROB));
		rng.shuffle_first_k(c_normal, nr_source_normal);
		nr_source_tumor = int(c_tumor.size()*ic.getVal(IC_MARGIN_TUMOR_VAS_FOLD)
			* params.getVal(NSCLC::PARAM_REC_PORT_PROB));
		rng.shuffle_first_k(c_tumor, nr_source_tumor);

		for (size_t i = 0; i < nr_source_normal; i++)
		{
			tumor.add_lymphocyte_source(c_normal[i]);
		}
		for (size_t i = 0; i < nr_source_tumor; i++)
		{
			tumor.add_lymphocyte_source(c_tumor[i]);
		}
		//std::cout << "margin nr sources: tumor: " << nr_source_tumor 
		//		<< ", normal: " << nr_source_normal << std::endl;
	}
	//std::cout << "sources generated" << std::endl;
}

void NSCLC_Core::timeSlice(const long slice){
	
	const double dt = params.getVal(NSCLC::PARAM_SEC_PER_TIME_SLICE);
	const double t0 = slice * dt;
	// std::cout << "RNG check (" << slice << ") START : " << rng.get_unif_01() << std::endl;

	/* update cancer number and blood concentration */
	auto& qsp_var = _lymph.get_var_exchange();
	double lymphCC = qsp_var[NSCLC::LymphCentral::QSPEX_TUM_C];

	/* if QSP halted, skip*/
	std::cout << "lymph CC: " << lymphCC << std::endl;
	//double abm_min_cc = params.getVal(NSCLC::PARAM_C1_MIN);
	//if (lymphCC > abm_min_cc)
	{

		for (auto& ptumor : _ROI_margin) {
			ptumor->update_abm_with_qsp(qsp_var);
		}
		for (auto& ptumor : _ROI_core) {
			ptumor->update_abm_with_qsp(qsp_var);
		}

		std::cout << "nivo: " << qsp_var[3] << std::endl;

		/*
		for (auto& v : qsp_var)
		{
		std::cout << v << ", ";
		}
		std::cout << std::endl;
		*/

		/* ABM time step */
		for (auto& ptumor : _ROI_core) {
			ptumor->timeSlice(slice);
		}
		for (auto& ptumor : _ROI_margin) {
			ptumor->timeSlice(slice);
		}
		//std::cout << "RNG check (" << slice << ") CORE: " << rng.get_unif_01() << std::endl;
		//std::cout << "RNG check (" << slice << ") MARGI : " << rng.get_unif_01() << std::endl;

		/* update QSP variables */
		size_t abm_var_len = NSCLC::Tumor::TUMEX_VAR_NUM;
		auto abm_var = std::vector<double>(abm_var_len, 0);
		auto abm_var_core = std::vector<double>(abm_var_len, 0);
		auto abm_var_margin = std::vector<double>(abm_var_len, 0);

		for (auto& ptumor : _ROI_core) {
			auto& _abm_var_core = ptumor->get_var_exchange();
			for (size_t i = 0; i < abm_var_len; i++)
			{
				abm_var_core[i] += _abm_var_core[i]; 
			}
		}
		for (auto& ptumor : _ROI_margin) {
			auto& _abm_var_margin = ptumor->get_var_exchange();
			for (size_t i = 0; i < abm_var_len; i++)
			{
				abm_var_margin[i] += _abm_var_margin[i]; 
			}
		}
		double w = params.getVal(NSCLC::PARAM_WEIGHT_QSP);

		double fraction_margin = slice > params.getVal(NSCLC::PARAM_RESECT_TIME_STEP)
			? ic.getVal(IC_FRACTION_MARGIN)
			: ic.getVal(IC_FRACTION_MARGIN_RES);

		double fraction_core = 1 - fraction_margin;

		double tumCC_core = abm_var_core[NSCLC::Tumor::TUMEX_CC];
		double tumCC_margin = abm_var_margin[NSCLC::Tumor::TUMEX_CC];

		double window_vol = _ROI_core[0]->getGridSize() * 1E-18 * 
			pow(params.getVal(SP_QSP_IO::SP_QSP_NSCLC::PARAM_VOXEL_SIZE), 3);
		double vol_tum = _lymph.calc_tumor_vol(w);
		double abm_scaler_core = (1 - w) * fraction_core * vol_tum / window_vol;
		double abm_scaler_margin = (1 - w) * fraction_margin * vol_tum / window_vol;

		/*
		double abm_scaler_core = (1 - w) / w * lymphCC / (tumCC_core + abm_min_cc )* fraction_core;
		double abm_scaler_margin = (1 - w) / w * lymphCC / (tumCC_margin  + abm_min_cc) * fraction_margin;
		*/

		std::cout << "scalor:\n" << "core:" << abm_scaler_core 
			<< "\nmargin: " << abm_scaler_margin << std::endl;

		for (size_t i = 0; i < abm_var_len; i++)
		{
			abm_var[i] = abm_var_core[i] * abm_scaler_core
				+ abm_var_margin[i] * abm_scaler_margin;
		}

		_lymph.update_qsp_var(abm_var);

	}

	/* QSP time step */
	_lymph.time_step(t0, dt);
	//std::cout << "RNG check (" << slice << ") QSP: " << rng.get_unif_01() << std::endl;
	return;

}

void NSCLC_Core::write_stats_header(void) const {

	for (int i = 0; i < ic.getVal(IC_NUM_ROI_core); i++)
	{
		//std::cout << "header: core: " << i << std::endl;
		auto& statsStream = output_hub.getStatsFstream(true, i);
		statsStream << _ROI_core[i]->get_stats().writeHeader();
		statsStream.flush();
	}
	for (int i = 0; i < ic.getVal(IC_NUM_ROI_margin); i++)
	{
		//std::cout << "header: margin: " << i << std::endl;
		auto& statsStream = output_hub.getStatsFstream(false, i);
		statsStream << _ROI_margin[i]->get_stats().writeHeader();
		statsStream.flush();
	}
	return;
}

void NSCLC_Core::write_stats_slice(unsigned long slice)const{
	for (int i = 0; i < ic.getVal(IC_NUM_ROI_core); i++)
	{
		auto& statsStream = output_hub.getStatsFstream(true, i);
		statsStream << _ROI_core[i]->get_stats().writeSlice(slice);
		statsStream.flush();
	}
	for (int i = 0; i < ic.getVal(IC_NUM_ROI_margin); i++)
	{
		auto& statsStream = output_hub.getStatsFstream(false, i);
		statsStream << _ROI_margin[i]->get_stats().writeSlice(slice);
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


void NSCLC_Core::briefStats(unsigned long slice){
	std::cout << "Time: " << slice << std::endl;
	for (int i = 0; i < ic.getVal(IC_NUM_ROI_core); i++)
	{
		const auto& stats = _ROI_core[i]->get_stats();
		std::cout << "Core_" << i << ": " << "nrCell: " << _ROI_core[i]->getNrCell()
			<< ", CD8: " << stats.getTCell()
			<< ", Treg: " << stats.getTreg()
			<< ", Cancer cell:" << stats.getCancerCell() << std::endl;
	}
	for (int i = 0; i < ic.getVal(IC_NUM_ROI_margin); i++)
	{
		const auto& stats = _ROI_margin[i]->get_stats();
		std::cout << "Margin_" << i << ": " << "nrCell: " << _ROI_margin[i]->getNrCell()
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
		for (int i = 0; i < ic.getVal(IC_NUM_ROI_core); i++)
		{
			std::string prefix = "cell_core_" + std::to_string(i) + "_";
			std::ofstream& snap = output_hub.getNewGridToSnapshotStream(slice, prefix);
			snap << _ROI_core[i]->compartment_cells_to_string();
			snap.close();
		}
		for (int i = 0; i < ic.getVal(IC_NUM_ROI_margin); i++)
		{
			std::string prefix = "cell_margin" + std::to_string(i) + "_";
			std::ofstream& snap = output_hub.getNewGridToSnapshotStream(slice, prefix);
			snap << _ROI_margin[i]->compartment_cells_to_string();
			snap.close();
		}
	}
	if (option == 2 || option == 3)
	{
		for (int i = 0; i < ic.getVal(IC_NUM_ROI_core); i++)
		{
			std::string prefix = "grid_core_" + std::to_string(i) + "_";
			std::ofstream&  snap = output_hub.getNewGridToSnapshotStream(slice, prefix);
			snap << _ROI_core[i]->printGridToFile();
			snap.close();

		}
		for (int i = 0; i < ic.getVal(IC_NUM_ROI_margin); i++)
		{
			std::string prefix = "grid_margin" + std::to_string(i) + "_";
			std::ofstream&  snap = output_hub.getNewGridToSnapshotStream(slice, prefix);
			snap << _ROI_margin[i]->printGridToFile();
			snap.close();
		}
	}
}

