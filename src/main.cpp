#include "mod/MyMod.h"

#include <pl/Mod.hpp>

// PL_REGISTER_MOD binds one long-lived C++ lifecycle object to the mod loader.
// This is the confirmed registration mechanism used by every LeviLaunchroid
// C++ example (the "Full C++ Lifecycle Mod Example") - not a replacement or
// invented system.
PL_REGISTER_MOD(waypointmanager::WaypointManagerMod, waypointmanager::WaypointManagerMod::instance());
