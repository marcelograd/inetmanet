/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef HWMP_RTABLE_H
#define HWMP_RTABLE_H

#include <map>
#include <omnetpp.h>
#include "MACAddress.h"

/**
 * \ingroup dot11s
 *
 * \brief Routing table for HWMP -- 802.11s routing protocol
 */

typedef struct
{
   MACAddress destination;
   uint32_t seqnum;
} HwmpFailedDestination;
class HwmpProtocol;
class HwmpRtable : public cObject
{
public:
  /// Means all interfaces
  const static uint32_t INTERFACE_ANY = 0xffffffff;
  /// Maximum (the best?) path metric
  const static uint32_t MAX_METRIC = 0xffffffff;

  /// Route lookup result, return type of LookupXXX methods
  struct LookupResult
  {
    MACAddress retransmitter;
    uint32_t ifIndex;
    uint32_t metric;
    uint32_t seqnum;
    simtime_t lifetime;
    LookupResult (MACAddress r = MACAddress::UNSPECIFIED_ADDRESS,
                 uint32_t i = INTERFACE_ANY,
                 uint32_t m = MAX_METRIC,
                 uint32_t s = 0,
                 simtime_t l=0)
    {
    	  retransmitter =r;
    	  ifIndex = i;
    	  metric =m;
    	  seqnum= s;
    	  lifetime = l;
    }
    /// True for valid route
    bool isValid () const {return !(retransmitter.isUnspecified() && ifIndex == INTERFACE_ANY && metric == MAX_METRIC
    	      && seqnum == 0);}
    /// Compare route lookup results, used by tests
    bool operator==(const LookupResult & o) const;
  };
  /// Path precursor = {MAC, interface ID}
  typedef std::vector<std::pair<uint32_t, MACAddress> > PrecursorList;

public:
  struct Precursor
  {
    MACAddress address;
    uint32_t interface;
    simtime_t whenExpire;
  };
  /// Route found in reactive mode
  struct ReactiveRoute
  {
    MACAddress retransmitter;
    uint32_t interface;
    uint32_t metric;
    simtime_t whenExpire;
    uint32_t seqnum;
    std::vector<Precursor> precursors;
  };
  
  //Route fond in proactive mode
  struct ProactiveRoute
  {
    MACAddress root;
    MACAddress retransmitter;
    uint32_t interface;
    uint32_t metric;
    simtime_t whenExpire;
    uint32_t seqnum;
    std::vector<Precursor> precursors;
  };

  HwmpRtable ();
  ~HwmpRtable ();
  
  ///\name Add/delete paths
  //\{
  void AddReactivePath (
    MACAddress destination,
    MACAddress retransmitter,
    uint32_t interface,
    uint32_t metric,
    simtime_t  lifetime,
    uint32_t seqnum
  );
  void AddProactivePath (
    uint32_t metric,
    MACAddress root,
    MACAddress retransmitter,
    uint32_t interface,
    simtime_t  lifetime,
    uint32_t seqnum
  );
  void AddPrecursor (MACAddress destination, uint32_t precursorInterface, MACAddress precursorAddress, simtime_t lifetime);
  PrecursorList GetPrecursors (MACAddress destination);
  void DeleteProactivePath ();
  void DeleteProactivePath (MACAddress root);
  void DeleteReactivePath (MACAddress destination);
  //\}

  ///\name Lookup
  //\{
  /// Lookup path to destination
  LookupResult LookupReactive (MACAddress destination);
  /// Return all reactive paths, including expired
  LookupResult LookupReactiveExpired (MACAddress destination);
  /// Find proactive path to tree root. Note that calling this method has side effect of deleting expired proactive path
  LookupResult LookupProactive ();
  /// Return all proactive paths, including expired
  LookupResult LookupProactiveExpired ();

  ReactiveRoute * getLookupReactivePtr (MACAddress destination);
  ProactiveRoute * getLookupProactivePtr ();

  //\}
  bool IsValid ();

  /// When peer link with a given MAC-address fails - it returns list of unreachable destination addresses
  std::vector<HwmpFailedDestination> GetUnreachableDestinations (MACAddress peerAddress);
  friend class HwmpProtocol;
private:
  /// List of routes
  std::map<MACAddress, ReactiveRoute>  m_routes;
  /// Path to proactive tree root MP
  ProactiveRoute  m_root;

      static uint64_t MacToUint64(const MACAddress &add)
      {
          uint64_t aux;
          uint64_t lo=0;
          for (int i=0; i<MAC_ADDRESS_BYTES; i++)
          {
              aux  = add.getAddressByte(MAC_ADDRESS_BYTES-i-1);
              aux <<= 8*i;
              lo  |= aux ;
          }
          return lo;
      }

      static MACAddress Uint64ToMac(uint64_t lo)
      {
          MACAddress add;
          add.setAddressByte(0, (lo>>40)&0xff);
          add.setAddressByte(1, (lo>>32)&0xff);
          add.setAddressByte(2, (lo>>24)&0xff);
          add.setAddressByte(3, (lo>>16)&0xff);
          add.setAddressByte(4, (lo>>8)&0xff);
          add.setAddressByte(5, lo&0xff);
          return add;
      }

};

#endif
