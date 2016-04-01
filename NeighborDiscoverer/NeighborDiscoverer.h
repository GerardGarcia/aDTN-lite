//
// Created by xeri on 11/4/15.
//

#ifndef ADTN_RELOADED_NEIGHBORDISCOVERY_H
#define ADTN_RELOADED_NEIGHBORDISCOVERY_H

#include <thread>
#include <atomic>

#include <Utils/Config.h>
#include <Executor/Executor.h>
#include <Patterns/Observable.h>
#include "NbsTable.h"

// Events that NeighborDiscoverer generates
enum class NbDiscoveryEvent { NEW_NB };
// Event info for each event
struct newNbEvent {
    Neighbor newNbDiscovered;
};
// Event info passed to notified method
struct NbDiscoveryEventInfo {
    NbDiscoveryEvent eventType;
    union {
        newNbEvent newNbEventInfo;
    };
};

class NeighborDiscoverer : public Observable<NbDiscoveryEvent, NbDiscoveryEventInfo> {
public:
    NeighborDiscoverer(Config &config) : m_config{config} { };

    ~NeighborDiscoverer();

    void start();
    void stop();

    class unknown_error: virtual std::exception{};
    class network_error: virtual std::exception{};

private:
    Config m_config;
    NbsTable m_nbs;
    int m_senderCmdPipe[2];
    int m_receiverCmdPipe[2];
    int m_cleanerCmdPipe[2];

    std::thread m_beaconSender;
    std::thread m_beaconReceiver;
    std::thread m_neighborCleaner;

    void beaconSender();
    void beaconReceiver();
    void neighborCleaner();
};


#endif //ADTN_RELOADED_NEIGHBORDISCOVERY_H
