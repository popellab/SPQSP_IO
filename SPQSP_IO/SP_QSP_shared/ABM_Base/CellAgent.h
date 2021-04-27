#ifndef __CELL_AGENT__
#define __CELL_AGENT__

#include <fstream>
#include <string>

#include "BaseAgent.h"
#include "Coord3D.h"
#include "ShapeBase.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace SP_QSP_IO{
typedef Coord3D Coord;
class SpatialCompartment;
//class TumorGeneric; //needed for serialization registry

//! Cellular agent base class
class CellAgent : public BaseAgent 
{
public:
	//! default constructor for serialization
	CellAgent(){};
	CellAgent(SpatialCompartment * c);
	//! copy constructor for proliferation and recruitment
	CellAgent(const CellAgent & c);

	virtual ~CellAgent();
	
	//! create a copy of current cell
	/* use this to create new cells instead of factory method,
		so that each derived cell class can have their own version.
		This is convinent when AgentType enums are to be re-used for 
		different cell types in mutually exclusive scenarios. (e.g. 
		CancerCell and CancerCell_Tumor can both use AgentType::CELL_TYPE_CANCER
	*/
	virtual CellAgent* createCellCopy() const = 0;
	//! pure virtual agent step function
	//virtual void agentStep(double t, double dt, AgentStep & as) = 0;
	virtual bool agent_movement_step(double t, double dt, Coord& c) = 0;
	virtual bool agent_state_step(double t, double dt, Coord& c) = 0;

	//! virtual step function for molecular events associated with an cellular agent, including ode step
	virtual void molecularStep(double t, double dt) = 0;
	//! virtual step function for ODE submodel: do nothing unless overriden by derived class
	virtual void odeStep(double t, double dt){};
	
	//! return ID of this cell agent
	unsigned int getID() const { return _id; };

	//! boolean test if cell is dead
	bool isDead()const { return _dead; } ;
	//! make cell dead
	virtual void setDead(){ _dead = true; };

	//! shape adjusted x coordinate of cell
	virtual double getAdjustedX(){ return _coord.x; };
	//! shape adjusted y coordinate of cell
	virtual double getAdjustedY(){ return _coord.y; };
	//! shape adjusted z coordinate of cell
	virtual double getAdjustedZ(){ return _coord.z; };

	//! set coordinate of cell agent
	virtual void setCoord(const Coord & c);
	//! get coordinate of cell agent
	Coord getCoord(void) const { return _coord; };
	
	//! return remaining life in time steps
	int getCellLife(void)const{ return _life; };
	//! set life count down 
	void setCellLife(int life) { _life = life; };

	//! get the shape of cell. 
	virtual const ShapeBase* getCellShape() const = 0;

	//! if cell can be pushed away
	//virtual bool isRelocatable(ElementType)const = 0;

	virtual std::string toString() const;

	//! class static member variable ID generator needs to be serialized explicitly 
	template<class Archive>
	static void classSerialize(Archive & ar, const unsigned int  version);

protected:
	
	//! pointer back to compartment the agent is in
	SpatialCompartment * _compartment;
	//! coordinate
	Coord _coord;
	//! life
	int _life;

private:

	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	//! static class member varible, generate a unique cell ID
	static unsigned int cIDGenerator;
	//! cell ID
	unsigned int _id;
	//! if cell agent is dead
	bool _dead;

};

inline void CellAgent::setCoord(const Coord & c){
	_coord = c;
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(CellAgent)

template<class Archive>
inline void CellAgent::serialize(Archive & ar, const unsigned int  version ){
	//ar.template register_type<TumorGeneric>();
	//std::cout << "cell agent start" << std::endl;
	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(BaseAgent);
	ar & BOOST_SERIALIZATION_NVP(_compartment);
	ar & BOOST_SERIALIZATION_NVP(_coord);
	ar & BOOST_SERIALIZATION_NVP(_life);
	ar & BOOST_SERIALIZATION_NVP(_id);
	ar & BOOST_SERIALIZATION_NVP(_dead);
}

template<class Archive>
void CellAgent::classSerialize(Archive & ar, const unsigned int  version){
	ar & BOOST_SERIALIZATION_NVP(cIDGenerator);
}

};
#endif

