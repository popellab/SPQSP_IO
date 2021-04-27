#pragma once

#include "Cell_Tumor.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>


namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

class Fib:
	public Cell_Tumor
{
public:
	Fib(){};
	Fib(SpatialCompartment* c );
	Fib(const Fib& c);
	virtual ~Fib();

	virtual CellAgent* createCellCopy() const { return new Fib(*this); };

	//! print cancer cell information
	virtual std::string toString() const;

	//! step function for cancer cell
	//virtual void agentStep(double t, double dt, AgentStep & as);
	//! get cell agent type, in this case cancer cell.
	virtual AgentType getType() const { return AgentTypeEnum::CELL_TYPE_FIB; };

private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);
};

//BOOST_CLASS_EXPORT_KEY(Fib)

template<class Archive>
inline void Fib::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Cell_Tumor);
}

};
};
