//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef __TCPMAIN_H
#define __TCPMAIN_H

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <omnetpp.h>
#include <map>
#include "IPAddress.h"


class TCPConnection;
class TCPSegment;

// macro for normal ev<< logging (note: deliberately no parens in macro def)
#define tcpEV (ev.disable_tracing||TCPMain::testing)?ev:ev

// macro for more verbose ev<< logging (note: deliberately no parens in macro def)
#define tcpEV2 (ev.disable_tracing||TCPMain::testing||!TCPMain::logverbose)?ev:ev

// testingEV writes log that automated test cases can check (*.test files)
#define testingEV (ev.disable_tracing||!TCPMain::testing)?ev:ev





/**
 * Implements the TCP protocol. This section describes the internal
 * architecture of the TCP model.
 *
 * Usage and compliance with various RFCs are discussed in the corresponding
 * NED documentation for TCPMain. Also, you may want to check the TCPSocket
 * class which makes it easier to use TCP from applications.
 *
 * The TCP protocol implementation is composed of several classes (discussion
 * follows below):
 *  - TCPMain: the module class
 *  - TCPConnection: manages a connection
 *  - TCPSendQueue, TCPReceiveQueue: abstract base classes for various types
 *    of send and receive queues
 *  - TCPVirtualDataSendQueue and TCPVirtualDataRcvQueue which implement
 *    queues with "virtual" bytes (byte counts only)
 *  - TCPAlgorithm: abstract base class for TCP algorithms
 *  - DummyTCPAlg and TCPTahoeReno, two concrete TCPAlgorithm implementations.
 *
 * TCPMain subclassed from cSimpleModule. It manages socketpair-to-connection
 * mapping, and dispatches segments and user commands to the appropriate
 * TCPConnection object.
 *
 * TCPConnection manages the connection, with the help of other objects.
 * TCPConnection itself implements the basic TCP "machinery": takes care
 * of the state machine, stores the state variables (TCB), sends/receives
 * SYN, FIN, RST, ACKs, etc.
 *
 * TCPConnection internally relies on 3 objects. The first two are subclassed
 * from TCPSendQueue and TCPReceiveQueue. They manage the actual data stream,
 * so TCPConnection itself only works with sequence number variables.
 * This makes it possible to easily accomodate need for various types of
 * simulated data transfer: real byte stream, "virtual" bytes (byte counts
 * only), and sequence of cMessage objects (where every message object is
 * mapped to a TCP sequence number range).
 *
 * Currently implemented send queue and receive queue classes are
 * TCPVirtualDataSendQueue and TCPVirtualDataRcvQueue which implement
 * queues with "virtual" bytes (byte conunts only).
 *
 * The third object is subclassed from TCPAlgorithm. Control over
 * retransmissions, congestion control and ACK sending are "outsourced"
 * from TCPConnection into TCPAlgorithm: delayed acks, slow start, fast rexmit,
 * etc. are all implemented in TCPAlgorithm subclasses. This simplifies the
 * design of TCPConnection and makes it a lot easier to implement new TCP
 * variations such as NewReno, Vegas or LinuxTCP as TCPAlgorithm subclasses.
 *
 * Currently implemented TCPAlgorithm classes are DummyTCPAlg and TCPTahoeReno.
 *
 * The concrete TCPAlgorithm class to use can be chosen per connection (in OPEN)
 * or in a module parameter.
 */
class TCPMain : public cSimpleModule
{
  protected:
    struct AppConnKey
    {
        int appGateIndex;
        int connId;

        inline bool operator<(const AppConnKey& b) const
        {
            if (appGateIndex!=b.appGateIndex)
                return appGateIndex<b.appGateIndex;
            else
                return connId<b.connId;
        }

    };
    struct SockPair
    {
        int localAddr;
        int remoteAddr;
        short localPort;
        short remotePort;

        inline bool operator<(const SockPair& b) const
        {
            if (remoteAddr!=b.remoteAddr)
                return remoteAddr<b.remoteAddr;
            else if (localAddr!=b.localAddr)
                return localAddr<b.localAddr;
            else if (remotePort!=b.remotePort)
                return remotePort<b.remotePort;
            else
                return localPort<b.localPort;
        }
    };

    typedef std::map<AppConnKey,TCPConnection*> TcpAppConnMap;
    typedef std::map<SockPair,TCPConnection*> TcpConnMap;

    TcpAppConnMap tcpAppConnMap;
    TcpConnMap tcpConnMap;

    short nextEphemeralPort;

    TCPConnection *findConnForSegment(TCPSegment *tcpseg, IPAddress srcAddr, IPAddress destAddr);
    TCPConnection *findConnForApp(int appGateIndex, int connId);
    void removeConnection(TCPConnection *conn);

  public:
    static bool testing;    // switches between tcpEV and testingEV
    static bool logverbose; // if !testing, turns on more verbose logging

  public:
    Module_Class_Members(TCPMain, cSimpleModule, 0);
    virtual ~TCPMain();
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    /**
     * To be called from TCPConnection when socket pair (key for TcpConnMap) changes
     * (e.g. becomes fully qualified).
     */
    void updateSockPair(TCPConnection *conn, IPAddress localAddr, IPAddress remoteAddr, int localPort, int remotePort);

    /**
     * To be called from TCPConnection: reserves an ephemeral port for the connection.
     */
    short getEphemeralPort();

};

#endif


