#pragma once

#include "scene.h"

#include <array>
#include <memory>

class SceneManager {
public:
  void Begin();
  void ChangeScene(int scene_index);
  void End();

  [[nodiscard]] Scene* current_scene() noexcept {
    return scenes_[current_scene_idx_].get();
  } 

  [[nodiscard]] int current_scene_idx() noexcept { return current_scene_idx_; }

private:
  std::array<std::unique_ptr<Scene>, 25> scenes_;
  int current_scene_idx_ = 0;
};