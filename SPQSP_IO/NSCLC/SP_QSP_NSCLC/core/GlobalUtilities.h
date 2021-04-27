#pragma once

#include <boost/random.hpp>
#include <boost/serialization/nvp.hpp>

#include <sstream>
#include <string>

#include "Param.h"
#include "SP_QSP_shared/ABM_Base/RNG.h"


extern RNG rng;

namespace SP_QSP_IO{
namespace SP_QSP_NSCLC{
/*!
\file
\brief Global variables declared in this file
*/

//! global variable, holds values of all parameters
extern Param params;
};
};

