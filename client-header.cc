#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "client-header.h"
#include <iostream>

using namespace ns3;

ClientHeader::ClientHeader ()
{

}
ClientHeader::~ClientHeader ()
{

}

TypeId
ClientHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ClientHeader")
    .SetParent<Header> ()
    .AddConstructor<ClientHeader> ()
  ;
  return tid;
}
TypeId
ClientHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ClientHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "current state=" << state;
  os << "current frame=" << currentFrame;
}
uint32_t
ClientHeader::GetSerializedSize (void) const
{
  // we reserve 2 bytes for our header.

  // 1 + 2 + 400
  return 403;
}
void
ClientHeader::Serialize (Buffer::Iterator start) const
{
  // we can serialize two bytes at the start of the buffer.
  // we write them in network byte order.
  start.WriteU8 (state);
  start.WriteHtonU16 (currentFrame);

  for(uint32_t i=0;i<100;i++){
	start.WriteHtonU32(retransmitRequest[i]);
  }

}
uint32_t
ClientHeader::Deserialize (Buffer::Iterator start)
{
  // we can deserialize two bytes from the start of the buffer.
  // we read them in network byte order and store them
  // in host byte order.
  state = start.ReadU8 ();
  currentFrame = start.ReadNtohU16 ();
  for(uint32_t i=0;i<100;i++){
	retransmitRequest[i] = start.ReadNtohU32();
  }

  // we return the number of bytes effectively read.
  return GetSerializedSize();
}

void 
ClientHeader::Set (uint8_t _state, uint16_t _currentFrame, uint32_t* _retransmitRequest)
{
  state = _state;
  currentFrame = _currentFrame;

  for(uint32_t i=0;i<100;i++){
	retransmitRequest[i] = _retransmitRequest[i];
  }
}
uint8_t
ClientHeader::GetState (void) const
{
  return state;
}
uint16_t 
ClientHeader::GetCurrentFrame (void) const
{
  return currentFrame;
}
uint32_t* 
ClientHeader::GetRetransmitRequest (void) 
{
  return retransmitRequest;
}

/*
int main (int argc, char *argv[])
{
  // Enable the packet printing through Packet::Print command.
  Packet::EnablePrinting ();

  // instantiate a header.
  ClientHeader sourceHeader;
  sourceHeader.SetData (2);

  // instantiate a packet
  Ptr<Packet> p = Create<Packet> ();

  // and store my header into the packet.
  p->AddHeader (sourceHeader);

  // print the content of my packet on the standard output.
  p->Print (std::cout);
  std::cout << std::endl;

  // you can now remove the header from the packet:
  ClientHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  // and check that the destination and source
  // headers contain the same values.
  NS_ASSERT (sourceHeader.GetCurrentFrame () == destinationHeader.GetCurrentFrame ());

  return 0;
}
*/
