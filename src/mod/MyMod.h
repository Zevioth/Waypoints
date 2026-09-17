#pragma once

#include "mod/Config.h"

#include <pl/Config.hpp>
#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace waypointmanager {

class WaypointManagerMod {
public:
  static WaypointManagerMod &instance();

  WaypointManagerMod(const WaypointManagerMod &) = delete;
  WaypointManagerMod &operator=(const WaypointManagerMod &) = delete;

  bool load();
  bool enable();
  bool disable();
  bool unload();

  [[nodiscard]] ll::mod::NativeMod &getSelf() const { return mSelf; }

private:
  WaypointManagerMod();

  bool registerWaypointButton();
  bool registerMainModule();
  bool registerStageModule();
  bool registerApplyModule();

  void onWaypointButtonEvent(std::string_view buttonId, pl::modmenu::ButtonEvent event, float value);
  void onMainModuleToggle(std::string_view moduleId, bool enabled);
  void onApplyModuleToggle(std::string_view moduleId, bool enabled);

  void rebuildMainModule();
  void cycleActiveWaypoint();
  void applyStagedAction();
  Waypoint *findWaypointById(int id);
  [[nodiscard]] std::string buildWaypointListDescription() const;

  ll::mod::NativeMod &mSelf;
  std::optional<pl::config::ConfigFile<ModConfig>> mConfigFile;
  ModConfig mConfig;

  bool mButtonRegistered = false;
  bool mMainModuleRegistered = false;
  bool mStageModuleRegistered = false;
  bool mApplyModuleRegistered = false;
};

} // namespace waypointmanager
