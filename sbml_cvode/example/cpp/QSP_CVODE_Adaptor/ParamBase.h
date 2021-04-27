#pragma once

#include <boost/property_tree/ptree.hpp>
#include <vector>
//#define PARAM_DESCRIPTION_FIELDS 3

class ParamBase
{
public:

	ParamBase();
	~ParamBase(){};
	//! initialize parameter object
	void initializeParams(std::string inFileName);
	//! export paramters to xml
	void writeParamsToXml(std::string);
	//! get parameter value (float)

protected:
	//! process float type parameters
	bool processEntryFloat(unsigned int, boost::property_tree::ptree &);
	//! process int type parameters
	bool processEntryInt(unsigned int, boost::property_tree::ptree &);
	//! process bool type parameters
	bool processEntryBool(unsigned int, boost::property_tree::ptree &);

	std::vector<std::vector<std::string>> _paramDesc;
	//! store parameter value (float)
	std::vector<double> _paramFloat;
	//! store parameter value (int)
	std::vector<int> _paramInt;
	//! store parameter value (bool)
	std::vector<bool> _paramBool;

	std::vector<double> _paramFloatInternal;
	std::vector<int> _paramIntInternal;
	//! store parameter value (bool, interval)
	std::vector<bool> _paramBoolInternal;

private:
	//! setup content of _paramDesc
	virtual void setupParam() = 0;
	//! process all internal parameters
	virtual void processInternalParams() = 0;
	//! Parse xml parameter file and store paramter values
	virtual bool readParamsFromXml(std::string inFileName);

};