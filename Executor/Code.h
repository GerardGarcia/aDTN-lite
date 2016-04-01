#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include <type_traits>

#include "CompilerBackend/ClangCompilerBackend.h"
#include "ExecBackend/eBPFExecBackend.h"

using std::string;
using std::exception;
using std::shared_ptr;
using std::make_shared;

template< 
    typename TCompilerBackend = ClangCompilerBackend<eBPFExecObject>, 
    typename TExecBackend = eBPFExecBackend<> > 
class Code {
    public:
        Code(string _code) :  
            m_compilerBackend {nullptr},
            m_execBackend {nullptr},
            m_code {_code} {};
        
        Code(shared_ptr<TCompilerBackend> _compilerBackend,
                shared_ptr<TExecBackend> _execBackend,
                string _code) :  
            m_compilerBackend {_compilerBackend},
            m_execBackend {_execBackend},
            m_code {_code} {
                static_assert(std::is_same<typename TExecBackend::exec_obj_type, typename TCompilerBackend::exec_obj_type>::value, "Execution backend and \
                        compialtion backend must use the same execution object type");
            };

        typename TExecBackend::return_type execute();

        const string &getCode() const {
            return m_code;
        }

        void setCompilerBackend(shared_ptr<TCompilerBackend> _compilerBackend) const {
            m_compilerBackend = _compilerBackend;
        }
        shared_ptr<TCompilerBackend> getCompilerBackend() {
            return m_compilerBackend;
        }

        void setExecBackend(shared_ptr<TCompilerBackend> _execBackend) const {
            m_execBackend = _execBackend;
        }
        shared_ptr<TCompilerBackend> getExecBackend() {
            return m_execBackend;
        }

        class invalid_code: virtual exception{};

    protected:
        shared_ptr<TCompilerBackend> m_compilerBackend;
        shared_ptr<TExecBackend> m_execBackend;
        string m_code;
};

// See: https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template<typename TCompilerBackend, typename TExecBackend>
typename TExecBackend::return_type Code<TCompilerBackend, TExecBackend>::execute() {
   if (!m_compilerBackend)
       m_compilerBackend = make_shared<TCompilerBackend>();

    typename TCompilerBackend::exec_obj_type execObj;
    if (m_compilerBackend->compile(m_code, execObj) != 0) 
        throw new invalid_code;

    if (!m_execBackend) 
        m_execBackend = make_shared<TExecBackend>();

    m_execBackend->loadObj(execObj);
    typename TExecBackend::return_type result;
    if(m_execBackend->execute(result) != 0) 
        throw new invalid_code;

    return result;
}
