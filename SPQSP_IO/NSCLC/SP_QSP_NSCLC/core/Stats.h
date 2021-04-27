#pragma once

#include "SP_QSP_shared/ABM_Base/StatsBase.h"
#include "AgentEnum.h"

#include <boost/serialization/base_object.hpp>

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

enum StatsEventShared
{
	STATS_EVENT_REC,
	STATS_EVENT_PROLIF,
	STATS_EVENT_DIE,
	STATS_EVENT_MOVE,
	STATS_EVENT_DROP_IN,
	STATS_EVENT_DROP_OUT,
	STATS_EVENT_SHARED_COUNT,
};

enum StatsEventSpecial
{
	STATS_KILLED_BY_T,
	STATS_EVENT_SPECIAL_COUNT,
};

enum StatsMisc {
	STATS_MISC_PDL1_POS,
	STATS_MISC_PD1_PDL1,
	STATS_MISC_COUNT,
};

//! record and output statistics 
class Stats : public StatsBase
{
public:
	Stats();
	~Stats();

	//! increment recruitment count
	void incRecruit(AgentType, AgentState);
	//! increment proliferation count
	void incProlif(AgentType, AgentState);
	//! increment death count
	void incDeath(AgentType, AgentState);
	//! increment movement count
	void incMove(AgentType, AgentState);
	//! increment cell generated otherwise 
	void incDropIn(AgentType, AgentState);
	//! increment cell dropping out of grid 
	void incDropOut(AgentType, AgentState);

	void inc_PD1_PDL1(double);

	//! get total number of T cells
	int getTCell() const;
	//! get total number of cancer cells
	int getCancerCell() const;
	//! get total number of Treg cell
	int getTreg() const;

	double get_mean_PD1_PDL1(void);
	void reset_PD1_PDL1(void);

private:
	
	friend class boost::serialization::access;

	virtual void initStats();

	int _n_PD1_PDL1;
	double _sum_PD1_PDL1;

	// interval stats accumulator
	template<class Archive>
	//! boost serialization 
	void serialize(Archive &ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(StatsBase);
	}

};

};
};
