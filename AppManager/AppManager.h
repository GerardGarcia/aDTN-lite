#include <vector>

#include "Application.h"

using namespace std;

class AppManager {
    public:
        AppManager();
        // ~AppManager();
        vector<Application> getApps() const;
        void addApp(const Application &app);
        void delApp(const string &name);

    private:
        vector<Application> apps;
};
