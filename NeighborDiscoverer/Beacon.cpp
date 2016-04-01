//
// Created by xeri on 11/3/15.
//

#include "Beacon.h"

Beacon::Beacon(std::vector<uint8_t> &raw, std::string ip) {
    this->parse(raw, ip);
}


void Beacon::parseCString(std::vector<uint8_t>::iterator &it, std::string &dest) {
    while (*it != '0') {
        dest.push_back(*it);
        ++it;
    }
    ++it;
}

void Beacon::rawCString(std::string &src, std::vector<uint8_t> &dest) {
    for (auto c: src) {
        dest.push_back(c);
    }
    dest.push_back('0');
}

void Beacon::parse(std::vector<uint8_t> &raw, std::string ip) {
    std::vector<uint8_t>::iterator it = raw.begin();

    parseCString(it, this->id);
    this->ip = ip;
}

std::vector<uint8_t> Beacon::raw() {
    std::vector<uint8_t> raw;
    rawCString(this->id, raw);

    return raw;
}
