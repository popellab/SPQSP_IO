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

#define SEC_PER_DAY 86400

namespace po = boost::program_options;
typedef MolecularModelCVode<CancerVCT::ODE_system> QSP;
using CancerVCT::Param;

int main(int argc, char* argv[])
{
	std::string _inputParam;
	std::string _outPath;
	std::string _outfile;
	// command line options
	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help,h", "produce help message")
			("input-file,i", po::value<std::string>(&_inputParam), "parameter file name")
			("output-path,o", po::value<std::string>(&_outPath)->default_value("output"), "output file path")
			("output-file-name,n", po::value<std::string>(&_outfile)->default_value("solution.csv"), "output file name")
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

	}catch (std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}

	// output directory 
	boost::filesystem::path pOut(_outPath);
	boost::filesystem::create_directories(pOut);// create output directory 

	// parameters
	Param params;
	params.initializeParams(_inputParam);

	CancerVCT::ODE_system::setup_class_parameters(params);
	// simulation object
	QSP model;
	// update variables from parameter file
	model.getSystem()->setup_instance_tolerance(params);
	model.getSystem()->setup_instance_varaibles(params);
	model.getSystem()->eval_init_assignment();

	//model.getSystem()->updateVar();

	// output file
	std::string output_file_name = _outPath + "/" + _outfile;
	std::ofstream f;
	f.open(output_file_name, std::ios::trunc);

	// header
	f << "t" << model.getSystem()->getHeader() << std::endl;

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
		f << t_start << model << std::endl;
		//std::cout << t_start / SEC_PER_DAY << std::endl;
	}
	f.close();
	//std::cout << "End of simulation" << std::endl;

	return 0;
}

