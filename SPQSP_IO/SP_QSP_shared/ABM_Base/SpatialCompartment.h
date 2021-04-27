#ifndef __SPATIAL_COMPARTMENT_H__
#define __SPATIAL_COMPARTMENT_H__

#include <vector>
#include <functional>

//#include "GridElement.h"
//#include "../pde/DiffuseGrid.h"
//#include "CellFactory.h"
#include "CellAgent.h"
#include "Grid3D.h"
#include "AgentGridVoxel.h"
#include "RNG.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/assume_abstract.hpp>


namespace SP_QSP_IO{

//! base class for 3D spatial compartments
class SpatialCompartment
{
//using std::vector;
public:
typedef std::vector<CellAgent *> CellVec;
typedef Grid3D<AgentGridVoxel*> AgentGrid;

protected:
	typedef BaseAgent::AgentType AgentType;
	typedef BaseAgent::AgentState AgentState;
public:
	//! default constructor for serialization
	SpatialCompartment(){};
	SpatialCompartment(int x, int y, int z);
	virtual ~SpatialCompartment();
	
	//! Initialize compartment
	virtual void initCompartment(std::string);

	//! pure virtual
	virtual void timeSlice(unsigned long slice)= 0;

	//! default header for extra remark column when writing cell grid to file
	virtual std::string getExtraRemarkHeader() const { return ""; };

	//void printCellVectorSequence() const;
	//! write information of all cells on grid to a snapshot file. 
	std::string compartment_cells_to_string(void) const;

	//! pure virtual
	virtual std::string printGridToFile() const = 0;
	//! pure virtual
	virtual void printCellOdeToFile(unsigned long slice) const = 0;
	//! pure virtual
	//virtual void printGridToScreen(unsigned long slice) const = 0;

	//! iterate over entire grid
	//void for_each_voxel(bool, bool, bool, void(*)(Coord3D&));
	void for_each_grid_coord(bool, bool, bool, std::function<void(Coord3D&)>);

	//! get total number of cells.
	int getNrCell()const { return _vecAgent.size(); };

	int getGridSize()const { return _agGrid.getSize(); };
	std::string getGridContent()const;

	//! examine neighboring voxels
	bool check_neighbor(const CoordVec& env, const Coord& current, 
		std::function<bool(const int, const Coord&)>) const;
	//! iterate through a list of coordinates and perform operation
	bool for_each_coord(const CoordVec& env, const Coord& current, 
		std::function<bool(const int, const Coord&)>)const;

	// iterate through a list of coordinates and perform operation on agents in these voxels
	bool for_each_neighbor_ag(const CoordVec& env, const Coord& current, 
		std::function<bool(BaseAgent*)>)const;

	//! examine neibhorhood: all voxels in shape
	bool check_neighbor_shape(const ShapeVec&, const CoordVec&,
		const Coord&, std::function<bool(const ShapeCoords&)>) const;
	//! Scan Moore neighborhood for cell of given type and state.
	bool hasTypeStateInTarget(const CoordVec& target, const Coord& c, AgentType t, AgentState s) const;

	//! find all qualifying coordinates neighborhood
	bool get_qualifying_voxels(const CoordVec&, const Coord&, 
		std::vector<int>&, std::function<bool(int, const Coord&)>)const;
	//! find all qualifying shape anchors in neighborhood
	bool get_qualifying_shape_anchors(const ShapeVec&, const CoordVec&,
		const Coord&, std::vector<int>&, std::function<bool(const ShapeCoords&)>)const;

	//! get all Open neighbor voxels
	bool getOpenVoxels(const ShapeVec& targets, const CoordVec& anchors, 
	const Coord & c, AgentType type, std::vector<int>& candidates)const;
	//! get one from the open voxels
	bool getOneOpenVoxel(const ShapeVec& targets, const CoordVec& anchors, 
	const Coord & c, AgentType type, int & idxFound, RNG& rng)const;

	//! check if any qualifying agent exist in neighborhood
	bool check_neighbor_agents(const CoordVec&, const Coord&,
		std::function<bool(const BaseAgent*)>)const;
	//! get pointers to qualifying agents 
	bool get_qualifying_neighbor_agents(const CoordVec&, const Coord&, 
		std::vector<BaseAgent*>&, std::function<bool(const BaseAgent*)>)const;
	//! get one random qualifying agent 
	bool get_random_qualifying_neighbor_agent(const CoordVec&, const Coord&, 
		BaseAgent *&, RNG&, std::function<bool(const BaseAgent*)>)const;

protected:

	//! Add a cell to spatial compartment
	void addAgentToGrid(const Coord & c, BaseAgent*);
	//! remove a cell from spatial compartment
	void removeAgentFromGrid(const Coord& c, BaseAgent*);
	//! Add a cell to spatial compartment
	//void addCellToGrid(Coord & c, CellAgent*);
	//! remove a cell from spatial compartment
	//void removeCellFromGrid(CellAgent*);


	//! initiate agent grid 
	virtual bool initAgentGrid() = 0;
	
	//! attempt recruit one single cell;
	bool recruitOneCellInMooreNeighborhood(const CellAgent * dummy, const Coord & crd, RNG& rng);

	//! size in x direction
	int _sizeX;
	//! size in y direction
	int _sizeY;
	//! size in z direction
	int _sizeZ;

	//! element grid
	/*!ElementGrid _eGrid;  Type of agent mapped to this layer,
						for more efficient scanning and type searching*/
	//! vector of cellular agents
	CellVec _vecAgent;
	//! grid of pointers to cellular agents. 

	//! AgentGrid
	AgentGrid _agGrid;

private:

	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	//! pure virtual: setup compartment environment
	virtual void initEnvironment()=0;
	//! pure virtual: setup initial cells 
	virtual void initCell(std::string filename)=0;

};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(SpatialCompartment)

template<class Archive>
inline void SpatialCompartment::serialize(Archive & ar, const unsigned int version){
	// _cGrid and _vecAgent contains pointers of base class which point to derived class instances.
	// register all pointer to all cell types so that items can be serialized correctly
	ar & BOOST_SERIALIZATION_NVP(_sizeX);
	ar & BOOST_SERIALIZATION_NVP(_sizeY);
	ar & BOOST_SERIALIZATION_NVP(_sizeZ);
	ar & BOOST_SERIALIZATION_NVP(_vecAgent);
	ar & BOOST_SERIALIZATION_NVP(_agGrid);
}

};
#endif

