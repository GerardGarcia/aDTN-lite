#pragma once

#include <string>
#include <vector>
#include <cstdint>

using std::string;

template<typename ExecObjT>
class CompilerBackend {
    public:
        CompilerBackend() {};
        virtual int compile(string _code, ExecObjT& _execObject) = 0; 
            
        // Allows us to know which type of execution objects 
        // generates this compiler backend
        using exec_obj_type = ExecObjT; 
};
