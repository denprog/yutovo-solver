#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "solver.h"
#include "config.h"

namespace yutovo_service
{

struct ServiceContext
{
    ServiceContext(Config* config) :
        solvers(config)
    {
    }

    Solvers solvers;
    bool exit = false;
};

}

#endif
