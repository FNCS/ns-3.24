/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "fncs-application.h"

#ifdef FNCS
#include <fncs.hpp>
#endif

#include <algorithm>
#include <sstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FncsApplication");

NS_OBJECT_ENSURE_REGISTERED (FncsApplication);

TypeId
FncsApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FncsApplication")
    .SetParent<Application> ()
    .AddConstructor<FncsApplication> ()
    .AddAttribute ("Name", 
                   "The name of the application",
                   StringValue (),
                   MakeStringAccessor (&FncsApplication::m_name),
                   MakeStringChecker ())
    .AddAttribute ("Sent", 
                   "The counter for outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&FncsApplication::m_sent),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LocalAddress", 
                   "The source Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&FncsApplication::m_localAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort", 
                   "The source port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&FncsApplication::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&FncsApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

FncsApplication::FncsApplication ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
}

FncsApplication::~FncsApplication()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void
FncsApplication::SetName (const std::string &name)
{
  NS_LOG_FUNCTION (this << name);
  m_name = name;
  Names::Add("fncs_"+name, this);
}

std::string
FncsApplication::GetName (void) const
{
  return m_name;
}

void 
FncsApplication::SetLocal (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_localAddress = ip;
  m_localPort = port;
}

void 
FncsApplication::SetLocal (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_localAddress = Address (ip);
  m_localPort = port;
}

void 
FncsApplication::SetLocal (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_localAddress = Address (ip);
  m_localPort = port;
}

void
FncsApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
FncsApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_localAddress) == true)
        {
          m_socket->Bind (InetSocketAddress (Ipv4Address::ConvertFrom(m_localAddress), m_localPort));
        }
      else if (Ipv6Address::IsMatchingType(m_localAddress) == true)
        {
          m_socket->Bind (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_localAddress), m_localPort));
        }
      else
        {
          m_socket->Bind();
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&FncsApplication::HandleRead, this));

  if (m_name.empty()) {
    NS_FATAL_ERROR("FncsApplication is missing name");
  }
}

void 
FncsApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

static uint8_t
char_to_uint8_t (char c)
{
  return uint8_t(c);
}

void 
FncsApplication::Send (Ptr<FncsApplication> to, std::string topic, std::string value)
{
  NS_LOG_FUNCTION (this << to << topic << value);

  Ptr<Packet> p;

  // Convert given value into a Packet.
  size_t total_size = topic.size() + 1 + value.size();
  uint8_t *buffer = new uint8_t[total_size];
  uint8_t *buffer_ptr = buffer;
  std::transform (topic.begin(), topic.end(), buffer_ptr, char_to_uint8_t);
  buffer_ptr += topic.size();
  buffer_ptr[0] = '=';
  buffer_ptr += 1;
  std::transform (value.begin(), value.end(), buffer_ptr, char_to_uint8_t);
  p = Create<Packet> (buffer, total_size);
  NS_LOG_INFO("buffer='" << p << "'");
  delete [] buffer;

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);

  if (Ipv4Address::IsMatchingType (m_localAddress))
    {
      InetSocketAddress address = to->GetLocalInet();
      NS_LOG_INFO ("At time "
          << Simulator::Now ().GetSeconds ()
          << "s '"
          << m_name
          << "' sent "
          << total_size
          << " bytes to '"
          << to->GetName()
          << "' at address "
          << address.GetIpv4()
          << " port "
          << address.GetPort());
      m_socket->SendTo(p, 0, address);
    }
  else if (Ipv6Address::IsMatchingType (m_localAddress))
    {
      Inet6SocketAddress address = to->GetLocalInet6();
      NS_LOG_INFO ("At time "
          << Simulator::Now ().GetSeconds ()
          << "s '"
          << m_name
          << "' sent "
          << total_size
          << " bytes to '"
          << to->GetName()
          << "' at address "
          << address.GetIpv6()
          << " port "
          << address.GetPort());
      m_socket->SendTo(p, 0, address);
    }

  ++m_sent;
}

InetSocketAddress FncsApplication::GetLocalInet (void) const
{
  return InetSocketAddress(Ipv4Address::ConvertFrom(m_localAddress), m_localPort);
}

Inet6SocketAddress FncsApplication::GetLocalInet6 (void) const
{
  return Inet6SocketAddress(Ipv6Address::ConvertFrom(m_localAddress), m_localPort);
}

void
FncsApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      uint32_t size = packet->GetSize();
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time "
                  << Simulator::Now ().GetSeconds ()
                  << "s received "
                  << size
                  << " bytes from "
                  << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
                  << " port "
                  << InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time "
                  << Simulator::Now ().GetSeconds ()
                  << "s received "
                  << size
                  << " bytes from "
                  << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ()
                  << " port "
                  << Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }
      std::ostringstream odata;
      packet->CopyData (&odata, size);
      std::string sdata = odata.str();
      size_t split = sdata.find('=', 0);
      if (std::string::npos == split) {
          NS_FATAL_ERROR("HandleRead could not locate '=' to split topic=value");
      }
      std::string topic = sdata.substr(0, split);
      std::string value = sdata.substr(split+1);
      fncs::publish(topic, value);
    }
}

} // Namespace ns3
