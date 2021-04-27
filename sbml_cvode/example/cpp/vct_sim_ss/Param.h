#ifndef __CancerVCT_PARAM__
#define __CancerVCT_PARAM__

#include "ParamBase.h"

namespace CancerVCT{

class Param: public ParamBase
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
#endif
