//
// Created by xeri on 11/3/15.
//

#ifndef ADTN_RELOADED_NBSTABLE_H
#define ADTN_RELOADED_NBSTABLE_H

#include <map>
#include <vector>
#include <utility>
#include <mutex>

#include "Neighbor.h"

class NbsTable {
public:
    NbsTable() {};
    bool update(Neighbor nb);
    bool remove(std::string id);
    std::vector<Neighbor> getNbs();

    // Debug
    std::string str();

private:
    std::map<std::string, Neighbor> nbs;
    std::mutex m_nbs;
};

#endif //ADTN_RELOADED_NBSTABLE_H
