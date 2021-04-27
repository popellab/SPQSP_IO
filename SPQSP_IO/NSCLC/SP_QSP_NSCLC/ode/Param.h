#pragma once

#include "SP_QSP_shared/ABM_Base/ParamBase.h"

namespace QSP_IO{

class Param: public SP_QSP_IO::ParamBase
{
public:
    Param();
    ~Param(){};
    //! get parameter value
    inline double getVal(unsigned int n) const { return _paramFloat[n];};

private:
    //! setup content of _paramDesc
    virtual void setupParam();
    //! process all internal parameters
    virtual void processInternalParams(){};
};

};
