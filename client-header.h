#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>

using namespace ns3;

class ClientHeader : public Header 
{
public:

  ClientHeader ();
  virtual ~ClientHeader ();

  void Set (uint8_t, uint16_t, uint32_t*);
  uint8_t GetState (void) const; 
  uint16_t GetCurrentFrame (void) const;
  uint32_t* GetRetransmitRequest (void);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;
private:
  uint8_t state; 
  uint16_t currentFrame;  
  uint32_t retransmitRequest[100] = {0};
};


