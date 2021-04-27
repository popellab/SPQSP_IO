#include "StatsBase.h"

#include <sstream>

namespace SP_QSP_IO{

using namespace std;

StatsBase::StatsBase()
: _tsMap()
, _typeStateHeader()
, _eventHeader()
, _eventHeaderSpecial()
, _miscHeader()
, _agCount()
, _eventTypeState()
, _eventSpecial()
, _misc()
{

}

StatsBase::~StatsBase()
{
}


std::string StatsBase::writeHeader() const{
	std::stringstream ss;
	ss << "time";

	for(const auto& ts: _typeStateHeader) 
	{
		ss << ",agentCount." << ts;
	}

	for(const auto& e:_eventHeader) 
	{
		for (const auto& ts: _typeStateHeader)
		{
			ss << "," << e << "." << ts;
		}
	}
	for(const auto& es:_eventHeaderSpecial) 
	{
		ss << "," << es;
	}
	for (const auto& s : _miscHeader) {
		ss << "," << s;
	}
	ss << endl;
	return ss.str();
}

std::string StatsBase::writeSlice(int slice)const {
	std::stringstream ss;
	ss << slice;

	for(const auto& i:_agCount) 
	{
		ss << "," << i;
	}
	
	for(const auto& e:_eventTypeState) 
	{
		for (const auto& i : e) 
		{
			ss << "," << i;
		}
	}
	for(const auto& es: _eventSpecial) 
	{
		ss << "," << es;
	}
	for (const auto& i : _misc) {
		ss << "," << i;
	}
	ss << endl;
	return ss.str();
}

/*!	these are snapshot stats of a slice. The record is taken at the end of 
	each slice when removing dead cells, or after creating initial cells for 
	a compartment.
*/
void StatsBase::incCellState(AgentType t, AgentState s){
	_agCount[getTypeStateKey(t,s)] ++;
}

/*! Get the number of agents with type e and state s
*/
int StatsBase::getCountTypeState(AgentType t, AgentState s)const{
	return _agCount[getTypeStateKey(t,s)];
}


/*!	Slice snapshot ABM stats are cleared at the beginning of each new slice
*/
void StatsBase::resetStats() {
	/*
	for (size_t i = 0; i < _typeStateHeader.size(); i++)
	{
		_agCount[i] = 0;
	}*/
	for (auto& i : _agCount) {
		i = 0;
	}
	for (auto& i : _misc) {
		i = 0;
	}
	return;
}
/*! event that can happen to many agent type/state combinations
*/
void StatsBase::incEventShared(unsigned e, AgentType t, AgentState s){
	_eventTypeState[e][getTypeStateKey(t, s)] ++;

}
/*! event that are specific to certain type state, or are unbound to agent types
*/
void StatsBase::incEventSpecial(unsigned e){
	_eventSpecial[e] ++;
}

void StatsBase::set_stats_misc(const unsigned i, double v)
{
	_misc[i] = v;
	return;
}

void StatsBase::setTypeStateIndex(AgentType t, AgentState s, int id){
	_tsMap.insert( {make_pair(t, s), id });
}

/*! get index key from a type state pair
*/
int StatsBase::getTypeStateKey(AgentType t, AgentState s) const{
	return _tsMap.at(make_pair(t, s));
}
};