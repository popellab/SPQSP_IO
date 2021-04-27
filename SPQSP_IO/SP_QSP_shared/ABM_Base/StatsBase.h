#ifndef __STATS_BASE_H__
#define __STATS_BASE_H__

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include <vector>
#include <map>
#include <string>

namespace SP_QSP_IO{
// use map instead of unordered_map: n is small; no native hash for pair

//! record and output statistics 
class StatsBase
{

protected:
	// These typedefs can be accessed by StatsBase and 
	// classes inheriting from it. 
	typedef unsigned int AgentType;
	typedef unsigned int AgentState;
	typedef std::map < std::pair <AgentType, AgentState>, int > typeStateMap;

public:
	StatsBase();
	~StatsBase();

	//! write header to general ABM stats file 
	std::string writeHeader()const;
	//! write slice stats to general ABM stats file
	std::string writeSlice(int slice) const;

	//! increment cell count of certain type/state
	void incCellState(AgentType t, AgentState s); 
	//! get the number of agent with specific type/state combination
	int getCountTypeState(AgentType e, AgentState s)const;
	//! reset slice snapshot stats
	void resetStats();

	//! increase the counter for a event e happening to type/state t/s
	void incEventShared(unsigned e, AgentType t, AgentState s);
	//! increase the counter for a special event e	
	void incEventSpecial(unsigned e);
	//! set misc stats value
	void set_stats_misc(const unsigned, double);

protected:
	//! insert a type-state/id combo into _tsMap
	void setTypeStateIndex(AgentType t, AgentState s, int id);
	//! header info
	std::vector< std::string > _typeStateHeader;
	std::vector< std::string > _eventHeader;
	std::vector< std::string > _eventHeaderSpecial;
	std::vector< std::string > _miscHeader;
	//! type-state to index map
	typeStateMap _tsMap;
	//! snapshot count
	std::vector<int> _agCount;
	//! event counter, accessed by [event_index][type_state_index]
	std::vector < std::vector<long>> _eventTypeState;
	//! event counter, accessed by [special_event_index]
	std::vector<long> _eventSpecial;
	//! other non-accumulative records
	std::vector<double> _misc;


private:
	
	friend class boost::serialization::access;
	//! boost serialization 
	template<class Archive>
	void serialize(Archive &ar, const unsigned int /*version*/);

	//! initializae cell type/state map to array idx
	virtual void initStats() = 0;

	int getTypeStateKey(AgentType t, AgentState s) const;

};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(StatsBase)

//! boost serialization 
template<class Archive>
inline void StatsBase::serialize(Archive &ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(_agCount);
	ar & BOOST_SERIALIZATION_NVP(_eventTypeState);
	ar & BOOST_SERIALIZATION_NVP(_eventSpecial);
	ar & BOOST_SERIALIZATION_NVP(_misc);
}

};
#endif
