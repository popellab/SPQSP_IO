#ifndef __CVODE_BASE__
#define __CVODE_BASE__

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/utility.hpp> /* std::pair */

#include <cvode/cvode.h>               /* prototypes for CVODE fcts., consts.  */
#include <nvector/nvector_serial.h>    /* access to serial N_Vector            */
#include <sunmatrix/sunmatrix_dense.h> /* access to dense SUNMatrix            */
#include <sunlinsol/sunlinsol_dense.h> /* access to dense SUNLinearSolver      */
#include <sundials/sundials_types.h>   /* defs. of realtype, sunindextype      */



#include <cmath>
#include <iostream>
#include <vector>

typedef std::vector< double > state_type;

//! Base class for CVode 
class CVODEBase
{
protected:
	enum EVENT_TRIGGER_ELEM_TYPE {
		TRIGGER_NON_INSTANT,
		TRIGGER_EQ,
		TRIGGER_NEQ
	};

public:
	CVODEBase(); 
	CVODEBase(const CVODEBase & c);
	~CVODEBase();

	//! simulate ODE model
	void simOdeStep(double tStart, double tStep);

	//! examples of optional output
	void PrintFinalStats(void *cvode_mem);

	//! ODE state to stream
	friend std::ostream & operator<<(std::ostream &os, const CVODEBase & ode) ;

	//! species varaible value with original units
	double getSpeciesVar(unsigned int idx, bool raw = true)const;
	//! set species varaible value with original units
	void setSpeciesVar(unsigned int idx, double val, bool raw = true);
	//! non species varaible value with original units
	double getParameterVal(unsigned int idx, bool raw = true)const;
	//! set non species varaible value with original units
	void setParameterVal(unsigned int idx, double val, bool raw = true);

	unsigned int get_num_variables(void)const { return _species_var.size(); };

	//! manually update solver variable values
	void updateVar(void);


protected:

	//! initial setup, prepare memory blocks for the solver
	void setupCVODE();
	//! pure virtual. Pass rhs function and initial conditions to solver, instantiated in derived class
	virtual void initSolver(realtype t0) = 0;
	//! setup variables
	virtual void setupVariables(void) = 0;
	//! setup events 
	virtual void setupEvents(void) = 0;
	//! reset starting time and _y after model is already initialted
	void resetSolver(realtype t0, realtype t1);
	//! copy _species_var and save to _y
	void restore_y();
	//! copy _y and save to _species_var
	void save_y();
	//! update species that are not parf of lhs of ode
	virtual void update_y_other(void) = 0;
	//! evaluate one trigger component 
    virtual bool triggerComponentEvaluate(int i, realtype t, bool curr) = 0;
	//! update trigger conditions when root found
	void updateTriggerComponentConditionsOnRoot(int* rootsFound);
	//! update trigger conditions at time t, without root 
	void updateTriggerComponentConditionsOnValue(realtype t);
	//! evaluate event triggers
	bool evaluateAllEvents(realtype t);
	//! resolve event assignments recursively
	void resolveEvents(realtype t);
	//! reset trigger after evaluation
	void resetEventTriggers();
	//! reset transient trigger conditions to false
	void resetTransient();
	//! trigger condition associated with one root 
	bool getSatisfied(int i);
	//! if trigger condition is '==' or '!='
	bool isTransient(int i);
	//! if trigger condition is '==' 
	bool isTransientEq(int i);
	//! if trigger condition is '!='
	bool isTransientNeq(int i);
	//! evaluate one event trigger
	virtual bool eventEvaluate(int i) = 0;
	//! execute one event
	virtual bool eventExecution(int i, bool delay, realtype& dt) = 0;
	//! get variable value with original unit
	double getVarOriginalUnit(int i) const;
	//! get unit conversion scalor
	virtual realtype get_unit_conversion_species(int i) const = 0;
	//! get unit conversion scalor
	virtual realtype get_unit_conversion_nspvar(int i) const = 0;
	//! check if a variable is allowed to become negative
	virtual bool allow_negative(int i) const {return true;};


	//! some functions defined by SBML interpretor
	inline static double root(double a, double b) { return std::pow(b, 1.0 / a); };

	//! check the return values of CVode related functions
	void check_flag(void *flagvalue, const char *funcname, int opt);

	//! variable species. Species in the left-hand side of ODEs 
	state_type _species_var;
	//! other speceis. Listed as species in SBML, but not in lhs. 
	state_type _species_other;
	//! Non-species variable subject to event assignments 
	state_type _nonspecies_var;
	//! constant species/parameters/compartments 
	//state_type _parameter_const;

	//! number of equations (same as nr of _species_var)
	int _neq;
	//! pointer to variable species, in format compatible with CVode
	N_Vector _y;
	//! number of rootfinding functions
	int _nroot;
	//! number of events
	int _nevent;

	//! delayed events sorted vector
	std::vector<std::pair <realtype, int> > _delayEvents;

	//! SUNMatrix for linear solver 
	SUNMatrix _A;
	//! linear solver object for CVode
	SUNLinearSolver _LS;
	//! solver memory block
	void * _cvode_mem;

	//! event only triggered when g(y, t) = 0. Serialization not needed. 
	std::vector<EVENT_TRIGGER_ELEM_TYPE>  _trigger_element_type;
	//! one event trigger element is satisfied 
	std::vector<bool>  _trigger_element_satisfied;
	//! one event is triggered
	std::vector<bool>  _event_triggered;


private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	bool freeMem();
	//! get next t for potential discontinuity.
	bool getNexTimeDisc(realtype& t);



};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(CVODEBase)

inline std::ostream & operator<<(std::ostream &os, const CVODEBase & ode){
	int nrSpecies = ode._neq + ode._species_other.size();
	for (auto i = 0; i < nrSpecies; i++)
	{
		os << "," << ode.getVarOriginalUnit(i);
	}
	return os;
}

inline bool CVODEBase::getSatisfied(int i) {
	return _trigger_element_satisfied[i];
}

inline bool CVODEBase::isTransient(int i) {
	return (_trigger_element_type[i] == TRIGGER_NON_INSTANT ? false: true);
}
inline bool CVODEBase::isTransientEq(int i) {
	return (_trigger_element_type[i] == TRIGGER_EQ ? true: false);
}
inline bool CVODEBase::isTransientNeq(int i) {
	return (_trigger_element_type[i] == TRIGGER_NEQ ? true: false);
}
template<class Archive>
inline void CVODEBase::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_NVP(_species_var);
	ar & BOOST_SERIALIZATION_NVP(_nonspecies_var);
	ar & BOOST_SERIALIZATION_NVP(_delayEvents);
	ar & BOOST_SERIALIZATION_NVP(_trigger_element_satisfied);
	ar & BOOST_SERIALIZATION_NVP(_event_triggered);
}

#endif
