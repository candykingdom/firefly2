#include "RadioStateMachine.hpp"

#include <cstdio>

#include "Debug.hpp"
#include "Effect.hpp"
#include "../generic/ControlEffect.hpp"

//#define DEBUG

RadioStateMachine::RadioStateMachine(NetworkManager *networkManager)
    : networkManager(networkManager) {
  // Note: this takes advantage of the fact that C++ enums are actually uints.
  for (int i = 0; i <= TIMER_TYPE_LAST; i++) {
    timers[i] = 0;
  }
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
  if (millis() > 2000) {
    RadioTick();
  }
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
    SetTimer(TimerChangeEffect,
             this->setEffectPacket.readDelayFromSetEffect() * 1000);
  } else {
    SetTimer(TimerChangeEffect, kChangeEffectInterval);
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
  data.timerExpired = TimerNone;

  RadioPacket packet;

  // Note: only allow one event type per iteration. This makes the handler
  // functions simpler (and less error prone).
  TimerType timerExpired = TimerExpired();
  if (networkManager->receive(packet)) {
    data.packet = &packet;
  } else if (timerExpired != TimerNone) {
    data.timerExpired = timerExpired;
  }

  if (data.packet != nullptr || data.timerExpired != TimerNone) {
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
    SetTimer(TimerHeartbeat, 0);

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
        SetTimer(TimerHeartbeat,
                 kSlaveNoPacketTimeout + rand() % kSlaveNoPacketRandom);
        break;

      case SET_EFFECT:
      {
        uint8_t newEffectIndex = data.packet->readEffectIndexFromSetEffect();
        uint8_t newPaletteIndex = data.packet->readPaletteIndexFromSetEffect();
        if (effectIndex != this->effectIndex ||
            newPaletteIndex !=
                this->setEffectPacket.readPaletteIndexFromSetEffect()) {
          effectChangeSeenAt = millis();
        }
        this->setEffectPacket = *data.packet;
        this->effectIndex = newEffectIndex;
        break;
      }

      case SET_CONTROL:
        effectChangeSeenAt = millis();
        this->setEffectPacket = *data.packet;
        break;
    }
  } else if (data.timerExpired == TimerHeartbeat) {
    nextState = RadioState::Master;
  }
}

void RadioStateMachine::PerformMasterElection(RadioPacket *receivedPacket) {
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
    switch (data.packet->type) {
      case HEARTBEAT:
        PerformMasterElection(data.packet);
        break;

      case CLAIM_MASTER:
        nextState = RadioState::Slave;
        break;

      case SET_EFFECT:
      {
        this->setEffectPacket = *data.packet;
        this->effectIndex = data.packet->readEffectIndexFromSetEffect();
        uint32_t setEfectTime =
            (uint32_t)(data.packet->readDelayFromSetEffect()) * 1000;
        SetTimer(TimerChangeEffect, setEfectTime);
        break;
      }

      case SET_CONTROL:
        this->setEffectPacket = *data.packet;
        uint32_t setControlTime =
            (uint32_t)(data.packet->readDelayFromSetControl()) * 1000;
        SetTimer(TimerChangeEffect, setControlTime);
        break;
    }
  } else if (data.timerExpired != TimerNone) {
    switch (data.timerExpired) {
      case TimerHeartbeat:
        SendHeartbeat();
        SetTimer(TimerHeartbeat, kMasterHeartbeatInterval);
        break;

      case TimerChangeEffect: {
        effectIndex = random(0, numEffects);
        const uint8_t paletteIndex = random(0, numPalettes);
        packet.writeSetEffect(effectIndex, /* delay= */ 0, paletteIndex);
        this->setEffectPacket = packet;
        networkManager->send(packet);
        SetTimer(TimerChangeEffect, kChangeEffectInterval);
        break;
      }

      case TimerBroadcastEffect:
        networkManager->send(this->setEffectPacket);
        SetTimer(TimerBroadcastEffect, kBroadcastEffectInterval);
        break;

      case TimerNone:
        // No-op
        break;
    }
  }
}

void RadioStateMachine::beginSlave() {
  SetTimer(TimerHeartbeat,
           kSlaveNoPacketTimeout + rand() % kSlaveNoPacketRandom);
}

void RadioStateMachine::beginMaster() {
  SendHeartbeat();
  SetTimer(TimerHeartbeat, kMasterHeartbeatInterval);
  SetTimer(TimerBroadcastEffect, kBroadcastEffectInterval);

  if (millis() - effectChangeSeenAt > kChangeEffectInterval) {
    // Change the effect on the next tick
    SetTimer(TimerChangeEffect, 1);
  } else {
    SetTimer(TimerChangeEffect,
             kChangeEffectInterval - (millis() - effectChangeSeenAt));
  }
}

void RadioStateMachine::SetTimer(TimerType timer, uint32_t delay) {
  if (delay == 0) {
    timers[timer] = 0;
  } else {
    timers[timer] = millis() + delay;
  }
}

TimerType RadioStateMachine::TimerExpired() {
  // TODO: Trellis hangs here if RadioTick is called twice the first time -
  // possibly a hardware/compiler bug. This only happens when the device is
  // first powered on (i.e. resetting using the button after it's on works
  // fine).
  for (int i = 0; i <= TIMER_TYPE_LAST; i++) {
    if (timers[i] != 0 && timers[i] < millis()) {
      return (TimerType)i;
    }
  }

  return TimerNone;
}

void RadioStateMachine::SendHeartbeat() {
  packet.writeHeartbeat(GetNetworkMillis());

  networkManager->send(packet);
}
