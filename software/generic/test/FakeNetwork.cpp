#include "FakeNetwork.hpp"

#include <Debug.hpp>
#include <cstdio>

#include "../NetworkManager.hpp"

//#define DEBUG

FakeNetwork::FakeNetwork() {
  setMillis(0);

  for (int i = 0; i < kNumNodes; i++) {
    advanceMillis(1);
    stateMachines[i] = new RadioStateMachine(new NetworkManager(&radios[i]));
    ledManagers[i] = new FakeLedManager(5, stateMachines[i]);
    stateMachines[i]->Tick();
  }
}

void FakeNetwork::Tick() {
  // Note: this means that a sender will receive its own packet. This is OK,
  // since senders should be ignoring their own packets anyway, because of the
  // repeater functionality.
  for (int i = 0; i < kNumNodes; i++) {
    if (packetLoss == 0 || rand() % packetLoss) {
      radios[i].setReceivedPacket(packet);
    } else {
      debug_printf("Randomly dropping packet\n");
    }
  }
  packet = nullptr;

  debug_printf("Running FakeNetwork::Tick at %u millis\n", millis());

  for (int i = 0; i < kNumNodes; i++) {
    debug_printf("Node %d state before: %d\n", i,
                 stateMachines[i]->GetCurrentState());
    stateMachines[i]->Tick();
    debug_printf("Node %d state after: %d\n", i,
                 stateMachines[i]->GetCurrentState());

    if (packet == nullptr) {
      packet = radios[i].getSentPacket();
      if (packet != nullptr) {
        debug_printf("Node %d sending type %d\n", i, packet->type);
      }
    }

    ledManagers[i]->RunEffect();

    debug_printf("\n");
  }
}

void FakeNetwork::reinitNode(int index) {
  delete stateMachines[index];
  stateMachines[index] =
      new RadioStateMachine(new NetworkManager(&radios[index]));
}

void FakeNetwork::setPacketLoss(int n) { packetLoss = n; }

void FakeNetwork::TransmitPacket(RadioPacket &packet) {
  for (int i = 0; i < kNumNodes; i++) {
    radios[i].setReceivedPacket(&packet);
  }
}
