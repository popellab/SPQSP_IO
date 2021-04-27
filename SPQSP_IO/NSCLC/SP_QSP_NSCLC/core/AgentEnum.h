#pragma once

#include <utility>

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

//! Agent type enum
/*!
  Each agent class has one unique type.
  Thus no need to use member variable to store.
  Just implement getType() function to access its type.
*/
enum AgentTypeEnum 
{
	AGENT_DUMMY,
	CELL_TYPE_CANCER,
	CELL_TYPE_T,
	CELL_TYPE_TREG,
	CELL_TYPE_MAC,
	CELL_TYPE_FIB
};

//! Agent state emum
/*! 
  Each Agent class may have multiple states.
  Default state for classes with only one state and will
  not change during a simulation.
*/
enum AgentStateEnum
{
	// default
	DEFAULT_STATE,

	// cancer cell states: PDL1
	CANCER_PDL1_POS,
	CANCER_PDL1_NEG,

	// t cell states
	T_CELL_EFF,
	T_CELL_CYT,
	T_CELL_SUPP,

	// cancer cell stem/progenitor/senescence
	CANCER_STEM,
	CANCER_PROGENITOR,
	CANCER_SENESCENT 

//typedef std::pair<AgentType, AgentState> AgentTypeState;

};

};
};
