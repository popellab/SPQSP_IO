#pragma once

#include "SP_QSP_shared/ABM_Base/CellAgent.h"
#include "../util/ShapeVoxel.h"
#include "../../pde/DiffuseGrid.h"

#include "../../core/AgentEnum.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/base_object.hpp>


namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

class Tumor;

class Cell_Tumor :
	public CellAgent
{
public:
	//! Hill equation
	static double get_Hill_equation(double X, double Ka, double n);
public:
	Cell_Tumor(){};
	Cell_Tumor(SpatialCompartment* c );
	Cell_Tumor(const Cell_Tumor& c);
	virtual ~Cell_Tumor();

	//! print cancer cell information
	virtual std::string toString() const;
	//! get cell coordinates, in real unit
	std::vector<double> get_coord_real(double)const;
	
	Tumor& get_tumor(void)const;

	virtual bool agent_movement_step(double t, double dt, Coord& c){ return false; };
	virtual void agent_state_scan(void){};
	virtual bool agent_state_step(double t, double dt, Coord& c);

	//! molecular step; doing nothing currently
	virtual void molecularStep(double t, double dt){};
	//! ODE step overiding base class empty version
	virtual void odeStep(double t, double dt){};
	//! set coordinates
	//void setCoord(const Coord& c);
	virtual void setCoord(const Coord & c);
	//! setup source
	void setup_chem_source(BioFVMSinkSource*&, chem_ID,  double rate);
	//! update source
	void update_chem_source(BioFVMSinkSource* const,  double rate);
	//! setup sink
	void setup_chem_sink(BioFVMSinkSource*&, chem_ID,  double rate);
	//! move one source/sink
	void move_source_sink(BioFVMSinkSource* const)const;
	//! move sink/source associated with this agent to new location.
	virtual void move_all_source_sink(void)const{};
	//! remove a source/sink from agent
	void remove_source_sink(BioFVMSinkSource*&);
	//! remove all sources
	virtual void remove_all_source_sink(void){};

	virtual void setDead();

	virtual const ShapeVoxel* getCellShape()const{ return &Cell_Tumor::_class_shape; };
	//! remarks to string
	virtual std::string getRemark() const; 

	//! PDL1 amount in synapse
	double get_PDL1(void)const{ return _PDL1_syn; };
	//! high PDL1
	bool is_PDL1_pos(void)const;
	//! set as dropped out
	void set_drop_out(void);
	//! if has dropped out
	bool is_drop_out(void)const{ return _drop_out; };

protected:

	static ShapeVoxel _class_shape;
	//! PDL1 in synapse
	double _PDL1_syn;
	//! dropped out of grid when grid shifts
	bool _drop_out;

private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	//! reset PDL1 level
	void reset_PDL1(void);

};

template<class Archive>
inline void Cell_Tumor::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(CellAgent);
	ar & BOOST_SERIALIZATION_NVP(_PDL1_syn);
	ar & BOOST_SERIALIZATION_NVP(_drop_out);

}

};
};
