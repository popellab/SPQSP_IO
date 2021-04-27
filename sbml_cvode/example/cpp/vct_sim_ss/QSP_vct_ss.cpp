/*
################################################################################
#                                                                              #
#                                                                              #
#                                                                              #
################################################################################
*/

#include "MolecularModelCVode.h"
#include "ODE_system.h"
#include "Param.h"

#include <iostream>
#include <string>
#include <algorithm> // min

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

// serialization
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/nvp.hpp>

typedef boost::archive::text_iarchive iax;
typedef boost::archive::text_oarchive oax;

#define SEC_PER_DAY 86400

namespace po = boost::program_options;
typedef MolecularModelCVode<CancerVCT::ODE_system> QSP;
using CancerVCT::Param;

int main(int argc, char* argv[])
{
	std::string _inputParam;
	std::string _outPath;
	std::string _outfile;
	std::string _savePath;
	std::string _loadFile;
	bool _save_stats = false;
	bool _save_steady_state = false;
	bool _load_steady_state = false;
	bool _use_resection = false;

	// command line options
	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help,h", "produce help message")
			("input-file,i", po::value<std::string>(&_inputParam), "parameter file name")
			("output-path,o", po::value<std::string>(&_outPath), "output file path")
			("output-file-name,n", po::value<std::string>(&_outfile), "output file name")
			("save-path,s", po::value<std::string>(&_savePath), "folder to save steady state serialization")
			("load-file,l", po::value<std::string>(&_loadFile), "file to load steady state serialization from")
			("use-resection,r", po::bool_switch(&_use_resection), "to perform resection")
			;
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			return 0;
		}

		if (!vm.count("input-file"))
		{
			std::cout << desc << "\n";
			std::cerr << "no input file specified!\n";
			return 1;
		}
		if (vm.count("output-file-name"))
		{
			_save_stats = true;
			if (!vm.count("output-path"))
			{
				std::cout << desc << "\n";
				std::cerr << "Output path not specified" << std::endl;
				return 1;
			}
		}
		if (vm.count("save-path"))
		{
			_save_steady_state = true;
			CancerVCT::ODE_system::use_steady_state = true;
		}
		if (vm.count("load-file"))
		{
			_load_steady_state = true;
		}
		if (_save_steady_state && _load_steady_state)
		{
			std::cout << desc << "\n";
			std::cerr << "Cannot do both save and load steady-states"
				<< " in the same simulation" << std::endl;
			return 1;
		}
	}catch (std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}

	std::ofstream f;
	// output file
	if (_save_stats)
	{
		boost::filesystem::path pOut(_outPath);
		boost::filesystem::create_directories(pOut);// create output directory 

		std::string output_file_name = _outPath + "/" + _outfile;
		f.open(output_file_name, std::ios::trunc);
	}

	// parameters
	Param params;
	params.initializeParams(_inputParam);

	CancerVCT::ODE_system::setup_class_parameters(params);

	if (_use_resection)
	{
		CancerVCT::ODE_system::use_resection = true;
	}
	// simulation object
	QSP model;
	// update variables from parameter file
	model.getSystem()->setup_instance_tolerance(params);
	model.getSystem()->setup_instance_varaibles(params);
	model.getSystem()->eval_init_assignment();



	int nTeff = 2;//cent.Teff
	
	// load steady-state variable
	if (_load_steady_state)
	{
		// deserialize
		QSP model_ss;
		std::ifstream in_save_ss;
		in_save_ss.open(_loadFile);
		iax iaState(in_save_ss);
		iaState >> BOOST_SERIALIZATION_NVP(model_ss);
		in_save_ss.close();

		// copy species values
		unsigned int n = model_ss.getSystem()->get_num_variables();
		for (size_t i = 0; i < n; i++)
		{
			double v = model_ss.getSystem()->getSpeciesVar(i);
			model.getSystem()->setSpeciesVar(i, v);
		}
		model.getSystem()->eval_init_assignment();
		model.getSystem()->updateVar();
	}

	//std::cout << "resection: " << CancerVCT::ODE_system::use_resection << std::endl;
	// header
	if (_save_stats)
	{
		f << "t" << model.getSystem()->getHeader() << std::endl;
	}

	// solve ODE system
	double t_start = params.getVal(0) * SEC_PER_DAY;
	double t_step = params.getVal(1) * SEC_PER_DAY ;
	int nrStep = int(params.getVal(2));
	auto t_end = t_start + t_step * nrStep;

	f << t_start << model << std::endl;

	while (t_start < t_end)
	{
		double t_remaining = t_end - t_start;
		double t_step_sim = std::min(t_remaining, t_step);

		model.solve(t_start, t_step_sim);

		t_start += t_step_sim;

		if (_save_stats)
		{
			f << t_start << model << std::endl;
		}
		//std::cout << t_start / SEC_PER_DAY << std::endl;
	}

	if (_save_stats)
	{
		f.close();
	}

	// serialization
	if (_save_steady_state)
	{
		boost::filesystem::path pOut(_savePath);
		boost::filesystem::create_directories(pOut);// create output directory 
		// save
		std::ofstream out_save;
		std::string save_file_name = _savePath + "/steady-state.dat";
		out_save.open(save_file_name, std::ios::trunc);
		oax oaState(out_save);
		//CancerVCT::ODE_system::classSerialize(oaState, 0);
		oaState << BOOST_SERIALIZATION_NVP(model);
		out_save.close();
	}
	//std::cout << "End of simulation" << std::endl;

	return 0;
}

