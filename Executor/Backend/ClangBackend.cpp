#include <sstream>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstdio>

#include <unistd.h>
#include <fcntl.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Basic/TargetInfo.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Module.h>

#include "Utils/Logger.h"
#include "ClangBackend.h"

using namespace std;
using namespace clang;
using namespace llvm;

vector<uint8_t> ClangBackend::compile(string _code) {
    // Check if the supplied target is supported
	switch (target) {
		case Target::eBPF_Object: {
            Hash codeHash = getHash(_code);
            Objects::iterator foundObject = EBPFObjects.find(codeHash);
            if (foundObject != EBPFObjects.end()){
                LOG(22) << "Code [" << codeHash << "] found in cache"; 
                return foundObject->second;
            } else {
                LOG(22) << "Code [" << codeHash << "] not found in cache, compiling.";
                INIT_CHRONO();

                START_CHRONO();
                vector<uint8_t> compiledObject = compileEBPFObject(_code);
                END_CHRONO();

                LOG(22) << "Compilation time of code [" << codeHash << "]: " << GET_CHRONO_SECS() << "s";
                
                EBPFObjects.insert(pair<Hash, vector<uint8_t> >(codeHash, compiledObject));
                return compiledObject;
            }
			break;
		}
	}

	throw new unknown_target();
}

ClangBackend::Hash ClangBackend::getHash(const string _code) {
    return XXH32( _code.c_str(), _code.length(), 0); 	
}

vector<uint8_t> ClangBackend::compileEBPFObject(const string _code) {
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
    args.push_back("-"); // Read code from stdin

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
    read(codeOutPipe[0], objBuffer.data(), objBuffer.size()); 

    // Cleanup
    dup2(stdinCopy, STDIN_FILENO);
    dup2(stdoutCopy, STDOUT_FILENO);

    return objBuffer;
}

