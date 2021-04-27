#pragma once
#include "SP_QSP_shared/ABM_Base/ParamBase.h"


//! enumerator for double type parameters 
enum ICFloat{
	// cell density: 
	IC_DENSITY_CSC,
	// entry point multiplier
	IC_TUMOR_VAS_FOLD,

	// dummy
	IC_FLOAT_COUNT // dummy for count
};

//! enumerator for int type parameters 
enum ICInt{
	// margin boundary for cancer cell
	IC_X_MIN,
	IC_Y_MIN,
	IC_Z_MIN,
	IC_X_SIZE,
	IC_Y_SIZE,
	IC_Z_SIZE,
	// dummy
	IC_INT_COUNT // dummy for count
};

//! enumerator for boolean type parameters 
enum ICBool{
	// 
	IC_STATIONARY,
	IC_GRID_SHIFT,
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

