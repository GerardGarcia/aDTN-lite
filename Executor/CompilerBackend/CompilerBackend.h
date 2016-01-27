#include <string>
#include <vector>
#include <cstdint>

using namespace std;

class CompilerBackend {
public:
    virtual vector<uint8_t> compile(string _code) = 0; 

};


