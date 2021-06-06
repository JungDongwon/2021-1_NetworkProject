#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/seq-ts-header.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include <random>

#include <algorithm>
#include "client-header.h"
#include "streaming-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StreamingClientApplication");

NS_OBJECT_ENSURE_REGISTERED (StreamingClient);

TypeId
StreamingClient::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::StreamingClient")
		.SetParent<Application> ()
		.SetGroupName("Applications")
		.AddConstructor<StreamingClient> ()
		.AddAttribute ("RemotePort", "Port on which we listen for incoming packets.",
									 UintegerValue (9),
									 MakeUintegerAccessor (&StreamingClient::m_port),
									 MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemoteAddress", 
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&StreamingClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("PacketSize", 
                   "The packet size",
									 UintegerValue (100),
									 MakeUintegerAccessor (&StreamingClient::m_packetSize),
									 MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BufferSize", 
                   "The frame buffer size",
									 UintegerValue (40),
									 MakeUintegerAccessor (&StreamingClient::m_bufferSize),
									 MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("PauseSize", 
                   "The pause size for the frame buffer",
									 UintegerValue (30),
									 MakeUintegerAccessor (&StreamingClient::m_pause),
									 MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ResumeSize", 
                   "The resume size for the frame buffer",
									 UintegerValue (5),
									 MakeUintegerAccessor (&StreamingClient::m_resume),
									 MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FramePackets", 
                   "# of packets in forms of a frame",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StreamingClient::m_fpacketN),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ConsumeStartTime", 
                   "Consumer Start Time",
                   DoubleValue (1.1),
                   MakeDoubleAccessor (&StreamingClient::m_consumeTime),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("PacketLossEnable", 
                   "Forced Packet Loss on/off",
                   BooleanValue (false),
                   MakeBooleanAccessor (&StreamingClient::m_lossEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("ErrorRate", 
                   "ErrorRate",
                   DoubleValue (0.01),
                   MakeDoubleAccessor (&StreamingClient::m_errorRate),
                   MakeDoubleChecker<double> ())
	.AddAttribute ("Buffering", 
                   "Buffering",
                   UintegerValue(15),
                   MakeUintegerAccessor (&StreamingClient::m_buffering),
                   MakeUintegerChecker<uint32_t> ())
		;
	return tid;
}

StreamingClient::StreamingClient ()
{
	NS_LOG_FUNCTION (this);
	m_seqNumber = 0;
	m_consumEvent = EventId ();
	m_genEvent = EventId ();
	m_frameCnt = 0;
	m_frameIdx = 0;
	m_throughputEvent = EventId ();
	m_bufferingEvent = EventId ();
}

StreamingClient::~StreamingClient ()
{
	NS_LOG_FUNCTION (this);
	m_socket = 0;
}

void
StreamingClient::FrameConsumer (void)
{
	// Frame Consume
	if (m_frameCnt >= 0)
	{
		if (m_frameBuffer.find (m_frameIdx) != m_frameBuffer.end() )
		{
			m_frameCnt -= 1;
			m_frameBuffer.erase(m_frameIdx);
			NS_LOG_INFO("FrameConsumerLog::Consume");
			// graph log
			/*
			double cond = Simulator::Now ().GetSeconds() - int(Simulator::Now ().GetSeconds());
			if ((cond >= double(0.99)) | (cond <= double(0.51) && cond >= double(0.49)))
			{
				uint32_t now_recv = m_recv;
				double throughput = (now_recv - prev_recv_packet) * m_packetSize * 8/ 500000.0;
				NS_LOG_INFO(Simulator::Now ().GetSeconds () << "\t" << 1 << "\t" << m_frameCnt << "\t" << throughput);
				prev_recv_packet = now_recv;
			}
			else NS_LOG_INFO(Simulator::Now ().GetSeconds () << "\t" << 1 << "\t" << m_frameCnt << "\t" << "0");
			*/
			// ~graph log
		}
		else if (m_frameCnt == 0)
		{
			NS_LOG_INFO("FrameConsumerLog::NoConsume");
			// graph log
			/*
			double cond = Simulator::Now ().GetSeconds() - int(Simulator::Now ().GetSeconds());
			if ((cond >= double(0.99)) | (cond <= double(0.51) && cond >= double(0.49)))
			{
				uint32_t now_recv = m_recv;
				double throughput = (now_recv - prev_recv_packet) * m_packetSize * 8/ 500000.0;
				NS_LOG_INFO(Simulator::Now ().GetSeconds () << "\t" << 0 << "\t" << m_frameCnt << "\t" << throughput);
				prev_recv_packet = now_recv;
			}
			else NS_LOG_INFO(Simulator::Now ().GetSeconds () << "\t" << 0 << "\t" << m_frameCnt << "\t" << "0");
			*/
			// ~graph log
		}
		else
		{
			NS_LOG_INFO("FrameConsumerLog::NoConsume");
			// graph log
			/*
			double cond = Simulator::Now ().GetSeconds() - int(Simulator::Now ().GetSeconds());
			if ((cond >= double(0.99)) | (cond <= double(0.51) && cond >= double(0.49)))
			{
				uint32_t now_recv = m_recv;
				double throughput = (now_recv - prev_recv_packet) * m_packetSize * 8/ 500000.0;
				NS_LOG_INFO(Simulator::Now ().GetSeconds () << "\t" << 0 << "\t" << m_frameCnt << "\t" << throughput);
				prev_recv_packet = now_recv;
			}
			else NS_LOG_INFO(Simulator::Now ().GetSeconds () << "\t" << 0 << "\t" << m_frameCnt << "\t" << "0");
			*/
			// ~graph log
		}
		NS_LOG_INFO("FrameConsumerLog::RemainFrames: " << m_frameCnt);
	}
	else if (m_frameCnt < 0)
	{
		NS_LOG_INFO("FrameCountError!");
		exit (1);
	}
	m_frameIdx += 1;

	// FrameBufferCheck
	if (m_frameCnt >= (int)m_pause)
	{
		Ptr<Packet> p;
		p = Create<Packet> (m_packetSize);
		ClientHeader header;
		uint32_t request[100] = {0};
		header.Set(1, m_frameIdx, request);
		p->AddHeader (header);

		Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
		udpSocket->SendTo (p, 0, m_peerAddress);
	}
	else if (m_frameCnt <= (int)m_resume)
	{
		Ptr<Packet> p;
		p = Create<Packet> (m_packetSize);
		ClientHeader header;
		uint32_t request[100] = {0};
		header.Set(2, m_frameIdx, request);
		p->AddHeader (header);

		Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
		udpSocket->SendTo (p, 0, m_peerAddress);
	}

	m_consumEvent = Simulator::Schedule ( Seconds ((double)1.0/60), &StreamingClient::FrameConsumer, this);
}


void 
StreamingClient::FrameGenerator (void)
{
	RequestRetransmit();
	if (m_frameCnt < (int)m_bufferSize)
	{
		std::map<uint32_t,FrameCheck>::iterator iter;

		for (iter = m_pChecker.begin(); iter != m_pChecker.end();)
		{
			uint32_t idx = iter->first;

			if (idx < m_frameIdx)
			{
				m_pChecker.erase(iter++);
			}
			else
			{
				uint32_t count = 0;
				for (uint32_t i=0; i<m_fpacketN; i++)
				{
					FrameCheck c = iter->second;
					if (c.c[i] == 1)
						count++;
				}

				if (count == m_fpacketN)
				{
					m_pChecker.erase(iter++);

					m_frameCnt++;
					m_frameBuffer.insert({idx, true});
				}
				else
				{
					++iter;
				}
			}
		}
	}

	m_genEvent = Simulator::Schedule ( Seconds ((double)1.0/20), &StreamingClient::FrameGenerator, this);
}


void 
StreamingClient::StartApplication (void)
{
	NS_LOG_FUNCTION (this);
	
	if (m_socket == 0)
  {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
    if (m_socket->Bind (local) == -1)
    {
			NS_FATAL_ERROR ("Failed to bind socket");
    }
    if (addressUtils::IsMulticast (m_local))
		{
			Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
      if (udpSocket)
      {
        // equivalent to setsockopt (MCAST_JOIN_GROUP)
        udpSocket->MulticastJoinGroup (0, m_local);
      }
      else
      {
        NS_FATAL_ERROR ("Error: Failed to join multicast group");
      }
    }
  }

	m_socket->SetRecvCallback (MakeCallback (&StreamingClient::HandleRead, this));
	m_bufferingEvent = Simulator::Schedule ( Seconds (m_consumeTime), &StreamingClient::BufferingChecker, this);
	m_throughputEvent = Simulator::Schedule ( Seconds (m_consumeTime), &StreamingClient::CalcThroughput, this);
	FrameGenerator ();
}

void
StreamingClient::StopApplication ()
{
	if (m_socket != 0) 
  {
		m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }

	Simulator::Cancel (m_consumEvent);
}

void 
StreamingClient::RequestRetransmit()
{	
	if (request_vector.size() > 0)
	{
		std::vector<uint32_t>::iterator iter;
		for (iter=request_vector.begin();iter!=request_vector.end();)
		{
			if (*iter<m_frameIdx*100)
			{
				iter = request_vector.erase(iter);
			}
			else
			{
				iter++;
			}
		}

		uint32_t request[100] = {0}; 
		uint32_t idx = 0;
		
		for(uint32_t i=0;i<std::min(uint32_t(request_vector.size()),uint32_t(100));i++)
		{
			request[idx] = request_vector[i];
			idx++;
		}
		
		Ptr<Packet> p;
		p = Create<Packet> (m_packetSize);
		ClientHeader header;
		header.Set(0, m_frameIdx, request);
		p->AddHeader (header);

		Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
		udpSocket->SendTo (p, 0, m_peerAddress);
	}
}

void StreamingClient::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;
	Address localAddress;
	while ((packet = socket->RecvFrom (from)))
	{
		socket->GetSockName (localAddress);

		// Packet Log
		/*
		if (InetSocketAddress::IsMatchingType (from))
		{
			NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
					InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
					InetSocketAddress::ConvertFrom (from).GetPort ());
		}
		*/

		if (m_lossEnable)
		{
			std::random_device rd;
      		std::mt19937 gen(rd());
      		std::uniform_int_distribution<int> dis(0, 99);
			if (double(dis(gen))/100.0 <= m_errorRate){
				continue;
			}
		}

		SeqTsHeader seqTs;
		packet->RemoveHeader (seqTs);
		uint32_t seqNumber = seqTs.GetSeq();
		uint32_t frameIdx = seqNumber/m_fpacketN;
		uint32_t seqN = seqNumber - frameIdx * m_fpacketN;

		if (m_pChecker.size() < (m_bufferSize * 2 * m_fpacketN) )
		{
			if (m_seqNumber == seqNumber)
			{	
				m_seqNumber++;
			}
			else if (m_seqNumber < seqNumber)
			{
				for(uint32_t i=m_seqNumber;i<seqNumber;i++)
				{
	    			request_vector.push_back(i);
				}
				m_seqNumber = seqNumber + 1;
			}
			else
			{
				std::vector<uint32_t>::iterator iter;
				for (iter=request_vector.begin();iter!=request_vector.end();)
				{
					if (*iter == seqNumber || *iter<m_frameIdx*100)
					{
						iter = request_vector.erase(iter);
					}
					else
					{
						iter++;
					}
				}
			}

			if (m_pChecker.find(frameIdx) != m_pChecker.end())
			{
				FrameCheck tc = m_pChecker[frameIdx];
				tc.c[seqN] = 1;
				m_pChecker[frameIdx]=tc;
			}
			else
			{
				FrameCheck c;
				c.c[seqN] = 1;
				m_pChecker.insert({frameIdx,c});
			}
		}

		m_recv += 1;
	}
}

void StreamingClient::CalcThroughput()
{
	// If you want to print out throughput, enable under codes.
	uint32_t now_recv = m_recv;
	// double throughput = (now_recv - prev_recv_packet) * m_packetSize * 8/ 500000.0;
	// NS_LOG_INFO(Simulator::Now ().GetSeconds () << "\t" << throughput);
	prev_recv_packet = now_recv;
	m_throughputEvent = Simulator::Schedule ( Seconds (0.5), &StreamingClient::CalcThroughput, this);
}

void StreamingClient::BufferingChecker()
{
	if (m_frameCnt < (int)m_buffering)
	{
		m_bufferingEvent = Simulator::Schedule ( Seconds ((double)1.0/60.0), &StreamingClient::BufferingChecker, this);
	}
	else
	{
		m_consumEvent = Simulator::Schedule ( Seconds (0.0), &StreamingClient::FrameConsumer, this);
	}
}

FrameCheck::FrameCheck ()
{
	for (uint32_t i=0; i<100; i++)
		c[i] = 0;
}

FrameCheck::~FrameCheck()
{
}

}
