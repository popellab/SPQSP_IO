//#include <boost/serialization/export.hpp>
#include "TReg.h" 

//BOOST_CLASS_EXPORT_IMPLEMENT(Treg)

#include <iostream>
#include <sstream>

#include "../../core/GlobalUtilities.h"
#include "../compartment/Tumor.h"
#include "TCell.h"

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

Treg::Treg(SpatialCompartment* c)
	:Cell_Tumor(c)
	//, _source_IL_10(NULL)
{
	_life = getTregLife();
}

Treg::Treg(const Treg& c)
	:Cell_Tumor(c)
	//, _source_IL_10(NULL)
{
	_life = getTregLife();
	//setup_chem_source(_source_IL_10, CHEM_IL_10, params.getVal(PARAM_IL_10_RELEASE));
}

Treg::~Treg()
{
}

std::string Treg::toString()const{
	std::stringstream ss;
	ss << Cell_Tumor::toString();
	return ss.str();
}

bool Treg::agent_movement_step(double t, double dt, Coord& c){
	bool move = false;
	/**/
	if (rng.get_unif_01() < params.getVal(PARAM_TREG_MOVE_PROB))
	{
		// move
		int idx;
		const auto shape = getCellShape();
		if (_compartment->getOneOpenVoxel(shape->getMoveDestinationVoxels(), 
			shape->getMoveDirectionAnchor(), _coord, getType(), idx, rng))
		{
			move = true;
			c = getCellShape()->getMoveDirectionAnchor()[idx] + _coord;
		}
	}
	return move;
}

bool Treg::agent_state_step(double t, double dt, Coord& c){
	bool divide = false;
	if (!isDead())
	{
		_life--;
		if (_life <= 0)
		{
			setDead();
			// remove source when cell die
			return divide;
		}
	}

	const auto shape = getCellShape();
	Cell_Tumor::agent_state_step(t, dt, c);

	return divide;
}

void Treg::move_all_source_sink(void) const
{
	//move_source_sink(_source_IL_10);
}

int Treg::getTregLife(){

	double lifeMean = params.getVal(PARAM_TREG_LIFE_MEAN);

	double tLifeD = rng.get_exponential(lifeMean);

	int tLife = int(tLifeD + 0.5);
	tLife = tLife > 1 ? tLife : 1;
	//std::cout << "random Treg life: " << tLife << std::endl;
	return tLife;
}

};
};