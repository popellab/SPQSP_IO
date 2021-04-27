#pragma once

#include "SP_QSP_shared/ABM_Base/ShapeBase.h"

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

class ShapeVoxel :
	public ShapeBase
{
public:
	ShapeVoxel();
	~ShapeVoxel();

	virtual const std::vector<CoordVec>& getProlifDestinationVoxels()const;
	virtual const CoordVec& getProlifDestinationAnchor()const;
	virtual const std::vector<CoordVec>& getMoveDestinationVoxels()const;
	virtual const CoordVec& getMoveDirectionAnchor()const;
	virtual const CoordVec& getEnvironmentLocations()const;

private:

	void setMoore();
	void setVonNeumann();
	void addAnchorVoxelPair(CoordVec&, std::vector<CoordVec>&, Coord3D);
	CoordVec getDestinationVoxels(Coord3D);

	CoordVec _moore;
	CoordVec _vonNeumann;
	std::vector<CoordVec> _mooreDestinations;
	std::vector<CoordVec> _vonNeumannDestinations;

};

};
};
