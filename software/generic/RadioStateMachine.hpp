#ifndef __RADIO_STATE_MACHINE_H__
#define __RADIO_STATE_MACHINE_H__

#include "NetworkManager.hpp"
#include "Types.hpp"

/**
 * The data passed to a state when an event occurs.
 */
struct RadioEventData {
  /** The packet that was received, or null if no packet was received. */
  RadioPacket *packet;

  /** Whether the heartbeat timer expired. */
  bool heartbeatTimerExpired;

  /** When master, this is used to occasionally update the current effect. */
  bool effectTimerExpired;
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
  static const uint32_t kSetEffectInterval = 30000;

 private:
  // Handler functions
  void handleSlaveEvent(RadioEventData &data);
  void handleMasterEvent(RadioEventData &data);

  // These are called when a state is entered.
  void beginSlave();
  void beginMaster();

  // Support functions
  /** Sets an event to fire delay milliseconds from now.  */
  void SetHeartbeatTimer(uint32_t delay);

  /** Sets the effect event to fire delay milliseconds from now.  */
  void SetEffectTimer(uint32_t delay);

  /** Performs master election based on the received heartbeat. */
  void PerformMasterElection(RadioPacket *receivedPacket);

  /** Sends a heartbeat packet. */
  void SendHeartbeat();

  NetworkManager *const networkManager;

  /** The current state. */
  RadioState state;

  /** If not equal to state, the next state. */
  RadioState nextState;

  /** The milliseconds when the heartbeat timer expires and fires an event. */
  uint32_t heartbeatTimerExpiresAt = 0;

  /** The milliseconds when the effect timer expires and fires an event. */
  uint32_t effectTimerExpiresAt = 0;

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
