#pragma once

#include <map>
#include <utility>

#include "xxhash.h"

#include "CompilerBackend.h"

using std::vector;
using std::map;

/*
 * Compiles codes using Clang/LLVM as backend
 * TODO: Initialize Clang once at constructor or first time is called
 * TODO: Optimizations
 * */

class eBPFExecObject {
    public:
        eBPFExecObject() {};
        eBPFExecObject(vector<uint8_t> _eBPFByteCode): 
            m_eBPFByteCode { _eBPFByteCode} {};

        const vector<uint8_t> geteBPFByteCode() const {
            return m_eBPFByteCode;
        }
        void seteBPFByteCode(const vector<uint8_t> &_bc){
            m_eBPFByteCode = _bc;
        }

    private:
        vector<uint8_t> m_eBPFByteCode;
};

template<typename T = eBPFExecObject>       
class ClangCompilerBackend: public CompilerBackend<T> {
    public:
        ClangCompilerBackend() {};
        int compile(const string _code, T& _execObject) override;
        using Hash = XXH32_hash_t;

    private:
        static constexpr auto m_codeHeader = "int main(){";
        static constexpr auto m_codeFooter = "}";

        using Objects = map<Hash, T>;

        Objects m_objects;

        Hash getHash(const string _code);
        vector<uint8_t> compileEBPFObject(const string _code);
};

