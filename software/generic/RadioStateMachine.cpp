#include "RadioStateMachine.hpp"
#include <cstdio>
#include "Debug.hpp"
#include "Effect.hpp"

//#define DEBUG

RadioStateMachine::RadioStateMachine(NetworkManager *networkManager)
    : networkManager(networkManager) {
  state = RadioState::Slave;
  nextState = RadioState::Slave;
  beginSlave();
  setEffectPacket.writeSetEffect(1, 0, 0);
}

RadioState RadioStateMachine::GetCurrentState() { return state; }

void RadioStateMachine::Tick() {
  // Run the radio state machine twice, since writing out LEDs may take several
  // ms.
  RadioTick();
  RadioTick();
}

uint32_t RadioStateMachine::GetNetworkMillis() {
  // DANGER: adding unsigned and signed types!
  return millis() + millisOffset;
}

uint8_t RadioStateMachine::GetEffectIndex() { return effectIndex; }

RadioPacket *const RadioStateMachine::GetSetEffect() {
  return &setEffectPacket;
}

void RadioStateMachine::SetEffect(RadioPacket *const setEffect) {
  this->setEffectPacket = *setEffect;
  this->networkManager->send(this->setEffectPacket);
  if (this->setEffectPacket.readDelayFromSetEffect()) {
    SetEffectTimer(this->setEffectPacket.readDelayFromSetEffect() * 1000);
  } else {
    SetEffectTimer(kSetEffectInterval);
  }
}

void RadioStateMachine::SetNumPalettes(uint8_t numPalettes) {
  this->numPalettes = numPalettes;
}

void RadioStateMachine::SetNumEffects(uint8_t numEffects) {
  this->numEffects = numEffects;
}

void RadioStateMachine::RadioTick() {
  RadioEventData data;
  data.packet = nullptr;
  data.heartbeatTimerExpired = false;
  data.effectTimerExpired = false;

  RadioPacket packet;

  // Note: only allow one event type per iteration. This makes the handler
  // functions simpler (and less error prone).
  if (networkManager->receive(packet)) {
    debug_printf("Received packet\n");
    data.packet = &packet;
  } else if (heartbeatTimerExpiresAt && millis() > heartbeatTimerExpiresAt) {
    debug_printf("Heartbeat timer expired\n");
    data.heartbeatTimerExpired = true;
  } else if (effectTimerExpiresAt && millis() > effectTimerExpiresAt) {
    debug_printf("SetEffect timer expired\n");
    data.effectTimerExpired = true;
  }

  if (data.packet != nullptr || data.heartbeatTimerExpired ||
      data.effectTimerExpired) {
    switch (state) {
      case RadioState::Slave:
        handleSlaveEvent(data);
        break;

      case RadioState::Master:
        handleMasterEvent(data);
        break;
    }
  }

  if (nextState != state) {
    // 0 is disabled. Clear the timer - the state will probably set this
    // anyway.
    SetHeartbeatTimer(0);

    switch (nextState) {
      case RadioState::Slave:
        beginSlave();
        break;

      case RadioState::Master:
        beginMaster();
        break;
    }
  }

  state = nextState;
}

void RadioStateMachine::handleSlaveEvent(RadioEventData &data) {
  // If the timer fired, then we haven't received a packet in a while and should
  // become master
  if (data.packet != nullptr) {
    switch (data.packet->type) {
      case HEARTBEAT:
        // DANGER: unsigned + signed math!
        this->millisOffset =
            data.packet->readTimeFromHeartbeat() - (int32_t)millis();

        // Fall through
      case CLAIM_MASTER:
        SetHeartbeatTimer(kSlaveNoPacketTimeout +
                          rand() % kSlaveNoPacketRandom);
        break;

      case SET_EFFECT:
        this->setEffectPacket = *data.packet;
        this->effectIndex = data.packet->readEffectIndexFromSetEffect();
        break;
    }
  } else if (data.heartbeatTimerExpired) {
    nextState = RadioState::Master;
  }
}

void RadioStateMachine::PerformMasterElection(RadioPacket *receivedPacket) {
  debug_printf("Performing master election\n");

  // Master election: generate a random number. If our number is greater
  // than the packet's ID, become master. Otherwise, become slave.
  const uint16_t ourId = random(1, 0xFFFF);
  if (ourId > receivedPacket->packetId) {
    packet.type = CLAIM_MASTER;
    packet.dataLength = 0;
    networkManager->send(packet);
  } else {
    nextState = RadioState::Slave;
  }
}

void RadioStateMachine::handleMasterEvent(RadioEventData &data) {
  if (data.packet != nullptr) {
    debug_printf("Received type %d\n", data.packet->type);
    switch (data.packet->type) {
      case HEARTBEAT:
        PerformMasterElection(data.packet);
        break;

      case CLAIM_MASTER:
        nextState = RadioState::Slave;
        break;

      case SET_EFFECT:
        this->setEffectPacket = *data.packet;
        this->effectIndex = data.packet->readEffectIndexFromSetEffect();
        uint32_t changeEffectTime =
            (uint32_t)(data.packet->readDelayFromSetEffect()) * 1000;
        SetEffectTimer(changeEffectTime);
        break;
    }
  } else if (data.heartbeatTimerExpired) {
    SendHeartbeat();
    SetHeartbeatTimer(kMasterHeartbeatInterval);
  } else if (data.effectTimerExpired) {
    effectIndex = random(0, numEffects);
    const uint8_t paletteIndex = random(0, numPalettes);
    packet.writeSetEffect(effectIndex, /* delay= */ 0, paletteIndex);
    this->setEffectPacket = packet;
    networkManager->send(packet);
    SetEffectTimer(kSetEffectInterval);
  }
}

void RadioStateMachine::beginSlave() {
  SetHeartbeatTimer(kSlaveNoPacketTimeout + rand() % kSlaveNoPacketRandom);
}

void RadioStateMachine::beginMaster() {
  SendHeartbeat();
  SetHeartbeatTimer(kMasterHeartbeatInterval);
  SetEffectTimer(kSetEffectInterval);
}

void RadioStateMachine::SetHeartbeatTimer(uint32_t delay) {
  if (delay == 0) {
    heartbeatTimerExpiresAt = 0;
  } else {
    heartbeatTimerExpiresAt = millis() + delay;
  }
}

void RadioStateMachine::SetEffectTimer(uint32_t delay) {
  if (delay == 0) {
    effectTimerExpiresAt = 0;
  } else {
    effectTimerExpiresAt = millis() + delay;
  }
}

void RadioStateMachine::SendHeartbeat() {
  packet.writeHeartbeat(GetNetworkMillis());

  networkManager->send(packet);
}
