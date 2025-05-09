#ifndef __RADIO_STATE_MACHINE_H__
#define __RADIO_STATE_MACHINE_H__

#include <Types.hpp>
#include <array>
#include <map>

#include "NetworkManager.hpp"

enum TimerType {
  TimerNone,
  TimerHeartbeat,
  TimerChangeEffect,
  TimerBroadcastEffect,
};
// This is used to define the length of the array of timers - update it if
// adding new enum values.
constexpr TimerType TIMER_TYPE_LAST = TimerBroadcastEffect;

/**
 * The data passed to a state when an event occurs.
 */
struct RadioEventData {
  /** The packet that was received, or null if no packet was received. */
  RadioPacket *packet;

  /** The timer that expired, or none if no timer expired. */
  TimerType timer_expired;
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
  explicit RadioStateMachine(NetworkManager *network_manager);
  ~RadioStateMachine();

  RadioState GetCurrentState() const;

  /** Runs the state machine and updates the LEDS. Should be called frequently.
   */
  void Tick();

  /** Runs the radio state machine. Typically you want to use just Tick. */
  void RadioTick();

  // Network properties - these are used to coordinate effects

  /** Returns the synchronized milliseconds time from the network. */
  uint32_t GetNetworkMillis() const;

  /** Returns the index of the current effect. */
  uint8_t GetEffectIndex() const;

  /** Returns the most recent SetEffect packet. */
  RadioPacket *GetSetEffect();

  /** Sets and broadcasts the current effect. */
  void SetEffect(RadioPacket *const setEffect);

  // See comment on num_palettes and num_effects below.
  void SetNumPalettes(uint8_t num_palettes);
  void SetNumEffects(uint8_t num_effects);

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
  TimerType TimerExpired() const;

  /** Performs master election based on the received heartbeat. */
  void PerformMasterElection(RadioPacket *received_packet);

  /** Sends a heartbeat packet. */
  void SendHeartbeat();

  NetworkManager *const network_manager_;

  /** The current state. */
  RadioState state_;

  /** If not equal to state, the next state. */
  RadioState next_state_;

  std::array<uint32_t, TIMER_TYPE_LAST + 1> timers_;

  /**
   * The last time we saw the effect change. This is so that we can preserve the
   * cadence of changing effects, even if the master changes.
   */
  uint32_t effect_change_seen_at_ = 0;

  /**
   * The offset between the local millis() and the network time, such that
   * millis + millis_offset = network time.
   */
  int32_t millis_offset_ = 0;

  /** The current effect index. */
  int8_t effect_index_ = 0;

  // The state machine needs to know how many effects and palettes there are, so
  // that it can set them in the SetEffect packets. These are set by LedManager
  // calling the corresponding setters.
  uint8_t num_palettes_ = 1;
  uint8_t num_effects_ = 1;

  RadioPacket packet_;
  RadioPacket set_effect_packet_;

  bool tick_run_ = false;
};

#endif
