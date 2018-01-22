#include "../NetworkManager.hpp"
#include "FakeNetwork.hpp"
#include <cstdio>

#define DEBUG

FakeNetwork::FakeNetwork() {
  setMillis(0);

  for (int i = 0; i < kNumNodes; i++) {
    advanceMillis(1);
    stateMachines[i] = new RadioStateMachine(new NetworkManager(&radios[i]));
    stateMachines[i]->Tick();
  }
}

void FakeNetwork::Tick() {
  // Note: this means that a sender will receive its own packet. This is OK,
  // since senders should be ignoring their own packets anyway, because of the
  // repeater functionality.
  for (int i = 0; i < kNumNodes; i++) {
    radios[i].setReceivedPacket(packet);
  }
  packet = nullptr;

#ifdef DEBUG
  printf("Running FakeNetwork::Tick at %u millis\n", millis());
#endif

  for (int i = 0; i < kNumNodes; i++) {
#ifdef DEBUG
    printf("Node %d state before: %d\n", i, stateMachines[i]->GetCurrentState());
#endif
    stateMachines[i]->Tick();
#ifdef DEBUG
    printf("Node %d state after: %d\n", i, stateMachines[i]->GetCurrentState());
#endif

    if (packet == nullptr) {
      packet = radios[i].getSentPacket();
      if (packet != nullptr) {
        printf("Node %d sending type %d\n", i, packet->type);
      }
    }

#ifdef DEBUG
  printf("\n");
#endif
  }
}
