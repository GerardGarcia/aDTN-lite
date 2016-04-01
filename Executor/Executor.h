#pragma once 

#include <map>
#include <vector>

#include "Applicaiton.h"

using namespace std;

class Executor {
public:
    Executor();
    class execution_error: virtual exception{};
};
