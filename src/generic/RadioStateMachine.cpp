#include "RadioStateMachine.hpp"

#include <Debug.hpp>
#include <Effect.hpp>
#include <Effects.hpp>
#include <cstdio>

//#define DEBUG

RadioStateMachine::RadioStateMachine(NetworkManager *networkManager)
    : network_manager_(networkManager) {
  // Note: this takes advantage of the fact that C++ enums are actually uints.
  for (int i = 0; i <= TIMER_TYPE_LAST; i++) {
    timers_[i] = 0;
  }
  state_ = RadioState::Slave;
  next_state_ = RadioState::Slave;
  beginSlave();
  set_effect_packet_.writeSetEffect(1, 0, 0);
}

RadioStateMachine::~RadioStateMachine() {}

RadioState RadioStateMachine::GetCurrentState() { return state_; }

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
  return millis() + millis_offset_;
}

uint8_t RadioStateMachine::GetEffectIndex() { return effect_index_; }

RadioPacket *RadioStateMachine::GetSetEffect() { return &set_effect_packet_; }

void RadioStateMachine::SetEffect(RadioPacket *const setEffect) {
  this->set_effect_packet_ = *setEffect;
  this->network_manager_->send(this->set_effect_packet_);
  if (this->set_effect_packet_.readDelayFromSetEffect()) {
    SetTimer(TimerChangeEffect,
             this->set_effect_packet_.readDelayFromSetEffect() * 1000);
  } else {
    SetTimer(TimerChangeEffect, kChangeEffectInterval);
  }
}

void RadioStateMachine::SetNumPalettes(uint8_t num_palettes) {
  this->num_palettes_ = num_palettes;
}

void RadioStateMachine::SetNumEffects(uint8_t num_effects) {
  this->num_effects_ = num_effects;
}

void RadioStateMachine::RadioTick() {
  RadioEventData data;
  data.packet = nullptr;
  data.timer_expired = TimerNone;

  RadioPacket packet;

  // Note: only allow one event type per iteration. This makes the handler
  // functions simpler (and less error prone).
  TimerType timer_expired = TimerExpired();
  if (network_manager_->receive(packet)) {
    data.packet = &packet;
  } else if (timer_expired != TimerNone) {
    data.timer_expired = timer_expired;
  }

  if (data.packet != nullptr || data.timer_expired != TimerNone) {
    switch (state_) {
      case RadioState::Slave:
        handleSlaveEvent(data);
        break;

      case RadioState::Master:
        handleMasterEvent(data);
        break;
    }
  }

  if (next_state_ != state_) {
    // 0 is disabled. Clear the timer - the state will probably set this
    // anyway.
    SetTimer(TimerHeartbeat, 0);

    switch (next_state_) {
      case RadioState::Slave:
        beginSlave();
        break;

      case RadioState::Master:
        beginMaster();
        break;
    }
  }

  state_ = next_state_;
}

void RadioStateMachine::handleSlaveEvent(RadioEventData &data) {
  // If the timer fired, then we haven't received a packet in a while and should
  // become master
  if (data.packet != nullptr) {
    switch (data.packet->type) {
      case HEARTBEAT:
        // DANGER: unsigned + signed math!
        this->millis_offset_ =
            data.packet->readTimeFromHeartbeat() - (int32_t)millis();

        // Fall through
      case CLAIM_MASTER:
        SetTimer(TimerHeartbeat,
                 kSlaveNoPacketTimeout + rand() % kSlaveNoPacketRandom);
        break;

      case SET_EFFECT: {
        uint8_t new_effect_index = data.packet->readEffectIndexFromSetEffect();
        uint8_t new_palette_index =
            data.packet->readPaletteIndexFromSetEffect();
        if (new_effect_index != this->effect_index_ ||
            new_palette_index !=
                this->set_effect_packet_.readPaletteIndexFromSetEffect()) {
          effect_change_seen_at_ = millis();
        }
        this->set_effect_packet_ = *data.packet;
        this->effect_index_ = new_effect_index;
        break;
      }

      case SET_CONTROL:
        effect_change_seen_at_ = millis();
        this->set_effect_packet_ = *data.packet;
        break;
    }
  } else if (data.timer_expired == TimerHeartbeat) {
    next_state_ = RadioState::Master;
  }
}

void RadioStateMachine::PerformMasterElection(RadioPacket *received_packet) {
  // Master election: generate a random number. If our number is greater
  // than the packet's ID, become master. Otherwise, become slave.
  const uint16_t our_id = random(1, 0xFFFF);
  if (our_id > received_packet->packet_id) {
    packet_.type = CLAIM_MASTER;
    packet_.dataLength = 0;
    network_manager_->send(packet_);
  } else {
    next_state_ = RadioState::Slave;
  }
}

void RadioStateMachine::handleMasterEvent(RadioEventData &data) {
  if (data.packet != nullptr) {
    switch (data.packet->type) {
      case HEARTBEAT:
        PerformMasterElection(data.packet);
        break;

      case CLAIM_MASTER:
        next_state_ = RadioState::Slave;
        break;

      case SET_EFFECT: {
        this->set_effect_packet_ = *data.packet;
        this->effect_index_ = data.packet->readEffectIndexFromSetEffect();
        uint32_t set_effect_time =
            (uint32_t)(data.packet->readDelayFromSetEffect()) * 1000;
        SetTimer(TimerChangeEffect, set_effect_time);
        break;
      }

      case SET_CONTROL:
        this->set_effect_packet_ = *data.packet;
        uint32_t set_control_time =
            (uint32_t)(data.packet->readDelayFromSetControl()) * 1000;
        SetTimer(TimerChangeEffect, set_control_time);
        break;
    }
  } else if (data.timer_expired != TimerNone) {
    switch (data.timer_expired) {
      case TimerHeartbeat:
        SendHeartbeat();
        SetTimer(TimerHeartbeat, kMasterHeartbeatInterval);
        break;

      case TimerChangeEffect: {
        effect_index_ = random(0, num_effects_);
        const uint8_t palette_index = random(0, num_palettes_);
        packet_.writeSetEffect(effect_index_, /* delay= */ 0, palette_index);
        this->set_effect_packet_ = packet_;
        network_manager_->send(packet_);
        SetTimer(TimerChangeEffect, kChangeEffectInterval);
        break;
      }

      case TimerBroadcastEffect:
        network_manager_->send(this->set_effect_packet_);
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

  if (millis() - effect_change_seen_at_ > kChangeEffectInterval) {
    // Change the effect on the next tick
    SetTimer(TimerChangeEffect, 1);
  } else {
    SetTimer(TimerChangeEffect,
             kChangeEffectInterval - (millis() - effect_change_seen_at_));
  }
}

void RadioStateMachine::SetTimer(TimerType timer, uint32_t delay) {
  if (delay == 0) {
    timers_[timer] = 0;
  } else {
    timers_[timer] = millis() + delay;
  }
}

TimerType RadioStateMachine::TimerExpired() {
  // TODO: Trellis hangs here if RadioTick is called twice the first time -
  // possibly a hardware/compiler bug. This only happens when the device is
  // first powered on (i.e. resetting using the button after it's on works
  // fine).
  for (int i = 0; i <= TIMER_TYPE_LAST; i++) {
    if (timers_[i] != 0 && timers_[i] < millis()) {
      return (TimerType)i;
    }
  }

  return TimerNone;
}

void RadioStateMachine::SendHeartbeat() {
  packet_.writeHeartbeat(GetNetworkMillis());

  network_manager_->send(packet_);
}
