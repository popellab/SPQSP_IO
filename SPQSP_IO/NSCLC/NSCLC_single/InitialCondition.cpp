#include "InitialCondition.h"
#include <iostream>

#define PARAM_DESCRIPTION_FIELD_COUNT 3
const char* _description[][PARAM_DESCRIPTION_FIELD_COUNT] =
{
	//{"fullpath", "desc", "constraint"}
	//------------------------ float --------------------------//
	{"Param.IC.density.csc", "cancer cell IC density in core", "prob"},
	{"Param.IC.vas.fold.tumor", "vascular density of tumor", "prob"},
	//------------------------ int   --------------------------//
	{"Param.IC.xmin", "cancer cell x axis min", "pos"},
	{"Param.IC.ymin", "cancer cell y axis min", "pos"},
	{"Param.IC.zmin", "cancer cell z axis min", "pos"},
	{"Param.IC.xlim", "cancer cell x axis size", "pos"},
	{"Param.IC.ylim", "cancer cell y axis size", "pos"},
	{"Param.IC.zlim", "cancer cell z axis size", "pos"},
	//------------------------ bool  --------------------------//
	{"Param.IC.stationary", "same densities everywhere in core", ""},
	{"Param.IC.shiftgrid", "allow core compartment grid to shift", ""},
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

