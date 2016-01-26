#include <chrono>

#include <magic.h>
#include "gtest/gtest.h"

#include "Utils/Logger.h"
#include "Executor/Backend/ClangBackend.h"

using namespace std;

TEST(ExecutorTest, ClangBackend) {
    ClangBackend clangBackend {ClangBackend::Target::eBPF_Object};
    string testCode {"int main() {return 4+4;}"};
   
    vector<uint8_t> obj = clangBackend.compile(testCode);

    // Analyze file using file command
    magic_t magicCookie = magic_open(MAGIC_NONE);
    magic_load(magicCookie, 0);
    const char *objType =  magic_buffer(magicCookie, obj.data(), obj.size());
    ASSERT_STREQ("ELF 64-bit LSB relocatable, no machine, version 1 (SYSV)", objType);
    magic_close(magicCookie);
}

TEST(ExecutorTest, DeliveryCode) {



}


int main(int argc, char *argv[])
{
    // Init logger
    Logger::getInstance()->setLoggerConfigAndStart("executorTest.log");
    Logger::getInstance()->setLogLevel(100);

    ::testing::InitGoogleTest(&argc, argv);    
    return RUN_ALL_TESTS();
}
