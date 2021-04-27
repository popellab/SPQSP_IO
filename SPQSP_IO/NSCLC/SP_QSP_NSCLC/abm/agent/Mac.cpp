//#include <boost/serialization/export.hpp>
#include "Mac.h" 

//BOOST_CLASS_EXPORT_IMPLEMENT(Mac)

#include <iostream>
#include <sstream>

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{

Mac::Mac(SpatialCompartment* c)
	:Cell_Tumor(c)
{
}

Mac::Mac(const Mac& c)
	:Cell_Tumor(c)
{
}

Mac::~Mac()
{
}

std::string Mac::toString()const{
	std::stringstream ss;
	ss << Cell_Tumor::toString();
	return ss.str();
}
};
};