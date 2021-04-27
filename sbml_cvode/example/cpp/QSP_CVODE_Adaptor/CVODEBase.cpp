#include "CVODEBase.h"

#include <iostream>
#include <sstream>
#include <string>

const int mxstep = 10000;

CVODEBase::CVODEBase()
: _species_var()
, _species_other()
, _nonspecies_var()
, _neq(0)
, _y(NULL)
, _nroot(0)
, _nevent(0)
, _delayEvents()
, _A(NULL)
, _LS(NULL)
, _cvode_mem(NULL)
, _trigger_element_type()
, _trigger_element_satisfied()
, _event_triggered()
{

}

CVODEBase::CVODEBase(const CVODEBase & c)
: _species_var(c._species_var)
, _species_other(c._species_other)
, _nonspecies_var(c._nonspecies_var)
, _neq(c._neq)
, _y(NULL)
, _nroot(0)
, _nevent(0)
, _delayEvents()
, _A(NULL)
, _LS(NULL)
, _cvode_mem(NULL)
, _trigger_element_type()
, _trigger_element_satisfied()
, _event_triggered()
{

}
CVODEBase::~CVODEBase()
{
	freeMem();
}

/*! simulate model for some time
\param [in] tStart: simulation starting time
\param [in] tStep: simulation interval

Before every sub step simulated, the solver is reset
due to the need of updating species concentration from
PDE module. While doing this, t and species are also reset
to accommodate the need to deserialize.

Similarly, after the interval, results are copied to vector
version of container for easier serialization and manipulation
by other modules.

TODO:
The error CV_TOO_CLOSE ("tout too close to t0 to start integration")
is issued if one of the following occurs:
(1) tout == t0, or
(2) |tout - t0| < 2 * eps * max(|t0|, |tout|)
In the future this function may need to be updated so that t0 is set
to 0 every time step, with solver reconstructed instead of reinitiated.
*/
void CVODEBase::simOdeStep(double tStart, double tStep){

	int flag;

	realtype t = tStart;
	realtype tEnd = tStart + tStep;
	realtype t1 = tEnd;

	//std::cout << "beginning: " << t << ", " << tEnd << std::endl;

	// in case events need to be executed at the beginning
	// of a step.
	// This could happen when:
	// 1. t = 0
	// 2. ODE modified externally between steps.
	resolveEvents(t);

	while (t < tEnd){

		bool delayedExecution = false;
		bool discontinuity = false;
		t1 = tEnd;

		realtype tNextDisc = 0;
		bool queueNotEmpty = getNexTimeDisc(tNextDisc);
		//std::cout << "delayed execution time: " << tNextDisc 
		//	<< ", delay: " << queueNotEmpty << std::endl;
		if (queueNotEmpty && tNextDisc < tEnd)
		{
			t1 = tNextDisc;
			delayedExecution = true;
		}
		//std::cout << "cycle: " << t << ", " << t1 << std::endl;

		resetSolver(t, t1);

		flag = CVode(_cvode_mem, t1, _y, &t, CV_NORMAL);

		if (flag == CV_TOO_CLOSE)
		{	// just move forward in this case
			t = t1;
		}
		else {
			check_flag(&flag, "CVode", 1);
		}

		if (flag == CV_ROOT_RETURN)
		{
			int* rootsFound = new int[_nroot];
			CVodeGetRootInfo(_cvode_mem, rootsFound);

			// update variable value triggered events
			updateTriggerComponentConditionsOnRoot(rootsFound);

			discontinuity = evaluateAllEvents(t);
			delete[] rootsFound;
		}
		else {
			/**/
			if (delayedExecution) {
				// update trigger components for non-persistent events
				// this is actually not necessary. we keep it there 
				// just in case.
				updateTriggerComponentConditionsOnValue(t);

				//std::cout << "delay queue size: " << _delayEvents.size() << std::endl;
				int eventToExecute = _delayEvents.back().second;
				realtype ttemp;
				bool delay = eventExecution(eventToExecute, true, ttemp);
				_delayEvents.pop_back();
				//std::cout << "delay queue size: " << _delayEvents.size() << std::endl;
				discontinuity = !delay;
			}
		}

		// either delayed execution or root found 
		if (discontinuity)
		{
			resolveEvents(t);
			resetTransient();
			/*
			std::cout << "t_off: " << _nonspecies_var[3] << std::endl;

			std::cout << "Trigger: ";
			for (auto i = 0; i < _nroot; i++)
			{
				std::cout << _trigger_element_satisfied[i] << ",";
			}
			std::cout << std::endl;*/
		}

		save_y();
		update_y_other();
	}
	std::cout << std::flush;
	std::cerr << std::flush;
	//std::cout << "End of step:" << t << *this << std::endl;
}

void CVODEBase::resolveEvents(realtype t){
	bool discontinuity = true;
	while (discontinuity){
		updateTriggerComponentConditionsOnValue(t);
		discontinuity = evaluateAllEvents(t);
	}
}

/*! Manually update variable values.
	Need to do this after altering variable values
	externally and need to get output before simulating 
	any time step.
*/
void CVODEBase::updateVar(void)
{
	restore_y();
	update_y_other();
}

/*! Setup the solver
This Should be called only once during construction, when no
prior allocation of memory block to solver has taken place.

In this function, serial type containers are constructed;
variable values are copied to serial type container;
memory block is allocated to solver, which is initiated to t=0;
Other settings are configured such as tolerance, user data
and linear solver.

*/
void CVODEBase::setupCVODE(){

	bool res = true;

	_neq = _species_var.size();

	try{
		int flag;

		_y = N_VNew_Serial(_neq);
		check_flag((void *)_y, "N_VNew_Serial", 0);

		//_abstol = N_VNew_Serial(_neq);
		//check_flag((void *)_abstol, "N_VNew_Serial", 0);

		_cvode_mem = CVodeCreate(CV_BDF);
		check_flag((void *)_cvode_mem, "CVodeCreate", 0);

		/* Call CVodeInit to initialize the integrator memory and specify the
		* user's right hand side function in y'=f(t,y), the inital time T0, and
		* the initial dependent variable vector y.
		* need to do this in derived class: f and g are defined in derived
		* class; they are static functions, which cannot be virtual functions
		* so we cannot declare them in the base class.
		*/

		initSolver(0);


		flag = CVodeSetMaxNumSteps(_cvode_mem, mxstep);

		/* Passing the pointer of this system to the solver, so that
		* the parameters etc. can be accessed from the static function f and g.
		* Function f () and g (root finding) need to access object
		* specific version of parameter values.
		* this has some cost on performance. */
		flag = CVodeSetUserData(_cvode_mem, this);
		check_flag(&flag, "CVodeSVtolerances", 1);

		/* Create dense SUNMatrix for use in linear solves */
		_A = SUNDenseMatrix(_neq, _neq);
		check_flag(&flag, "SUNDenseMatrix", 1);

		/* Create dense SUNLinearSolver object for use by CVode */
		_LS = SUNLinSol_Dense(_y, _A);
		check_flag(&flag, "SUNLinSol_Dense", 1);

		/* Call CVodeSetLinearSolver to attach the matrix and linear solver to CVode */
		flag = CVodeSetLinearSolver(_cvode_mem, _LS, _A);
		check_flag(&flag, "CVodeSetLinearSolver", 1);

	}
	catch (std::string s){
		std::cerr << "Initiating CVODE solver, error: " << s;
		exit(1);
	}
	//std::cout << "_neq: " << _neq << std::endl;
}

/*! get the t of the next potential discontinuity in simulation.
This can be:
1. Delay of execution from variable-associated trigger condition
(detected with root finding function)
Time-associated trigger condition in one event used to be handled here.
Now they are delt with in rootfinding function.
*/
bool CVODEBase::getNexTimeDisc(realtype& t){
	/**/
	if (!_delayEvents.empty())
	{
		t = _delayEvents.back().first;
		return true;
	}
	else {
		return false;
	}
	return false;
}
/* update trigger conditions
*/
void CVODEBase::updateTriggerComponentConditionsOnRoot(int* rootsFound) {
	for (auto i = 0; i < _nroot; i++)
	{
		if (isTransient(i))
		{
			if (rootsFound[i] != 0)
			{
				_trigger_element_satisfied[i] = isTransientEq(i);
			}
		}
		else {
			if (rootsFound[i] == 1 && !_trigger_element_satisfied[i])
			{
				_trigger_element_satisfied[i] = true;
			}
			else if (rootsFound[i] == -1 && _trigger_element_satisfied[i])
			{
				_trigger_element_satisfied[i] = false;
			}
		}
	}
	return;
}

void CVODEBase::updateTriggerComponentConditionsOnValue(realtype t) {
	for (auto i = 0; i < _nroot; i++)
	{
		_trigger_element_satisfied[i] = triggerComponentEvaluate(i, t,
			_trigger_element_satisfied[i]);
	}
	//std::cout << std::endl;
	return;
}

/* evaluate event triggers
*/
bool CVODEBase::evaluateAllEvents(realtype t) {
	// evaluate all events, check if they are up for execution.
	//std::cout << "trigger evaluations: " << std::endl;
	bool exec = false;
	for (auto i = 0; i < _nevent; i++)
	{
		bool trigger = eventEvaluate(i);
		//std::cout << "event " << i <<
		//	", evaluation result: " << trigger << std::endl;
		// only execute when evaluation result change from false to true
		if (trigger && !_event_triggered[i])
		{
			realtype dt = 0;
			bool setDelay = eventExecution(i, false, dt);
			/**/
			if (setDelay)
			{
				_delayEvents.push_back(std::make_pair(t + dt, i));
				std::sort(_delayEvents.rbegin(), _delayEvents.rend());
			}
			//std::cout << "execution: event " << i << std::endl;
			// any event executed?
			exec |= !setDelay;
		}
		_event_triggered[i] = trigger;
	}
	return exec;
}
/*  reset triggers
*/
void CVODEBase::resetEventTriggers() {
	resetTransient();
	for (auto i = 0; i < _nevent; i++)
	{
		_event_triggered[i] = eventEvaluate(i);
	}
	return;
}
/*  reset all transient conditions to false
this is called after event evaluation.
*/
void CVODEBase::resetTransient() {
	for (auto i = 0; i < _nroot; i++)
	{
		if (isTransientEq(i))
		{
			_trigger_element_satisfied[i] = false;
		}
		else {
			if (isTransientNeq(i)) {
				_trigger_element_satisfied[i] = true;
			}
		}
	}
}
/*!
* Get and print some final statistics
*/
void CVODEBase::PrintFinalStats(void *cvode_mem)
{
	long int nst, nfe, nsetups, nje, nfeLS, nni, ncfn, netf, nge;
	int flag;

	flag = CVodeGetNumSteps(cvode_mem, &nst);
	check_flag(&flag, "CVodeGetNumSteps", 1);
	flag = CVodeGetNumRhsEvals(cvode_mem, &nfe);
	check_flag(&flag, "CVodeGetNumRhsEvals", 1);
	flag = CVodeGetNumLinSolvSetups(cvode_mem, &nsetups);
	check_flag(&flag, "CVodeGetNumLinSolvSetups", 1);
	flag = CVodeGetNumErrTestFails(cvode_mem, &netf);
	check_flag(&flag, "CVodeGetNumErrTestFails", 1);
	flag = CVodeGetNumNonlinSolvIters(cvode_mem, &nni);
	check_flag(&flag, "CVodeGetNumNonlinSolvIters", 1);
	flag = CVodeGetNumNonlinSolvConvFails(cvode_mem, &ncfn);
	check_flag(&flag, "CVodeGetNumNonlinSolvConvFails", 1);

	flag = CVodeGetNumJacEvals(cvode_mem, &nje);
	check_flag(&flag, "CVDlsGetNumJacEvals", 1);
	flag = CVodeGetNumLinRhsEvals(cvode_mem, &nfeLS);
	check_flag(&flag, "CVDlsGetNumRhsEvals", 1);

	flag = CVodeGetNumGEvals(cvode_mem, &nge);
	check_flag(&flag, "CVodeGetNumGEvals", 1);

	printf("\nFinal Statistics:\n");
	printf("nst = %-6ld nfe  = %-6ld nsetups = %-6ld nfeLS = %-6ld nje = %ld\n",
		nst, nfe, nsetups, nfeLS, nje);
	printf("nni = %-6ld ncfn = %-6ld netf = %-6ld nge = %ld\n \n",
		nni, ncfn, netf, nge);
}

double CVODEBase::getSpeciesVar(unsigned int idx, bool raw) const
{
	if (idx < _species_var.size()){
		if (raw)
		{
			return _species_var[idx] / get_unit_conversion_species(idx);
		}
		else{
			return _species_var[idx];
		}
	}
	else{
		throw std::invalid_argument("Accessing ODE variable: out of range");
	}
}

void CVODEBase::setSpeciesVar(unsigned int idx, double val, bool raw)
{
	if (idx < _species_var.size()){
		if (raw)
		{
			_species_var[idx] = val * get_unit_conversion_species(idx);
		}
		else{
			_species_var[idx] = val;
		}
	}
	else{
		throw std::invalid_argument("Assignment to ODE variable: out of range");
	}
	return;
}

double CVODEBase::getParameterVal(unsigned int idx, bool raw) const
{
	if (idx < _nonspecies_var.size()){
		if (raw)
		{
			return _nonspecies_var[idx] / get_unit_conversion_nspvar(idx);
		}
		else{
			return _nonspecies_var[idx];
		}
	}
	else{
		throw std::invalid_argument("Accessing ODE instance parameter: out of range");
	}
}

void CVODEBase::setParameterVal(unsigned int idx, double val, bool raw)
{
	if (idx < _nonspecies_var.size()){
		if (raw)
		{
			_nonspecies_var[idx] = val * get_unit_conversion_nspvar(idx);
		}
		else{
			_nonspecies_var[idx] = val;
		}
	}
	else{
		throw std::invalid_argument("Assignment to ODE instance parameter: out of range");
	}
	return;
}

/*! copy _species_var to the serial container _y;
reset start time and initial condition.
\param [in] t0: start time
\param [in] t1: end time
*/
void CVODEBase::resetSolver(realtype t0, realtype t1){
	int flag = 0;
	restore_y();
	flag = CVodeSetStopTime(_cvode_mem, t1);
	flag = CVodeReInit(_cvode_mem, t0, _y);
	return;
}
/*! copy variable value from vector to serial
*/
void CVODEBase::restore_y(){
	for (auto i = 0; i < _neq; i++)
	{
		NV_DATA_S(_y)[i] = _species_var[i];
	}
	/* Clear _species_var to save memory
	_species_var.clear(); */
}

/*! copy variable value from serial to vector
*/
void CVODEBase::save_y(){

	//_species_var.resize(_neq, 0);
	for (auto i = 0; i < _neq; i++)
	{
		_species_var[i] = NV_DATA_S(_y)[i] *
			(NV_DATA_S(_y)[i] < 0 ? allow_negative(i) : 1 );
	}
}

/*! get variable value with original unit
*/
realtype CVODEBase::getVarOriginalUnit(int i) const{
	realtype v = 0;
	if (i < _neq){
		v = _species_var[i];
	}
	else{
		v = _species_other[i - _neq];
	}
	v /= get_unit_conversion_species(i);
	return v;
}

/*! free memory blocks.
This include solver memory block
and any other serial type blocks
*/
bool CVODEBase::freeMem(){

	/* Free y and abstol vectors */
	N_VDestroy_Serial(_y);
	//N_VDestroy_Serial(_abstol);

	/* Free integrator memory */
	CVodeFree(&_cvode_mem);
	return true;
}
/*!
* Check function return value...
*   - opt == 0 means SUNDIALS function allocates memory so check if
*            returned NULL pointer
*   - opt == 1 means SUNDIALS function returns a flag so check if
*            flag >= 0
*   - opt == 2 means function allocates memory so check if returned
*            NULL pointer
*/
void CVODEBase::check_flag(void *flagvalue, const char *funcname, int opt)
{
	int *errflag;
	std::stringstream ss;
	bool res = false;

	/* Check if SUNDIALS function returned NULL pointer - no memory allocated */
	if (opt == 0 && flagvalue == NULL) {
		ss << "\nSUNDIALS_ERROR: " << funcname << "() failed - returned NULL pointer\n";
		throw ss.str();
	}

	/* Check if flag < 0 */
	else if (opt == 1) {
		errflag = (int *)flagvalue;
		if (*errflag < 0) {
			ss << "\nSUNDIALS_ERROR: " << funcname << "() failed with flag = " << *errflag << "\n";
			throw ss.str();
		}
	}

	/* Check if function returned NULL pointer - no memory allocated */
	else if (opt == 2 && flagvalue == NULL) {

		ss << "\nMEMORY_ERROR: " << funcname << "() failed - returned NULL pointer\n";
		throw ss.str();
	}
}