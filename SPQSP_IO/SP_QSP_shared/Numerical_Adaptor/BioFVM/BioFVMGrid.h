#pragma once

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <vector>
#include <list>

#include "BioFVM/BioFVM.h"
#include "SP_QSP_shared/ABM_Base/Coord3D.h"

namespace SP_QSP_IO{

#define SOURCE_SINK_SPLIT_SECOND_ORDER 0 

#define SOURCE_SINK_IMPLICIT 0 
#define SOURCE_SINK_ANALYTICAL 1 
#define SOURCE_SINK_NO_SATURATION 2 

#define SOURCE_SINK_INTERNAL SOURCE_SINK_NO_SATURATION 

/*! This class handles point sink/source
    A more light-weight version of BioFVM::Basice_Agent,
	Which is more difficult to serialize
*/
class BioFVMSinkSource{

public:
	BioFVMSinkSource();
	~BioFVMSinkSource(){};
	//index in the grid
	size_t get_index(void)const { return _voxel_index; };
	//! set/reset voxel index
	void set_location(size_t idx);
	size_t get_substrate()const { return _substrate; };
	void set_substrate(size_t sub){ _substrate = sub; };
	//! flag to remove
	void set_remove(void) { _dead = true; };
	//! is flagged to remove
	bool is_to_remove(void)const { return _dead; };
	//! update source parameters
	void update_source(double vol, double volV, double dt, double sec, double saturate);
	//! update sink parameters
	void update_sink(double vol, double volV, double dt, double uptake);
	//! update sink/source parameters
	void update_sink_source(double vol, double volV, double dt, double sec, double saturate, double uptake);
	//! calculate new concentration after dt
	double evaluate_source_sink(double c, double dt)const;

private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	//! calculate internal rates from parameters
	void set_internal_rates(double vol, double volV, double dt,
		double sec, double saturate, double uptake);

	//! index of substrate
	size_t _substrate;
	//! index in the voxel vector
	size_t _voxel_index;
	double _solver_temp1;
	double _solver_temp2;
	//! to remove. Should not persist to next agent step so no need to serialize
	bool _dead;
};

template<class Archive>
inline void BioFVMSinkSource::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_NVP(_substrate);
	ar & BOOST_SERIALIZATION_NVP(_voxel_index);
	ar & BOOST_SERIALIZATION_NVP(_solver_temp1);
	ar & BOOST_SERIALIZATION_NVP(_solver_temp2);
}

/*! Wrapper for BioFVM microenvironment.
TODO:
1. Bulk source/sink
2. Dirichlet Boundary Conditions
*/
class BioFVMGrid
{
public:
typedef std::vector<std::vector<double>> chem_grid;

public:
	BioFVMGrid();
	virtual ~BioFVMGrid();

	//! initialize tme diffusion grid 
	virtual void setup_biofvm_grid()=0;

	//! source/sink step
	virtual void source_sink_step(double dt);

	//! get value of(x,y,z); x, y, z are integer voxel coordinates
	double operator()(const Coord3D& c, size_t i)const;
	//! get value at (x,y,z); x,y,z are real number coordinates
	double operator()(double x, double y, double z, size_t i)const;
	//! initialize grid with values
	void setup_concentrations(const chem_grid&);
	//! get concentration values
	chem_grid get_concentrations(void) const;
	//! get total amount of cm in grid 
	double get_total_amount(size_t chem);
	//! get average concentration
	double get_average_concentration(size_t chem);
	//! get number of source/sinks
	int get_num_source_sink(void) const { return _sink_source.size(); };
	//! get substrate name
	std::string get_substrate_names(void)const;
	//! get number of substrate types
	int get_num_substrates(void) const {return _nrSubstrate;};

	//! solve pde for time step
	void timestep(double dt);

	//! get voxel index from xyz coordinates (real value)
	unsigned int get_voxel_idx(std::vector<double> &pos) const;
	//! get voxel index from xyz coordinates (integer)
	unsigned int get_voxel_idx(const Coord3D& c) const;

	//! get integer coords from voxel index 
	Coord3D idx_to_coords(unsigned int idx) const;

	//! vector of source/sink
	std::vector<BioFVMSinkSource*>& get_sink_source(void){ return _sink_source; };

	//! move source location
	void move_source_sink(BioFVMSinkSource* s, std::vector<double>& pos);

	//! move source location (by integer voxel coordinate)
	void move_source_sink(BioFVMSinkSource* s, const Coord3D& c);

	//! add a point source
	BioFVMSinkSource* add_point_source(size_t chem, std::vector<double>& pos,
		double vol, double rate, double saturation, double dt);
	//! add a point sink
	BioFVMSinkSource* add_point_sink(size_t chem, std::vector<double>& pos,
		double vol, double rate, double dt);
	//! add a point sink/source
	BioFVMSinkSource* add_point_sink_source(size_t chem, std::vector<double>& pos,
		double vol, double rate, double saturation, double uptake, double dt);
	//! remove all source/sinks flagged dead
	void remove_dead_source_sink(void);

	//! set voxel containing pos to Dirichlet condition with value val
	void set_dirichlet(std::vector<double>pos, std::vector<double> val);

	//! get BioFVM_microenvironment object
	BioFVM::Microenvironment& get_microenvironment(void){ return _tme; };
	//! get constant version of microenvironment
	const BioFVM::Microenvironment& get_microenvironment(void)const{ return _tme; };

	//! override operator<<
	friend std::ostream & operator<<(std::ostream &os, const BioFVMGrid& g); 

protected:
	//! BioFVM microenvironment object
	BioFVM::Microenvironment _tme;
	//! list of sink and source objects
	std::vector<BioFVMSinkSource*> _sink_source;
	//! total number of voxels
	size_t _nrVoxel;
	//! total number of substrates
	size_t _nrSubstrate;
private:
	friend class boost::serialization::access;
	template<class Archive>
	void save(Archive & ar, const unsigned int /*version*/) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int /*version*/);
	BOOST_SERIALIZATION_SPLIT_MEMBER();

	void set_source_sink_substrate_location(BioFVMSinkSource *, size_t, std::vector<double>);

};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(BioFVMGrid)

template<class Archive>
inline void BioFVMGrid::save(Archive & ar, const unsigned int version) const
{
	chem_grid c = get_concentrations();
	ar << boost::serialization::make_nvp("chem_grid", c);
	ar & BOOST_SERIALIZATION_NVP(_sink_source);
	ar & BOOST_SERIALIZATION_NVP(_nrVoxel);
	ar & BOOST_SERIALIZATION_NVP(_nrSubstrate);
}

template<class Archive>
inline void BioFVMGrid::load(Archive & ar, const unsigned int version)
{
	chem_grid c;
	ar >> boost::serialization::make_nvp("chem_grid", c);
	setup_concentrations(c);
	ar & BOOST_SERIALIZATION_NVP(_sink_source);
	ar & BOOST_SERIALIZATION_NVP(_nrVoxel);
	ar & BOOST_SERIALIZATION_NVP(_nrSubstrate);
}

inline std::ostream & operator<<(std::ostream &os, const BioFVMGrid& g) {
	for (auto const& v : g.get_concentrations()){
		auto delim = "";
		for (auto s : v){
			os << delim << s;
			delim = ",";
		}
		os << std::endl;
	}
	return os;
};

};
