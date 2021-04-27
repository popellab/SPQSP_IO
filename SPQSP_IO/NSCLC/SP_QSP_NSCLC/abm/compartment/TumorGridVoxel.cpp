#include "TumorGridVoxel.h"
#include <math.h>

#include "../../core/GlobalUtilities.h"

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

TumorGridVoxel::TumorGridVoxel(int x, int y, int z)
	: AgentGridVoxel()
	, _distToOrigin(sqrt((double)x*x+y*y+z*z))
	//, _str_loc()
{
	//_str_loc = "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
}


TumorGridVoxel::~TumorGridVoxel()
{
}

/*!
  Check if this voxel is open to type
  \param[in] AgentType t: type to check against.
*/
bool TumorGridVoxel::isOpenToType(AgentType t)const {
	bool res = false;
	int count;
	if (t == AgentTypeEnum::AGENT_DUMMY)
	{
		res = AgentGridVoxel::isOpenToType(t);
	}
	else if (t == AgentTypeEnum::CELL_TYPE_CANCER)
	{
		res = !countNumAgentByType(AgentTypeEnum::CELL_TYPE_CANCER, count, true);
	}
	else if (t == AgentTypeEnum::CELL_TYPE_T ||
		t == AgentTypeEnum::CELL_TYPE_TREG)
	{
		int c = countNumAgentByType(AgentTypeEnum::CELL_TYPE_T, count, true);
		int teff = countNumAgentByType(AgentTypeEnum::CELL_TYPE_T, count, false);
		int treg = countNumAgentByType(AgentTypeEnum::CELL_TYPE_TREG, count, false);
		res = (c && teff + treg < params.getVal(PARAM_N_T_VOXEL_C)) || 
			(!c && teff + treg < params.getVal(PARAM_N_T_VOXEL));
	}
	return res;
}

/*!
  distance to origin (0,0,0).
  used to determine whether this voxel is in range of 
  invasive front when comparing simulation with model.
*/
double TumorGridVoxel::getDistToOrigin(void)const{
	return _distToOrigin;
}

/*!
  content of this voxel. currently a placeholder.
*/
int TumorGridVoxel::getVoxelContent() const{
	int sum = 0;
	for (auto && ag : _agents) {
		sum += ag->getTestValue();
	}
	return sum;
}

};
};