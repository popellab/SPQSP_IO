#pragma once

#include "SP_QSP_shared/ABM_Base/Coord3D.h"
#include "SP_QSP_shared/ABM_Base/BaseAgent.h"
#include "SP_QSP_shared/ABM_Base/RNG.h"
#include "../../core/AgentEnum.h"

#include <boost/serialization/nvp.hpp>

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

/*! This class contains functions that
	populate voxels randomly.
	Can be used during grid initialization or when new 
	voxels enters grid.
*/
class VoxelContentGen
{
private:
typedef BaseAgent::AgentType AgentType;
typedef BaseAgent::AgentState AgentState;

public:
	VoxelContentGen();
	~VoxelContentGen();
	//! populate one voxel with cell agents
	bool get_type_state(const Coord3D& c, RNG& rng, 
		AgentType& type, AgentState& state, int& div)const;
	//! setup variables, equilibrium IC
	void setup(bool stationary, double cancer_prob, 
		int xlim, int ylim, int zlim); 
	//! setup variables, mandate IC
	void setup(double pstem, int xlim, int ylim, int zlim, int x0, int y0, int z0); 
private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	//! same density everywhere
	bool _stationary;
	//! min/max xyz for initial population
	int _x_min;
	int _y_min;
	int _z_min;
	int _x_max;
	int _y_max;
	int _z_max;
	//! probability of subtypes and division numbers
	std::vector<double> _celltype_cdf;
};

template<class Archive>
inline void VoxelContentGen::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_NVP(_stationary);
	ar & BOOST_SERIALIZATION_NVP(_x_min);
	ar & BOOST_SERIALIZATION_NVP(_y_min);
	ar & BOOST_SERIALIZATION_NVP(_z_min);
	ar & BOOST_SERIALIZATION_NVP(_x_max);
	ar & BOOST_SERIALIZATION_NVP(_y_max);
	ar & BOOST_SERIALIZATION_NVP(_z_max);
	ar & BOOST_SERIALIZATION_NVP(_celltype_cdf);
}

};// end of namespace
};