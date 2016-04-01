//
// Created by xeri on 11/3/15.
//

#ifndef ADTN_RELOADED_BEACON_H
#define ADTN_RELOADED_BEACON_H

#include <string>
#include <vector>

#include "Utils/SDNV.h"

class Beacon {
private:
    std::string id;
    std::string ip;
    void parseCString(std::vector<uint8_t>::iterator &it, std::string &dest);
    void rawCString(std::string &src, std::vector<uint8_t> &dest);

public:
    Beacon() {}
    Beacon(std::string nodeId): id {nodeId} {}
    Beacon(std::vector<uint8_t> &raw, std::string ip);

    static constexpr auto MAX_BEACON_SIZE = 128; // Bytes

    const std::string &getId() const {
        return id;
    }

    void setId(const std::string &id) {
        Beacon::id = id;
    }

    const std::string &getIp() const {
        return ip;
    }

    void setIp(const std::string &ip) {
        Beacon::ip = ip;
    }

    void parse(std::vector<uint8_t> &raw, const std::string ip);
    std::vector<uint8_t> raw();

};


#endif //ADTN_RELOADED_BEACON_H
