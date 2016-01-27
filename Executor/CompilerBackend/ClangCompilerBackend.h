#include <map>
#include <utility>

#include "xxhash.h"

#include "CompilerBackend.h"

using namespace std;

/*
 * Compiles codes using Clang/LLVM as backend
 * TODO: Try to initialize Clang once at constructor or first time is called
 * */
class ClangCompilerBackend: public CompilerBackend {
    public:
        enum class Target : int { eBPF_Object };

        ClangCompilerBackend(Target _target): 
            target {_target} {};
        vector<uint8_t> compile(const string _code) override;

        class unknown_target : public exception{};
    private:
        using Hash = XXH32_hash_t;
        using Objects = map<Hash, vector<uint8_t> >;

        Objects EBPFObjects;
        Target target;

        Hash getHash(const string _code);
        vector<uint8_t> compileEBPFObject(const string _code);
};
