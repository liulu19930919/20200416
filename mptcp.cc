/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Morteza Kheirkhah <m.kheirkhah@sussex.ac.uk>
 */

// Network topology
//
//       n0 ----------- n1
// - Flow from n0 to n1 using MpTcpBulkSendApplication.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/rtt-estimator.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/delay-jitter-estimation.h"


#include <iostream> 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MpTcpBulkSendExample");

Ptr<PacketSink> reciver1, reciver2;					 /* Pointer to the packet sink application */
uint64_t lastTotalRx1 = 0, lastTotalRx2 = 0;         /* The value of the last total received bytes */
DelayJitterEstimation delayJitter;

void
CalculateThroughput()
{
	Time now = Simulator::Now();    /* Return the simulator's virtual time. */
	double cur_throughput1 = (reciver1->GetTotalRx() - lastTotalRx1) * 8.0 / 1e6;     /* Convert Application RX Packets to MBits. */
	std::cout << now.GetSeconds() << "s: \t" << cur_throughput1 << " Mbit/s" << std::endl;
	lastTotalRx1 = reciver1->GetTotalRx();
	double cur_throughput2 = (reciver2->GetTotalRx() - lastTotalRx2) * 8.0 / 1e6;     /* Convert Application RX Packets to MBits.*/
	std::cout << now.GetSeconds() << "s: \t" << cur_throughput2 << " Mbit/s" << std::endl;
	lastTotalRx2 = reciver2->GetTotalRx();
	Simulator::Schedule(Seconds(1.0), &CalculateThroughput);
}
static void
CalculateDelay(Ptr<const Packet> p, const Address &address)
{
	static int k = 0;
	k++;
	delayJitter.RecordRx(p);
	Time t = delayJitter.GetLastDelay();
	std::cout << Simulator::Now().GetSeconds() << "\t" << t.GetMilliSeconds() << std::endl;
	//delayfile << Simulator::Now().GetSeconds() << "\t" << t.GetMilliSeconds() << std::endl;

}
static void RxDrop(Ptr<const Packet> p) 
{
	NS_LOG_INFO("RxDrop at " << Simulator::Now().GetSeconds());
}

int
main(int argc, char *argv[])
{
  LogComponentEnable("MpTcpBulkSendExample", LOG_INFO);
  std::string prefix_file_name = "TcpVariantsComparison";
  bool pcap = true;

  Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
  Config::SetDefault("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(100));
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(MpTcpSocketBase::GetTypeId()));
  Config::SetDefault("ns3::MpTcpSocketBase::MaxSubflows", UintegerValue(8)); // Sink
  //Config::SetDefault("ns3::MpTcpSocketBase::CongestionControl", StringValue("Uncoupled_TCPs"));
  //Config::SetDefault("ns3::MpTcpSocketBase::PathManagement", StringValue("NdiffPorts"));

  NodeContainer nodes;
  nodes.Create(2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install(nodes);
  devices.Get(0)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&RxDrop));
  
  InternetStackHelper internet;
  internet.Install(nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  uint16_t port1 = 9, port2 = 911;
  MpTcpPacketSinkHelper sink1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port1));
  ApplicationContainer sinkApps1 = sink1.Install(nodes.Get(1));
  sinkApps1.Start(Seconds(0.0));
  sinkApps1.Stop(Seconds(10.0));
  reciver1 = StaticCast<PacketSink>(sinkApps1.Get(0));  
  
  MpTcpPacketSinkHelper sink2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port2));
  ApplicationContainer sinkApps2 = sink2.Install(nodes.Get(1));
  sinkApps2.Start(Seconds(0.0));
  sinkApps2.Stop(Seconds(10.0));
  reciver2 = StaticCast<PacketSink>(sinkApps2.Get(0));


  MpTcpBulkSendHelper source1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address(i.GetAddress(1)), port1));
  source1.SetAttribute("MaxBytes", UintegerValue(0));
  source1.SetAttribute("SendSize", UintegerValue(1400));
  ApplicationContainer sourceApps1 = source1.Install(nodes.Get(0));
  sourceApps1.Start(Seconds(0.0));
  sourceApps1.Stop(Seconds(10.0));


  MpTcpBulkSendHelper source2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address(i.GetAddress(1)), port2));
  source2.SetAttribute("MaxBytes", UintegerValue(0));
  source2.SetAttribute("SendSize", UintegerValue(1400));
  ApplicationContainer sourceApps2 = source2.Install(nodes.Get(0));
  sourceApps2.Start(Seconds(0.0));
  sourceApps2.Stop(Seconds(10.0));

 
  sinkApps1.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&CalculateDelay)); 
  
  //Simulator::Schedule(Seconds(1.0), &CalculateThroughput);
 

  if (pcap)
  {
	  pointToPoint.EnablePcapAll("prefix_file_name", true);
  }


  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(20.0));
  Simulator::Run();

  Simulator::Destroy();
  NS_LOG_INFO ("Done.");

  double averageThroughput1 = ((reciver1->GetTotalRx() * 8) / (1e6  * 10));
  std::cout << "Average throughput1: " << averageThroughput1 << " Mbit/s" << std::endl;

  double averageThroughput2 = ((reciver2->GetTotalRx() * 8) / (1e6 * 10));
  std::cout << "Average throughput2: " << averageThroughput2 << " Mbit/s" << std::endl;

  return 0;
}
