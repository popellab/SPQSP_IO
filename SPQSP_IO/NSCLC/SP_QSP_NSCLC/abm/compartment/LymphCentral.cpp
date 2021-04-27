#include "LymphCentral.h"
#include "Tumor.h"
#include "../../core/GlobalUtilities.h"

#include <fstream>

// shorthands
// get raw value (original units)
#define GET_PARAM_RAW(x) _QSP_model.getSystem()->getParameterVal(x, true)
#define SET_PARAM_RAW(x, y) _QSP_model.getSystem()->setParameterVal(x, y, true)
#define GET_VAR_RAW(x) _QSP_model.getSystem()->getSpeciesVar(x, true)
#define SET_VAR_RAW(x, y) _QSP_model.getSystem()->setSpeciesVar(x, y, true)
// get value (SI units)
#define GET_PARAM(x) _QSP_model.getSystem()->getParameterVal(x, false)
#define SET_PARAM(x, y) _QSP_model.getSystem()->setParameterVal(x, y, false)
#define GET_VAR(x) _QSP_model.getSystem()->getSpeciesVar(x, false)
#define SET_VAR(x, y) _QSP_model.getSystem()->setSpeciesVar(x, y, false)
// parameter (SI units)
#define QSP_CONST(x) LymphBloodQSP::get_class_param(x)

// indices of parameter/variables in their vectors
// y
#define QSP_ID_TUM_C1 7
#define QSP_ID_TUM_APC 9 
#define QSP_ID_TUM_mAPC 10
#define QSP_ID_TUM_TREG 12
#define QSP_ID_TUM_TEFF 23
#define QSP_ID_TUM_TEFF_EXH 24

#define QSP_ID_CENT_TREG 1
#define QSP_ID_CENT_TEFF 2
#define QSP_ID_TUM_NIVO 6
#define QSP_ID_CP 8
#define QSP_ID_CKINE_MAT  11
#define QSP_ID_D1 22 

// class_param
#define QSP_CELL 7
#define QSP_VOL_TUM 16 //initial tumor volume
#define QSP_VOL_TUM_MAX 18 //max tumor volume
#define QSP_C1_MIN 36
#define QSP_K_C_MAX 38
#define QSP_D_PER_C 43
#define QSP_CP_PER_C 44
#define QSP_DAMP_PER_C 45
#define QSP_DOSE_MG 61
#define QSP_N_CLONE_TREG 96
#define QSP_N_CLONE_P10 97

// class_param: tumor size calculation
#define QSP_F_VOL 22
#define QSP_VOL_C 28
#define QSP_VOL_T 29
#define QSP_VOL_APC 30 

// constants
#define AVOGADROS 6.022140857E23 
#define SEC_PER_DAY 86400
#define INIT_SIM_DAY_MAX 8000

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

LymphCentral::LymphCentral()
	: _QSP_model()
	, _success_ic(false)
	, _var_qsp_to_abm()
{
	// Cent.Teff, Cent.Treg, Cent.Nivo
	_var_qsp_to_abm = std::vector<double>(QSPEX_VAR_NUM, 0);
}

LymphCentral::~LymphCentral()
{
}

void LymphCentral::setup_param(LymphBloodParam& p){

	//bool steadystate = true;
	bool steadystate = false;
	bool initialization_sim = true;
	unsigned int n = _QSP_model.getSystem()->get_num_variables();
	std::vector<double> ss_val(n, 0);

	/*Old Initialization: run model for a predetermined period of time*/
	if (steadystate)
	{
		QSP ss;
		LymphBloodQSP::_QSP_weight = 1;
		LymphBloodQSP::use_steady_state = true;
		LymphBloodQSP::use_resection = false;
		LymphBloodQSP::setup_class_parameters(p);
		LymphBloodQSP::set_class_param(QSP_DOSE_MG, 0);
		ss.getSystem()->setup_instance_tolerance(p);
		ss.getSystem()->setup_instance_varaibles(p);
		ss.getSystem()->eval_init_assignment();

		// run to steady state
		double tss = params.getVal(PARAM_QSP_STEADYSTATE) * SEC_PER_DAY;
		ss.solve(0, tss);
		for (size_t i = 0; i < n; i++)
		{
			ss_val[i] = ss.getSystem()->getSpeciesVar(i);
		}
	}
	/*New initialization: run model until initial tumor size is reached*/
	if (initialization_sim)
	{
		QSP ss;

		/*
		double init_teff = ss.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF, true);
		double init_texh = ss.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF_EXH, true);
		double init_treg = ss.getSystem()->getSpeciesVar(QSP_ID_CENT_TREG, true);
		double init_APC = ss.getSystem()->getSpeciesVar(QSP_ID_TUM_APC, true);
		std::cout << "Initial: " << init_teff << "; " << init_texh << "; "
			<< init_treg << "; " << init_APC << std::endl;
*/
		LymphBloodQSP::_QSP_weight = 1;
		LymphBloodQSP::use_steady_state = false;
		LymphBloodQSP::use_resection = false;
		LymphBloodQSP::setup_class_parameters(p);
		LymphBloodQSP::set_class_param(QSP_DOSE_MG, 0);
		ss.getSystem()->setup_instance_tolerance(p);
		ss.getSystem()->setup_instance_varaibles(p);
		ss.getSystem()->eval_init_assignment();

		// need to set cancer cell number to larger than minimum 
		// (or it will force simulation to stop by event)
		ss.getSystem()->setSpeciesVar(QSP_ID_TUM_C1, 2.0*QSP_CONST(QSP_C1_MIN), false);


		// run to initial tumor diameter 
		int t_ic_max = SEC_PER_DAY * INIT_SIM_DAY_MAX;
		int t_ic = 0;
		double V_tumor_t0 = QSP_CONST(QSP_VOL_TUM);
		double V_tumor_t = calc_tumor_vol(ss, LymphBloodQSP::_QSP_weight);

		std::ofstream init_ode; 

		/*
		init_ode.open("./initial_ODE.csv", std::ios::out | std::ios::trunc);
		init_ode << "time, t_vol" << getQSPHeaders() << std::endl;
		init_ode << t_ic << "," << V_tumor_t << ss << std::endl;
		*/

		while(t_ic < t_ic_max && V_tumor_t < V_tumor_t0){
			ss.solve(t_ic, SEC_PER_DAY);
			t_ic += SEC_PER_DAY;
			V_tumor_t = calc_tumor_vol(ss,  LymphBloodQSP::_QSP_weight);
			//init_ode << t_ic << "," << V_tumor_t << ss << std::endl;
		}
		/*
		init_teff = ss.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF, true);
		init_texh = ss.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF_EXH, true);
		init_treg = ss.getSystem()->getSpeciesVar(QSP_ID_CENT_TREG, true);
		init_APC = ss.getSystem()->getSpeciesVar(QSP_ID_TUM_APC, true);
		std::cout << "after init: " << init_teff << "; " << init_texh << "; "
			<< init_treg << "; " << init_APC << std::endl;
		*/
		std::cout << "init simulation simulation done" << std::endl;
		std::cout << "vol_0: " << V_tumor_t0<< std::endl;
		std::cout << "vol: " << V_tumor_t << std::endl;

		if (V_tumor_t >= V_tumor_t0)
		{
			_success_ic = true;
		}
		//init_ode.close();
		for (size_t i = 0; i < n; i++)
		{
			ss_val[i] = ss.getSystem()->getSpeciesVar(i);
		}
	}
	//std::cout << "steady-state simulation done" << std::endl;

	// setup
	LymphBloodQSP::_QSP_weight = params.getVal(PARAM_WEIGHT_QSP);
	LymphBloodQSP::use_steady_state = false;
	LymphBloodQSP::use_resection = params.getVal(PARAM_QSP_RESECTION);
	LymphBloodQSP::setup_class_parameters(p);
	_QSP_model.getSystem()->setup_instance_tolerance(p);
	_QSP_model.getSystem()->setup_instance_varaibles(p);

	/*
	double init_teff = _QSP_model.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF, true);
	double init_texh = _QSP_model.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF_EXH, true);
	double init_treg = _QSP_model.getSystem()->getSpeciesVar(QSP_ID_CENT_TREG, true);
	double init_APC = _QSP_model.getSystem()->getSpeciesVar(QSP_ID_TUM_APC, true);
		std::cout << init_teff << "; " << init_texh << "; "
			<< init_treg << "; " << init_APC << std::endl;
*/
	// load steady state
	if (steadystate)
	{
		for (size_t i = 0; i < n; i++)
		{
			_QSP_model.getSystem()->setSpeciesVar(i, ss_val[i]);
		}
		_QSP_model.getSystem()->eval_init_assignment();
	}

	if (initialization_sim)
	{
		_QSP_model.getSystem()->eval_init_assignment();
		for (size_t i = 0; i < n; i++)
		{
			_QSP_model.getSystem()->setSpeciesVar(i, ss_val[i]);
		}

		_QSP_model.getSystem()->adjust_hybrid_variables();
	}

	_QSP_model.getSystem()->updateVar();

	/*
	init_teff = _QSP_model.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF, true);
	init_texh = _QSP_model.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF_EXH, true);
	init_treg = _QSP_model.getSystem()->getSpeciesVar(QSP_ID_CENT_TREG, true);
	init_APC = _QSP_model.getSystem()->getSpeciesVar(QSP_ID_TUM_APC, true);
	std::cout << init_teff << "; " << init_texh << "; "
			<< init_treg << "; " << init_APC << std::endl;
			*/

}

/*! solve QSP from t to t + dt
*/
void LymphCentral::time_step(double t, double dt){

	// solve QSP for dt
	_QSP_model.solve(t, dt);
	return;
}

/*! Get QSP variables for ABM.
	
	# Tum.C1 (unit: cell)
    # Cent.Teff (unit: convert from cell to cell/mm^3)
	# Cent.Treg (unit: convert from cell to cell/mm^3)
	# Cent.Nivo (unit: mole/L)
*/
const std::vector<double>& LymphCentral::get_var_exchange(void){

	// need to be cell count for calculating ABM scalor
	_var_qsp_to_abm[QSPEX_TUM_C] = GET_VAR_RAW(QSP_ID_TUM_C1);
	// internal SI unit for calculating rates and probabilities.
	_var_qsp_to_abm[QSPEX_CENT_TEFF] = GET_VAR(QSP_ID_CENT_TEFF);
	_var_qsp_to_abm[QSPEX_CENT_TREG] = GET_VAR(QSP_ID_CENT_TREG);
	_var_qsp_to_abm[QSPEX_TUM_NIVO] = GET_VAR(QSP_ID_TUM_NIVO);
	return _var_qsp_to_abm;
}

/*! update QSP module with output from ABM
	unit convert: item to mole
    # cancer cell death (total)
	# cancer cell death (Teff kill)
	# Teff recruitment
	# Treg recruitment
*/
void LymphCentral::update_qsp_var(const std::vector<double>& var_abm){

	// convert item to internal units
	double scalar = 1 / AVOGADROS;

	// CC death total, CC death Teff, Teff recruit, Treg recruit
	// units: cell
	double cc_death_total = var_abm[Tumor::TUMEX_CC_DEATH] * scalar;
	double cc_death_Teff = var_abm[Tumor::TUMEX_CC_T_KILL] * scalar;
	double Teff_recruit = var_abm[Tumor::TUMEX_TEFF_REC] * scalar;
	double Treg_recruit = var_abm[Tumor::TUMEX_TREG_REC] * scalar;

	// update system
	double tum_c1 = GET_VAR(QSP_ID_TUM_C1);
	double cp = GET_VAR(QSP_ID_CP);
	double d1 = GET_VAR(QSP_ID_D1);
	double ckine = GET_VAR(QSP_ID_CKINE_MAT);

	double V_tum = (tum_c1 + QSP_CONST(QSP_CELL)) / QSP_CONST(QSP_K_C_MAX) * QSP_CONST(QSP_VOL_TUM_MAX);
	double factor_cp = QSP_CONST(QSP_N_CLONE_TREG) * QSP_CONST(QSP_CP_PER_C);
	double factor_d = QSP_CONST(QSP_N_CLONE_P10) * QSP_CONST(QSP_D_PER_C);
	double factor_DAMP = QSP_CONST(QSP_DAMP_PER_C);

	double factor_extra = params.getVal(PARAM_QSP_EXTRA);
	double abm_weight = 1 - params.getVal(PARAM_WEIGHT_QSP);

	//std::cout << "cc death: " << cc_death_total << " (by Teff: "
	//	<< cc_death_Teff << ")" << std::endl;
	std::cout << "killed: " << cc_death_Teff << std::endl;
	std::cout << "d1 (pM/L): " << d1 * 1e9 << "; ";

	//cp += cc_death_total * factor_cp / V_tum ;
	//d1 += cc_death_total *  factor_d / V_tum ;

	double inc_cp = cc_death_Teff* factor_cp / V_tum * factor_extra;
	double inc_d1 = cc_death_Teff*  factor_d / V_tum * factor_extra;
	double inc_ckine =  cc_death_Teff * factor_DAMP / V_tum * factor_extra;

	cp += inc_cp;
	d1 += inc_d1;
	ckine += inc_ckine;

	std::cout << "add d1 (pM/L): " << inc_d1 * 1e9;
	std::cout << "; new d1 (pM/L): " << d1 * 1e9 << std::endl;

	SET_VAR(QSP_ID_CP, cp);
	SET_VAR(QSP_ID_D1, d1);
	SET_VAR(QSP_ID_CKINE_MAT, ckine);

	double cent_t_eff = GET_VAR(QSP_ID_CENT_TEFF);
	double cent_t_reg = GET_VAR(QSP_ID_CENT_TREG);
	
	cent_t_eff -= Teff_recruit;
	cent_t_reg -= Treg_recruit;
	
	SET_VAR(QSP_ID_CENT_TEFF, cent_t_eff);
	SET_VAR(QSP_ID_CENT_TREG, cent_t_reg);
	return;
}

/*! calculate tumor volume from cell numbers
*/
double LymphCentral::calc_tumor_vol(const QSP& q, const double weight_qsp)const{
	double f_vol_tumor = QSP_CONST(QSP_F_VOL);
	double V_T = QSP_CONST(QSP_VOL_T);
	double V_C = QSP_CONST(QSP_VOL_C);
	double V_APC = QSP_CONST(QSP_VOL_APC);
	double N_T = (q.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF, false) 
				+ q.getSystem()->getSpeciesVar(QSP_ID_TUM_TEFF_EXH, false) 
				+ q.getSystem()->getSpeciesVar(QSP_ID_TUM_TREG, false)) / weight_qsp ;
	double N_C = q.getSystem()->getSpeciesVar(QSP_ID_TUM_C1, false)/weight_qsp;
	double N_APC = q.getSystem()->getSpeciesVar(QSP_ID_TUM_APC, false) 
				+ q.getSystem()->getSpeciesVar(QSP_ID_TUM_mAPC, false);
	double V_cells = V_T * N_T + V_C * N_C + V_APC * N_APC;
	double V_tumor = V_cells / (1 - f_vol_tumor);
	/*
	std::cout << V_T << ", " << V_C << ", " << V_APC << ", " ;
	std::cout << N_T << ", " << N_C << ", " << N_APC << std::endl;
	std::cout << V_cells<< std::endl;
	*/
	return V_tumor;
}

/*! calculate tumor volume, with _QSP_model
*/
double LymphCentral::calc_tumor_vol(const double weight_qsp)const{
	return calc_tumor_vol(_QSP_model, weight_qsp);
}

//! get nivolumab concentration (nM)
double LymphCentral::get_nivo_nM(void) const{
	return GET_VAR(QSP_ID_TUM_NIVO) * 1e6;
}

};
};
