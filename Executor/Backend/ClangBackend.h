#include <map>
#include <utility>

#include "xxhash.h"

#include "Backend.h"

using namespace std;

/*
 * Compiles codes using Clang/LLVM as backend
 * TODO: Try to initialize Clang once at constructor or first time is called
 * */
class ClangBackend: public Backend {
	public:
        enum class Target : int { eBPF_Object };
		
        ClangBackend(Target _target): 
			target {_target} {};
		vector<uint8_t> compile(const string _code);

	private:
        using Hash = XXH32_hash_t;
        using Objects = map<Hash, vector<uint8_t> >;

        Objects EBPFObjects;
        Target target;
        
        Hash getHash(const string _code);
        vector<uint8_t> compileEBPFObject(const string _code);
};
