#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/seq-ts-header.h"
#include "ns3/double.h"
#include "ns3/boolean.h"

#include <algorithm>
#include "client-header.h"
#include "streaming-streamer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StreamingStreamerApplication");

NS_OBJECT_ENSURE_REGISTERED (StreamingStreamer);

TypeId
StreamingStreamer::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::StreamingStreamer")
		.SetParent<Application> ()
		.AddConstructor<StreamingStreamer> ()
    .AddAttribute ("RemoteAddress", 
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&StreamingStreamer::m_peerAddress),
                   MakeAddressChecker ())
		.AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&StreamingStreamer::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
		.AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StreamingStreamer::SetDataSize,
                                         &StreamingStreamer::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("StreamingFPS", 
                   "Stream FPS",
                   UintegerValue (90),
                   MakeUintegerAccessor (&StreamingStreamer::m_fps),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FramePackets", 
                   "# of packets in forms of a frame",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StreamingStreamer::m_fpacketN),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("PacketLossEnable", 
                   "Forced Packet Loss on/off",
                   BooleanValue (false),
                   MakeBooleanAccessor (&StreamingStreamer::m_lossEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("ErrorRate", 
                   "ErrorRate",
                   DoubleValue (0.01),
                   MakeDoubleAccessor (&StreamingStreamer::m_errorRate),
                   MakeDoubleChecker<double> ())
	;
	return tid;
}

StreamingStreamer::StreamingStreamer ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
	m_seqNumber = 0;
	m_pause = false;
	currentFrame = 0;
}

StreamingStreamer::~StreamingStreamer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void 
StreamingStreamer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
  {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);
    if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
    {
      if (m_socket->Bind () == -1)
      {
        NS_FATAL_ERROR ("Failed to bind socket");
      }
      m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
    }
    else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
    {
      if (m_socket->Bind () == -1)
      {
        NS_FATAL_ERROR ("Failed to bind socket");
      }
      m_socket->Connect (m_peerAddress);
    }
    else
    {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
    }
  }

  m_socket->SetRecvCallback (MakeCallback (&StreamingStreamer::HandleRead, this));
  m_socket->SetAllowBroadcast (true);
  ScheduleTx (Seconds (0.));
}

void 
StreamingStreamer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void 
StreamingStreamer::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);
  m_size = dataSize;
}

uint32_t 
StreamingStreamer::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
StreamingStreamer::ScheduleTx (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &StreamingStreamer::SendPacket, this);
}

void 
StreamingStreamer::SendPacket (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

	NS_LOG_INFO(Simulator::Now ().GetSeconds () << "    " << m_pause);
	if (!m_pause)
	{
		uint32_t retransmit_count=0;
		if (retransmit_queue.size()>0){
			std::unique(retransmit_queue.begin(), retransmit_queue.end());

			std::deque<uint32_t>::iterator iter;
			for(iter=retransmit_queue.begin();iter!=retransmit_queue.end();)
			{
				if (*iter < currentFrame*100)
				{
					iter = retransmit_queue.erase(iter);
				}
				else
				{
					iter++;
				}
			}

			for(uint32_t i=0;i<100;i++)
			{
				if (retransmit_queue.empty())
					break;
				retransmit_count++;
				uint32_t retransmit_packet = retransmit_queue.front();
				Ptr<Packet> p;
				p = Create<Packet> (m_size);
				Address localAddress;
				m_socket->GetSockName (localAddress);
				SeqTsHeader seqTs;	
				seqTs.SetSeq (retransmit_packet);
				p->AddHeader (seqTs);
				m_socket->Send (p);
				retransmit_queue.pop_front();
			}

			printf("retransmitted %d packets from streamer  \n",retransmit_count);
		}
//		if (m_fpacketN - retransmit_count > 0)
//		{
			for (uint32_t i=0; i<m_fpacketN; i++)
			{
				Ptr<Packet> p;
				p = Create<Packet> (m_size);

				Address localAddress;
				m_socket->GetSockName (localAddress);

				SeqTsHeader seqTs;	
				seqTs.SetSeq (m_seqNumber++);
				p->AddHeader (seqTs);

				m_socket->Send (p);
				++m_sent;
				  if (Ipv4Address::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s streamer sent " << m_size << " bytes to " <<
		Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
	{
		NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s streamer sent " << m_size << " bytes to " <<
    InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
  }
			}
//		}
	}

	// Packet Log
	/*
  if (Ipv4Address::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s streamer sent " << m_size << " bytes to " <<
		Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
	{
		NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s streamer sent " << m_size << " bytes to " <<
    InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
  }
	*/

	m_sendEvent = Simulator::Schedule ( Seconds ((double)1.0/m_fps), &StreamingStreamer::SendPacket, this);

}

void
StreamingStreamer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  
	while ((packet = socket->RecvFrom (from)))
  {
    if (InetSocketAddress::IsMatchingType (from))
    {
			// Packet Log
			/*
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s streamer received " << packet->GetSize () << " bytes from " <<
      InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
      InetSocketAddress::ConvertFrom (from).GetPort ());
			*/

			if (m_lossEnable)
			{
				double prob = (double)rand() / RAND_MAX;
				if (prob <= m_errorRate)
					continue;
			}

			//dongwon
			ClientHeader header;
			packet->RemoveHeader (header);
			uint8_t state = header.GetState();
			uint16_t currentFrame = header.GetCurrentFrame ();
			uint32_t* requests =  header.GetRetransmitRequest ();
			
			if (state == 1)  // pause packet
				m_pause = true;
			else if (state == 2)  // resume packet
				m_pause = false;
			else if (state == 0)  // retransmit request packet
			{
				std::deque<uint32_t>::iterator iter;
				for(iter=retransmit_queue.begin();iter!=retransmit_queue.end();)
				{
					if (*iter < currentFrame*100)
					{
						iter = retransmit_queue.erase(iter);
					}
					else
					{
						iter++;
					}
				}

				//printf("retransmit request received... from %d..(current frame: %d) \n", requests[0], currentFrame);
				for(uint32_t i=0;i<100;i++){
					if (requests[i] == 0) 
						break;	
					retransmit_queue.push_back(requests[i]);
				}
			}
				
			
    }
    socket->GetSockName (localAddress);
	}
}
}
