//
// Created by xeri on 11/3/15.
//

#ifndef ADTN_RELOADED_NEIGHBOR_H
#define ADTN_RELOADED_NEIGHBOR_H

#include <string>
#include <chrono>

class Neighbor {
public:

    Neighbor(std::string id, std::string ip): m_id{id},
                                              m_ip{ip},
                                              m_lastSeen{std::chrono::system_clock::now()} {}

    const std::string &getId() const {
        return m_id;
    }

    void setId(const std::string &id) {
        Neighbor::m_id = id;
    }

    const std::string &getIp() const {
        return m_ip;
    }

    void setIp(const std::string &ip) {
        Neighbor::m_ip = ip;
    }

    const std::chrono::time_point<std::chrono::system_clock> &getLastSeen() const {
        return m_lastSeen;
    }

    void setLastSeen(const std::chrono::time_point<std::chrono::system_clock> &lastSeen) {
        Neighbor::m_lastSeen = lastSeen;
    }

private:
    std::string m_id;
    std::string m_ip;
    std::chrono::time_point<std::chrono::system_clock> m_lastSeen;

};


#endif //ADTN_RELOADED_NEIGHBOR_H
