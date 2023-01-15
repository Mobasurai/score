#pragma once
#include <ossia/detail/config.hpp>

#include <wobjectdefs.h>

#include <cinttypes>

namespace Process
{
class ProcessModel;
class ProcessModelFactory;
class LayerFactory;
class EffectFactory;

#define SCORE_FLAG(i) (1 << i)
/**
 * @brief Various settings for processes.
 */
enum ProcessFlags : int64_t
{
  //! Can be loaded as a process of an interval
  SupportsTemporal = SCORE_FLAG(0),

  //! The process won't change if the parent duration changes (eg it's a filter)
  TimeIndependent = SCORE_FLAG(1),

  //! Can be loaded in a state
  SupportsState = SCORE_FLAG(2),

  //! Action from the user required upon creation
  RequiresCustomData = SCORE_FLAG(3),

  //! When created in an interval, go on the top slot or in a new slot
  PutInNewSlot = SCORE_FLAG(4),

  //! The presenter / view already handles rendering when the model loops.
  HandlesLooping = SCORE_FLAG(5),

  //! The process supports being exposed to the ControlSurface
  ControlSurface = SCORE_FLAG(6),

  //! The process's item handles all the decoration (won't be title, etc)
  FullyCustomItem = SCORE_FLAG(7),

  //! The process supports adding new controls (for audio plug-ins, LV2 etc):
  //! this will show the UI in the inspector
  CanCreateControls = SCORE_FLAG(8),

  //! The process is currently creating new controls (the runtime state changed by the user)
  CreateControls = SCORE_FLAG(9),

  //! The process is snapshottable
  Snapshottable = SCORE_FLAG(10),

  //! The process is recordable
  Recordable = SCORE_FLAG(11),

  SupportsLasting = SupportsTemporal | TimeIndependent,
  ExternalEffect
  = SupportsTemporal | TimeIndependent | RequiresCustomData | ControlSurface,
  SupportsAll = SupportsTemporal | TimeIndependent | SupportsState
};

constexpr ProcessFlags operator|(ProcessFlags a, ProcessFlags b) noexcept
{
  return ProcessFlags((int64_t)a | (int64_t)b);
}

constexpr ProcessFlags operator|=(ProcessFlags& a, ProcessFlags b) noexcept
{
  return a = a | b;
}

/**
 * \class ProcessFlags_k
 * \brief Metadata to retrieve the ProcessFlags of a process
 */
class ProcessFlags_k;

enum NetworkFlags : int8_t
{
  //! Set: compensated / unset: uncompensated
  Uncompensated = 0,
  Compensated = SCORE_FLAG(0),

  //! Set: sync / unset: async
  Async = 0,
  Sync = SCORE_FLAG(1),

  //! 00: Free
  //! 01: Mixed
  //! 10: Mixed(?)
  //! 11: Fully shared
  Free = 0,
  Mixed = SCORE_FLAG(2),
  Shared = SCORE_FLAG(2) | SCORE_FLAG(3),
};

constexpr NetworkFlags operator|(NetworkFlags a, NetworkFlags b) noexcept
{
  return NetworkFlags((int8_t)a | (int8_t)b);
}

constexpr NetworkFlags operator|=(NetworkFlags& a, NetworkFlags b) noexcept
{
  return a = a | b;
}
}

W_REGISTER_ARGTYPE(Process::NetworkFlags)
