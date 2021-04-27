#pragma once

#include "SP_QSP_shared/ABM_Base/ParamBase.h"

#include <string>

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

//! enumerator for double type parameters 
enum ParamFloat{
	// QSP
	PARAM_WEIGHT_QSP,
	PARAM_QSP_EXTRA,
	PARAM_QSP_STEADYSTATE,
	// ENV
	PARAM_SEC_PER_TIME_SLICE,
	PARAM_REC_SITE_FACTOR,
	// T cell
	PARAM_T_CELL_LIFE_MEAN,
	PARAM_T_CELL_LIFE_SD,
	PARAM_T_CELL_MOVE_PROB,
	PARAM_IL_2_RELEASE_TIME,
	PARAM_IL_2_PROLIF_TH,
	PARAM_IFN_RELEASE_TIME,
	// Treg
	PARAM_TREG_MOVE_PROB,
	PARAM_T_REG_LIFE_MEAN,
	// Cancer cell
	PARAM_CANCER_STEM_ASYMMETRIC_DIV_PROB,
	PARAM_CANCER_PROG_GROWTH_RATE,
	PARAM_CANCER_SENESCENT_DEATH_RATE,
	PARAM_CANCER_STEM_KILL_FACTOR,
	PARAM_CANCER_STEM_MOVE_PROB,
	PARAM_CANCER_CELL_MOVE_PROB,
	PARAM_IFN_G_UPTAKE,
	// Agent cytokine 
	PARAM_PDL1_HIGH_TH,
	PARAM_IFN_G_PDL1_HALF,
	PARAM_IFN_G_PDL1_N,
	PARAM_PDL1_DECAY_DAY,
	// diffusion grid
	PARAM_IFN_G_DIFFUSIVITY,
	PARAM_IFN_G_RELEASE,
	PARAM_IFN_G_DECAY_RATE,
	PARAM_IL_2_DIFFUSIVITY,
	PARAM_IL_2_RELEASE,
	PARAM_IL_2_DECAY_RATE,
	// dummy
	PARAM_FLOAT_COUNT // dummy for count
};

//! enumerator for int type parameters 
enum ParamInt{
	PARAM_TUMOR_X,
	PARAM_TUMOR_Y,
	PARAM_TUMOR_Z,
	PARAM_VOXEL_SIZE,
	PARAM_N_T_VOXEL,
	PARAM_N_T_VOXEL_C,
	PARAM_SHUFFLE_CELL_VEC_INTERVAL,
	PARAM_SHIFTGRID_INTERVAL,
	PARAM_T_DIV_INTERVAL,
	PARAM_T_DIV_LIMIT,
	PARAM_CANCER_CELL_PROGENITOR_DIV_MAX,
	// lymphatic compartment
	// molecular
	PARAM_MOLECULAR_STEP_PER_SLICE,
	// dummy
	PARAM_INT_COUNT // dummy for count
};

//! enumerator for boolean type parameters 
enum ParamBool{
	PARAM_QSP_RESECTION,
	PARAM_ALL_MOLECULAR_OFF,
	PARAM_DIFFUSION_OFF,
	PARAM_BOOL_COUNT
};

//! parameters not directly from paramter file (float type, place holder)
enum ParamFloatInternal{
	PARAM_VOXEL_SIZE_CM,
	PARAM_CANCER_SENESCENT_MEAN_LIFE,
	PARAM_T_CELL_LIFE_MEAN_SLICE,
	PARAM_T_CELL_LIFE_SD_SLICE,
	PARAM_PDL1_DECAY_SLICE,
	// parameters calculated from QSP param
	PARAM_AVOGADROS,
	PARAM_PD1_PDL1_HALF,
	PARAM_PD1_SYN,
	PARAM_PDL1_SYN_MAX,
	PARAM_PDL1_K1,
	PARAM_PDL1_K2,
	PARAM_PDL1_K3,
	PARAM_N_PD1_PDL1,
	PARAM_ESCAPE_BASE,
	PARAM_EXHUAST_BASE_PDL1,
	PARAM_EXHUAST_BASE_TREG,
	PARAM_RESECT_TIME_STEP,
	PARAM_REC_PORT_PROB,
	PARAM_TEFF_RECRUIT_K,
	PARAM_TREG_RECRUIT_K,
	PARAM_C1_MIN,
	PARAM_TUM_MAX_C,
	PARAM_TREG_LIFE_MEAN,
	PARAM_CSC_GROWTH_RATE,
	PARAM_FLOAT_INTERNAL_COUNT
};

//! parameters not directly from paramter file (int type, place holder)
enum ParamIntInternal{
	PARAM_INT_CANCER_CELL_PROGENITOR_DIV_INTERVAL_SLICE,
	PARAM_INT_CANCER_CELL_STEM_DIV_INTERVAL_SLICE,
	PARAM_INT_INTERNAL_COUNT
};

//! parameters not directly from paramter file (boolean type)
enum ParamBoolInternal{
	PARAM_MOLECULAR_MODULES_ON,
	PARAM_DIFFUSION_ON,
	PARAM_IFN_SINK_ON,
	PARAM_BOOL_INTERNAL_COUNT
};
//! Model prameters
class Param: public ParamBase
{
public:
	Param();
	~Param(){};
	//! get parameter value (float)
	inline double getVal(ParamFloat n) const { return _paramFloat[n];};
	//! get parameter value (int)
	inline int getVal(ParamInt n) const { return _paramInt[n]; };
	//! get parameter value (bool)
	inline bool getVal(ParamBool n) const { return _paramBool[n]; };

	inline double getVal(ParamFloatInternal n) const { return _paramFloatInternal[n];};
	inline int getVal(ParamIntInternal n) const { return _paramIntInternal[n]; };
	inline bool getVal(ParamBoolInternal n) const { return _paramBoolInternal[n]; };

	//! update from QSP parameters
	void update_from_qsp(void);

private:

	//! setup content of _paramDesc
	virtual void setupParam();
	//! process all internal parameters
	virtual void processInternalParams();
};

};
};

