#include <vector>

#include "Application.h"

using namespace std;

class AppManager {
    public:
        AppManager();
        ~AppManager();
        vector<Application> getApps();

    private:
        vector<Application> apps;
};
