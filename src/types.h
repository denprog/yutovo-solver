#ifndef __TYPES_H__
#define __TYPES_H__

#include <vector>

typedef unsigned int uint;

namespace yutovo_service
{

enum class SolverType
{
    NONE = 0,
    CALCULATOR = 1,
    PYTHON = 2
};

//Returned result type
enum class ResultType
{
	NONE = 0, 
	REAL, 
	INTEGER, 
	RATIONAL, 
	COMPLEX, 
	AUTO
};

enum class ErrorCode
{
    OK = 0,
    UNKNOWN_COMMAND,
    JSON_ERROR, //json was not parsed
    NO_FIELD_ERROR, //there is no field requeried
    FIELD_ERROR, //a field is wrong
    SOLVER_ERROR, //solver was not created or executed
    PARSER_ERROR, //expression was not parsed
    OPERATION_ERROR, //there was error
    SOLVER_RESTARTED_ERROR //all connected code blocks need to be reevaluated
};

struct ServiceException
{
    ErrorCode error_code;
};

typedef std::vector<uint> ElementId;

}

#endif
