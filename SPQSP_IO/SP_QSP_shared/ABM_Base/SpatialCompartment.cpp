#include <algorithm>    // std::remove_if
#include <exception>
#include "SpatialCompartment.h"
//#include "../GlobalUtilities.h"


#include <boost/range/adaptor/reversed.hpp>


namespace SP_QSP_IO{

using namespace std;

SpatialCompartment::SpatialCompartment(int x, int y, int z)
: _sizeX(x)
, _sizeY(y)
, _sizeZ(z)
//, _eGrid(x, y, z, GridElement())
, _vecAgent()
//, _cGrid(x, y, z, NULL)
, _agGrid(AgentGrid(x,y,z, NULL))
{

}


SpatialCompartment::~SpatialCompartment()
{
	//lambda: delete agent 
	auto new_end = remove_if(_vecAgent.begin(), _vecAgent.end(), 
		[](CellAgent* ptrCell){delete ptrCell; return true; });
	_vecAgent.erase(new_end, _vecAgent.end());
}

/*! setup compartment environment and then intial cells. 
	Derived compartment class should define specific functions to do these tasks.
	\param [in] init_cell: files for initial cells
*/
void SpatialCompartment::initCompartment(std::string init_cell_filename){
	
	//std::cout << "initEnv" << std::endl;
	initEnvironment();
	//std::cout << "initCell" << std::endl;
	initCell(init_cell_filename);
}

/*!
	attempt to recruit one cell of type dummy to the Moore neighborhood of crd.
*/
bool SpatialCompartment::recruitOneCellInMooreNeighborhood(const CellAgent * dummy, const Coord & crd, RNG& rng){
	Coord c(0, 0, 0);
	int idx;
	//bool res = getOneOpenDestination(dummy->getCellShape()->getProlifNewOccupy(), crd, idx);
	bool res = getOneOpenVoxel(dummy->getCellShape()->getProlifDestinationVoxels(), 
		dummy->getCellShape()->getProlifDestinationAnchor(),
		crd, dummy->getType(), idx, rng);
	if(res)
	{
		/*cout << "idx: " << idx << ", crd: " << crd << endl;
		cout << "relative: " << dummy->getCellShape()->getProlif()[idx] << endl;
		cout << "recruit at: " << dummy->getCellShape()->getProlif()[idx] + crd << endl;
		*/
		//CellAgent *ptR = CellFactory::createNewCell(*dummy);
		CellAgent *ptR = dummy->createCellCopy();

		c = dummy->getCellShape()->getProlifDestinationAnchor()[idx] + crd;

		ptR->setCoord(c);
		addAgentToGrid(c, ptR);
		_vecAgent.push_back(ptR);
	}
	return res;
}



/*! 
  find all Open neighbor voxels
  \param[in] const CoordVec& target: relative target locations to search 
  \param[in] const Coord & c: offset to current location
  \param[in] AgentType type: for each type "open" could be different.
  \param[in,out] vector<int> & candidates: open locations found
*/
/*! 
  get one from the open voxels
bool SpatialCompartment::getOneOpenVoxel(const vector<CoordVec>& target, 
	const Coord & c, AgentType type, int& idxFound, RNG& rng)const {
	vector<int> candidates;
	bool res = getOpenVoxels(target, c, type, candidates);
	if (res)
	{
		int r = int(rng.get_unif_01() *candidates.size());
		idxFound = candidates[r];
		return true;
	}
	else{
		return false;
	}
}
*/

/*! include x, y, z coordinates and type/state combination
*/
std::string SpatialCompartment::compartment_cells_to_string(void) const{
	std::stringstream ss;
	// header
	ss << "x,y,z,Type,State," << getExtraRemarkHeader() << endl;
	for (CellVec::size_type i = 0 ; i < _vecAgent.size(); i++)
	{
		CellAgent * ptr = _vecAgent[i];
		ss << ptr->getAdjustedX() << "," << ptr->getAdjustedY()
			<< "," << ptr->getAdjustedZ() << "," << ptr->getType() 
			<< "," << ptr->getState() <<"," << ptr->getRemark() << endl;
	}
	return ss.str();

}

/*! iterate over entire grid
    x/y/z_forward: from which of the eight corners do we start the iteration
	\param[in] x_forward: starting from x=0 if true, else x=x_max.
	\param[in] y_forward: starting from y=0 if true, else y=y_max.
	\param[in] z_forward: starting from z=0 if true, else z=z_max.
	\param[in] function f: operation on each voxel
*/
void SpatialCompartment::for_each_grid_coord(bool x_forward, bool y_forward, bool z_forward, 
	std::function<void(Coord3D&)>f){
	using boost::adaptors::reversed;
	int x_begin, x_end, x_step;
	if (x_forward)
	{
		x_begin = 0;
		x_end = _sizeX;
		x_step = 1;
	}
	else
	{
		x_begin = _sizeX - 1;
		x_end = -1;
		x_step = -1;
	}
	int y_begin, y_end, y_step;
	if (y_forward)
	{
		y_begin = 0;
		y_end = _sizeX;
		y_step = 1;
	}
	else
	{
		y_begin = _sizeX - 1;
		y_end = -1;
		y_step = -1;
	}
	int z_begin, z_end, z_step;
	if (z_forward)
	{
		z_begin = 0;
		z_end = _sizeX;
		z_step = 1;
	}
	else
	{
		z_begin = _sizeX - 1;
		z_end = -1;
		z_step = -1;
	}

	for (int i = x_begin; i != x_end; i += x_step)
	{
		for (int j = y_begin; j != y_end; j += y_step)
		{
			for (int k = z_begin; k != z_end; k += z_step)
			{
				auto c = Coord3D(i, j, k);
				f(c);
			}
		}
	}
	return;
}

/*! examine neibhorhood: list of anchor voxels alone
	Iterate through the neighboring voxels to check if any of them pass the test 
	of the user provided function.
	Stop when first instance satisfying condition is encountered.

	\param[in] const CoordVec& env: vector of relative coordinate of neighbor voxels
	\param[in] const Coord& current: current voxel coordinate
	\param[in] function<bool(const int, const Coord&)>f: lambda for criteria
	\return: true if any neighboring voxel pass condition

	f: criteria function to determine if one voxel pass check:
	\param[in] const int i: position of voxel in neighbor anchor vector 
	\param[in] const Coord& c: absolute coordinate of voxel/shape anchor
*/
bool SpatialCompartment::check_neighbor(const CoordVec& env, 
	const Coord& current, std::function<bool(const int, const Coord&)>f) const{
	bool res = false;
	unsigned int n = env.size();
	for (size_t i = 0; i < n; i++)
	{
		auto c = env[i] + current;
		if (_agGrid.inGrid(c))
		{
			res |= f(i, c);
			if (res)
			{
				break;
			}
		}
	}
	return res;
}

/*! examine neibhorhood: all voxels in shape
	\params[in] const ShapeVec& shapes: shape voxel vectors corresponding to each anchor
	\params[in] const CoordVec& anchors: anchor voxel vector
	\params[in]	const Coord& current: current voxel coordinate
	\params[in] function<bool(const ShapeVoxel&)>f: lambda for criteria

	f: criteria function to determine if a "shape" (assocated with one anchor) pass the check
	\params[in] const ShapeCoords&: relative (to agent anchor) coordinates of voxels forming a shape
*/
bool SpatialCompartment::check_neighbor_shape(const ShapeVec& shapes, const CoordVec& anchors,
	const Coord& current, std::function<bool(const ShapeCoords&)>f)const{
	bool res = check_neighbor(anchors, current, [&](const int i, const Coord& c){
		return res = f(shapes[i]);
	});
}

/*! Check if any agent of specified type/state combination is in the given set of voxels.
  \param[in] target: set of relative coords
  \param[in] c: search coord0
  \param[in] AgentType t: type (this is not necessary because states are in a shared enum class)
  \param[in] AgentState s: agent state.
  \return: true if any found
*/
bool SpatialCompartment::hasTypeStateInTarget(const CoordVec& target, const Coord& c,
	AgentType t, AgentState s) const {

	bool res = false;
	int count = 0;

	res = check_neighbor(target, c, [&](const int i, const Coord& c){
		bool found = _agGrid(c)->countNumAgentByState(s, count, true);
		return found;
	});
	return res;
}

/*! iterate through the whole vector of coordinates and perform operation.
	\param[in] const CoordVec& v: a list of (relative) locations to look into
	\param[in] const Coord& current: coordinates of current location
	\param[in] function<bool(const int, const Coord&)>f: lambda for operation 
	\return: if for any location, f return true.

	f: operation to perform for one of the locations 
	\param[in] const int i:  position of voxel in neighbor anchor vector 
	\param[in] const Coord& c: absolute coordinate of voxel/shape anchor
*/
bool SpatialCompartment::for_each_coord(const CoordVec& v, const Coord& current,
	std::function<bool(const int, const Coord&)>f)const{
	bool res = false;
	unsigned int n = v.size();
	for (size_t i = 0; i < n; i++)
	{
		auto c = v[i] + current;
		if (_agGrid.inGrid(c))
		{
			bool res_local = f(i, c);
			res |= res_local;
		}
	}
	return res;
}

/*! iterate through a list of coordinates and perform operation on agents in these voxels
	\param[in] const CoordVec& v: a list of (relative) locations to look into
	\param[in] const Coord& current: coordinates of current location
	\param[in] function<bool(const int, const Coord&)>f: lambda for operation 
	\return: if for any location, f return true.

	f: operation to perform for one of the locations 
	\param[in] BaseAgent* ag:  pointer to agent
*/
bool SpatialCompartment::for_each_neighbor_ag(const CoordVec& env, const Coord& current,
	std::function<bool(BaseAgent* ag)>f)const{
	bool res = for_each_coord(env, current, [&](const int i, const Coord& c){
		bool res_f = false;
		for (auto ag : _agGrid(c)->get_agents()){
			res_f |= f(ag);
		}
		return res_f;
	});
	return res;
}

/*! find all qualifying coordinates neighborhood
	\param[in] const CoordVec& env: a list of (relative) locations to look into
	\param[in] const Coord& current: coordinates of current location
	\param[in, out] vector<int>& candidates: vector to hold indices of qualifying locations
	\param[in] function<bool(const int, const Coord&)>f: lambda for operation 
	\return: if for any location, f return true.

	f: qualification condition for one location to check
	\param[in] const int i:  position of voxel in neighbor anchor vector 
	\param[in] const Coord& c: absolute coordinate of voxel/shape anchor
*/
bool SpatialCompartment::get_qualifying_voxels(const CoordVec& env, const Coord& current,
	std::vector<int>& candidates, std::function<bool(int, const Coord&)>f)const{
	bool res = for_each_coord(env, current, [&](const int i, const Coord& c){
		bool res_local = f(i, c);
		if (res_local)
		{
			candidates.push_back(i);
		}
		return res_local;
	});
	return res;

}
/*! find all qualifying shape anchors in neighborhood
*/
bool SpatialCompartment::get_qualifying_shape_anchors(const ShapeVec& shapes, const CoordVec& anchors,
	const Coord& current, std::vector<int>& candidates, std::function<bool(const ShapeCoords&)>f)const{
	bool res = get_qualifying_voxels(anchors, current, candidates, [&](const int i, const Coord& c){
		return res = f(shapes[i]);
	});
	return res;
}

/*! all Open neighbor voxels
	\param[in] const ShapeVec& targets: shapes to look into
	\param[in] const CoordVec& anchors: anchors associated with each shape
	\param[in] const Coord& c: current location
	\param[in] AgentType type: for each type "open" could be different.
	\param[in,out] vector<int> & candidates: open locations found
	\return: true if any found.
*/
bool SpatialCompartment::getOpenVoxels(const ShapeVec& targets, const CoordVec& anchors,
	const Coord & c, AgentType type, std::vector<int>& candidates)const{
	bool res = get_qualifying_shape_anchors(targets, anchors, c, candidates,
		[&](const ShapeCoords& shape){
		bool res = true;
		for (const auto& tc : shape)
		{
			auto crd = tc + c;
			// This cannot be skipped because the check in "for_each_coord" only apply to anchors
			if (_agGrid.inGrid(crd))
			{
				res &= _agGrid(crd)->isOpenToType(type);
				if (!res){
					break;
				}
			}
		}
		return res;
	});
	return res;
}

/*! randomly draw one from the all the open locations. 
	\param[in] const ShapeVec& targets: shapes to look into
	\param[in] const CoordVec& anchors: anchors associated with each shape
	\param[in] const Coord& c: current location
	\param[in] AgentType type: for each type "open" could be different.
	\param[in,out] int& idx_found: the index of randomly chosen location from all open locations found
	\return: true if any found.
*/
bool SpatialCompartment::getOneOpenVoxel(const ShapeVec& targets, const CoordVec& anchors,
	const Coord & c, AgentType type, int & idx_found, RNG& rng)const{
	auto n = anchors.size();
	std::vector<int> candidates;
	bool res = getOpenVoxels(targets, anchors, c, type, candidates);
	if (res)
	{
		idx_found = candidates[rng.single_draw(candidates)];
	}
	return res;
}

/*! check if any qualifying agent exist in neighborhood. Stop on first hit.
	\param[in] const CoordVec& env: (relative) locations to look into
	\param[in] const Coord& current: current location
	\param[in] function<bool(const BaseAgent*)> f: lambda for condition
	\return: true if any qualifying agent found
*/
bool SpatialCompartment::check_neighbor_agents(const CoordVec& env, const Coord& current,
	std::function<bool(const BaseAgent*)>f)const{
	bool res = check_neighbor(env, current, [&](const int i, const Coord& c){
		for (const auto ag : _agGrid(c)->get_agents()){
			if (f(ag))
			{
				return true;
			}
		}
		return false;
	});
	return res;
}

/*! get pointers to qualifying agents 
	\param[in] const CoordVec& env: (relative) locations to look into
	\param[in] const Coord& current: current location
	\param[in, out] vector<BaseAgent*>& ags: list of qualifying agents
	\param[in] function<bool(const BaseAgent*)> f: lambda for condition
	\return: true if any qualifying agent found
*/
bool SpatialCompartment::get_qualifying_neighbor_agents(const CoordVec& env, const Coord& current,
	std::vector<BaseAgent*>& ags, std::function<bool(const BaseAgent*)>f)const{

	bool res = for_each_coord(env, current, [&](const int i, const Coord& c){
		// check all agents in each voxel
		for (const auto ag: _agGrid(c)->get_agents()){
			if (f(ag))
			{
				ags.push_back(ag);
			}
		}
		return (!ags.empty());
	});
	return res;
}
/*! get one random qualifying agent 
*/
bool SpatialCompartment::get_random_qualifying_neighbor_agent(const CoordVec& env, const Coord& current,
	BaseAgent *&ag, RNG& rng, std::function<bool(const BaseAgent*)>f)const{

	std::vector<BaseAgent*> ags;
	bool res = get_qualifying_neighbor_agents(env, current, ags, f);
	if (res)
	{
		ag = ags[rng.single_draw(ags)];
	}
	return res;
}

/*!
  \param [in] Coord& c: location to add agent 
  \param [in] BaseAgent* ptrAg: pointer to agent to add
  Check if c is in grid of _agGrid. If so:
  -# add ptrAg to voxel
*/
void SpatialCompartment::addAgentToGrid(const Coord & c, BaseAgent* ptrAg) {
	bool inGrid = _agGrid.inGrid(c.x, c.y, c.z);
	if (inGrid)
	{
		bool res = _agGrid(c.x, c.y, c.z)->addAgent(ptrAg);
		if (!res)
		{
			const std::string errorMsg = "Agent already in destination voxel";
			//already in that location
			throw std::invalid_argument(errorMsg);
		}
	}
	else {
		const std::string errorMsg = "Adding agent to location outside of grid";
		throw std::out_of_range(errorMsg);
	}
}
/*!	
  \param [in] Coord& c: location to remove agent from 
  \param [in] BaseAgent* ptrAg: Agent to be removed from grid.
  Remove ptrAg from its current location.
  -# remove pointer from _agGrid;

*/
void SpatialCompartment::removeAgentFromGrid(const Coord &c, BaseAgent* ptrAg) {
	bool inGrid = _agGrid.inGrid(c.x, c.y, c.z);
	if (inGrid)
	{
		bool res = _agGrid(c.x, c.y, c.z)->removeAgent(ptrAg);
		if (!res)
		{
			const std::string errorMsg = "Agent not found in this voxel\n";
			throw std::invalid_argument(errorMsg);
		}
	}
	else {
		const std::string errorMsg = "Removing agent from location outside of grid";
		throw std::out_of_range(errorMsg);
	}
}
std::string SpatialCompartment::getGridContent()const {
	stringstream ss;
	ss << _agGrid;
	return ss.str();
}
};
