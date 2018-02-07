#ifndef __FAKE_NETWORK_H__
#define __FAKE_NETWORK_H__

#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeRadio.hpp"

class FakeNetwork {
 public:
  FakeNetwork();

  /**
   * Runs one cycle of packet distribution. Runs the Tick method on each
   * state manager, and sends up to one packet to the network.
   */
  void Tick();

  /**
   * Re-initializes the state machine at index.
   */
  void reinitNode(int index);

  static const long kNumNodes = 5;
  RadioStateMachine *stateMachines[kNumNodes];

 private:
  FakeRadio radios[kNumNodes];
  RadioPacket *packet = nullptr;
};

#endif
