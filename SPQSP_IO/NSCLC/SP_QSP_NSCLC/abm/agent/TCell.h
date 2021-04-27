#pragma once

#include "Cell_Tumor.h"
#include "../../core/AgentEnum.h"
#include "../../pde/DiffuseGrid.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

class TCell :
	public Cell_Tumor
{
public:
	TCell();
	TCell(SpatialCompartment* c);
	TCell(const TCell & c);
	virtual ~TCell();
	virtual CellAgent* createCellCopy() const { return new TCell(*this); };

	static int getTCellLife();
	//! PD1_PDL1 bond in synapse
	static double get_PD1_PDL1(double PDL1, double Nivo);
	//! get suppression from PD1_PDL1 bond
	static double get_PD1_supp(double bond, double n);
	//! calculate probability of Cancer cell killed by Teff
	static double get_kill_prob(double supp, double q);
	//! probability of turning exhausted from PDL1-PD1 interaction
	static double get_exhaust_prob_PDL1(double supp, double q);
	//! probability of turning exhausted from Treg
	static double get_exhaust_prob_Treg(double supp, double q);

	//! print T cell information
	virtual std::string toString() const;

	//! set to suppressed state
	void set_suppressed(void);

	//! one ABM step for T cells
	//virtual void agentStep(double t, double dt, AgentStep & as);
	virtual bool agent_movement_step(double t, double dt, Coord& c);
	void agent_state_scan(void);
	virtual bool agent_state_step(double t, double dt, Coord& c);
	//! move sources (IFN and IL2)
	void move_all_source_sink(void)const;

	//! remove all sources
	void remove_all_source_sink(void);

	//! molecular step
	virtual void molecularStep(double t, double dt) {};
	//! ODE step overiding base class empty version
	virtual void odeStep(double t, double dt) {};

	virtual AgentType getType() const { return AgentTypeEnum::CELL_TYPE_T; };

	void inc_neighbor_cancer(void){ _count_neighbor_cancer++; };
	void inc_neighbor_Treg(void){ _count_neighbor_Treg++; };
	void update_neighbor_PDL1(double PDL1);


	//! return source pointer
	BioFVMSinkSource*& get_source_IFNg(void) { return _source_IFNg; };
	BioFVMSinkSource*& get_source_IL_2(void) { return _source_IL_2; };

	//! T cell to stream
	friend std::ostream & operator<<(std::ostream &os, const TCell & t);

	//! get the shape of T cell
	//const ShapeSingle* getCellShape()const{ return &TCell::_shapeT; };


private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);



	//! flag that divison is to occur
	bool _divide_flag;
	//! cool down timer for division
	int _divide_cd;
	//! number of remaining division
	int _divide_limit;
	//! accumulative IL2 exposure, sec*ng/mL 
	double _IL2_exposure;
	//! IL2 release time remaining, sec
	double _IL2_release_remain;
	//! IFNg release time remaining, sec
	double _IFN_release_remain;

	BioFVMSinkSource* _source_IFNg;
	BioFVMSinkSource* _source_IL_2;
	//static ShapeSingle _shapeT;

	/* These variables do not need to be serialized,
		but should be set to 0 with default constructor
		during deserialization.
	*/
	//! number of Cancer cell in neighborhood
	int _count_neighbor_cancer;
	//! number of Treg cell in neighborhood
	int _count_neighbor_Treg;
	//! number of total neighbor
	int _count_neighbor_all;
	//! neighbor PDL1
	double _max_neighbor_PDL1;

};

//BOOST_CLASS_EXPORT_KEY(TCell)

inline std::ostream & operator<<(std::ostream &os, const TCell & t) {
	os << t.getID();
	return os;
}

template<class Archive>
inline void TCell::serialize(Archive & ar, const unsigned int /* version */) {
	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Cell_Tumor);
	ar & BOOST_SERIALIZATION_NVP(_life);
	ar & BOOST_SERIALIZATION_NVP(_divide_flag);
	ar & BOOST_SERIALIZATION_NVP(_divide_cd);
	ar & BOOST_SERIALIZATION_NVP(_divide_limit);
	ar & BOOST_SERIALIZATION_NVP(_IL2_exposure);
	ar & BOOST_SERIALIZATION_NVP(_IL2_release_remain);
	ar & BOOST_SERIALIZATION_NVP(_IFN_release_remain);
	ar & BOOST_SERIALIZATION_NVP(_source_IFNg);
	ar & BOOST_SERIALIZATION_NVP(_source_IL_2);
}

};
};

