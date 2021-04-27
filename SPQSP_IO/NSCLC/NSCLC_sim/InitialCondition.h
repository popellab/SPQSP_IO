#pragma once
#include "SP_QSP_shared/ABM_Base/ParamBase.h"


//! enumerator for double type parameters 
enum ICFloat{
	IC_FRACTION_MARGIN,
	IC_FRACTION_MARGIN_RES,
	// cell density: core
	IC_DENSITY_CORE_TCELL,
	IC_DENSITY_CORE_CANCER,
	IC_DENSITY_CORE_TREG,
	// cell density: margin
	IC_DENSITY_MARGIN_TCELL,
	IC_DENSITY_MARGIN_CANCER,
	IC_DENSITY_MARGIN_TREG,
	// 
	// entry point multiplier
	IC_CORE_TUMOR_VAS_FOLD,
	IC_MARGIN_TUMOR_VAS_FOLD,
	IC_MARGIN_NORMAL_VAS_FOLD,
	// other
	IC_MARGIN_GRID_SHIFT_TH,
	// dummy
	IC_FLOAT_COUNT // dummy for count
};

//! enumerator for int type parameters 
enum ICInt{
	// margin boundary for cancer cell
	IC_MARGIN_CANCER_BOUNDARY,
	// dummy
	IC_INT_COUNT // dummy for count
};

//! enumerator for boolean type parameters 
enum ICBool{
	// core
	IC_CORE_STATIONARY,
	IC_CORE_GRID_SHIFT,
	// margin
	IC_MARGIN_STATIONARY,
	IC_MARGIN_GRID_SHIFT,
	// dummy
	IC_BOOL_COUNT
};

class InitialCondition :
	public SP_QSP_IO::ParamBase
{
public:
	InitialCondition();
	~InitialCondition();
	//! get parameter value (float)
	inline double getVal(ICFloat n) const { return _paramFloat[n];};
	//! get parameter value (int)
	inline int getVal(ICInt n) const { return _paramInt[n]; };
	//! get parameter value (bool)
	inline bool getVal(ICBool n) const { return _paramBool[n]; };
private:

	//! setup content of _paramDesc
	void setupParam();
	//! process all internal parameters
	virtual void processInternalParams(){};
};

