#ifndef __SHAPE_BASE__
#define __SHAPE_BASE__

#include "Coord3D.h" 
#include <set>
#include <vector>


namespace SP_QSP_IO{

typedef std::set<Coord3D> CoordSet;
typedef std::vector<Coord3D> CoordVec;
typedef std::vector<Coord3D> ShapeCoords;
typedef std::vector<ShapeCoords> ShapeVec;

/*! relative coordinates of voxels of various geometric shapes associated with an agent.
	All are relative to the anchor coordinate of an agent.
*/
class ShapeBase
{
public:
	ShapeBase();
	~ShapeBase();
	
	//! voxels to examine when allocating a daughter cell from proliferation
	virtual const ShapeVec& getProlifDestinationVoxels()const = 0;
	//! anchor coordinates of proliferation destination 
	virtual const CoordVec& getProlifDestinationAnchor()const = 0;
	//! voxels to examine when allocating the current cell after movement
	virtual const ShapeVec& getMoveDestinationVoxels()const = 0;
	//! anchor coordinates of movement destination 
	virtual const CoordVec& getMoveDirectionAnchor()const = 0;
	//! coordinates to examine in the environment
	virtual const CoordVec& getEnvironmentLocations()const = 0;

protected:
};

};
#endif

