#include "BioFVMGrid.h"

#include <stdexcept>
#include <sstream>
#include <algorithm>

//using namespace BioFVM;

namespace SP_QSP_IO{
/*
*/
BioFVMGrid::BioFVMGrid()
	:_tme()
	, _sink_source()
	, _nrVoxel(0)
	, _nrSubstrate(0)
{
}


/*!
    Do not try to access sink/source agents from other objects
	After this desctructor is called. (e.g. attempting to flag
	them dead in CellAgent in that desctructor, if it's called
	later than this).
*/
BioFVMGrid::~BioFVMGrid()
{
	//delete all sink/source if they still exist
	auto last = std::remove_if(_sink_source.begin(), _sink_source.end(),
		[](BioFVMSinkSource* pS){delete pS; return true; });
	_sink_source.erase(last, _sink_source.end());

}

/*! solve source/sink dynamics for dt
*/
void BioFVMGrid::source_sink_step(double dt){
	//_tme.simulate_cell_sources_and_sinks( dt );
	for (auto const &pS : _sink_source)
	{
		int substrate = pS->get_substrate();
		int index = pS->get_index();
		double c = _tme.density_individual(index, substrate);
		double c_new = pS->evaluate_source_sink(c, dt);
		_tme.density_vector(index)[substrate] = c_new;
	}
}


/*! get value of(x,y,z); x, y, z are integer voxel coordinates
    substrate i
*/
double BioFVMGrid::operator()(const Coord3D& c, size_t i)const{
	unsigned int idx = get_voxel_idx(c);
	return _tme.density_individual(idx, i);
}

/*! get value at (x,y,z); x,y,z are real number coordinates
    substrate i
*/
double BioFVMGrid::operator()(double x, double y, double z, size_t i)const{
	auto coord_r = std::vector<double>({ x, y, z });
	size_t idx = _tme.nearest_voxel_index(coord_r);
	return _tme.density_individual(idx, i);
}
/*! set values of entire grid.
    Used in deserialization or initial concentration setup.
    Should be called after setup_biofvm_grid
*/
void BioFVMGrid::setup_concentrations(const chem_grid& g){
	//! check if size matches
	if (g.size()!= _nrVoxel || g[0].size()!= _nrSubstrate)
	{
		std::stringstream ss;
		ss << "Substrate grid dimensions do not match:" << std::endl;
		ss << "input: voxel number: " << g.size();
		ss << ", substrate number: " << g[0].size() << std::endl;
		ss << "Grid: voxel number: " << _nrVoxel;
		ss << ", substrate number: " << _nrSubstrate << std::endl;
		throw std::invalid_argument(ss.str());
	}
	//! assign values
	for (size_t i = 0; i < _nrVoxel; i++)
	{
		for (size_t j = 0; j < _nrSubstrate; j++)
		{
			_tme.density_vector(i)[j] = g[i][j];
		}
	}

	return;
}

/*! Get the concentrations of all substrates. 
    This can be used to interface with other 
	part of the simulation. Update in case that
	BioFVM internal storage structure or access
	methods changes
*/
BioFVMGrid::chem_grid BioFVMGrid::get_concentrations(void) const{
	//size_t nrSubstrate = _tme.number_of_densities();
	chem_grid g;
	for (size_t i = 0; i < _nrVoxel; i++)
	{
		std::vector<double> v;
		for (size_t j = 0; j < _nrSubstrate; j++)
		{
			v.push_back(_tme.density_individual(i, j));
		}
		g.push_back(v);
	}
	return g;
}

/*! get total amount of cm in grid 
*/
double BioFVMGrid::get_total_amount(size_t chem){
	double amount = 0;
	double v_voxel = (_tme.voxels(0)).volume;
	for (size_t i = 0; i < _nrVoxel; i++)
	{
		amount += _tme.density_individual(i, chem);
	}
	amount *= v_voxel;
	return amount;

}

//! get average concentration
double BioFVMGrid::get_average_concentration(size_t chem){
	double amount = get_total_amount(chem);
	double v_voxel = (_tme.voxels(0)).volume;
	return amount/(v_voxel*_nrVoxel);
}

std::string BioFVMGrid::get_substrate_names(void)const {
	
	std::stringstream ss;
	auto delim = "";
	for (size_t i = 0; i < _nrSubstrate; i++)
	{
		ss << delim << _tme.density_names[i];
		delim = ",";
	}
	return ss.str();
}

/*! Solve for dt*/
void BioFVMGrid::timestep(double dt){
#if SOURCE_SINK_SPLIT_SECOND_ORDER 
	source_sink_step(dt/2);
	_tme.simulate_diffusion_decay( dt );
	source_sink_step(dt/2);
#else
	source_sink_step(dt);
	_tme.simulate_diffusion_decay( dt );
#endif
}

//! get voxel index from xyz coordinates (real value)
unsigned int BioFVMGrid::get_voxel_idx(std::vector<double>& pos) const {
	unsigned int idx = _tme.nearest_voxel_index(pos);
	return idx;
}

//! get voxel index from xyz coordinates (integer)
unsigned int BioFVMGrid::get_voxel_idx(const Coord3D& c) const {
	static int offset_z = _tme.mesh.y_coordinates.size() * _tme.mesh.x_coordinates.size();
	static int offset_y = _tme.mesh.x_coordinates.size();
	unsigned int idx = c.z * offset_z + c.y * offset_y + c.x;
	return idx;
}

//! get integer coords from voxel index 
Coord3D BioFVMGrid::idx_to_coords(unsigned int idx) const{
	static int offset_z = _tme.mesh.y_coordinates.size() * _tme.mesh.x_coordinates.size();
	static int offset_y = _tme.mesh.x_coordinates.size();
	int x, y, z, xy;
	z = idx / offset_z;
	xy = idx % offset_z;
	y = xy / offset_y;
	x = xy % offset_y;
	return Coord3D(x, y, z);
}



//! move source location
void BioFVMGrid::move_source_sink(BioFVMSinkSource* s, std::vector<double>& pos) {
	unsigned int idx = get_voxel_idx(pos);
	s->set_location(idx);
	return;
}

//! move source location (by integer voxel coordinate)
void BioFVMGrid::move_source_sink(BioFVMSinkSource* s, const Coord3D& c){
	unsigned int idx = get_voxel_idx(c);
	s->set_location(idx);
	return;
}

//! add a point source
BioFVMSinkSource* BioFVMGrid::add_point_source(size_t chem, std::vector<double>& pos,
	double vol, double rate, double saturation, double dt){
	auto s = new BioFVMSinkSource();
	double volV = (_tme.voxels(0)).volume;
	set_source_sink_substrate_location(s, chem, pos);
	s->update_source(vol, volV, dt, rate, saturation);
	_sink_source.push_back(s);
	return s;
}

	//! add a point sink
BioFVMSinkSource* BioFVMGrid::add_point_sink(size_t chem, std::vector<double>& pos,
	double vol, double rate, double dt){
	auto s = new BioFVMSinkSource();
	double volV = (_tme.voxels(0)).volume;
	set_source_sink_substrate_location(s, chem, pos);
	s->update_sink(vol, volV, dt, rate);
	_sink_source.push_back(s);
	return s;
}

BioFVMSinkSource* BioFVMGrid::add_point_sink_source(size_t chem, std::vector<double>& pos,
	double vol, double rate, double saturation, double uptake, double dt){
	auto s = new BioFVMSinkSource();
	double volV = (_tme.voxels(0)).volume;
	set_source_sink_substrate_location(s, chem, pos);
	s->update_sink_source(vol, volV, dt, rate, saturation, uptake);
	_sink_source.push_back(s);
	return s;

}

/* Remove sink/source flagged dead
   Only need to do this after agent steps.
   Iterate through vector, if a source/sink is dead,
   delete the object and move it to the end; 
   earase element after the last living source/sink.
*/
void BioFVMGrid::remove_dead_source_sink(void){
	auto last = std::remove_if(_sink_source.begin(), _sink_source.end(), 
		[](BioFVMSinkSource* pS){
		if (pS->is_to_remove())
		{
			delete pS;
			return true;
		}
		else{
			return false;
		}
		});
	_sink_source.erase(last, _sink_source.end());
}

void BioFVMGrid::set_dirichlet(std::vector<double>pos, std::vector<double> val) {
	int idx = _tme.nearest_voxel_index(pos);
	_tme.add_dirichlet_node( idx , val);
	return;
}
void BioFVMGrid::set_source_sink_substrate_location(BioFVMSinkSource* s, size_t c, std::vector<double> pos){
	s->set_substrate(c);
	s->set_location(get_voxel_idx(pos));
	return;
}
/*****************************************************/
//
// BioFVMSinkSource related function definitions
//
/*****************************************************/

BioFVMSinkSource::BioFVMSinkSource()
	:_substrate(0)
	,_voxel_index(0)
	, _solver_temp1(0)
	, _solver_temp2(0)
	, _dead(false)
{

}

/*!
*/
void BioFVMSinkSource::set_location(size_t idx){
	_voxel_index = idx;
	return;
}

//! update source parameters
void BioFVMSinkSource::update_source(double vol, double volV, double dt, double sec, double saturate){
	update_sink_source(vol, volV, dt, sec, saturate, 0);
}
//! update sink parameters
void BioFVMSinkSource::update_sink(double vol, double volV, double dt, double uptake){
	update_sink_source(vol, volV, dt, 0, 0, uptake);
}
/*! update point sink/source parameters
*/
void BioFVMSinkSource::update_sink_source(double vol, double volV, double dt, 
	double sec, double saturation, double uptake){
#if SOURCE_SINK_SPLIT_SECOND_ORDER 
	set_internal_rates(vol, volV, dt/2, sec, saturation, uptake);
#else
	set_internal_rates(vol, volV, dt, sec, saturation, uptake);
#endif
}

/*! calculate new concentration after dt
    both implicit and analytical versions conform to this equation
*/
double BioFVMSinkSource::evaluate_source_sink(double c, double dt)const{
	return (c + _solver_temp1) / _solver_temp2;
}

/*! set internal rates
    choose which version to use to discretize source/sink
void BioFVMSinkSource::set_internal_rates(double vol, double volV, double dt, 
	double sec, double saturate, double uptake){
	//set_internal_rates_implicit(vol, volV, dt, sec, saturate, uptake);
	//set_internal_rates_analytical(vol, volV, dt, sec, saturate, uptake);
	set_internal_rates_analytical(volV, dt, sec, uptake);
	return;
}
*/

#if SOURCE_SINK_INTERNAL == SOURCE_SINK_IMPLICIT
/*! Internal rates
	Implicit time discretization
    Based on:
	void BioFVM::Basic_Agent::set_internal_uptake_constants( double dt );
*/
void BioFVMSinkSource::set_internal_rates(double vol, double volV, double dt, 
	double sec, double saturate, double uptake){
	// overall form: dp/dt = S*(T-p) - U*p 
	//   p(n+1) - p(n) = dt*S(n)*T(n) - dt*( S(n) + U(n))*p(n+1)
	//   p(n+1)*temp2 =  p(n) + temp1
	//   p(n+1) = (  p(n) + temp1 )/temp2
	//int nearest_voxel= current_voxel_index;
	double internal_constant_to_discretize_the_delta_approximation = dt * vol / volV;

	// temp1 = dt*(V_cell/V_voxel)*S*T 
	_solver_temp1 = 0;
	_solver_temp1 += sec; 
	_solver_temp1 *= saturate; 
	_solver_temp1 *= internal_constant_to_discretize_the_delta_approximation; 

	// temp2 = 1 + dt*(V_cell/V_voxel)*( S + U )
	_solver_temp2 = 1;
	_solver_temp2 += internal_constant_to_discretize_the_delta_approximation * sec;
	_solver_temp2 += internal_constant_to_discretize_the_delta_approximation * uptake;
	return;
}

#elif SOURCE_SINK_INTERNAL == SOURCE_SINK_ANALYTICAL 
/*! solve linear ODE
    Units of sec and uptake are fractional concentration change rates within cell
    dp/dt = s*(T-p) - u*p
	p = c*exp(-(u+s)*t)+sT/(u+s)
	p(t+dt) = (p(t) - sT/(u+s))*exp(-(u+s)*dt) + sT/(u+s)
			= (p(t) + temp1)/temp2
	temp2 = exp((s+u)*dt)
	temp1 = (temp2-1)*sT/(u+s)
*/
void BioFVMSinkSource::set_internal_rates(double vol, double volV, double dt, 
	double sec, double saturate, double uptake){

	double internal_constant_to_discretize_the_delta_approximation = dt * vol / volV;

	// temp2 = exp(dt*(V_cell/V_voxel)*(s+u))
	_solver_temp2 = sec + uptake;
	_solver_temp2 *= internal_constant_to_discretize_the_delta_approximation;
	_solver_temp2 = exp(_solver_temp2);

	// temp1 = (temp2-1)*T*s/(u+s)
	_solver_temp1 = _solver_temp2 - 1;
	_solver_temp1 *= saturate*sec / (sec + uptake);
	return;
}

#else
/*! analytically solve source/sink step, no saturation
	Units of sec: amounts per time.
	Units of uptake: fractional rate (time^-1).
    different from above assumption in that:
	# no saturation
	# source unit is amount/time, instead of concentation/time. no need to multiply with cell volume
    dp/dt = s - u*p
	i. u!=0:
	p(t+dt) = (p(t) - s/u)*exp(-u*dt) + s/u
			= (p(t) + temp1)/temp2
	temp2 = exp(u*dt)
	temp1 = (temp2-1)*(s/u)
	ii. u == 0:
    p(t+dt) = p(t)+s*dt
	temp1 = s*dt
	temp2 = 1
*/
void BioFVMSinkSource::set_internal_rates(double vol, double volV, double dt, 
	double sec, double saturate, double uptake){
	double s = sec  / volV;
	
	double uptake_min = 1e-10;
	if ( uptake > uptake_min) {
		_solver_temp2 = uptake * dt;
		_solver_temp2 = exp(_solver_temp2);

		_solver_temp1 = _solver_temp2 - 1;
		_solver_temp1 *= s / uptake;
	}
	else {
		_solver_temp1 = s * dt;
		_solver_temp2 = 1;
	}
	return;
}
};
#endif