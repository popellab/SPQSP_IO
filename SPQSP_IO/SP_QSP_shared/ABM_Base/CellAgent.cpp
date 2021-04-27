#include <iostream>
#include <sstream>
#include "CellAgent.h"
#include "SpatialCompartment.h"

namespace SP_QSP_IO{

using std::string;
using std::stringstream;

CellAgent::CellAgent(SpatialCompartment * c)
	:BaseAgent()
	, _compartment(c)
	, _coord()
	, _life(0)
	, _id(CellAgent::cIDGenerator++)
	, _dead(false)
{
	
}
CellAgent::CellAgent(const CellAgent & c)
	: BaseAgent()
	, _compartment(c._compartment)
	, _coord(c._coord)
	, _life(c._life)
	, _id(CellAgent::cIDGenerator++)
	, _dead(false)
{
	_state = c._state;
}

CellAgent::~CellAgent()
{
}

unsigned int CellAgent::cIDGenerator = 0;

string CellAgent::toString()const{
	stringstream ss;
	ss << "cellular agent: id: " << _id << ", type: " << getType() << std::endl;
	ss <<"coordinates: " << _coord << ", life: " << _life << std::endl;
	return ss.str();
}
};