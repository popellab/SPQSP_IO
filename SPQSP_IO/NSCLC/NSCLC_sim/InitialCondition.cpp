#include "InitialCondition.h"
#include <iostream>

#define PARAM_DESCRIPTION_FIELD_COUNT 3
const char* _description[][PARAM_DESCRIPTION_FIELD_COUNT] =
{
	//{"fullpath", "desc", "constraint"}
	//------------------------ float --------------------------//
	{"Param.IC.margin.fraction", "fraction of volume in invasive margin", "prob"},
	{"Param.IC.margin.fraction_res", "fraction of volume in invasive margin, after resection", "prob"},
	{"Param.IC.core.density.tcell", "t cyt IC density in core", "prob"},
	{"Param.IC.core.density.cancer", "cancer cell IC density in core", "prob"},
	{"Param.IC.core.density.treg", "t reg IC density in core", "prob"},
	{"Param.IC.margin.density.tcell", "t cyt IC density in margin", "prob"},
	{"Param.IC.margin.density.cancer", "cancer cell IC density in margin", "prob"},
	{"Param.IC.margin.density.treg", "t reg IC density in margin", "prob"},
	{"Param.IC.core.vas.fold.tumor", "vascular density of tumor", "prob"},
	{"Param.IC.margin.vas.fold.tumor", "vascular density of tumor", "prob"},
	{"Param.IC.margin.vas.fold.normal", "vascular density of normal tissue", "prob"},
	{"Param.IC.margin.shiftgridTh", "Fraction of initial count before margin compartment grid shift", ""},
	//------------------------ int   --------------------------//
	{"Param.IC.margin.zlim", "cancer cell z axis limit in margin compartment", "pos"},
	//------------------------ bool  --------------------------//
	{"Param.IC.core.stationary", "same densities everywhere in core", ""},
	{"Param.IC.core.shiftgrid", "allow core compartment grid to shift", ""},
	{"Param.IC.margin.stationary", "same densities everywhere in margin", ""},
	{"Param.IC.margin.shiftgrid", "allow margin compartment grid to shift", ""},
};

InitialCondition::InitialCondition()
	:ParamBase()
{
	setupParam();
}


InitialCondition::~InitialCondition()
{
}

void InitialCondition::setupParam(){

	size_t nrExternalParam = IC_FLOAT_COUNT + IC_INT_COUNT + IC_BOOL_COUNT;
	for (size_t i = 0; i < nrExternalParam; i++)
	{
		_paramDesc.push_back(std::vector<std::string>(_description[i], 
			_description[i]+ PARAM_DESCRIPTION_FIELD_COUNT));
	}
	_paramFloat = std::vector<double>(IC_FLOAT_COUNT, 0);
	_paramInt= std::vector<int>(IC_INT_COUNT, 0);
	_paramBool= std::vector<bool>(IC_BOOL_COUNT, false);
	_paramFloatInternal = std::vector<double>(0, 0);
	_paramIntInternal = std::vector<int>(0, 0);
	_paramBoolInternal = std::vector<bool>(0, false);
}

