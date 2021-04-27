#include "AgentGridVoxel.h"


namespace SP_QSP_IO{

AgentGridVoxel::AgentGridVoxel()
	:_agents()
{
}


AgentGridVoxel::~AgentGridVoxel()
{
}

/*!
  add agent pointer to the voxel.
  \param[in] BaseAgent* ag: pointer of agent to add
  \return: true if successfully added; false if already in.
*/
bool AgentGridVoxel::addAgent(BaseAgent* ag){
	int i;
	if (locateAgent(ag, i))
	{
		return false;
	}
	else{
		_agents.push_back(ag);
		return true;
	}
}

/*
  remove agent pointer from voxel.
  \param[in] BaseAgent* ag: pointer of agent to remove 
  \return: true if successfully removed.
*/
//! remove the pointer of an agent from the voxel.
bool AgentGridVoxel::removeAgent(BaseAgent* ag){

	int i;
	if (locateAgent(ag, i))
	{
		_agents.erase(_agents.begin() + i);
		return true;
	}
	else{
		return false;
	}
}

/* find location of the agent in this voxel.
  \param[in] BaseAgent* ag: pointer to wanted agent
  \param[in,out] int& i: location
  \return: if successfully found 
*/
bool AgentGridVoxel::locateAgent(BaseAgent* ag, int& i) const{
	auto found = std::find(_agents.begin(), _agents.end(), ag);
	i = std::distance(_agents.begin(), found);
	return found != _agents.end();
}

/*! remove all agents from voxel
*/
void AgentGridVoxel::remove_all_agents(void){
	_agents.clear();
	return;
}
/*
  get ith agent in this voxel.
  \param[in] int i: index
  \return: ith agent pointer
*/
BaseAgent* AgentGridVoxel::getAgentByIndex(unsigned int i) const{

	BaseAgent * ag = NULL;
	if (i < _agents.size())
	{
		ag = _agents[i];
	}
	return ag;
}

/*!
  Get agent of a certain type in this voxel.
  \param[in] AgentType type: type to find.
  \param[in,out] vector<BaseAgent*> & agVec: vector holding qualifying agents
  \return: true if found.
*/
bool AgentGridVoxel::getAgentByType(AgentType type, 
	std::vector<BaseAgent*> & agVec)const {
	for (auto && ag : _agents) {
		if (type == ag->getType())
		{
			agVec.push_back(ag);
		}
	}
	return !agVec.empty();
}
/*!
  Count agent of a certain type in this voxel.
  \param[in] AgentType type: type to find.
  \param[in,out] int & count: number of agent. 
		Only when checkExist == false.
  \param[in] bool checkExist: if true, stop when first is found.
  \return: true if found.
*/
bool AgentGridVoxel::countNumAgentByType(AgentType type, 
	int & count, bool checkExist)const {
	count = 0;
	bool res = false;
	for (auto && ag : _agents) {
		if (type == ag->getType())
		{
			count++;
			if (!res)
			{
				res = true;
				if (checkExist)
				{
					return res;

				}
			}
		}
	}
	return res;
}
/*!
  Get agent of a certain state in this voxel.
  \param[in] AgentState state: state to find.
  \param[in,out] vector<BaseAgent*> & agVec: vector holding qualifying agents
  \return: true if found.
*/
bool AgentGridVoxel::getAgentByState(AgentState state,
	std::vector<BaseAgent*> & agVec) const{
	bool res = false;
	for (auto && ag : _agents) {
		if (state == ag->getState())
		{
			agVec.push_back(ag);
		}
	}
	return !agVec.empty();
}
/*!
  Count agent of a certain state in this voxel.
  \param[in] AgentState state: state to find.
  \param[in,out] int & count: number of agent. 
		Only when checkExist == false.
  \param[in] bool checkExist: if true, stop when first is found.
  \return: true if found.
*/
bool AgentGridVoxel::countNumAgentByState(AgentState state, 
	int & count, bool checkExist)const {
	count = 0;
	bool res = false;
	for (auto && ag : _agents) {
		if (state == ag->getState())
		{
			count++;
			if (!res)
			{
				res = true;
				if (checkExist)
				{
					return res;

				}
			}
		}
	}
	return res;
}
/*!
  default: for any type, voxel is open when nothing else is 
  in the voxel.
*/
bool AgentGridVoxel::isOpenToType(AgentType t)const {
	bool res = (_agents.size() == 0);
	return res;
}

/*
  Base version returns the number of agents in this voxel
*/
int AgentGridVoxel::getVoxelContent() const{
	return getNumAgents();
}
};