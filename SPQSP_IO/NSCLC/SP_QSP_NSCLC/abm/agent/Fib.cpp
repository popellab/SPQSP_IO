//#include <boost/serialization/export.hpp>
#include "Fib.h" 

//BOOST_CLASS_EXPORT_IMPLEMENT(Fib)

#include <iostream>
#include <sstream>

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

Fib::Fib(SpatialCompartment* c)
	:Cell_Tumor(c)
{
}

Fib::Fib(const Fib& c)
	:Cell_Tumor(c)
{
}

Fib::~Fib()
{
}

std::string Fib::toString()const{
	std::stringstream ss;
	ss << Cell_Tumor::toString();
	return ss.str();
}
};
};