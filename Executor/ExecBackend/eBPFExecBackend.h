#pragma once

#include <cstdio>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>

#ifdef PERF
#include "papi.h"
#endif

extern "C" {
#include "vm/ubpf_int.h"
}

#include "Utils/Logger.h"
#include "ExecBackend.h"
#include "Executor/CompilerBackend/ClangCompilerBackend.h"

using std::vector;
using std::string;
using std::unordered_map;
using std::mutex;
using std::endl;

template<typename ResultTypeT = int> 
class eBPFExecBackend : public ExecBackend<eBPFExecObject, ResultTypeT> {
    public:
        eBPFExecBackend() {};
        eBPFExecBackend(unordered_map<string, string> *_contextMap) {};

        void setJIT();
        void unsetJIT();

        int loadObj(eBPFExecObject _execObj) override;

        void registerContextFunctions();
        void setContextMap(unordered_map<string, string> *_contextMap);
       
        vector<uint64_t> geteBPFInstructions();
        unsigned int getNumberOfeBPFInstructions();

        void *getJittedFunction();
        unsigned int getJittedSize();
        
        int execute(ResultTypeT &_result) override;

    private:
        unordered_map<string, string> *m_contextMap; 
        mutex m_exec_mutex;
        bool m_jitEnabled;

        struct ubpf_vm *m_vm;
        ubpf_jit_fn m_fn;
        vector<uint8_t> m_obj;
};


// Unammed namespace to avoid exporting symbols outside this file
namespace {
    unordered_map<string, string> *g_contextMap; 

    extern "C" void set(const char *key, const char *value) {
        g_contextMap->emplace(key, value);
    }

    extern "C" const char *get(const char *key) {
        auto value = g_contextMap->find(key);
        if (value != g_contextMap->end()) 
            return value->second.c_str();
        else
            return "";
    }
    
    extern "C" int strcmp_ext(const char *a, const char *b) {
        return strcmp(a,b);
    }
}

template<>
eBPFExecBackend<>::eBPFExecBackend(): m_jitEnabled {true} {
    m_vm = ubpf_create();
}

template<>
eBPFExecBackend<>::eBPFExecBackend(unordered_map<string, string> *_contextMap): m_jitEnabled {true} {
    m_vm = ubpf_create();
    setContextMap(_contextMap);
    registerContextFunctions();
}

template<typename ResultTypeT>
void eBPFExecBackend<ResultTypeT>::setJIT() {
    m_jitEnabled = true;
}

template<typename ResultTypeT>
void eBPFExecBackend<ResultTypeT>::unsetJIT() {
    m_jitEnabled = false;
}

template<typename ResultTypeT>
int eBPFExecBackend<ResultTypeT>::loadObj(eBPFExecObject _execObj){
    m_obj = _execObj.geteBPFByteCode();

    // Load file into VM
    char *errmsg;
    if (ubpf_load_elf(m_vm, m_obj.data(), m_obj.size(), &errmsg) < 0){
        LOG(2) << "Failed to load code: " << errmsg << endl;
        return 1;
    }

    // JIT compilation
    if (m_jitEnabled) {

#ifdef PERF
        INIT_CHRONO();
        START_CHRONO();
        int Events[2] = { PAPI_TOT_CYC, PAPI_TOT_INS };
        PAPI_start_counters(Events, 2);
#endif
    
        m_fn = ubpf_compile(m_vm, &errmsg);
    
#ifdef PERF
        long long values[2] = {0};
        PAPI_stop_counters(values, 2);
        END_CHRONO();
        LOG(100) << "eBPF->Native: " <<  GET_CHRONO_SECS() << "s, " << values[0] << " instructions, " << values[1] << " cycles." ;
#endif

        if (!m_fn) {
            LOG(2) << "Failed to JIT compile code: " << errmsg;
            return 1;
        }
    }

    return 0;
}

template<typename ResultTypeT>
unsigned int eBPFExecBackend<ResultTypeT>::getNumberOfeBPFInstructions() {
    return m_vm->num_insts;
}

template<typename ResultTypeT>
vector<uint64_t> eBPFExecBackend<ResultTypeT>::geteBPFInstructions() {
    vector<uint64_t> eBPFInstructions;

    for (unsigned i = 0; i < getNumberOfeBPFInstructions(); ++i){
        uint64_t nextInst;
        memcpy(&nextInst, &m_vm->insts[i], sizeof(nextInst));
        eBPFInstructions.push_back(nextInst);
    }

    return eBPFInstructions;
}

template<typename ResultTypeT>
unsigned int eBPFExecBackend<ResultTypeT>::getJittedSize() {
    return m_vm->jitted_size;
}

template<typename ResultTypeT>
void eBPFExecBackend<ResultTypeT>::registerContextFunctions() {
    ubpf_register(m_vm, 1, "set", (void *) set);
    ubpf_register(m_vm, 2, "get", (void *) get);
    ubpf_register(m_vm, 3, "strcmp_ext", (void *) strcmp_ext);
}

template<typename ResultTypeT>
void eBPFExecBackend<ResultTypeT>::setContextMap(unordered_map<string, string> *_contextMap) {
    m_contextMap = _contextMap;
}

template<typename ResultType>
int eBPFExecBackend<ResultType>::execute(ResultType &_result) {
    if (m_jitEnabled && !m_fn) {
        LOG(1) << "Function not loaded.";
        return 1;
    }
    
    std::lock_guard<mutex> lock(m_exec_mutex);

    g_contextMap = m_contextMap;

#ifdef PERF
    int Events[2] = { PAPI_TOT_CYC, PAPI_TOT_INS };
    PAPI_start_counters(Events, 2);
#endif
    
    // Execute main function
    if (m_jitEnabled)
        _result =  m_fn(NULL, 0);
    else
        _result = ubpf_exec(m_vm, NULL,0);

#ifdef PERF
    long long values[2] = {0};
    PAPI_stop_counters(values, 2);
    LOG(100) << "Softawre code instructions: " << values[0];
#endif

    return 0;
}
