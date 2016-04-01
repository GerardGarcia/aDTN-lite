#include <sstream>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstdio>

#include <unistd.h>
#include <fcntl.h>

#ifdef PERF
#include <papi.h>
#endif

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Basic/TargetInfo.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Module.h>

#include "Utils/Logger.h"
#include "ClangCompilerBackend.h"

using std::vector;
using std::map;
using std::pair;

using namespace clang;
using namespace llvm;

template<>
int ClangCompilerBackend<eBPFExecObject>::compile(const string _code, eBPFExecObject &_execObject) {
    Hash codeHash = getHash(_code);
    Objects::iterator foundObject = m_objects.find(codeHash);
    if (foundObject != m_objects.end()){
        LOG(22) << "Code [" << codeHash << "] found in cache"; 
        _execObject = foundObject->second;
    } else {
        LOG(22) << "Code [" << codeHash << "] not found in cache, compiling.";

#ifdef PERF
        INIT_CHRONO();
        START_CHRONO();
        int Events[2] = { PAPI_TOT_CYC, PAPI_TOT_INS };
        PAPI_start_counters(Events, 2);
#endif

        vector<uint8_t> eBPFByteCode = compileEBPFObject(m_codeHeader + _code + m_codeFooter);

#ifdef PERF  
        long long values[2] = {0};
        PAPI_stop_counters(values, 2);
        END_CHRONO();

        LOG(100) << "C->eBPF: " <<  GET_CHRONO_SECS() << "s, " << values[0] << " instructions, " << values[1] << " cycles." ;
#endif

        _execObject.seteBPFByteCode(eBPFByteCode);

        m_objects.insert(pair<Hash, eBPFExecObject>(codeHash, _execObject));
    }

    return 0;
}

template <typename T>
typename ClangCompilerBackend<T>::Hash ClangCompilerBackend<T>::getHash(const string _code) {
    return XXH32( _code.c_str(), _code.length(), 0); 	
}

template <typename T>
vector<uint8_t> ClangCompilerBackend<T>::compileEBPFObject(const string _code) {
    // Send code trough pipe to stdin
    int codeInPipe[2];
    pipe2(codeInPipe, O_NONBLOCK);
    write(codeInPipe[1], (void *) _code.c_str(), _code.length());
    close(codeInPipe[1]); // We need to close the pipe to send an EOF
    int stdinCopy = dup(STDIN_FILENO);
    dup2(codeInPipe[0], STDIN_FILENO);

    // Prepare reception of code trought stdou
    int codeOutPipe[2];
    pipe(codeOutPipe);
    int stdoutCopy = dup(STDOUT_FILENO);
    dup2(codeOutPipe[1], STDOUT_FILENO);

    // Initialize various LLVM/Clang components
    InitializeAllTargetMCs();   
    InitializeAllAsmPrinters();
    InitializeAllTargets();

    // Prepare compilation arguments
    vector<const char *> args;
    args.push_back("--target=bpf"); // Target is bpf assembly
    args.push_back("-xc"); // Code is in c language
    args.push_back("-"); // Read code from stdiargs.push_back("-strip-debug"); // Strip symbols

    // Initialize CompilerInvocation
    CompilerInvocation *CI = createInvocationFromCommandLine(makeArrayRef(args) , NULL);

    // Create CompilerInstance
    CompilerInstance Clang;
    Clang.setInvocation(CI);

    // Initialize CompilerInstace
    Clang.createDiagnostics();

    // Create and execute action
    CodeGenAction *compilerAction; 
    compilerAction = new EmitObjAction();
    Clang.ExecuteAction(*compilerAction);

    // Get compiled object
    close(codeInPipe[0]);
    vector<uint8_t> objBuffer(2048);
    int bytesRead = read(codeOutPipe[0], objBuffer.data(), objBuffer.size()); 
    objBuffer.resize(bytesRead);

    // Cleanup
    dup2(stdinCopy, STDIN_FILENO);
    dup2(stdoutCopy, STDOUT_FILENO);

    return objBuffer;
}

