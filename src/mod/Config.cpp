#include "mod/Config.h"

#include <pl/Config.hpp>

namespace pl::config {

// Optional editor metadata (per docs/api/config.md). Purely cosmetic labels
// for the launcher's config editor - safe to lose if the exact FieldSchema
// member names differ slightly from what's shown below; nothing functional
// depends on this specialization compiling.
template <> struct Schema<waypointmanager::ModConfig> {
  static constexpr std::string_view title = "Waypoint Manager";
  static constexpr std::string_view description =
      "Persistent waypoint storage plus the staging fields used by the "
      "Waypoint Manager Mod Menu entries.";

  static constexpr FieldSchema field(std::string_view name) {
    if (name == "version") {
      return {.title = "Version", .readOnly = true};
    }
    if (name == "enabled") {
      return {.title = "Mod Enabled"};
    }
    if (name == "maxVisible") {
      return {.title = "Max Visible Waypoints", .minimum = 1, .maximum = 16};
    }
    if (name == "stageX" || name == "stageZ") {
      return {.title = "Staged Horizontal Coordinate",
              .description = "Manual entry only - no player-position API is "
                              "available in this SDK build."};
    }
    if (name == "stageY") {
      return {.title = "Staged Y Coordinate", .minimum = -64, .maximum = 320};
    }
    return {};
  }
};

} // namespace pl::config
