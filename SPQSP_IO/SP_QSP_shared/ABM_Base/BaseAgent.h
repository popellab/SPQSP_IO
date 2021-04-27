#ifndef __BASE_AGENT_H__
#define __BASE_AGENT_H__

#include <boost/serialization/nvp.hpp>

#include <string>
//#include "AgentEnum.h"

namespace SP_QSP_IO{
//! base class for all agents
class BaseAgent 
{
public:
	typedef unsigned int AgentType;
	typedef unsigned int AgentState;

public:
	BaseAgent();
	virtual ~BaseAgent();

	//! return element type of this cell agent
	virtual AgentType getType() const { return 0; };

	//! (pure virtual) return state of cell (defined in derived classes seperately) 
	virtual AgentState getState() const { return _state; };

	//! default content for extra remark column when writing cell grid to file
	virtual std::string getRemark() const { return ""; };
	//! set agent state eternally. Mostly for testing.
	void setAgentState(AgentState s) { _state = s; };

	void setTestValue(int v) { _test = v; };
	int getTestValue(void) { return _test; };

protected:
	//! agent state. 
	AgentState _state;
	int _test;

private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);
};

template<class Archive>
inline void BaseAgent::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_NVP(_state);
	//std::cout << "Base agent serialized" << std::endl;
}

};
#endif
