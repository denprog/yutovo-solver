#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "solver.h"

namespace yutovo_service
{

struct ServiceContext
{
    Solvers solvers;
    bool exit = false;
};

}

#endif
