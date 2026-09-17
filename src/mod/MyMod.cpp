#include "mod/MyMod.h"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <filesystem>

namespace waypointmanager {

namespace {

constexpr std::array<const char *, 10> kPresetNames = {
    "Home",       "Base",       "Mine",       "Farm",      "Shop",
    "Portal",     "NetherBase", "EndPortal",  "WaypointA", "WaypointB"};

constexpr std::array<const char *, 3> kDimensionNames = {"Overworld", "Nether", "End"};

constexpr const char *ButtonId = "waypointmanager.wp_button";
constexpr const char *MainModuleId = "waypointmanager.main";
constexpr const char *StageModuleId = "waypointmanager.stage";
constexpr const char *ApplyModuleId = "waypointmanager.apply";

const char *presetName(int index) {
  return kPresetNames[static_cast<std::size_t>(index) % kPresetNames.size()];
}

const char *dimensionName(int index) {
  return kDimensionNames[static_cast<std::size_t>(index) % kDimensionNames.size()];
}

} // namespace

WaypointManagerMod &WaypointManagerMod::instance() {
  static WaypointManagerMod instance;
  return instance;
}

WaypointManagerMod::WaypointManagerMod() : mSelf(*ll::mod::NativeMod::current()) {}

bool WaypointManagerMod::load() {
  auto &self = getSelf();
  self.getLogger().debug("Loading...");

  std::error_code ec;
  std::filesystem::create_directories(self.getDataDir(), ec);
  if (ec) {
    self.getLogger().error("Failed to create data directory {}: {}", self.getDataDir().string(),
                            ec.message());
    return false;
  }
  std::filesystem::create_directories(self.getConfigDir(), ec);
  if (ec) {
    self.getLogger().error("Failed to create config directory {}: {}",
                            self.getConfigDir().string(), ec.message());
    return false;
  }

  mConfigFile.emplace();
  if (!mConfigFile->load()) {
    self.getLogger().warn("Failed to load typed config");
    return false;
  }
  mConfig = mConfigFile->value();

  self.getLogger().info("Loaded {} from {}", self.getName(), self.getModDir().string());
  return true;
}

bool WaypointManagerMod::enable() {
  auto &self = getSelf();
  self.getLogger().debug("Enabling...");

  if (!mConfig.enabled) {
    self.getLogger().info("Waypoint Manager is disabled by config");
    return true;
  }

  bool ok = true;
  ok = registerMainModule() && ok;
  ok = registerStageModule() && ok;
  ok = registerApplyModule() && ok;
  ok = registerWaypointButton() && ok;
  return ok;
}

bool WaypointManagerMod::disable() {
  auto &self = getSelf();
  self.getLogger().debug("Disabling...");

  if (mButtonRegistered) {
    pl::modmenu::unregisterButton(ButtonId);
    mButtonRegistered = false;
  }
  if (mApplyModuleRegistered) {
    pl::modmenu::unregisterModule(ApplyModuleId);
    mApplyModuleRegistered = false;
  }
  if (mStageModuleRegistered) {
    pl::modmenu::unregisterModule(StageModuleId);
    mStageModuleRegistered = false;
  }
  if (mMainModuleRegistered) {
    pl::modmenu::unregisterModule(MainModuleId);
    mMainModuleRegistered = false;
  }
  return true;
}

bool WaypointManagerMod::unload() {
  getSelf().getLogger().debug("Unloading...");
  mConfigFile.reset();
  return true;
}

bool WaypointManagerMod::registerWaypointButton() {
  bool ok = pl::modmenu::ButtonBuilder(ButtonId, "Waypoints")
                .modId(getSelf().getId())
                .moduleId(MainModuleId)
                .label("WP")
                .androidKeyCode(0)
                .behavior(pl::modmenu::ButtonBehavior::Click)
                .onEvent([this](std::string_view buttonId, pl::modmenu::ButtonEvent event,
                                float value) { onWaypointButtonEvent(buttonId, event, value); })
                .registerButton();

  if (ok) {
    getSelf().getLogger().info("Registered WAYPOINTS button ({})", ButtonId);
  } else {
    getSelf().getLogger().error("Failed to register WAYPOINTS button ({})", ButtonId);
  }
  mButtonRegistered = ok;
  return ok;
}

void WaypointManagerMod::onWaypointButtonEvent(std::string_view buttonId,
                                                pl::modmenu::ButtonEvent event, float value) {
  // Behavior::Click means every dispatched call already represents one
  // completed tap, so there's nothing to branch on here. Tapping WAYPOINTS
  // advances to the next visible waypoint - the closest real, useful action
  // available without a player-position API (see the chat reply notes).
  (void)buttonId;
  (void)event;
  (void)value;
  cycleActiveWaypoint();
}

bool WaypointManagerMod::registerMainModule() {
  bool ok = pl::modmenu::ModuleBuilder(MainModuleId, "Waypoint Manager")
                .modId(getSelf().getId())
                .description(buildWaypointListDescription())
                .defaultEnabled(mConfig.enabled)
                .onToggle([this](std::string_view moduleId, bool enabled) {
                  onMainModuleToggle(moduleId, enabled);
                })
                .config("maxVisible", "Max Visible Waypoints", pl::modmenu::ConfigType::SliderInt,
                        std::to_string(mConfig.maxVisible), "1", "16")
                .registerModule();

  if (ok) {
    getSelf().getLogger().info("Registered module {}", MainModuleId);
  } else {
    getSelf().getLogger().error("Failed to register module {}", MainModuleId);
  }
  mMainModuleRegistered = ok;
  return ok;
}

bool WaypointManagerMod::registerStageModule() {
  bool ok =
      pl::modmenu::ModuleBuilder(StageModuleId, "Waypoint Manager: Add / Edit")
          .modId(getSelf().getId())
          .description(
              "Set fields below, then flip 'Waypoint Manager: Apply' (either direction) to "
              "save. Slot Id = 0 creates a NEW waypoint; set it to an existing waypoint's id "
              "(shown on the Waypoint Manager entry) to edit that one instead.\n"
              "Preset names: 0=Home 1=Base 2=Mine 3=Farm 4=Shop 5=Portal 6=NetherBase "
              "7=EndPortal 8=WaypointA 9=WaypointB\n"
              "Dimensions: 0=Overworld 1=Nether 2=End\n"
              "No player-position API is available, so X/Y/Z must be entered manually.")
          .defaultEnabled(true)
          .config("stageSlotId", "Slot Id (0 = new)", pl::modmenu::ConfigType::SliderInt, "0",
                  "0", "64")
          .config("stageNameIndex", "Preset Name Index", pl::modmenu::ConfigType::SliderInt, "0",
                  "0", std::to_string(kPresetNames.size() - 1))
          .config("stageX", "X", pl::modmenu::ConfigType::SliderInt, "0", "-2000000", "2000000")
          .config("stageY", "Y", pl::modmenu::ConfigType::SliderInt, "64", "-64", "320")
          .config("stageZ", "Z", pl::modmenu::ConfigType::SliderInt, "0", "-2000000", "2000000")
          .config("stageDimensionIndex", "Dimension Index", pl::modmenu::ConfigType::SliderInt,
                  "0", "0", "2")
          .config("stageVisible", "Visible(1)/Hidden(0)", pl::modmenu::ConfigType::SliderInt, "1",
                  "0", "1")
          .config("stageDeleteId", "Delete Id (0 = none)", pl::modmenu::ConfigType::SliderInt,
                  "0", "0", "64")
          .registerModule();

  if (ok) {
    getSelf().getLogger().info("Registered module {}", StageModuleId);
  } else {
    getSelf().getLogger().error("Failed to register module {}", StageModuleId);
  }
  mStageModuleRegistered = ok;
  return ok;
}

bool WaypointManagerMod::registerApplyModule() {
  bool ok = pl::modmenu::ModuleBuilder(ApplyModuleId, "Waypoint Manager: Apply")
                .modId(getSelf().getId())
                .description(
                    "Flip this switch (either direction) to apply whatever is staged on the "
                    "'Waypoint Manager: Add / Edit' entry - create, edit, or delete.")
                .defaultEnabled(false)
                .onToggle([this](std::string_view moduleId, bool enabled) {
                  onApplyModuleToggle(moduleId, enabled);
                })
                .registerModule();

  if (ok) {
    getSelf().getLogger().info("Registered module {}", ApplyModuleId);
  } else {
    getSelf().getLogger().error("Failed to register module {}", ApplyModuleId);
  }
  mApplyModuleRegistered = ok;
  return ok;
}

void WaypointManagerMod::onMainModuleToggle(std::string_view moduleId, bool enabled) {
  (void)moduleId;
  if (!mConfigFile) return;
  mConfigFile->load();
  mConfig = mConfigFile->value();
  mConfig.enabled = enabled;
  mConfigFile->value() = mConfig;
  mConfigFile->save();
  getSelf().getLogger().info("Waypoint Manager module toggled: {}", enabled);
}

void WaypointManagerMod::onApplyModuleToggle(std::string_view moduleId, bool enabled) {
  (void)moduleId;
  (void)enabled; // Both directions of this switch mean "apply now".
  applyStagedAction();
}

void WaypointManagerMod::rebuildMainModule() {
  if (mMainModuleRegistered) {
    pl::modmenu::unregisterModule(MainModuleId);
    mMainModuleRegistered = false;
  }
  registerMainModule();
}

Waypoint *WaypointManagerMod::findWaypointById(int id) {
  for (auto &wp : mConfig.waypoints) {
    if (wp.id == id) return &wp;
  }
  return nullptr;
}

void WaypointManagerMod::cycleActiveWaypoint() {
  if (!mConfigFile) return;
  mConfigFile->load();
  mConfig = mConfigFile->value();

  if (mConfig.waypoints.empty()) {
    getSelf().getLogger().info("No waypoints yet - add one from the Mod Menu screen first.");
    return;
  }

  std::vector<int> visibleIds;
  for (auto &wp : mConfig.waypoints) {
    if (wp.visible) visibleIds.push_back(wp.id);
  }
  if (visibleIds.empty()) {
    getSelf().getLogger().info("All waypoints are hidden - nothing to select.");
    return;
  }

  auto it = std::find(visibleIds.begin(), visibleIds.end(), mConfig.activeWaypointId);
  int nextId = (it == visibleIds.end() || std::next(it) == visibleIds.end())
                   ? visibleIds.front()
                   : *std::next(it);
  mConfig.activeWaypointId = nextId;

  mConfigFile->value() = mConfig;
  mConfigFile->save();

  if (auto *active = findWaypointById(nextId)) {
    getSelf().getLogger().info("Active waypoint -> {} ({}, {}, {}) [{}]", presetName(active->nameIndex),
                                active->x, active->y, active->z, dimensionName(active->dimensionIndex));
  }

  rebuildMainModule();
}

void WaypointManagerMod::applyStagedAction() {
  if (!mConfigFile) return;
  mConfigFile->load();
  mConfig = mConfigFile->value();

  if (mConfig.stageDeleteId > 0) {
    auto &wps = mConfig.waypoints;
    auto before = wps.size();
    wps.erase(std::remove_if(wps.begin(), wps.end(),
                              [&](const Waypoint &w) { return w.id == mConfig.stageDeleteId; }),
              wps.end());
    if (wps.size() != before) {
      getSelf().getLogger().info("Deleted waypoint id {}", mConfig.stageDeleteId);
      if (mConfig.activeWaypointId == mConfig.stageDeleteId) mConfig.activeWaypointId = 0;
    } else {
      getSelf().getLogger().warn("No waypoint with id {} to delete", mConfig.stageDeleteId);
    }
    mConfig.stageDeleteId = 0;
  } else {
    Waypoint *target = nullptr;
    if (mConfig.stageSlotId > 0) {
      target = findWaypointById(mConfig.stageSlotId);
    }
    if (!target) {
      Waypoint fresh;
      fresh.id = mConfig.nextWaypointId++;
      mConfig.waypoints.push_back(fresh);
      target = &mConfig.waypoints.back();
      getSelf().getLogger().info("Created waypoint id {}", target->id);
    } else {
      getSelf().getLogger().info("Updated waypoint id {}", target->id);
    }
    target->nameIndex = mConfig.stageNameIndex;
    target->x = mConfig.stageX;
    target->y = mConfig.stageY;
    target->z = mConfig.stageZ;
    target->dimensionIndex = mConfig.stageDimensionIndex;
    target->visible = mConfig.stageVisible != 0;

    mConfig.stageSlotId = 0; // avoid re-editing the same entry on the next Apply tap
  }

  mConfigFile->value() = mConfig;
  mConfigFile->save();
  rebuildMainModule();
}

std::string WaypointManagerMod::buildWaypointListDescription() const {
  std::string text =
      "Distance/direction are not shown: this SDK build has no confirmed player-position "
      "API. Manage waypoints from the 'Add / Edit' and 'Apply' entries below.\n\n";

  text += fmt::format("Waypoints: {} total (showing up to {}), active id = {}\n\n",
                       mConfig.waypoints.size(), mConfig.maxVisible, mConfig.activeWaypointId);

  if (mConfig.waypoints.empty()) {
    text += "(no waypoints yet)\n";
  }

  int shown = 0;
  for (const auto &wp : mConfig.waypoints) {
    if (shown >= mConfig.maxVisible) {
      text += "... more hidden, raise Max Visible Waypoints above\n";
      break;
    }
    text += fmt::format("{} id={} {}  x={} y={} z={}  {}{}\n",
                         wp.id == mConfig.activeWaypointId ? "*" : " ", wp.id, presetName(wp.nameIndex),
                         wp.x, wp.y, wp.z, dimensionName(wp.dimensionIndex),
                         wp.visible ? "" : " (hidden)");
    ++shown;
  }

  return text;
}

} // namespace waypointmanager
