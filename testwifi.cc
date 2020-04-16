#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/gnuplot.h"
#include "ns3/stats-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/netanim-module.h"
#include "ns3/ssid.h"
#include "ns3/bridge-module.h"
#include <iostream>
#include "ns3/applications-module.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace std;
Gnuplot createPlot(string graphicsName, string title, string legendx, string legendy, string terminal, string appendextra)
{
    Gnuplot gnuplot(graphicsName.c_str());
    gnuplot.SetTitle(title);
    gnuplot.SetLegend(legendx, legendy);
    gnuplot.SetTerminal(terminal);
    gnuplot.AppendExtra(appendextra.c_str());
    return gnuplot;
}
Gnuplot2dDataset createDataset(string title)
{
    Gnuplot2dDataset dataset;
    dataset.SetTitle(title.c_str());
    dataset.SetStyle(Gnuplot2dDataset::LINES);
    return dataset;
}
Gnuplot2dDataset createDataset()
{
    Gnuplot2dDataset dataset;
    //    dataset.SetTitle(title.c_str());
    dataset.SetStyle(Gnuplot2dDataset::LINES);
    return dataset;
}

void addDataset(Gnuplot *gnuplot, Gnuplot2dDataset *dataset)
{
    gnuplot->AddDataset(*dataset);
}
void addData(Gnuplot2dDataset *dataset, double x, double y)
{
    dataset->Add(x, y);
}
void createPlotFile(Gnuplot *gnuplot, string fileName)
{
    ofstream plotFile(fileName.c_str());
    gnuplot->GenerateOutput(plotFile);
    plotFile.close();
}

void deAssocTrace(Ptr<Node> node, Mac48Address bssid)
{
    cout << "at time " << Simulator::Now().GetSeconds()  << " ";
    cout << "lost from the " << bssid << endl;
    cout << "now the position is : ";
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
    NS_ASSERT(mob != 0);
    Vector pos = mob->GetPosition();
    cout << pos.x << " " << pos.y << endl;
}
void AssocTrace(Ptr<Node> node, Mac48Address bssid)
{
    cout << "at time " << Simulator::Now().GetSeconds()  << " ";
    cout << "get from the " << bssid << endl;
    cout << "now the position is : ";
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
    NS_ASSERT(mob != 0);
    Vector pos = mob->GetPosition();
    cout << pos.x << " " << pos.y << endl;
}
void queueSizeTrace(Gnuplot2dDataset *dataset, uint32_t oldSize, uint32_t newSize)
{
    dataset->Add(Simulator::Now().GetSeconds(), newSize-oldSize+oldSize);
}

int main(int argc, char ** argv)
{
    bool verbose = false;
    uint32_t nWifi = 4;
    bool tracing = false;
    bool activeProbe = true;
    double minChannelTime = 0.010;
    double maxChannelTime = 0.040;
    uint32_t distance = 40;
    uint32_t operationalChannelNumber = 11;
    Gnuplot plt = createPlot("Mac-Queue-Size-over.png", "mac-queue-size", "Time(s)", "Size", "png", "set yrange[0:600]");
    Gnuplot2dDataset dataset = createDataset("packets in queue");
    /* Gnuplot2dDataset dataset = createDataset(); */
    CommandLine cmd;

    cmd.AddValue("minChannelTime", "min channel time for scanning", minChannelTime);
    cmd.AddValue("maxChannelTime", "max channel time for scanning", maxChannelTime);
    cmd.AddValue("nWifi", "number of aps", nWifi);
    cmd.AddValue("verbose", "Tell application to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);
    cmd.AddValue("activeProbe", "scan mode", activeProbe);
    cmd.AddValue("distance", "the distance form sta to ap", distance);
    cmd.AddValue("operationalChannelNumber", "operation channel number", operationalChannelNumber);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::StaWifiMac::MinChannelTimeout", TimeValue(Seconds(minChannelTime)));
    Config::SetDefault("ns3::StaWifiMac::MaxChannelTimeout", TimeValue(Seconds(maxChannelTime)));
    if (verbose)
    {
        /* LogComponentEnable("UdpServer", LOG_LEVEL_INFO); */
        LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    }

    // 创建节点
    vector<NodeContainer> apNodes(5);
    vector<NodeContainer> staNodes(5);
    NodeContainer robotNodes;
    NodeContainer bridgeNodes;
    NodeContainer serverNodes;

    // 网卡
    vector<NetDeviceContainer> apWifiDevices(5);
    vector<NetDeviceContainer> staDevices(5);
    vector<NetDeviceContainer> apCsmaDevices(5);
    vector<NetDeviceContainer> apBridgeDevices(5);
    NetDeviceContainer robotDevices;
    NetDeviceContainer bridgeDevices;
    NetDeviceContainer serverDevices;

    vector<Ipv4InterfaceContainer> apIpv4Interfaces(5);
    vector<Ipv4InterfaceContainer> staIpv4Interfaces(5);
    Ipv4InterfaceContainer robotIpv4Interfaces;
    Ipv4InterfaceContainer serverIpv4Interfaces;

    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        apNodes[i].Create(1);
    }
    // 创建静止sta节点
    staNodes[1].Create(2);
    staNodes[2].Create(3);
    staNodes[3].Create(3);
    staNodes[4].Create(2);
    // 创建移动机器人节点
    robotNodes.Create(1);
    // 创建交换节点
    bridgeNodes.Create(1);
    // 创建服务器节点
    serverNodes.Create(1);

    // 设置wifi
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

    // 设置phy和channel
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());

    // 设置mac
    WifiMacHelper mac;
    Ssid ssid = Ssid("wifi-handoff");
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    // 设置ap的信道，安装网卡
    vector<uint32_t> channelNumber = {0, 1, 6, 11, 1};
    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        phy.Set("ChannelNumber", UintegerValue(channelNumber[i]));
        apWifiDevices[i] = wifi.Install(phy, mac, apNodes[i].Get(0));
    }

    // 安装sta网卡
    {

        phy.Set("ChannelNumber", UintegerValue(1));
        mac.SetType("ns3::StaWifiMac", 
                    "Ssid", SsidValue(ssid),
                    "ActiveProbing", BooleanValue(activeProbe));
        staDevices[1] = wifi.Install(phy, mac, staNodes[1]);
    }
    {
        phy.Set("ChannelNumber", UintegerValue(6));
        mac.SetType("ns3::StaWifiMac", 
                    "Ssid", SsidValue(ssid),
                    "ActiveProbing", BooleanValue(activeProbe));
        staDevices[2] = wifi.Install(phy, mac, staNodes[2]);
    }
    {
        phy.Set("ChannelNumber", UintegerValue(11));
        mac.SetType("ns3::StaWifiMac", 
                    "Ssid", SsidValue(ssid),
                    "ActiveProbing", BooleanValue(activeProbe));
        staDevices[3] = wifi.Install(phy, mac, staNodes[3]);
    }
    {
        phy.Set("ChannelNumber", UintegerValue(1));
        mac.SetType("ns3::StaWifiMac", 
                    "Ssid", SsidValue(ssid),
                    "ActiveProbing", BooleanValue(activeProbe));
        staDevices[4] = wifi.Install(phy, mac, staNodes[4]);
    }
    {
        phy.Set("ChannelNumber", UintegerValue(11));
        mac.SetType("ns3::StaWifiMac", 
                    "Ssid", SsidValue(ssid),
                    "ActiveProbing", BooleanValue(activeProbe));
        robotDevices = wifi.Install(phy, mac, robotNodes);
    }


    // 机器人添加操作信道
    Ptr<WifiNetDevice> devicePtr = DynamicCast<WifiNetDevice>(robotDevices.Get(0));
    Ptr<YansWifiPhy> phyPtr = DynamicCast<YansWifiPhy>(devicePtr->GetPhy());
    for (uint32_t i = 1; i <= operationalChannelNumber; ++i)
    {
        phyPtr->AddOperationalChannel(i);
    }

    CsmaHelper csma; 
    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        NetDeviceContainer apBrigeLink = csma.Install(NodeContainer(apNodes[i].Get(0), bridgeNodes));
        apCsmaDevices[i].Add(apBrigeLink.Get(0));
        bridgeDevices.Add(apBrigeLink.Get(1));
    }
    NetDeviceContainer serverBrigeLink = csma.Install(NodeContainer(serverNodes, bridgeNodes));
    serverDevices.Add(serverBrigeLink.Get(0));
    bridgeDevices.Add(serverBrigeLink.Get(1));

    BridgeHelper bridge;
    bridge.Install(bridgeNodes.Get(0), bridgeDevices);
    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        apBridgeDevices[i] = bridge.Install(apNodes[i].Get(0), 
                                            NetDeviceContainer(apWifiDevices[i].Get(0), apCsmaDevices[i].Get(0)));
    }

    // 设置移动模型
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> alloc = CreateObject<ListPositionAllocator>();
    if (nWifi == 4)
    {
        // ap节点位置
        alloc->Add(Vector(0.0, 0.0, 0.0));
        alloc->Add(Vector(51, 0.0, 0.0));
        alloc->Add(Vector(0, 51, 0));
        alloc->Add(Vector(51, 51, 0));

        // ap1的节点位置
        alloc->Add(Vector(-20, -15, 0));
        alloc->Add(Vector(0, -30, 0));

        // ap2的节点位置
        alloc->Add(Vector(35, -20, 0));
        alloc->Add(Vector(60, -29, 0));
        alloc->Add(Vector(65, 5, 0));

        // ap3的节点位置
        alloc->Add(Vector(-10, 40, 0));
        alloc->Add(Vector(15, 37, 0));
        alloc->Add(Vector(-20, 70, 0));

        // ap4的节点位置
        alloc->Add(Vector(65, 65, 0));
        alloc->Add(Vector(40, 75, 0));
    }
    // 交换机位置
    alloc->Add(Vector(100, 100, 0));
    // 服务器位置
    alloc->Add(Vector(120, 100, 0));
    // 机器人位置
    alloc->Add(Vector(-44.1, 20.5, 0));
    mobility.SetPositionAllocator(alloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        mobility.Install(apNodes[i]);
    }
    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        mobility.Install(staNodes[i]);
    }

    mobility.Install(bridgeNodes);
    mobility.Install(serverNodes);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(robotNodes);

    Ptr<ConstantVelocityMobilityModel>  mob =robotNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>();
    mob->SetVelocity(Vector(2, 0, 0));

    InternetStackHelper stack;
    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        stack.Install(staNodes[i]);
        stack.Install(apNodes[i]);
    }
    stack.Install(robotNodes);
    stack.Install(serverNodes);

    // 安装ip地址
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        apIpv4Interfaces[i] =  address.Assign(apBridgeDevices[i]);
    }
    for (uint32_t i = 1; i <= nWifi; ++i)
    {
        staIpv4Interfaces[i] = address.Assign(staDevices[i]);
    }
    robotIpv4Interfaces = address.Assign(robotDevices);
    serverIpv4Interfaces = address.Assign(serverDevices);

    vector<ApplicationContainer> clientApps(5); 
    vector<ApplicationContainer> serverApps(5);
    {
        OnOffHelper client1("ns3::UdpSocketFactory", InetSocketAddress(staIpv4Interfaces[1].GetAddress(0), 9));
        client1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        client1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client1.SetAttribute("DataRate", DataRateValue(DataRate(1000*1000*8)));
        client1.SetAttribute("PacketSize", UintegerValue(1024));
        clientApps[1] = client1.Install(serverNodes.Get(0));
        clientApps[1].Start(Seconds(1));
        clientApps[1].Stop(Seconds(50));

        PacketSinkHelper server("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
        serverApps[1] = server.Install(staNodes[1].Get(0));
        serverApps[1].Start(Seconds(0.9));
        serverApps[1].Stop(Seconds(50));
    }
    {
        OnOffHelper client1("ns3::UdpSocketFactory", InetSocketAddress(staIpv4Interfaces[2].GetAddress(0), 10));
        client1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        client1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client1.SetAttribute("DataRate", DataRateValue(DataRate(1000*1000*4)));
        client1.SetAttribute("PacketSize", UintegerValue(1024));
        clientApps[2] = client1.Install(serverNodes.Get(0));
        clientApps[2].Start(Seconds(1.001));
        clientApps[2].Stop(Seconds(50));

        PacketSinkHelper server("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10));
        serverApps[2] = server.Install(staNodes[2].Get(0));
        serverApps[2].Start(Seconds(0.9));
        serverApps[2].Stop(Seconds(50));
    }
    {
        OnOffHelper client1("ns3::UdpSocketFactory", InetSocketAddress(staIpv4Interfaces[3].GetAddress(0),11));
        client1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        client1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client1.SetAttribute("DataRate", DataRateValue(DataRate(1000*1000*4)));
        client1.SetAttribute("PacketSize", UintegerValue(1024));
        clientApps[3] = client1.Install(serverNodes.Get(0));
        clientApps[3].Start(Seconds(1.002));
        clientApps[3].Stop(Seconds(50));

        PacketSinkHelper server("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),11));
        serverApps[3] = server.Install(staNodes[3].Get(0));
        serverApps[3].Start(Seconds(0.9));
        serverApps[3].Stop(Seconds(50));
    }
    {
        OnOffHelper client1("ns3::UdpSocketFactory", InetSocketAddress(staIpv4Interfaces[4].GetAddress(0), 12));
        client1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        client1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client1.SetAttribute("DataRate", DataRateValue(DataRate(1000*1000*8)));
        client1.SetAttribute("PacketSize", UintegerValue(1024));
        clientApps[4] = client1.Install(serverNodes.Get(0));
        clientApps[4].Start(Seconds(1.003));
        clientApps[4].Stop(Seconds(50));

        PacketSinkHelper server("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 12));
        serverApps[4] = server.Install(staNodes[4].Get(0));
        serverApps[4].Start(Seconds(0.9));
        serverApps[4].Stop(Seconds(50));
    }

    Config::ConnectWithoutContext("/NodeList/14/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/Assoc", 
                                  MakeBoundCallback(&AssocTrace, robotNodes.Get(0)));
    Config::ConnectWithoutContext("/NodeList/14/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/DeAssoc", 
                                  MakeBoundCallback(&deAssocTrace, robotNodes.Get(0)));
    //   Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/Txop/Queue/PacketsInQueue",
    //                               MakeBoundCallback(&queueSizeTrace, &dataset));

    if (tracing)
    {
        phy.EnablePcapAll("testwifi");
        AsciiTraceHelper ascii;
        MobilityHelper::EnableAsciiAll(ascii.CreateFileStream("./files/mymobility.mob"));
    }
    AnimationInterface anim("./files/mymobility.xml");
    for (uint32_t i = 0; i < 2; ++i)
    {
        anim.UpdateNodeColor(staNodes[1].Get(i), 0, 0, 0);
        anim.UpdateNodeSize(4+i, 10, 10);
    }
    for (uint32_t i = 0; i < 3; ++i)
    {
        anim.UpdateNodeColor(staNodes[2].Get(i), 0, 255, 0);
        anim.UpdateNodeSize(6+i, 10, 10);
    }
    for (uint32_t i = 0; i < 3; ++i)
    {
        anim.UpdateNodeColor(staNodes[3].Get(i), 0, 0, 255);
        anim.UpdateNodeSize(9+i, 10, 10);
    }
    for (uint32_t i = 0; i < 2; ++i)
    {
        anim.UpdateNodeColor(staNodes[4].Get(i), 100, 100, 100);
        anim.UpdateNodeSize(12+i, 10, 10);
    }
    anim.UpdateNodeColor(robotNodes.Get(0), 255, 255, 0);
    anim.UpdateNodeSize(14, 10, 10);
    anim.UpdateNodeColor(serverNodes.Get(0), 255, 0, 255);
    anim.UpdateNodeSize(15, 10, 10);
    anim.UpdateNodeColor(bridgeNodes.Get(0), 0, 255, 255);
    anim.UpdateNodeSize(16, 10, 10);
    anim.SetMaxPktsPerTraceFile(1310720*50);
    for (uint32_t i = 0; i < nWifi; ++i)
    {
        anim.UpdateNodeSize(i, 10, 10);
        stringstream ss;
        ss << "AP" << i+1;
        anim.UpdateNodeDescription(i, ss.str().c_str());
    }
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Simulator::Stop(Seconds(75));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        {
            if (i->first == 1 || i->first == 4)
            {
                Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
                std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
                std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
                std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
                std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 49 / 1000 / 1000  << " Mbps\n";
                std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
                std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
                std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 49.0 / 1000 / 1000  << " Mbps\n";

            }
            else 
            {
                Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
                std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
                std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
                std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
                std::cout << "  TxOffered:  " << i->second.txBytes * 4.0 / 49 / 1000 / 1000  << " Mbps\n";
                std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
                std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
                std::cout << "  Throughput: " << i->second.rxBytes * 4.0 / 49.0 / 1000 / 1000  << " Mbps\n";

            }
        }
        cout << endl << endl;
    }

    /* addDataset(&plt, &dataset); */
    /* createPlotFile(&plt, "MacQueueSize.plt"); */
    Simulator::Destroy();

    return 0;

}
