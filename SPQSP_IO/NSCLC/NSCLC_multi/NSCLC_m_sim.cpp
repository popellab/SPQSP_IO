#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp> //include all types plus i/o
#include <boost/program_options.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/nvp.hpp>

#include "SP_QSP_shared/ABM_Base/RNG.h"
#include "NSCLC/SP_QSP_NSCLC/core/Param.h"
#include "NSCLC/SP_QSP_NSCLC/ode/Param.h"
#include "NSCLC_core.h"
#include "InitialCondition.h"

// Global variables
FileOutputHub output_hub;

RNG rng;

//ABM parameters
//extern SP_QSP_IO::Param params;

/*
*/
namespace SP_QSP_IO {
	namespace SP_QSP_NSCLC {
		extern Param params;
	}
};
static auto& params = SP_QSP_IO::SP_QSP_NSCLC::params;
// QSP parameter
QSP_IO::Param qsp_params;

// Initial conditions
InitialCondition ic;

namespace po = boost::program_options;

typedef boost::archive::text_iarchive iax;
typedef boost::archive::text_oarchive oax;

std::string timeStamp() {
	std::stringstream ss;

	auto now = boost::posix_time::second_clock::local_time();
	ss << to_simple_string(now);
	/*
	auto st = std::chrono::system_clock::now();
	auto t = std::chrono::system_clock::to_time_t(st);
	std::tm buf;
	localtime_s(&buf, &t);
	ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
	*/
	return ss.str();
}

int main(int argc, char* argv[])
{

	//std::cout << "start" << std::endl;
	unsigned long _nrStep;
	unsigned long _seedVal;

	std::string _inputParam;
	std::string _qspParam;
	std::string _outPath;
	std::string _outParamFile;
	std::string _init_rand;
	std::string _initCell_0;
	std::string _initCell_1;
	bool ic_rand = true;
	//bool ic_file;

	//MemoryUsage memUsage;

	bool _briefStdOut;

	bool _saveStats;
	unsigned int _saveStatsInterval;

	unsigned int _saveGrid = 0;
	unsigned int _saveGridInterval;

	bool _saveState = false;
	unsigned int _saveStateStart;
	unsigned int _saveStateInterval;

	bool _loadFromSavedState = false;
	std::string _loadStateFile;

	unsigned int _convertArchiveFormat = 0;

	//std::cout << "before arg" << std::endl;
	//std::cout << "starting processing options" << std::endl;
	//--------------------------------------------------------------------
	//                     Process Command Line Options
	//--------------------------------------------------------------------
	try {

		po::options_description desc("Allowed options");
		desc.add_options()
			("help,h", "produce help message")
			("seed,s", po::value<unsigned long>(&_seedVal)->default_value(0), "seed value")
			("time,t", po::value<unsigned long>(&_nrStep)->default_value(0), "total number of steps")
			("param-file,p", po::value<std::string>(&_inputParam), "parameter file name")
			("output-path,o", po::value<std::string>(&_outPath)->default_value("defaultOut"), "output file base path")
			("outParam", po::value<std::string>(&_outParamFile)->default_value("outParam.xml"), "save a copy of parameter file")
			("brief,B", po::bool_switch(&_briefStdOut), "print brief tracking info to stdout")
			("stats,S", po::bool_switch(&_saveStats), "whether to print stats")
			("stats-interval", po::value<unsigned int>(&_saveStatsInterval)->default_value(1), "interval to save stats")
			("grid,G", po::value<unsigned int>(&_saveGrid)->default_value(0), "whether to print grid info. 0: nothing; 1: cell only; 2: grid only; 3: both.")
			("grid-interval", po::value<unsigned int>(&_saveGridInterval)->default_value(1), "interval to print grid information")
			("save-state-start", po::value<unsigned int>(&_saveStateStart), "save state starting slice")
			("save-state-interval", po::value<unsigned int>(&_saveStateInterval), "save state interval")
			("load-state", po::value<std::string>(&_loadStateFile), "load save state file")
			;
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			return 0;
		}

		if (!vm.count("param-file"))
		{
			std::cout << desc << "\n";
			std::cerr << "no input file specified!\n";
			return 1;
		}
		else {
			_qspParam = _inputParam;
		}
		/*
		ic_rand = vm.count("init-cell-random");
		ic_file = vm.count("init-cell-0") && vm.count("init-cell-1");
		if (ic_rand == ic_file)
		{
			std::cout << desc << "\n";
			std::cerr << "initial condition file not properly specified\n";
			return 1;
		}*/
		if (vm.count("save-state-start") != vm.count("save-state-interval"))
		{
			std::cerr << "--save-state-start and --save-state-interval must appear in as a pair" << std::endl;
			return 1;
		}
		else if (vm.count("save-state-start")) {
			_saveState = true;
		}

		if (vm.count("load-state"))
		{
			_loadFromSavedState = true;
		}
		if (_saveGrid > 0 && !vm.count("grid-interval"))
		{
			std::cerr << "Grid output enabled without specifying interval" << std::endl;
			return 1;
		}
	}
	catch (std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}
	catch (...) {
		std::cerr << "Exception of unknown type occured when processing command line options!\n";
		return 1;
	}

	//std::cout << "end processing options" << std::endl;

	//--------------------------------------------------------------------
	//    Initialize Variables; load from saved states if specified
	//--------------------------------------------------------------------

	//std::cout << "arg done" << std::endl;

	// parameters
	qsp_params.initializeParams(_qspParam);
	params.initializeParams(_inputParam);

	//std::cout << "processed parameters" << std::endl << std::flush;
	try {
		// random inital condition
		_init_rand = _inputParam;
		ic.initializeParams(_init_rand);
	}
	catch (std::exception & e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}

	//std::cout << "processed IC" << std::endl << std::flush;

	// initialize output hub 
	output_hub.setup(_seedVal, _outPath);

	//std::cout << "output done" << std::endl;

	params.writeParamsToXml(_outPath + "/" + _outParamFile);

	NSCLC_Core simulation;
	//std::cout << "simulation declared" << std::endl;
	unsigned int sliceStart = 0;

	simulation.setup_qsp(qsp_params);

	bool qsp_ic = simulation.setup_qsp(qsp_params);
	if (!qsp_ic)
	{
		auto& f = output_hub.getNewMakeshiftStream("QSP_IC_FAILURE");
		f.close();
		exit(1);
	}
	else{
		auto& f = output_hub.getNewMakeshiftStream("QSP_IC_SUCCESS");
		f.close();
	}

	//std::cout << "qsp done" << std::endl;
	bool newSeed = false;
	if (_loadFromSavedState)
	{
		try {
			std::ifstream ifState;
			ifState.open(_loadStateFile);
			iax iaState(ifState);

			unsigned long archiveSeed;
			iaState >> boost::serialization::make_nvp("_seedVal", archiveSeed);
			iaState >> BOOST_SERIALIZATION_NVP(rng);
			if (archiveSeed != _seedVal) {
				// if _seedVal (from argument) is different from archiveSeed (from archive)
				newSeed = true;
				rng.seed(_seedVal);
			}
			iaState >> boost::serialization::make_nvp("slice", sliceStart);
			iaState >> BOOST_SERIALIZATION_NVP(simulation);
		}
		catch (std::exception & e) {
			std::cerr << "error: " << e.what() << "\n";
			return 1;
		}
	}
	else {
		try {
			rng.seed(_seedVal);
			if (ic_rand)
			{
				simulation.initializeSimulation();
			}
			else {
				std::cerr << "error: explicit initial arrangement is disabled." << std::endl;
				return 1;
				//simulation.initializeSimulation(_initCell_0, _initCell_1);
			}
		}
		catch (std::exception & e) {
			std::cerr << "error: " << e.what() << "\n";
			return 1;
		}
	}

	//std::cout << "processed initialization" << std::endl;

	// need new stats file header when new simulation or change seeds for loaded state
	if (!_loadFromSavedState || newSeed)
	{
		simulation.write_stats_header();
		simulation.write_QSP(0, true);
	}

	//std::cout << "write headers" << std::endl;
	//--------------------------------------------------------------------
	//                     Entry Log
	//--------------------------------------------------------------------

	auto& logStream = output_hub.getLogFstream();
	logStream << std::endl << "Seed: " << _seedVal << std::endl;
	logStream << "Parameters: " << _inputParam << std::endl;
	logStream << "Start time: " << timeStamp() << std::endl;
	logStream << "Start slice: " << sliceStart << std::endl;

	//std::cout << "After entry log" << std::endl;
	//--------------------------------------------------------------------
	//                     Simulation
	//--------------------------------------------------------------------

	//std::cout << "starting main loop\n"<< std::endl;

	// main loop
	for (size_t slice = sliceStart; slice <= _nrStep; slice++)
	{
		//std::cout << "start" << std::endl;

		if (_saveStats && slice % _saveStatsInterval == 0 && slice != _nrStep)
		{
			simulation.write_stats_slice(slice);
			simulation.write_QSP(slice, false);
		}
		//std::cout << "wirte stats" << std::endl;

		if (_saveGrid && slice% _saveGridInterval == 0)
		{
			simulation.writeGrids(slice, _saveGrid);
		}
		try {
			// save state
			if (_saveState && slice >= _saveStateStart && slice% _saveStateInterval == 0)
			{
				auto& saveStateofs = output_hub.getNewSaveStateStream(slice, "state_");
				oax oaState(saveStateofs);
				oaState << BOOST_SERIALIZATION_NVP(_seedVal);
				oaState << BOOST_SERIALIZATION_NVP(rng);
				oaState << boost::serialization::make_nvp("slice", slice);
				oaState << BOOST_SERIALIZATION_NVP(simulation);
				saveStateofs.close();
			}
		}
		catch (std::exception & e) {
			std::cerr << "error serializing: " << e.what() << "\n";
			return 1;
		}

		// track progress
		if (_briefStdOut) {
			simulation.briefStats(slice);
			//memUsage.reportMemoryUsage();
		}
		unsigned int pctDone = 100 * slice / _nrStep;

		if (pctDone > 100 * (slice - 1) / _nrStep) // not in the current bracket in the previous step
		{
			if (_briefStdOut) {
				std::cout << pctDone << "% finished" << std::endl;
			}

			logStream << pctDone << "%; " << timeStamp() << std::endl;
		}

		//std::cout << "simulate" << std::endl;
		if (slice != _nrStep)
		{
			try {
				simulation.timeSlice(slice);
			}
			catch (std::exception & e) {
				std::cerr << "error in time slice: " << e.what() << "\n";
				return 1;
			}
		}
		//std::cout << "simulate done" << std::endl;

	}

	// termination

	// endPoint stats
	simulation.write_stats_slice(_nrStep);
	simulation.write_QSP(_nrStep, false);

	//--------------------------------------------------------------------
	//                     Exit Log
	//--------------------------------------------------------------------
	logStream << "End time: " << timeStamp() << std::endl;

	auto& f = output_hub.getNewMakeshiftStream("SIMULATION_SUCCESS");
	f.close();

	return 0;

}
