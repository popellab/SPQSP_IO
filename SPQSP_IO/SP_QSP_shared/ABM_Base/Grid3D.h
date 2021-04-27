#ifndef __GRID_3D__
#define __GRID_3D__

#include <vector>
#include <iostream>
#include <string>
#include <sstream>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>

#include "Coord3D.h"

namespace SP_QSP_IO{

template <typename T>
class Grid3D
{
public:
	//default constructor for serialization
	Grid3D(){};
	Grid3D(unsigned int size, unsigned int y, unsigned int z, T element);
	~Grid3D(){};
	bool inGrid(int x, int y, int z) const;
	bool inGrid(const Coord3D&) const;
	bool isType(int x, int y, int z, T val) const;
	int getSize(void)const{ return _data.size(); };

	//! content of grid at x, y, z
	T& operator()(int x, int y, int z); 
	T& operator()(const Coord3D& c); 
	const T& operator()(const Coord3D& c)const; 
	//! content of grid at x, y, z; when accessed from const function
	T get(int x, int y, int z) const;
	//! get coord assuming grid is toroidal
	Coord3D get_toroidal(const Coord3D& c)const;
	Coord3D get_coord(int i)const;

	//! Another way to change the value of an element.
	void set(int x, int y, int z, T val);
	
	
	//! output grid data, in C-style order (row-major)
	inline
	friend std::ostream & operator<<(std::ostream &os, const Grid3D& g) {

		os << "x:" << g._xSize << ",";
		os << "y:" << g._ySize << ",";
		os << "z:" << g._zSize << ",";
		for (size_t i = 0; i < g._data.size(); i++)
		{
			os << g._data[i] << ",";
		}
		return os;
	}

	//! output grid data, when T is a class pointer typ and operator<< do not apply.
	std::string toStringFromPtr(void)const;

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);
	
	unsigned int _xSize;
	unsigned int _ySize;
	unsigned int _zSize;
	unsigned int _xshift;
	unsigned int _yshift;
	std::vector< T > _data;

	T get(std::size_t idx){ return _data[idx]; };
	void set(std::size_t idx, T val){ _data[idx] = val; };
	bool coordToIdx(int x, int y, int z, unsigned int & idx) const;
	
};

template <typename T> 
template<class Archive>
inline void Grid3D< T >::serialize(Archive & ar, const unsigned int /* version */){
	ar & BOOST_SERIALIZATION_NVP(_xSize);
	ar & BOOST_SERIALIZATION_NVP(_ySize);
	ar & BOOST_SERIALIZATION_NVP(_zSize);
	ar & BOOST_SERIALIZATION_NVP(_xshift);
	ar & BOOST_SERIALIZATION_NVP(_yshift);
	ar & BOOST_SERIALIZATION_NVP(_data);
}

template <typename T>
Grid3D< T >::Grid3D(unsigned int x, unsigned int y, unsigned int z, T element)
: _xSize(x)
, _ySize(y)
, _zSize(z)
, _xshift(y*z)
, _yshift(z)
, _data(x*y*z, element)
{
}

template <typename T>
bool Grid3D< T >::inGrid(int x, int y, int z)const{
	return !(x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize || z < 0 || z >= (int)_zSize);
}

template <typename T>
bool Grid3D< T >::inGrid(const Coord3D& c)const{
	return inGrid(c.x, c.y, c.z);
}

template <typename T>
bool Grid3D< T >::isType(int x, int y, int z ,T val) const {
	unsigned int idx = 0;
	if (coordToIdx(x, y, z, idx))
	{
		return _data[idx] == val;
	}
	else{
		return false;
	}
}

template <typename T>
T& Grid3D <T>::operator()(int x, int y, int z){
	unsigned int idx = 0;
	if (coordToIdx(x, y, z, idx))
	{
		return _data[idx];
	}
	else{
		throw std::invalid_argument("Grid3D::get: Coordinates out of grid");
	}
}

template <typename T>
T& Grid3D <T>::operator()(const Coord3D& c){
	return operator()(c.x, c.y, c.z);
}

template <typename T>
const T& Grid3D <T>::operator()(const Coord3D& c)const{
	unsigned int idx = 0;
	if (coordToIdx(c.x, c.y, c.z, idx))
	{
		return _data[idx];
	}
	else{
		throw std::invalid_argument("Grid3D::get: Coordinates out of grid");
	}
}

template<typename T>
std::string Grid3D<T>::toStringFromPtr(void)const {
	std::stringstream ss;
	for (size_t i = 0; i < _data.size(); i++)
	{
		ss << *(_data[i]) << ",";
	}
	ss << std::endl;
	return ss.str();
}

template<typename T>
T Grid3D<T>::get(int x, int y, int z) const {
	unsigned int idx = 0;
	if (coordToIdx(x, y, z, idx))
	{
		return _data[idx];
	}
	else{
		throw std::invalid_argument("Grid3D::get: Coordinates out of grid");
	}
}


template<typename T>
Coord3D Grid3D<T>::get_toroidal(const Coord3D& c0) const {
	//auto c = Coord3D(c0.x % _xSize, c0.y % _ySize, c0.z % _zSize);
	auto c = c0 % Coord3D(_xSize, _ySize, _zSize);
	return c;
}

template<typename T>
Coord3D Grid3D<T>::get_coord(int i) const {
	int x, y, z;
	x = i / _xshift;
	y = (i - x*_xshift) / _yshift;
	z = i%_yshift;
	auto c = Coord3D(x, y, z);
	return c;
}
template<typename T>
void Grid3D< T >::set(int x, int y, int z, T val){
	unsigned int idx = 0;
	if (coordToIdx(x, y, z, idx))
	{
		set(idx, val);
	}
	else{
		throw std::invalid_argument("Grid3D::set: Coordinates out of grid");
	}
}

template <typename T>
bool Grid3D < T >::coordToIdx(int x, int y, int z, unsigned int & idx) const{
	if (inGrid(x, y, z))
	{
		idx = x*_xshift + y*_yshift+ z;
		return true;
	}
	else{
		return false;
	}
}

};
#endif