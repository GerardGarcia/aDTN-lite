#include <chrono>
#include <fstream>

#include <magic.h>
#include <bzlib.h>
#include "gtest/gtest.h"

#include "Utils/Logger.h"
#include "Executor/CompilerBackend/ClangCompilerBackend.h"
#include "Executor/ExecBackend/eBPFExecBackend.h"
#include "Executor/Code.h"

#define END_CHRONO() std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now()-start 

using namespace std;

TEST(ExecutorTest, ClangCompilerBackend) {
    ClangCompilerBackend<eBPFExecObject> clangBackend {};
    string testCode {"return 4+4;"};
  
    eBPFExecObject execObj;
    clangBackend.compile(testCode, execObj);
    vector<uint8_t> obj = execObj.geteBPFByteCode();

    // Analyze file using file command
    magic_t magicCookie = magic_open(MAGIC_NONE);
    magic_load(magicCookie, 0);
    const char *objType =  magic_buffer(magicCookie, obj.data(), obj.size());
    ASSERT_STREQ("ELF 64-bit LSB relocatable, no machine, version 1 (SYSV)", objType);
    magic_close(magicCookie);
}

TEST(ExecutorTest, eBPFExecBackend) {
    ClangCompilerBackend<eBPFExecObject> clangBackend {};
    string testCode {"return 4+4;"};

    eBPFExecObject execObj;
    clangBackend.compile(testCode, execObj);
    vector<uint8_t> obj = execObj.geteBPFByteCode();
    
    // Write compiled object to disk
    ofstream fout("eBPFExecBackend.o", ios::out | ios::binary);
    fout.write((char *)obj.data(), obj.size()*sizeof(uint8_t));
    fout.close();

    eBPFExecBackend<> execBackend {};
    execBackend.loadObj(obj);

    int result;
    ASSERT_TRUE(execBackend.execute(result) == 0);
    ASSERT_EQ(result, 8);
}

/*
TEST(ExecutorTest, ContexteBPFExecBackendJIT) {
    unordered_map<string, string> context;
   
    string testCode {
        "char key[] = \"k\";"
        "char value[] = \"v\";"
        "set(key, value);"
        "char *v = (void *)get(key);"
        "if (strcmp_ext(v, value) == 0)"
        " return 0;\n"
        "else"
        " return 1;"
    };

    ClangCompilerBackend<eBPFExecObject> clangBackend {};
    eBPFExecObject execObj;
    clangBackend.compile(testCode, execObj);
    vector<uint8_t> obj = execObj.geteBPFByteCode();

    // Write compiled object to disk
    ofstream fout("ContexteBPFExecBackend.o", ios::out | ios::binary);
    fout.write((char *)obj.data(), obj.size()*sizeof(uint8_t));
    fout.close();
    
    eBPFExecBackend<> execBackend {&context};
    execBackend.loadObj(obj);

    int result;
    ASSERT_TRUE(execBackend.execute(result) == 0);
    ASSERT_EQ(result, 0);
}
*/

TEST(ExecutorTest, ContexteBPFExecBackendNoJIT) {
    unordered_map<string, string> context;
   
    string testCode {
        "char key[] = \"k\";"
        "char value[] = \"v\";"
        "set(key, value);"
        "char *v = (void *)get(key);"
        "if (strcmp_ext(v, value) == 0)"
        " return 0;\n"
        "else"
        " return 1;"
    };

    ClangCompilerBackend<eBPFExecObject> clangBackend {};
    eBPFExecObject execObj;
    clangBackend.compile(testCode, execObj);
    vector<uint8_t> obj = execObj.geteBPFByteCode();

    // Write compiled object to disk
    ofstream fout("ContexteBPFExecBackend.o", ios::out | ios::binary);
    fout.write((char *)obj.data(), obj.size()*sizeof(uint8_t));
    fout.close();
    
    eBPFExecBackend<> execBackend {&context};
    execBackend.unsetJIT();
    execBackend.loadObj(obj);

    int result;
    ASSERT_TRUE(execBackend.execute(result) == 0);
    ASSERT_EQ(result, 0);
}
TEST(ExecutorTest, SimpleCode) {
    string simpleCode {"return 4+4;"};
    Code<> testCode {simpleCode};

    ASSERT_TRUE(testCode.execute() == 8);
}


using std::shared_ptr;
using std::make_shared;

TEST(ExecutorTest, CachedCode) {
    auto compilerBackend = make_shared<ClangCompilerBackend<eBPFExecObject> > (); 
    auto execBackend = make_shared<eBPFExecBackend<int> > ();

    string simpleCode {"return 4+4;"};
    Code<> code1 {compilerBackend, nullptr, simpleCode};
    Code<> code2 {compilerBackend, nullptr, simpleCode};

    ASSERT_TRUE(code1.execute() == 8);
    ASSERT_TRUE(code1.execute() == 8);
    // Should be cached
    ASSERT_TRUE(code2.execute() == 8);
}

/*
TEST(ExecutorTest, CToeBPFtoNativeCodeSizes) {
    // List of test codes
    vector<string> codesFileNames = {"code1.c", "code2.c", "code3.c", "code4.c", "code5.c"};

    LOG(100) << "CToeBPFtoNativeCodeSizes test: ";
    for(auto &codeFilename: codesFileNames) {
        LOG(100) << codeFilename << ":";
        
        ifstream codeStream {codeFilename};
        
        // Get code length
        codeStream.seekg(0, codeStream.end);
        int codeLength = codeStream.tellg();
        LOG(100) << "Uncompressed code length: " << codeLength;
        codeStream.seekg(0, codeStream.beg);

        // Get compressed code length
        bz_stream strm = {0};
        BZ2_bzCompressInit(&strm, 9, 0, 30);
        
        string codeString (codeLength, ' ');
        codeStream.read(&*codeString.begin(), codeLength);
        unsigned int compressedBufferLength = 2048;
        string compressedString (compressedBufferLength, ' ');
        
        BZ2_bzBuffToBuffCompress(
                &*compressedString.begin(),
                &compressedBufferLength,
                &*codeString.begin(),
                codeString.length(),
                8,0,30); 

        LOG(100) << "BZ2 compressed code length: " << compressedBufferLength << "B";

        // Compile code
        ClangCompilerBackend<eBPFExecObject> clangBackend {};
        eBPFExecObject execObj;
        clangBackend.compile(codeString, execObj);
        vector<uint8_t> compiledCodeObj = execObj.geteBPFByteCode();

        // Get object size
        LOG(100) << "eBPF bytecode object size: " << compiledCodeObj.size() << "B";

        // Insert code into eBPF vm
        eBPFExecBackend<> execBackend {};
        execBackend.loadObj(compiledCodeObj);

        // Get number of eBPF instructions and total size
        LOG(100) << "Number of eBPF instructions: " << execBackend.getNumberOfeBPFInstructions();
        LOG(100) << "Size of eBPF instrucions (num * 8B): " << execBackend.getNumberOfeBPFInstructions() * 8 << "B";

        // Write eVPF instructions to disk
        ofstream eBPFBytecode {codeFilename + ".ebpf", ios::out | ios::binary};
        eBPFBytecode.write((char *) &*execBackend.geteBPFInstructions().begin(), execBackend.getNumberOfeBPFInstructions() * sizeof(uint64_t));
        eBPFBytecode.close();

        // Get native code size
        LOG(100) << "Function size (x86_64 compiled): " << execBackend.getJittedSize() << "B";

        // Execute code
        int result;
        execBackend.execute(result);
    }
}
*/

int main(int argc, char *argv[])
{
    // Init logger
    Logger::getInstance()->setLoggerConfigAndStart("executorTest.log");
    Logger::getInstance()->setLogLevel(100);

    ::testing::InitGoogleTest(&argc, argv);    
    return RUN_ALL_TESTS();
}
