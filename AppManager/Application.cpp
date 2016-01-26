#include <cassert>

#include "Application.h"

unordered_map<string, string> Application::getProps() {
    return props;
}

void Application::setProp(const string key, const string value) {
    if (!key.length()  || !value.length()) {
        return;
    }
   props.insert({key, value}); 
}

string Application::getProp(const string key) {
    if (!key.length())
        return "";
    
    auto value = props.find(key);
    if (value != props.end())
        return value->second;
    else
        return "";
}
