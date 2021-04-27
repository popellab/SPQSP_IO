#include "ParamBase.h"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <iostream>

namespace SP_QSP_IO{

namespace pt = boost::property_tree;

ParamBase::ParamBase()
	:_paramDesc()
	, _paramFloat()
	, _paramInt()
	, _paramBool()
	, _paramFloatInternal()
	, _paramIntInternal()
	, _paramBoolInternal()
{
}

/*! initializae parameter object
	\param [in] inFileName: parameter file name.
*/
void ParamBase::initializeParams(std::string inFileName){
	
	bool res = readParamsFromXml(inFileName);
	
	if (!res)
	{
		// reading paramter not successful
		std::cerr << "Error reading paramters, exiting" << std::endl;
		exit(1);
	}

	//process internal paramters/*

	processInternalParams();
	
}

/*! read xml paramter file, save in a boost property tree.
	process float, int, and boolean parameters sequntially
	\param [in] inFileName: parameter file name.
*/
bool ParamBase::readParamsFromXml( std::string inFileName){

	
	pt::ptree tree;
	//reading xml
	try{
		pt::read_xml(inFileName, tree, boost::property_tree::xml_parser::trim_whitespace);

		bool res = true;
		for (size_t i = 0; i < _paramFloat.size(); i++)
		{
			res &= processEntryFloat(i, tree);
		}

		for (size_t i = 0; i < _paramInt.size(); i++)
		{
			res &= processEntryInt(i, tree);
		}

		for (size_t i = 0; i < _paramBool.size(); i++)
		{
			res &= processEntryBool(i, tree);
		}
		return res;

	}
	catch(std::exception& e){
		std::cerr << "Error reading " << e.what() << std::endl;
		return false;
	}
}
/*! extract float type parameters from property tree.
	\param [in] i: idx in the float-type parameter enumerator
	\param [in] tree: property tree constructed from xml file
	from path determined in _description, get leaf element's 
	content and save as parameter value.

	Verify parameters values according to tags: "pos" for positive; 
	"pr" for probability, [0, 1].

	attribute of element is reserved for paramter space sweep.
*/
bool ParamBase::processEntryFloat(unsigned int i, boost::property_tree::ptree & tree){

	try{
		double temp = tree.get<double>(_paramDesc[i][0]);
		if ((_paramDesc[i][2] == "pos" && temp < 0) || 
			(_paramDesc[i][2] == "pr" && (temp < 0 || temp > 1)))
		{
			std::cerr <<  "\"" << _paramDesc[i][0] << "\" out of range" << std::endl;
			return false;
		}
		else{
			//std::cout <<  "\"" << _paramDesc[i][0] << "\", value: " << temp << std::endl;

			_paramFloat[i] = temp;
			return true;
		}
	}
	catch (pt::ptree_bad_path){
		std::cerr << "(double type) \"" << _paramDesc[i][0] << "\"not found" << std::endl;
	}
	catch (pt::ptree_bad_data){
		std::cerr << "(double type) \"" << _paramDesc[i][0] << "\"wrong format" << std::endl;
	}
	catch(std::exception& e){
		std::cerr << _paramDesc[i][0] << "Error: " << e.what() << std::endl;
	}
	return false;
	
}

/*! extract int type parameters from property tree.
	\param [in] i: idx in the int type parameter enumerator
	\param [in] tree: property tree constructed from xml file
	from path determined in _description, get leaf element's 
	content and save as parameter value.

	Verify parameters values according to tags: "pos" for positive 

	attribute of element is reserved for paramter space sweep.
*/
bool ParamBase::processEntryInt(unsigned int i, boost::property_tree::ptree & tree){
	
	//unsigned int j = PARAM_FLOAT_COUNT + i;
	size_t nrFloatParam = _paramFloat.size();
	unsigned int j = i + nrFloatParam;
	try{
		int temp = tree.get<int>(_paramDesc[j][0]);
		if ((_paramDesc[j][2] == "pos" && temp < 0)){
			std::cerr <<  "\"" << _paramDesc[j][0] << "\" out of range" << std::endl;
			return false;
		}
		else{
			_paramInt[i] = temp;
			return true;
		}
	}
	catch (pt::ptree_bad_path){
		std::cerr  << "(int type) \"" << _paramDesc[j][0] << "\" not found" << std::endl;
	}
	catch (pt::ptree_bad_data){
		std::cerr << "(int type)\"" << _paramDesc[j][0] << "\" wrong format" << std::endl;
	}
	catch(std::exception& e){
		std::cerr << _paramDesc[j][0] << "Error: " << e.what() << std::endl;
	}
	return false;
}

/*! extract bool type parameters from property tree.
	\param [in] i: idx in the bool type parameter enumerator
	\param [in] tree: property tree constructed from xml file
	from path determined in _description, get leaf element's 
	content and save as parameter value.

	attribute of element is reserved for paramter space sweep.
*/
bool ParamBase::processEntryBool(unsigned int i, boost::property_tree::ptree & tree){
	
	//unsigned int j = PARAM_FLOAT_COUNT + PARAM_INT_COUNT + i;
	size_t nrFloatParam = _paramFloat.size();
	size_t nrIntParam = _paramInt.size();
	unsigned int j = i + nrFloatParam + nrIntParam;
	try{
		bool temp = tree.get<bool>(_paramDesc[j][0]);
		//std::cout <<  "\"" << _paramDesc[j][0] << "\", value: "<< temp << std::endl;
		_paramBool[i] = temp;
		return true;
	}
	catch (pt::ptree_bad_path){
		std::cerr  << "(bool type)\"" << _paramDesc[j][0] << "\" not found" << std::endl;
	}
	catch (pt::ptree_bad_data){
		std::cerr << "(bool type)\"" << _paramDesc[j][0] << "\" wrong format" << std::endl;
	}
	catch(std::exception& e){
		std::cerr << _paramDesc[j][0] << "Error: " << e.what() << std::endl;
	}
	return false;
}

/*!	save parameters values to a xml file
	construct a property tree with the path saved in in _description and add
	parameter values to elements.
	Then write property tree to xml file.
*/
void ParamBase::writeParamsToXml(std::string outFileName){
	
	pt::ptree tree;
	size_t nrFloatParam = _paramFloat.size();
	size_t nrIntParam = _paramInt.size();
	size_t nrBoolParam = _paramBool.size();
	for (size_t i = 0; i < nrFloatParam; i++)
	{
		tree.put(_paramDesc[i][0], _paramFloat[i]);
	}

	for (size_t i = 0; i < nrIntParam; i++)
	{
		tree.put(_paramDesc[nrFloatParam+i][0], _paramInt[i]);
	}
	
	for (size_t i = 0; i < nrBoolParam; i++)
	{
		tree.put(_paramDesc[nrFloatParam + nrIntParam +i][0], _paramBool[i]);
	}

	pt::xml_writer_settings<std::string> settings('\t', 1);
	pt::write_xml(outFileName, tree, std::locale(), settings);

}
};
