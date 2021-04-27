#include "Cell_Tumor.h"
#include "../../core/GlobalUtilities.h"

#include <iostream>
#include <sstream>

#include "../compartment/Tumor.h"

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

Cell_Tumor::Cell_Tumor(SpatialCompartment* c)
	:CellAgent(c)
	, _PDL1_syn(0)
	, _drop_out(false)
{
}

Cell_Tumor::Cell_Tumor(const Cell_Tumor& c)
	:CellAgent(c)
	, _PDL1_syn(c._PDL1_syn)
	, _drop_out(false)
{
}

Cell_Tumor::~Cell_Tumor()
{
}

std::string Cell_Tumor::toString()const{
	std::stringstream ss;
	ss << CellAgent::toString();
	return ss.str();
}

bool Cell_Tumor::agent_state_step(double t, double dt, Coord& c){ 
	reset_PDL1();
	return false; 
};

void Cell_Tumor::setDead(void){
	CellAgent::setDead(); 
	remove_all_source_sink();
	return;
}

void Cell_Tumor::set_drop_out(){
	_drop_out = true;
	remove_all_source_sink();
	return;
}


/* set coordinates of cell
	reset source/sink location too if this cell has them.
*/
void Cell_Tumor::setCoord(const Coord& c){
	CellAgent::setCoord(c);
	move_all_source_sink();
};

//! get cell coordinates, in real unit
std::vector<double> Cell_Tumor::get_coord_real(double voxel_size)const {
	Coord c = getCoord();
	std::vector<double> pos = { c.x*voxel_size,  c.y*voxel_size,  c.z*voxel_size };
	return pos;
}

Tumor& Cell_Tumor::get_tumor(void)const{
	return dynamic_cast<Tumor&>(*_compartment);
}

/*! setup source
*/
void Cell_Tumor::setup_chem_source(BioFVMSinkSource*& s, chem_ID i, double rate){
	static double voxel_size = params.getVal(PARAM_VOXEL_SIZE_CM);
	static double dt = params.getVal(PARAM_SEC_PER_TIME_SLICE) / 
		params.getVal(PARAM_MOLECULAR_STEP_PER_SLICE);
	auto pos = get_coord_real(voxel_size);
	//double release_rate= params.getVal(PARAM_IL_2_RELEASE);// amount/sec
	s = get_tumor().get_chem_grid().add_point_source(i, pos, 0.0, rate, 0.0, dt);
	return;
}

void Cell_Tumor::update_chem_source(BioFVMSinkSource* const s, double rate){
	static double voxel_size = params.getVal(PARAM_VOXEL_SIZE_CM);
	static double volV = voxel_size * voxel_size * voxel_size;
	static double dt = params.getVal(PARAM_SEC_PER_TIME_SLICE) / 
		params.getVal(PARAM_MOLECULAR_STEP_PER_SLICE);
	s->update_source(0.0, volV, dt, rate, 0.0);
}

void Cell_Tumor::setup_chem_sink(BioFVMSinkSource*& s, chem_ID i, double rate){
	static double voxel_size = params.getVal(PARAM_VOXEL_SIZE_CM);
	static double dt = params.getVal(PARAM_SEC_PER_TIME_SLICE) / 
		params.getVal(PARAM_MOLECULAR_STEP_PER_SLICE);
	auto pos = get_coord_real(voxel_size);
	s = get_tumor().get_chem_grid().add_point_sink(i, pos, 0.0, rate, dt);
	return;
}

void Cell_Tumor::move_source_sink(BioFVMSinkSource* const s)const{
	if (s)
	{
		get_tumor().get_chem_grid().move_source_sink(s, _coord);
	}
	return;
}
/*! remove a source/sink from agent
*/
void Cell_Tumor::remove_source_sink(BioFVMSinkSource*& s){
	if (s){
		s->set_remove();
		s= NULL;
	}
	return;
}

/*! Hill equation
	h = X^n/(Ka^n+X^n)
*/
double Cell_Tumor::get_Hill_equation(double X, double Ka, double n){
	double L = std::pow(X/Ka, n);
	return L / (1 + L);
}
std::string Cell_Tumor::getRemark() const{
	std::ostringstream os;
	os << _PDL1_syn * params.getVal(PARAM_AVOGADROS);
	return os.str();
}

//! high PDL1
bool Cell_Tumor::is_PDL1_pos(void)const{
	static double highPDL1 = params.getVal(PARAM_PDL1_HIGH_TH) * params.getVal(PARAM_PDL1_SYN_MAX);
	return _PDL1_syn > highPDL1;

}
/*! reset PDL1 level
	IFNg concentration will affect this.
*/
void Cell_Tumor::reset_PDL1(void){
	double minPDL1 = params.getVal(PARAM_PDL1_SYN_MAX);
	if (params.getVal(PARAM_DIFFUSION_ON))
	{

		double IFNg = get_tumor().get_chem(_coord, CHEM_IFN);
		double IFNg50 = params.getVal(PARAM_IFN_G_PDL1_HALF);
		double coef = params.getVal(PARAM_IFN_G_PDL1_N);
		double hill = get_Hill_equation(IFNg, IFNg50, coef);
		minPDL1 *= hill;
	}

	// decay
	_PDL1_syn *= params.getVal(PARAM_PDL1_DECAY_SLICE);

	if (_PDL1_syn < minPDL1)
	{
		_PDL1_syn = minPDL1;
	}
	/*
	std::cout <<"IFNg: " << IFNg << ", PDL1_syn: " << _PDL1_syn <<
		", hill: " << hill << ", min: " << minPDL1 << std::endl;
	*/
	return;
}

ShapeVoxel Cell_Tumor::_class_shape = ShapeVoxel();

};
};