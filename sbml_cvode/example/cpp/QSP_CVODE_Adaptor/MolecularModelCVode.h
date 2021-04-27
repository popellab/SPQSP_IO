#ifndef __MOLECULAR_MODEL_CVODE__
#define __MOLECULAR_MODEL_CVODE__

#include <iostream>
#include <fstream>
#include <math.h>
#include <assert.h>

#include <vector>
#include <string>

#include <boost/serialization/nvp.hpp>

template <class T>
class MolecularModelCVode {
public:
	MolecularModelCVode() :_model(){};
	~MolecularModelCVode(){};
	bool solve(double tStart,  double tStep);
	T* getSystem(void) { return & _model; };

	friend std::ostream & operator<<(std::ostream &os, const MolecularModelCVode & m){
		return os << m._model;
	};

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	T _model;
};
template <class T> bool MolecularModelCVode < T >::solve(double tStart, double tStep){
	try{
		_model.simOdeStep(tStart, tStep);
	}
	catch (std::string s){
		std::cerr << s << std::endl;
	}
	catch (...) {
		std::cerr << "Error solving ODE" << std::endl;
	}
	return true;
}

template <class T> 
template<class Archive>
inline void MolecularModelCVode< T >::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_NVP(_model);
}


#endif