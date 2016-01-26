#include <string>
#include <vector>
#include <cstdint>

using namespace std;

class Backend {
public:
    virtual vector<uint8_t> compile(string _code) = 0; 

	class unknown_target : public exception{};
};


