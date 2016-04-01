//
// Created by xeri on 11/3/15.
//

#include <sstream>
#include "NbsTable.h"

bool NbsTable::update(Neighbor nb) {
    m_nbs.lock();
    auto insertResult = nbs.insert(std::make_pair<>(nb.getId(),nb));
    bool isNewNb = insertResult.second;
    if (!isNewNb) {
        // Update element
        insertResult.first->second.setIp(nb.getIp());
        insertResult.first->second.setLastSeen(std::chrono::system_clock::now());
    }
    m_nbs.unlock();

    return isNewNb;
}

bool NbsTable::remove(std::string id) {
    bool deleted = false;

    m_nbs.lock();
    for (auto it = nbs.begin(); it != nbs.end(); ){
        if (it->first == id) {
            nbs.erase(it);
            deleted = true;
            break;
        } else {
            ++it;
        }
    }
    m_nbs.unlock();

    return deleted;
}

std::vector<Neighbor> NbsTable::getNbs() {
    std::vector<Neighbor> nbsList;

    m_nbs.lock();
    for (auto &nb: nbs)
        nbsList.push_back(nb.second);
    m_nbs.unlock();

    return nbsList;
}

std::string NbsTable::str() {
    std::stringstream ss;

    m_nbs.lock();
    for (auto &nb: nbs) {
        ss << nb.second.getId();
        ss << " { ";
        ss << "ip: " << nb.second.getIp() << ", ";
        const time_t time = std::chrono::system_clock::to_time_t(nb.second.getLastSeen());
        char buffer[26];
        ctime_r(&time, buffer);
        buffer[24] = '\0';
        ss << "last seen: " << buffer;
        ss << " } ";
    }
    m_nbs.unlock();

    return ss.str();
}

