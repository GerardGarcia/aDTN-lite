//
// Created by xeri on 11/4/15.
//

#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <Net/SocketWrapper.h>
#include <Utils/Commander.h>

#include "Utils/Logger.h"
#include "NeighborDiscoverer.h"
#include "Beacon.h"

NeighborDiscoverer::~NeighborDiscoverer() {
    stop();
}

void NeighborDiscoverer::start() {
    LOG(6) << "Starting Neighbor Discovery";

    // Prepare commands pipes
    if (pipe(m_senderCmdPipe) == -1) {
        LOG(1) << "Error setting up command pipe: " << strerror(errno);
        throw unknown_error();
    }
    if (pipe(m_receiverCmdPipe) == -1) {
        LOG(1) << "Error setting up command pipe: " << strerror(errno);
        throw unknown_error();
    }
    if (pipe(m_cleanerCmdPipe) == -1) {
        LOG(1) << "Error setting up command pipe: " << strerror(errno);
        throw unknown_error();
    }

    // Launch threads
    m_beaconSender = std::thread {&NeighborDiscoverer::beaconSender, this};
    m_beaconSender.detach();

    m_beaconReceiver = std::thread {&NeighborDiscoverer::beaconReceiver, this};
    m_beaconReceiver.detach();

    m_neighborCleaner = std::thread {&NeighborDiscoverer::neighborCleaner, this};
    m_neighborCleaner.detach();
}

void NeighborDiscoverer::stop() {
    LOG(6) << "Stopping Neighbor Discovery";

    try {
        Commander::sendCommand(m_receiverCmdPipe[1], Commander::Commands::STOP);
    } catch (Commander::command_error &e) {
        LOG(2) << "Error stopping NeighborDiscoverer::beaconReceiver()";
    }

    try {
        Commander::sendCommand(m_senderCmdPipe[1], Commander::Commands::STOP);
    } catch (Commander::command_error &e) {
        LOG(2) << "Error stopping NeighborDiscoverer::beaconSender()";
    }

    try {
        Commander::sendCommand(m_cleanerCmdPipe[1], Commander::Commands::STOP);
    } catch (Commander::command_error &e) {
        LOG(2) << "Error stopping NeighborDiscoverer::beaconCleaner()";
    }
}

void NeighborDiscoverer::beaconSender() {
    // Create socket
    SocketWrapper fd(socket(AF_INET, SOCK_DGRAM, 0));
    if (*fd == -1) {
        LOG(1) << "Error creating NeighborDiscoverer sender socker: " << strerror(errno);
        throw network_error();
    }

    // Set interface for outbound multicast datagrams
    struct in_addr localAddr = {0};
    localAddr.s_addr = inet_addr(this->m_config.getIfaceIp().c_str());
    if (setsockopt(*fd, IPPROTO_IP, IP_MULTICAST_IF, &localAddr, sizeof(localAddr)) < 0) {
        LOG(1) << "Error setting local interface " << this->m_config.getIfaceIp()
               << " for outbound multicast datagrams: " << strerror(errno);
        throw network_error();
    }

    // Disable loopback to not receive our own beacons
    int optval = 1;
    if (setsockopt(*fd, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof(optval)) < 0) {
        LOG(1) << "Error disabling multicast loop: " << strerror(errno);
        throw network_error();
    }

    // Prepare destination address
    struct sockaddr_in destAddr = {0};
    destAddr.sin_family = AF_INET;
    destAddr.sin_addr.s_addr = inet_addr(this->m_config.getNeighborDiscoveryAddress().c_str());
    destAddr.sin_port = htons(this->m_config.getNeighborDiscoveryPort());

    // Send beacons periodically
    fd_set readfd;
    FD_ZERO(&readfd);

    timeval waitingTimeout= {0};
    bool running = true;
    while (running) {
        // Set command fd to set
        FD_SET(m_senderCmdPipe[0], &readfd);

        // Set period
        waitingTimeout.tv_sec = this->m_config.getNeighborDiscoveryPeriod();

        // Create beacon
        Beacon b(this->m_config.getNodeId());
        std::vector<uint8_t> bRaw = b.raw();

        // Send beacon
        if (sendto(*fd, bRaw.data(), bRaw.size(), 0, reinterpret_cast<sockaddr *>(&destAddr), sizeof(destAddr))
            != bRaw.size()) LOG(2) << "Error sending beacon to " << this->m_config.getNeighborDiscoveryAddress() << ":"
                                   << this->m_config.getNeighborDiscoveryPort() << ": " << strerror(errno);
        else LOG(28) << "Beacon sent to " << this->m_config.getNeighborDiscoveryAddress() << ":"
                     << this->m_config.getNeighborDiscoveryPort();

        // Wait that timeout expires or that we receive a command
        int ready = select(m_senderCmdPipe[0] + 1, &readfd, NULL, NULL, &waitingTimeout);
        switch (ready) {
            case -1: {
                if (errno == EINTR) {
                    continue;
                } else if (errno == EBADF) {
                    LOG(1) << "Invalid command file descriptor: " << strerror(errno);
                    throw unknown_error();
                }
            }
            case 0: {
                break;
            }
            default: {
                Commander::Commands command = Commander::recvCommand(m_senderCmdPipe[0]);
                if (command == Commander::Commands::STOP) {
                    LOG(6) << "Shutting down NeighborDiscoverer:: beaconSender()";
                    running = false;
                } else {
                    LOG(2) << "Unkown command " << static_cast<char>(command) << " received";
                }
                break;
            }
        }
    }
}

void NeighborDiscoverer::beaconReceiver() {
    // Create socket
    SocketWrapper fd(socket(AF_INET, SOCK_DGRAM, 0));
    if (*fd == -1) {
        LOG(1) << "Error creating NeighborDiscoverer receiving socker";
        throw network_error();
    }

    // Allow more than one process to bind to the multivast address
    int optval = 1;
    if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval)) == -1) {
        LOG(1) << "Error setting SO_REUSEADDR To receiving socket";
        throw network_error();
    }

    // Bind to address
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(this->m_config.getNeighborDiscoveryPort());

    if (bind(*fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(struct sockaddr_in)) == -1) {
        LOG(1) << "Error binding to " << this->m_config.getIfaceIp() << ":"
               << this->m_config.getNeighborDiscoveryPort();
        throw network_error();
    }

    // Subscribe to multicast address
    struct ip_mreq mreq = {0};
    mreq.imr_multiaddr.s_addr = inet_addr(this->m_config.getNeighborDiscoveryAddress().c_str());
    mreq.imr_interface.s_addr = inet_addr(this->m_config.getIfaceIp().c_str());
    if (setsockopt(*fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
        LOG(1) << "Error subscribing to multicast address " << this->m_config.getNeighborDiscoveryAddress()
               << " at interface " << this->m_config.getIfaceIp();
        throw network_error();
    }

    // Receive beacons (launch beacon processing as task)
    std::vector<uint8_t> buffer(Beacon::MAX_BEACON_SIZE);
    fd_set readfd;
    FD_ZERO(&readfd);
    int maxfd = *fd > m_receiverCmdPipe[0] ? *fd: m_receiverCmdPipe[0];

    bool running = true;
    while (running) {
        FD_SET(*fd, &readfd);
        FD_SET(m_receiverCmdPipe[0], &readfd);

        struct sockaddr_in srcAddr = {0};
        socklen_t srcAddrLen = sizeof(srcAddr);

        int ready = select(maxfd + 1, &readfd, NULL, NULL, NULL);
        switch (ready) {
            case -1: {
                if (errno == EINTR) {
                    continue;
                } else if (errno == EBADF) {
                    LOG(1) << "Invalid file descriptor: " << strerror(errno);
                    throw unknown_error();
                }
            }
            default: {
                if (FD_ISSET(*fd, &readfd)) {
                    ssize_t receivedBytes = recvfrom(*fd, buffer.data(), Beacon::MAX_BEACON_SIZE, 0,
                                                     reinterpret_cast<sockaddr *>(&srcAddr), &srcAddrLen);

                    std::string neighborIP {inet_ntoa(srcAddr.sin_addr)};
                    int neighborPort {ntohs(srcAddr.sin_port)};

                    if (receivedBytes == -1) {
                        if (errno == EINTR) {
                            continue;
                        } else {
                            LOG(2) << "Error receiving beacon from " << neighborIP << ":"
                                   << neighborPort << ": " << strerror(errno);
                            continue;
                        }
                    } else {
                        LOG(6) << "Beacon received from " << neighborIP << ":"
                               << neighborPort;
                    }

                    // Parse beacon
                    Beacon b(buffer, neighborIP);

                    // Update nb table
                    m_nbs.update(Neighbor(b.getId(), b.getIp()));

                    LOG(28) << "Neighbor Table: " << m_nbs.str();
                }

                if (FD_ISSET(m_receiverCmdPipe[0], &readfd)) {
                    Commander::Commands command = Commander::recvCommand(m_receiverCmdPipe[0]);
                    if (command == Commander::Commands::STOP) {
                        LOG(6) << "Shutting down NeighborDiscoverer:: beaconReceiver()";
                        running = false;
                    } else {
                        LOG(2) << "Unkown command " << static_cast<char>(command) << " received";
                    }
                    break;
                }
            }
        }


        ssize_t receivedBytes = recvfrom(*fd, buffer.data(), Beacon::MAX_BEACON_SIZE, 0,
                                         reinterpret_cast<sockaddr *>(&srcAddr), &srcAddrLen);
        if (receivedBytes == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                LOG(2) << "Error receiving beacon from " << std::string(inet_ntoa(srcAddr.sin_addr)) << ":"
                       << ntohs(srcAddr.sin_port) << ": " << strerror(errno);
                continue;
            }
        } else {
            LOG(6) << "Beacon received from "  << std::string(inet_ntoa(srcAddr.sin_addr)) << ":"
                      << ntohs(srcAddr.sin_port);
        }

        // Parse beacon
        Beacon b(buffer, std::string(inet_ntoa(srcAddr.sin_addr)));

        // Update nb table
        m_nbs.update(Neighbor(b.getId(), b.getIp()));

        LOG(28) << "Neighbor Table: " << m_nbs.str();
    }

    if (setsockopt(*fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
        LOG(1) << "Error dropping multicast subscription at address " << this->m_config.getNeighborDiscoveryAddress()
               << " at interface " << this->m_config.getIfaceIp();
        throw network_error();
    }
}

void NeighborDiscoverer::neighborCleaner() {
    // Clean Neighbor Table periodically
    fd_set readfd;
    FD_ZERO(&readfd);

    timeval waitingTimeout= {0};
    bool running = true;
    while (running) {
        FD_SET(m_receiverCmdPipe[0], &readfd);

        // Set period
        std::chrono::seconds cleaningPeriod(this->m_config.getNeighborDiscoveryPeriod() * 2);
        waitingTimeout.tv_sec = cleaningPeriod.count();

        // Clean neighbors
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::vector<Neighbor> nbs = m_nbs.getNbs();
        for (auto &nb: nbs) {
            if (nb.getLastSeen() + cleaningPeriod < now) {
                m_nbs.remove(nb.getId());
            }
        }

        // Wait that timeout expires or that we receive a command
        int ready = select(m_senderCmdPipe[0] + 1, &readfd, NULL, NULL, &waitingTimeout);
        switch (ready) {
            case -1: {
                if (errno == EINTR) {
                    continue;
                } else if (errno == EBADF) {
                    LOG(1) << "Invalid command file descriptor: " << strerror(errno);
                    throw unknown_error();
                }
            }
            case 0: {
                break;
            }
            default: {
                Commander::Commands command = Commander::recvCommand(m_senderCmdPipe[0]);
                if (command == Commander::Commands::STOP) {
                    LOG(6) << "Shutting down NeighborDiscoverer:: neighborCleaner()";
                    running = false;
                } else {
                    LOG(2) << "Unkown command " << static_cast<char>(command) << " received";
                }
                break;
            }
        }
    }
}
