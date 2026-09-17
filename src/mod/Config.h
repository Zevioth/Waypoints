#pragma once

#include <vector>

namespace waypointmanager {

// One saved waypoint. Names and dimensions are stored as indexes into fixed
// preset lists (see kPresetNames / kDimensionNames in MyMod.cpp) instead of
// free text, because the Mod Menu config surface I could verify only supports
// numeric sliders (see the notes in the chat reply for why).
struct Waypoint {
  int id = 0;
  int nameIndex = 0;
  int x = 0;
  int y = 64;
  int z = 0;
  int dimensionIndex = 0; // 0 = Overworld, 1 = Nether, 2 = End
  bool visible = true;
};

struct ModConfig {
  int version = 1;
  bool enabled = true;

  int maxVisible = 8;

  std::vector<Waypoint> waypoints;
  int nextWaypointId = 1;
  int activeWaypointId = 0; // 0 = none selected

  // Staging fields for the "Waypoint Manager: Add / Edit" Mod Menu entry.
  // Field names here must match the ".config(name, ...)" key strings used
  // when registering that module in MyMod.cpp.
  int stageSlotId = 0; // 0 = create new; existing waypoint id = edit that one
  int stageNameIndex = 0;
  int stageX = 0;
  int stageY = 64;
  int stageZ = 0;
  int stageDimensionIndex = 0;
  int stageVisible = 1; // 1 = visible, 0 = hidden
  int stageDeleteId = 0; // >0 and Apply flipped = delete that waypoint id
};

} // namespace waypointmanager
