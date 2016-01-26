#include <string>
#include <unordered_map>

using namespace std;

class Application {
    public:
        Application(string _name): name {_name} {};
        // ~Application();

        unordered_map<string, string> getProps();
        void setProp(const string key, const string value);
        string getProp(const string key);

    private:
        unordered_map<string, string> props;
        string name;
};
