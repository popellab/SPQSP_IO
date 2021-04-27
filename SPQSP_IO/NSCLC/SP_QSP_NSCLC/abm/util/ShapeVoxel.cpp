#include "ShapeVoxel.h"

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

using std::vector;
ShapeVoxel::ShapeVoxel()
	:ShapeBase()
	, _moore()
	, _vonNeumann()
{
	setMoore();
	setVonNeumann();
}


ShapeVoxel::~ShapeVoxel()
{
}

const std::vector<CoordVec>& ShapeVoxel::getProlifDestinationVoxels()const{
	return _mooreDestinations;
};
const CoordVec& ShapeVoxel::getProlifDestinationAnchor()const{
	return _moore;
};
const std::vector<CoordVec>& ShapeVoxel::getMoveDestinationVoxels()const{
	return _mooreDestinations;
	//return _vonNeumannDestinations;
};
const CoordVec& ShapeVoxel::getMoveDirectionAnchor()const{
	return _moore;
	//return _vonNeumann;
};

/*! anchor coordinates when interacting with environment 
*/
const CoordVec& ShapeVoxel::getEnvironmentLocations()const{
	return _moore;
}

void ShapeVoxel::setMoore(){
	for (int i = -1; i < 2; i++)
	{
		for (int j = -1; j < 2; j++)
		{
			for (int k = -1; k < 2; k++)
			{
				if (i || j || k){
					auto c = Coord3D(i, j, k);
					addAnchorVoxelPair(_moore, _mooreDestinations, c);
				}
			}
		}
	}
	//std::cout << _moore.size() << std::endl;
	//std::cout << _mooreDestinations.size() << std::endl;
}

void ShapeVoxel::setVonNeumann(){
	Coord3D c;
	addAnchorVoxelPair(_vonNeumann, _vonNeumannDestinations, Coord3D(1, 0, 0));
	addAnchorVoxelPair(_vonNeumann, _vonNeumannDestinations, Coord3D(-1, 0, 0));
	addAnchorVoxelPair(_vonNeumann, _vonNeumannDestinations, Coord3D(0, 1, 0));
	addAnchorVoxelPair(_vonNeumann, _vonNeumannDestinations, Coord3D(0, -1, 0));
	addAnchorVoxelPair(_vonNeumann, _vonNeumannDestinations, Coord3D(0, 0, 1));
	addAnchorVoxelPair(_vonNeumann, _vonNeumannDestinations, Coord3D(0, 0, -1));
	//std::cout << _vonNeumann.size() << std::endl;
	//std::cout << _vonNeumannDestinations.size() << std::endl;

}

/*! Add anchor coordinate/shape voxel coordinates pairs into
	two vectors where they have corresponding positions.
*/
void ShapeVoxel::addAnchorVoxelPair(CoordVec& anchors, std::vector<CoordVec>& voxels, Coord3D c){
	anchors.push_back(c);
	voxels.push_back(getDestinationVoxels(c));
}

/*! The vector of voxels forming wanted shape that are
	associated with anchor voxel c.
	In this version, all cells only occupy one voxel. Thus, 
	The shape is anchor voxel alone.
*/
CoordVec ShapeVoxel::getDestinationVoxels(Coord3D c){
	return CoordVec({c});
}
};
};