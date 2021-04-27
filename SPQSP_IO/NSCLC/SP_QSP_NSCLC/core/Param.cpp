#include "Param.h"
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <math.h>
#include "../ode/Param.h"
#include "../ode/ODE_system.h"

// parameters from QSP module. 
extern QSP_IO::Param qsp_params;

#define QP(x) QSP_IO::ODE_system::get_class_param(x)
#define AVOGADROS 6.022140857E23 

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

namespace pt = boost::property_tree;

static int SEC_PER_DAY = 86400;
static int HOUR_PER_DAY = 24;

// must match the order in the ParamInt and ParamFloat enums
#define PARAM_DESCRIPTION_FIELD_COUNT 3
const char* _description[][PARAM_DESCRIPTION_FIELD_COUNT] =
{
	//{"fullpath", "desc", "constraint"}

	//------------------------ float --------------------------//
	/* QSP */
	{ "Param.QSP.simulation.weight_qsp", "", "prob" },
	{ "Param.QSP.simulation.qsp_extra", "additional scaling factor, dimensionless", "pos" },
	{ "Param.QSP.simulation.t_steadystate", "", "pos" },
	/* ABM */
	//environmental
	{ "Param.ABM.Environment.SecPerSlice", "", "pos" },
	{ "Param.ABM.Environment.recSiteFactor", "number of adhesion site per port voxel", "pos" },
	//T cell
	{ "Param.ABM.TCell.lifespanMean", "days", "pos" },
	{ "Param.ABM.TCell.lifespanSD", "days", "pos" },
	{ "Param.ABM.TCell.moveProb", "", "pr" },
	{ "Param.ABM.TCell.IL2_release_time", "amount of time to release IL2 after stimulation, sec", "pos" },
	{ "Param.ABM.TCell.IL2_prolif_th", "accumulative IL2 exposure to proliferate, sec*ng/mL", "pos" },
	{ "Param.ABM.TCell.IFNg_release_time", "amount of time to release IFNg after stimulation, sec", "pos" },
	// Treg
	{ "Param.ABM.Treg.moveProb", "", "pr" },
	{ "Param.ABM.Treg.lifespanMean", "days", "pos" },
	//cancer cell
	{"Param.ABM.CancerCell.asymmetricDivProb", "", "pr"},
	{"Param.ABM.CancerCell.progGrowthRate", "per day", "pos"},
	{"Param.ABM.CancerCell.senescentDeathRate", "per day", "pos"},
	{"Param.ABM.CancerCell.cscKillFactor", "dimensionless", "pos"},
	{"Param.ABM.CancerCell.moveProb_csc", "", "pr"},
	{"Param.ABM.CancerCell.moveProb", "", "pr"},
	{"Param.ABM.CancerCell.IFNgUptake", "per sec", "pos"},
	//agent chemokine interaction
	{"Param.ABM.cell.PDL1_th", "percent of max PDL1_syn to be detectable", "prob"},
	{"Param.ABM.cell.IFNg_PDL1_half", "c of IFNg to induce PDL1 to half maximal level, ng/mL", "pos"},
	{"Param.ABM.cell.IFNg_PDL1_n", "hill coef for PDL1 expression", "pos"},
	{"Param.ABM.cell.PDL1_halflife", "halflife of PDL1 expression, days", "pos"},
	/* molecular level */
	// diffusion grid
	{"Param.Molecular.biofvm.IFNg.diffusivity","micron^2/sec", "pos"},
	{"Param.Molecular.biofvm.IFNg.release","ng/sec, one cell", "pos"},
	{"Param.Molecular.biofvm.IFNg.decayRate","1/sec", "pos"},
	{"Param.Molecular.biofvm.IL_2.diffusivity","micron^2/sec", "pos"},
	{"Param.Molecular.biofvm.IL_2.release","ng/sec, one cell", "pos"},
	{"Param.Molecular.biofvm.IL_2.decayRate","1/sec", "pos"},

	//------------------------ int ----------------------------//
	{"Param.ABM.Environment.Tumor.XSize", "", "pos"},
	{"Param.ABM.Environment.Tumor.YSize", "", "pos"},
	{"Param.ABM.Environment.Tumor.ZSize", "", "pos"},
	{"Param.ABM.Environment.Tumor.VoxelSize", "voxel resolution, microns", "pos"},
	{"Param.ABM.Environment.Tumor.nr_T_voxel", "", "pos"},
	{"Param.ABM.Environment.Tumor.nr_T_voxel_C", "", "pos"},
	{"Param.ABM.Environment.ShuffleInterval", "", "pos"},
	{"Param.ABM.Environment.gridshiftInterval", "", "pos"},
	{"Param.ABM.TCell.div_interval", "", "pos"},
	{"Param.ABM.TCell.div_limit", "", "pos"},
	{"Param.ABM.CancerCell.progenitorDivMax", "", "pos"},
	{"Param.Molecular.stepPerSlice","", "pos"},

	// ---------------------- bool -----------------------------//
	{"Param.QSP.simulation.use_resection", "", "" },
	{"Param.Molecular.allMolecularOff", "", ""},
	{"Param.Molecular.diffusionOff", "", ""},
};

Param::Param()
	:ParamBase()
{
	setupParam();
}

/*! Setup parameter storage
	instantiation of pure virtual member of the base class.
	setup description vector;
	initialize parameter value vectors with 0/false, 
	with size determined by enums, 
	so that other base class members can access vector sizes
*/
void Param::setupParam(){

	size_t nrExternalParam = PARAM_FLOAT_COUNT + PARAM_INT_COUNT + PARAM_BOOL_COUNT;
	for (size_t i = 0; i < nrExternalParam; i++)
	{
		_paramDesc.push_back(std::vector<std::string>(_description[i], 
			_description[i]+ PARAM_DESCRIPTION_FIELD_COUNT));
	}
	_paramFloat = std::vector<double>(PARAM_FLOAT_COUNT, 0);
	_paramInt= std::vector<int>(PARAM_INT_COUNT, 0);
	_paramBool= std::vector<bool>(PARAM_BOOL_COUNT, false);
	_paramFloatInternal = std::vector<double>(PARAM_FLOAT_INTERNAL_COUNT, 0);
	_paramIntInternal = std::vector<int>(PARAM_INT_INTERNAL_COUNT, 0);
	_paramBoolInternal = std::vector<bool>(PARAM_BOOL_INTERNAL_COUNT, false);
}

/*! Calculate internal parameters
*/
void Param::processInternalParams(){
	
	_paramFloatInternal[PARAM_AVOGADROS] = AVOGADROS;

	//micrometer to cm
	_paramFloatInternal[PARAM_VOXEL_SIZE_CM] = _paramInt[PARAM_VOXEL_SIZE] / 1e4;

	_paramFloatInternal[PARAM_T_CELL_LIFE_MEAN_SLICE] = _paramFloat[PARAM_T_CELL_LIFE_MEAN]
		/ _paramFloat[PARAM_SEC_PER_TIME_SLICE] * SEC_PER_DAY;

	_paramFloatInternal[PARAM_T_CELL_LIFE_SD_SLICE] = _paramFloat[PARAM_T_CELL_LIFE_SD]
		/ _paramFloat[PARAM_SEC_PER_TIME_SLICE] * SEC_PER_DAY;

	_paramFloatInternal[PARAM_PDL1_DECAY_SLICE] = std::exp(-_paramFloat[PARAM_PDL1_DECAY_DAY]
		* _paramFloat[PARAM_SEC_PER_TIME_SLICE] / SEC_PER_DAY);

	_paramBoolInternal[PARAM_MOLECULAR_MODULES_ON]
		= !getVal(PARAM_ALL_MOLECULAR_OFF);

	_paramBoolInternal[PARAM_DIFFUSION_ON] 
		= getVal(PARAM_MOLECULAR_MODULES_ON) && !getVal(PARAM_DIFFUSION_OFF);
	
	_paramBoolInternal[PARAM_IFN_SINK_ON] 
		= getVal(PARAM_DIFFUSION_ON) && getVal(PARAM_IFN_G_UPTAKE)>0;

}

//! update from QSP parameters
void Param::update_from_qsp(void){

	// number of PD1/PDL1 binding for half maximal inhibition 
	_paramFloatInternal[PARAM_PD1_PDL1_HALF] = QP(77);

	// total number of PD1 per synapse
	_paramFloatInternal[PARAM_PD1_SYN] = QP(72)*QP(34)/QP(32);

	// total number of PDL1 per synapse
	_paramFloatInternal[PARAM_PDL1_SYN_MAX] = QP(73)*QP(34)/QP(31);
	
	// k1 for PDL1-PD1 calculation
	_paramFloatInternal[PARAM_PDL1_K1] = 1 / QP(75) / QP(34);

	// k2 for PDL1-PD1 calculation
	_paramFloatInternal[PARAM_PDL1_K2] = 2 / QP(58) / QP(22);

	// k3 for PDL1-PD1 calculation
	_paramFloatInternal[PARAM_PDL1_K3] = QP(63) / (2 * QP(58) * QP(35) * QP(34) * QP(6));
	// hill coefficient
	_paramFloatInternal[PARAM_N_PD1_PDL1] = QP(80);

	/*
	std::cout << "k1, k2, k3, T1, PDL1_tot" 
		<< ": " << _paramFloatInternal[PARAM_PDL1_K1]
		<< ", " << _paramFloatInternal[PARAM_PDL1_K2]
		<< ", " << _paramFloatInternal[PARAM_PDL1_K3]
		<< ", " << _paramFloatInternal[PARAM_PD1_SYN]
		<< ", " << _paramFloatInternal[PARAM_PDL1_SYN_MAX]
		<< std::endl;
	std::cout << "k50: " << _paramFloatInternal[PARAM_PD1_PDL1_HALF]<< std::endl;
	*/

	// Parameters calculated from QSP parameter values
	double t_step_sec = _paramFloat[PARAM_SEC_PER_TIME_SLICE];

	// T cell killing of Cancer cell
	// QP(40): k_C_death_by_T (day^-1, sec^-1 internal)
	_paramFloatInternal[PARAM_ESCAPE_BASE] = std::exp(-t_step_sec * QP(40));

	// T cell exhaustion from PDL1
	_paramFloatInternal[PARAM_EXHUAST_BASE_PDL1] = std::exp(-t_step_sec * QP(125));

	// T cell exhaustion from Treg inhibition 
	_paramFloatInternal[PARAM_EXHUAST_BASE_TREG] = std::exp(-t_step_sec * QP(149));

	// time for resection
	_paramFloatInternal[PARAM_RESECT_TIME_STEP] = QP(47) / t_step_sec;
	// Recruitment
	/* density of adhesion sites in tumor vasculature is calculated based on QSP parameter S_adhesion_tot
	*/
	//The number of adhesion site per voxel is:
	double site_per_voxel = std::pow(double(_paramInt[PARAM_VOXEL_SIZE]) / 1e6, 3) * QP(129) * AVOGADROS;
	//The number of adhesion site represented by each ABM recruitment port
	double site_per_port = _paramFloat[PARAM_REC_SITE_FACTOR];
	//percentage of voxels to be assigned as ports
	_paramFloatInternal[PARAM_REC_PORT_PROB] = site_per_voxel / site_per_port;

	// T effector recruitment
	/* for each mole of adhesion site, the amount of T cell recruited (in unit of mole):
	dt * k_transmig * Cent.T * (Tum.C1/K_C_Max) * vol_tum_max * f_vol_BV / vol_cent 
	# Weighted QSP:
	Tum.C1 and K_C_Max are both weighted and cancel out each other;
	vol_tum_max is also weighted (when initializing K_C_Max) and need to be reversed.
	# Units:
	All parameters come in SI units.
	Cent.T should use SI units
	Tum.C1 / K_C_max will be available as _f_tum_cap (dimensionless)

	The result is mole recruited per mole site, or number per site, so no conversion needed.
	When calculating recruitment probability:
	p = k (1/mol) * _f_tum_cap (1) * Tum.T (mol)
	*/
	double  w = _paramFloat[PARAM_WEIGHT_QSP];
	// Teff
	_paramFloatInternal[PARAM_TEFF_RECRUIT_K] = QP(127) * t_step_sec * site_per_port * QP(24) * QP(18) / w / QP(14); 
	// Treg
	_paramFloatInternal[PARAM_TREG_RECRUIT_K] = QP(147) * t_step_sec * site_per_port * QP(24) * QP(18) / w / QP(14);

	/*
	std::cout << "recruitment: Teff" << _paramFloatInternal[PARAM_TEFF_RECRUIT_K] 
		<< ", Treg: " << _paramFloatInternal[PARAM_TREG_RECRUIT_K] << std::endl;
	*/

	// min cancer cell number
	_paramFloatInternal[PARAM_C1_MIN] = QP(36)*AVOGADROS;
	// maximum cancer number
	_paramFloatInternal[PARAM_TUM_MAX_C] = QP(38)*AVOGADROS;

	// mean life of Treg, unit: time step
	double treg_life_qsp = 1 / QP(145) / t_step_sec;
	double treg_life_abm = _paramFloat[PARAM_T_REG_LIFE_MEAN] 
			/ _paramFloat[PARAM_SEC_PER_TIME_SLICE] * SEC_PER_DAY;
	_paramFloatInternal[PARAM_TREG_LIFE_MEAN] = treg_life_abm;
	//_paramFloatInternal[PARAM_TREG_LIFE_MEAN] = treg_life_qsp;

	//std::cout << "Treg life (slice): qsp: " << treg_life_qsp << ", abm: " << treg_life_abm << std::endl;
	/*
	std::cout
	<< t_step_sec << ", " << QP(40) << ", " << QP(148) << "\n"
	<< "PARAM_ESCAPE_BASE, " << _paramFloatInternal[PARAM_ESCAPE_BASE] << "\n"
	<< "PARAM_EXHUAST_BASE_PDL1, " << _paramFloatInternal[PARAM_EXHUAST_BASE_PDL1] << "\n"
	<< "PARAM_EXHUAST_BASE_TREG, " << _paramFloatInternal[PARAM_EXHUAST_BASE_TREG] << "\n"
	<< std::endl;
	*/

	/*Cancer cell dynamics parameters*/

	// stem cell division rate is calculated from QSP parameter
	// unit: s^-1
	double rs = QP(37) / (1 - _paramFloat[PARAM_CANCER_STEM_ASYMMETRIC_DIV_PROB]);
	// unit: day^-1
	_paramFloatInternal[PARAM_CSC_GROWTH_RATE] = rs * SEC_PER_DAY;

	_paramIntInternal[PARAM_INT_CANCER_CELL_STEM_DIV_INTERVAL_SLICE]
		= int(std::log(2)/rs / getVal(PARAM_SEC_PER_TIME_SLICE) + .5);

	_paramFloatInternal[PARAM_CANCER_SENESCENT_MEAN_LIFE] =
		1 / _paramFloat[PARAM_CANCER_SENESCENT_DEATH_RATE]
		/ _paramFloat[PARAM_SEC_PER_TIME_SLICE] * SEC_PER_DAY;
		

	_paramIntInternal[PARAM_INT_CANCER_CELL_PROGENITOR_DIV_INTERVAL_SLICE]
		= int(std::log(2)/ getVal(PARAM_CANCER_PROG_GROWTH_RATE) 
		* SEC_PER_DAY / getVal(PARAM_SEC_PER_TIME_SLICE) + .5);

	/*
	std::cout << getVal(PARAM_FLOAT_CANCER_SENESCENT_DEATH_PROB)
		<< ", " << getVal(PARAM_INT_CANCER_CELL_PROGENITOR_DIV_INTERVAL_SLICE) 
		<< ", " << rs << ": " << getVal(PARAM_INT_CANCER_CELL_STEM_DIV_INTERVAL_SLICE) << std::endl;
	std::cout << getVal(PARAM_INT_CANCER_CELL_STEM_DIV_INTERVAL_SLICE)
		<< ", "<< getVal(PARAM_CANCER_SENESCENT_MEAN_LIFE)
		<< ", "<< getVal(PARAM_INT_CANCER_CELL_PROGENITOR_DIV_INTERVAL_SLICE)
		<< std::endl;
	*/

	return;
}

};
};