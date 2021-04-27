#ifndef __AGENT_GRID_VOXEL__
#define __AGENT_GRID_VOXEL__

#include <vector>
#include <iostream>

#include <boost/serialization/vector.hpp>

#include "BaseAgent.h"

namespace SP_QSP_IO{
//class TCell;
//class CancerCell;

//! Grid Voxel base class
/*!
  A vector of BaseAgent pointers is stored in each
  AgentGridVoxel object. 
  Derived class from BaseGridVoxel is responsible to implement 
  interacitons between derived classes from BaseAgent.
  Instances of voxels can be used as element of Grid3D template.
*/
class AgentGridVoxel
{
protected:
	typedef BaseAgent::AgentType AgentType;
	typedef BaseAgent::AgentState AgentState;
public:
	AgentGridVoxel();
	virtual ~AgentGridVoxel();

	//! add the pointer of an agent to the voxel.
	bool addAgent(BaseAgent*);
	//! remove the pointer of an agent from the voxel.
	bool removeAgent(BaseAgent*);
	//! find location of agent
	bool locateAgent(BaseAgent*, int&) const;
	//! get ith agent
	BaseAgent* getAgentByIndex(unsigned int i) const;
	//! get number of agents in the voxel.
	int getNumAgents(void)const{ return _agents.size(); }
	//! get agent vector
	std::vector<BaseAgent*>& get_agents(void){ return _agents; };
	//! remove all agents from voxel
	void remove_all_agents(void);

	//! get agent of given type
	bool getAgentByType(AgentType type, std::vector<BaseAgent*> & agVec)const;
	//! get number of agent of given type
	bool countNumAgentByType(AgentType type, int& count, bool checkExist)const;
	//! get agent of given state 
	bool getAgentByState(AgentState state, std::vector<BaseAgent*> & agVec)const;
	//! get number of agent of given state 
	bool countNumAgentByState(AgentState state, int& count, bool checkExist)const;
	//! if this voxel is open to Agent of given type
	virtual bool isOpenToType(AgentType t)const;
	//! content of voxel to stream
	friend std::ostream & operator<<(std::ostream &os, const AgentGridVoxel &voxel); 

protected:
	//! content of voxel
	virtual int getVoxelContent()const;

	//! vector of agents in this voxel.
	std::vector<BaseAgent*> _agents;

private:

	friend class boost::serialization::access;

	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);
};


template<class Archive>
inline void AgentGridVoxel::serialize(Archive & ar, const unsigned int  version ){
	//ar.template register_type<CancerCell>();
	//ar.template register_type<TCell>();
	ar & BOOST_SERIALIZATION_NVP(_agents);
}

inline
std::ostream & operator<<(std::ostream &os, const AgentGridVoxel& voxel) {
	os << voxel.getVoxelContent();
	return os;
}

};
#endif
