#ifndef __RADIO_STATE_MACHINE_H__
#define __RADIO_STATE_MACHINE_H__

#include <map>
#include "NetworkManager.hpp"
#include "Types.hpp"

enum TimerType {
  TimerNone,
  TimerHeartbeat,
  TimerChangeEffect,
  TimerBroadcastEffect,
};
// This is used to define the length of the array of timers - update it if
// adding new enum values.
#define TIMER_TYPE_LAST TimerBroadcastEffect

/**
 * The data passed to a state when an event occurs.
 */
struct RadioEventData {
  /** The packet that was received, or null if no packet was received. */
  RadioPacket *packet;

  /** The timer that expired, or none if no timer expired. */
  TimerType timerExpired;
};

/**
 * The possible states for this state machine.
 */
enum class RadioState {
  Slave,
  Master,
};

class RadioStateMachine {
 public:
  RadioStateMachine(NetworkManager *networkManager);

  RadioState GetCurrentState();

  /** Runs the state machine and updates the LEDS. Should be called frequently.
   */
  void Tick();

  /** Runs the radio state machine. Typically you want to use just Tick. */
  void RadioTick();

  // Network properties - these are used to coordinate effectrs

  /** Returns the synchronized milliseconds time from the network. */
  uint32_t GetNetworkMillis();

  /** Returns the index of the current effect. */
  uint8_t GetEffectIndex();

  /** Returns the most recent SetEffect packet. */
  RadioPacket *const GetSetEffect();

  /** Sets and broadcasts the current effect. */
  void SetEffect(RadioPacket *const setEffect);

  // See comment on numPalettes and numEffects below.
  void SetNumPalettes(uint8_t numPalettes);
  void SetNumEffects(uint8_t numEffects);

  // Tuning constants. Public for testing.
  /**
   * If a slave doesn't receive a heartbeat for this long, it'll become master.
   */
  static const uint32_t kSlaveNoPacketTimeout = 5000;

  /**
   * The maximum random delay added to kSlaveNoPacketTimeout.
   */
  static const uint32_t kSlaveNoPacketRandom = 2000;

  /**
   * When master, send a heartbeat this often.
   */
  static const uint32_t kMasterHeartbeatInterval = 1000;

  /** When master, change the effect this often. */
  static const uint32_t kChangeEffectInterval = 60000;

  /** When master, re-broadcast the current effect this often. */
  static const uint32_t kBroadcastEffectInterval = 2000;

 private:
  // Handler functions
  void handleSlaveEvent(RadioEventData &data);
  void handleMasterEvent(RadioEventData &data);

  // These are called when a state is entered.
  void beginSlave();
  void beginMaster();

  // Support functions
  /** Sets a timer to go off delay millis from now. */
  void SetTimer(TimerType timer, uint32_t delay);

  /** Whether a time expired, or None. */
  TimerType TimerExpired();

  /** Performs master election based on the received heartbeat. */
  void PerformMasterElection(RadioPacket *receivedPacket);

  /** Sends a heartbeat packet. */
  void SendHeartbeat();

  NetworkManager *const networkManager;

  /** The current state. */
  RadioState state;

  /** If not equal to state, the next state. */
  RadioState nextState;

  uint32_t timers[TIMER_TYPE_LAST + 1];

  /** The milliseconds when the heartbeat timer expires and fires an event. */
  uint32_t heartbeatTimerExpiresAt = 0;

  /** The milliseconds when the effect timer expires and fires an event. */
  uint32_t effectTimerExpiresAt = 0;

  /**
   * The last time we saw the effect change. This is so that we can preserve the
   * cadence of changing effects, even if the master changes.
   */
  uint32_t effectChangeSeenAt = 0;

  /**
   * The offset between the local millis() and the network time, such that
   * millis + millisOffset = network time.
   */
  int32_t millisOffset = 0;

  /** The current effect index. */
  int8_t effectIndex = 0;

  // The state machine needs to know how many effects and palettes there are, so
  // that it can set them in the SetEffect packets. These are set by LedManager
  // calling the corresponding setters.
  uint8_t numPalettes = 1;
  uint8_t numEffects = 1;

  RadioPacket packet;
  RadioPacket setEffectPacket;
};

#endif
