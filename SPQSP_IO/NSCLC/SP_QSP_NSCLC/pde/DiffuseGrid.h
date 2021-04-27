#pragma once

#include "SP_QSP_shared/Numerical_Adaptor/BioFVM/BioFVMGrid.h"
#include <boost/serialization/base_object.hpp>

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

enum chem_ID{
	CHEM_IFN,
	CHEM_IL_2,
	//CHEM_IL_10
	NUM_CHEM_GRID
};

class DiffuseGrid :
	public BioFVMGrid
{
public:
	DiffuseGrid();
	~DiffuseGrid();

	//! initialize tme diffusion grid 
	void setup_biofvm_grid(void);
	//! check if tme can be skipped
	bool grid_skippable(void);

private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	//! minimum total amount of substrates to activate grid
	std::vector<double> _min_substrate; // no need to serialize
};


template<class Archive>
inline void DiffuseGrid::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(BioFVMGrid);
}

};
};
