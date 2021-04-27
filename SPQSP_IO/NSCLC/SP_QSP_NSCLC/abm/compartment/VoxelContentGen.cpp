#include "VoxelContentGen.h"
#include "../../core/GlobalUtilities.h"

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

VoxelContentGen::VoxelContentGen()
	: _stationary(true)
	, _x_min(0)
	, _y_min(0)
	, _z_min(0)
	, _x_max(0)
	, _y_max(0)
	, _z_max(0)
	, _celltype_cdf()
{
	int dmax = params.getVal(PARAM_CANCER_CELL_PROGENITOR_DIV_MAX);
	// 0: stem; 1-dmax: progenitor; dmax+1: senescent; dmax+2: empty
	_celltype_cdf = std::vector<double>(dmax + 3, 0.0);
	return;
}


VoxelContentGen::~VoxelContentGen()
{
}

/*! return true if any cell is to be created
*/
bool VoxelContentGen::get_type_state(const Coord3D& c, RNG& rng,
	AgentType& type, AgentState& state, int&div)const{

	bool create_cell = false;
	int dmax = params.getVal(PARAM_CANCER_CELL_PROGENITOR_DIV_MAX);
	type = AgentTypeEnum::CELL_TYPE_CANCER;
	state = AgentStateEnum::CANCER_PROGENITOR;
	/*
	std::cout << c << ", "
		<< _x_lim << ", " << _y_lim << ", " << _z_lim << std::endl;
	*/
	if (_stationary || ((c.x >=_x_min && c.x < _x_max) 
		&& (c.y >= _y_min && c.y < _y_max)
		&& (c.z >= _z_min && c.z < _z_max)))
	{
		int i = rng.sample_cdf(_celltype_cdf);
		if (i <= dmax+1)
		{
			create_cell = true;
			if (i==0)
			{
				state = AgentStateEnum::CANCER_STEM;
			}
			else if (i==dmax+1)
			{
				state = AgentStateEnum::CANCER_SENESCENT;
			}
			else{
				state = AgentStateEnum::CANCER_PROGENITOR;
				div = dmax + 1 - i;
			}
		}
	}
	return create_cell;
}

void VoxelContentGen::setup(bool stationary, double cancer_prob,
	int xlim, int ylim, int zlim){
	_stationary = stationary;

	_x_min= 0;
	_y_min= 0;
	_z_min= 0;
	_x_max= _x_min + xlim;
	_y_max= _y_min + ylim;
	_z_max= _z_min + zlim;
	unsigned int dmax = params.getVal(PARAM_CANCER_CELL_PROGENITOR_DIV_MAX);
	double k, r, rs, rp, mu, l0, l1, l2;
	k = params.getVal(PARAM_CANCER_STEM_ASYMMETRIC_DIV_PROB);
	rs = params.getVal(PARAM_CSC_GROWTH_RATE);
	rp = params.getVal(PARAM_CANCER_PROG_GROWTH_RATE);
	mu = params.getVal(PARAM_CANCER_SENESCENT_DEATH_RATE);
	r = rs * (1 - k);
	l0 = k*rs / (r + rp);
	l1 = 2 * rp / (r + rp);
	l2 = 2 * rp / (r + mu);
	double C = 1 + l0*(std::pow(l1, dmax) - 1) / (l1 - 1) + l0*l2*std::pow(l1,(dmax - 1));
	double p ;
	_celltype_cdf[0] = p = 1 / C * cancer_prob; // joint P
	p *= l0;
	_celltype_cdf[1] = _celltype_cdf[0] + p;

	for (size_t i = 2; i <= dmax; i++)
	{
		p *= l1;
		_celltype_cdf[i] = _celltype_cdf[i - 1] + p;
	}
	p *= l2;
	_celltype_cdf[dmax + 1] = _celltype_cdf[dmax] + p;
	_celltype_cdf[dmax + 2] = 1.0;

	/*
	std::cout << "cancer cell cdf:" << std::endl;
	for (size_t i = 0; i < _celltype_cdf.size(); i++)
	{
		std::cout << i << ", " << _celltype_cdf[i] << std::endl;
	}
	*/
	return;
}

/* Fill (0,0,0), (xlim, ylim, zlim) with cancer cells
*/
void VoxelContentGen::setup( double pstem, int xlim, int ylim, int zlim,
	int x0 = 0, int y0=0, int z0 = 0) {

	_stationary = false;

	_x_min= x0;
	_y_min= y0;
	_z_min= z0;
	_x_max= _x_min + xlim;
	_y_max= _y_min + ylim;
	_z_max= _z_min + zlim;

	unsigned int dmax = params.getVal(PARAM_CANCER_CELL_PROGENITOR_DIV_MAX);

	_celltype_cdf[0] = pstem;
	_celltype_cdf[1] = 1.0;
	for (size_t i = 2; i <= dmax+2; i++)
	{
		_celltype_cdf[i] = 1.0;
	}
	/*
	std::cout << "cancer cell cdf:" << std::endl;
	for (size_t i = 0; i < _celltype_cdf.size(); i++)
	{
		std::cout << i << ", " << _celltype_cdf[i] << std::endl;
	}
	*/
	return;
}
};// end of namespace
};