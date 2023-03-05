#ifndef __FAKE_NETWORK_H__
#define __FAKE_NETWORK_H__

#include <Radio.hpp>

#include "RadioStateMachine.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"

class FakeNetwork {
 public:
  FakeNetwork();
  ~FakeNetwork();

  /**
   * Runs one cycle of packet distribution. Runs the Tick method on each
   * state manager, and sends up to one packet to the network.
   */
  void Tick();

  /**
   * Re-initializes the state machine at index.
   */
  void reinitNode(int index);

  /**
   * Sets the packet loss to one in every n packets, on average. 0 disables.
   */
  void setPacketLoss(int n);

  /**
   * Transmits a packet to all nodes on the network.
   */
  void TransmitPacket(RadioPacket &packet);

  static const long kNumNodes = 5;
  NetworkManager *networkManagers[kNumNodes];
  RadioStateMachine *stateMachines[kNumNodes];
  FakeLedManager *ledManagers[kNumNodes];

 private:
  int packet_loss = 0;
  FakeRadio radios[kNumNodes];
  RadioPacket *packet = nullptr;
  RadioPacket *previous_packet = nullptr;
  DeviceDescription device =
      DeviceDescription(2000, {StripDescription(5, {})});
};

#endif
