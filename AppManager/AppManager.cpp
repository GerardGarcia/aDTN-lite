#include "AppManager.h"

using namespace std;

vector<Application> AppManager::getApps() const {
    vector<Application> appsDup(apps.begin(), apps.end());

    return appsDup;
}

void AppManager::addApp(const Application &app) {
    apps.emplace_back(app);
}

void AppManager::delApp(const string &name) {
    for (auto it = apps.begin(); it != apps.end(); ++it) {
        if (it->getName().compare(name) == 0){
            apps.erase(it);
            break;
        }
    }
}
