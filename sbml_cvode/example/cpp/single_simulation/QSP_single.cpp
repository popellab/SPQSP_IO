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

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>

/*
-------------------------------
 Main Program
-------------------------------
*/

#define TASK_SIM 0
#define TASK_SERIALIZATION 1  // example code for saving and loading a simulation

//#define TASK TASK_SIM
#define TASK TASK_SERIALIZATION 

#define SEC_PER_DAY 86400

int main()
{
	CancerQSP::Param param;

	std::ostream &os = std::cout;

	param.initializeParams("CancerQSP_params.xml");

	double tStart = param.getVal(0);
	double stepInterval = 1.0 * SEC_PER_DAY * param.getVal(1);
	int nrStep = int(param.getVal(2));

	// class parameters
	CancerQSP::ODE_system::setup_class_parameters(param);

	MolecularModelCVode<CancerQSP::ODE_system> tmodel;
	tmodel.getSystem()->setup_instance_tolerance(param);

	tmodel.getSystem()->setup_instance_varaibles(param);
	tmodel.getSystem()->eval_init_assignment();

#if TASK == TASK_SIM
	//------------------------------ simulation ------------------------------//

	std::ofstream fs;
	fs.open("sim_results.csv", std::ios::trunc);

	// header
	fs << "t" << tmodel.getSystem()->getHeader() << std::endl;

	// t = 0
	fs << tStart << tmodel << std::endl;
	// simulation: t: [0:360:1] days
	for (auto i = 0; i < nrStep; i++)
	{
		tmodel.solve(tStart, stepInterval);
		tStart += stepInterval;
		fs << tStart << tmodel << std::endl;
	}
	fs.close();

#elif TASK == TASK_SERIALIZATION
	//------------------------------  serialization ------------------------------//
	typedef boost::archive::xml_iarchive iax;
	typedef boost::archive::xml_oarchive oax;
	std::ofstream fs0, fs1, out_save;
	std::ifstream in_save;

	// simulation: t: [0:180:1] days
	for (auto i = 0; i < nrStep/2; i++)
	{
		tmodel.solve(tStart, stepInterval);
		tStart += stepInterval;
	}

	double tPause = tStart;

	// save data
	out_save.open("save.xml", std::ios::trunc);
	oax oaState(out_save);
	oaState << BOOST_SERIALIZATION_NVP(tmodel);
	CancerQSP::ODE_system::classSerialize(oaState, 0);
	out_save.close();


	// change class parameter to test class serialization
	CancerQSP::ODE_system::set_class_param(47, 250 * SEC_PER_DAY);

	// simulation: t: [180:360:1] days
	fs0.open("sim_results_0.csv", std::ios::trunc);
	fs0 << "t" << tmodel.getSystem()->getHeader() << std::endl;
	tStart = tPause;
	for (auto i = nrStep/2; i < nrStep; i++)
	{
		tmodel.solve(tStart, stepInterval);
		tStart += stepInterval;
		fs0 << tStart << tmodel << std::endl;
	}
	fs0.close();


	// load save
	MolecularModelCVode<CancerQSP::ODE_system> tmodel_2;
	tmodel_2.getSystem()->setup_instance_tolerance(param);
	in_save.open("save.xml");
	iax iaState(in_save);
	iaState >> BOOST_SERIALIZATION_NVP(tmodel_2);
	CancerQSP::ODE_system::classSerialize(iaState, 0);
	in_save.close();

	// simulation: t: [180:360:1] days (load save)
	fs1.open("sim_results_1.csv", std::ios::trunc);
	fs1 << "t" << tmodel_2.getSystem()->getHeader() << std::endl;
	tStart = tPause;
	for (auto i = nrStep/2; i < nrStep; i++)
	{
		tmodel_2.solve(tStart, stepInterval);
		tStart += stepInterval;
		fs1 << tStart << tmodel_2 << std::endl;
	}
	fs1.close();

#endif

	return 0;
}