#include "FileOutputHub.h"

#include <boost/filesystem.hpp>

using namespace std;

const char* LogFileName = "simulation_log.txt";
const char* StatsFilePrefix = "stats_";
const char* QSP_prefix = "QSP_";
const char* OdeStatsFilePrefix = "odeStats_";
const char* SnapShotDir = "/snapShots";
const char* SaveStateDir = "/savedStates";

FileOutputHub::FileOutputHub(void)
: _seed()
, _outDirBase()
, _log()
, _generalStats()
, _lymph_blood_QSP()
, _odeStatsT()
, _odeStatsCancer()
, _gridSnapshotFstream()
, _makeshiftSteam()
{
	
}

FileOutputHub::~FileOutputHub()
{
	_log.close();
	_generalStats.close();
	_odeStatsT.close();
	_odeStatsCancer.close();
}

/*!
	Process paths and setup output file streams
	\param [in] seed: seed is used as part of the ABM/ODE stats file names
	\param [in] baseDirectory: base directory for all output files

	Simulation log file is saved in the base directory. If a new simulation's output
	directory is set to be the same as an older one, new log will append to the end of
	existing log.

	ABM/ODE stats are placed inside the base directory, which has a file name including
	the seed of the simulation. If output directory exists, stats will be attached to
	existing files. This is because when loading from a saved state, if the seed is the same,
	we assume the simulation is continuation of the saved progress, and append statas to
	the end of existing stats file without writing new headers; otherwise, new header is writen,
	so output directory should be set to a new location.

	all grid snapshot output file are placed in a sub-folder "snapShots".

	all serialization (saved states) are placed in a sub-folder "savedStates".
	*/
void FileOutputHub::setup(long seed, string baseDirectory){

	_seed = seed;
	_outDirBase = baseDirectory;
	
	// create directories
	boost::filesystem::path pSnap(_outDirBase + SnapShotDir);
	boost::filesystem::create_directories(pSnap);// create snapshot directory tree
	
	boost::filesystem::path pState(_outDirBase + SaveStateDir);
	boost::filesystem::create_directories(pState);// create save states directory tree

	// initialize filestreams
	string logFileName = _outDirBase + "/" + LogFileName;
	_log.open(logFileName, ios::out | ios::app);

	string statsFileName;
	statsFileName = _outDirBase + "/" + StatsFilePrefix + to_string(_seed) + ".csv";
	_generalStats.open(statsFileName, ios::out | ios::app);

	string QSP_FileName = _outDirBase + "/" + QSP_prefix + to_string(_seed) + ".csv";
	_lymph_blood_QSP.open(QSP_FileName, ios::out | ios::app);

}


ofstream& FileOutputHub::getStatsFstream(void){ 
	return _generalStats; 
}
/*! Create a new output file stream for grid snapshot and return the pointer
	\param [in] time: used to time stamp the file name
	\param [in] tag: goes to first part of file name 
	use case:
	std::ofstream * snap;
	snap = <FileOutputHub instance>.getNewGridToSnapshot(time, tag);
	*snap << [contents] << std::endl;
	snap->close();
*/
ofstream& FileOutputHub::getNewGridToSnapshotStream(unsigned long time, std::string tag){

	string gridSnapFileName = _outDirBase + SnapShotDir+ "/" + tag + to_string(time) + ".csv";
	_gridSnapshotFstream.open(gridSnapFileName, ios::out | ios::trunc);
	return _gridSnapshotFstream;
}

/*! Create a new output file stream for serialzation and return the pointer
	\param [in] time: used to time stamp the file name
	\param [in] tag: goes to first part of file name 
	use case:
	std::ofstream * pState;
	pState = <FileOutputHub instance>.getNewSaveStateStream(time, tag);
	*pState << BOOST_SERIALIZATION_NVP(item1) << [item2] ...;
	pState->close();
	note that the NVP macro uses the variable name as xml tag; if want to use a different 
	name, use boost::serialization::make_nvp("tag", variable)
*/
ofstream& FileOutputHub::getNewSaveStateStream(unsigned long time, std::string tag){
	string saveStateFileName = _outDirBase + SaveStateDir+ "/" + tag + to_string(time) + ".dat";
	_saveStateStream.open(saveStateFileName, ios::out | ios::trunc);
	return _saveStateStream;
}

/*!
	Create a new output file stream for temporary and general purposes
	\Param [in] tag: file name
	use case:
	std::ofstream * pOFS;
	pOFS = <output overseer instance>.getNewMakeshiftStream(tag);
	*pOFS << ...
	pOFS->CLOSE()
*/
ofstream& FileOutputHub::getNewMakeshiftStream(std::string tag){

	string makeShiftFileName = _outDirBase + "/" + tag;
	_makeshiftSteam.open(makeShiftFileName, ios::out | ios::trunc);
	return _makeshiftSteam;
}