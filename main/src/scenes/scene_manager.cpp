#include "scene_manager.h"
#include "hello_triangle.h"
#include "hello_triangle_vbo.h"
#include "hello_quad.h"
#include "uniform_quad.h"
#include "texture_quad.h"
#include "phong_light.h"
#include "light_casters.h"
#include "multiple_lights.h"
#include "hello_model.h"
#include "outline_cube.h"
#include "mirror.h"
#include "blending.h"
#include "face_culling.h"
#include "cube_map.h"
#include "post_processing.h"
#include "normal_map.h"
#include "asteroids.h"
#include "bloom_scene.h"
#include "shadow_scene.h"
#include "cascaded_shadow_scene.h"
#include "deferred_shading_scene.h"
#include "pbr_scene.h"
#include "ssao_scene.h"
#include "pb_bloom_scene.h"
#include "final_scene.h"

void SceneManager::Begin() {
  scenes_[24] = std::make_unique<HelloTriangle>();
  scenes_[1] = std::make_unique<HelloTriangleVBO>();
  scenes_[2] = std::make_unique<HelloQuad>();
  scenes_[3] = std::make_unique<UniformQuad>();
  scenes_[4] = std::make_unique<TextureQuad>();
  scenes_[5] = std::make_unique<PhongLight>();
  scenes_[6] = std::make_unique<LightCasters>();
  scenes_[7] = std::make_unique<MultipleLights>();
  scenes_[8] = std::make_unique<HelloModel>();
  scenes_[9] = std::make_unique<OutlineCube>();
  scenes_[10] = std::make_unique<Mirror>();
  scenes_[11] = std::make_unique<Blending>();
  scenes_[12] = std::make_unique<FaceCulling>();
  scenes_[13] = std::make_unique<CubeMap>();
  scenes_[14] = std::make_unique<PostProcessing>();
  scenes_[15] = std::make_unique<NormalMap>();
  scenes_[16] = std::make_unique<Asteroids>();
  scenes_[17] = std::make_unique<BloomScene>();
  scenes_[18] = std::make_unique<ShadowScene>();
  scenes_[19] = std::make_unique<CascadedShadowScene>();
  scenes_[20] = std::make_unique<DeferredShadingScene>();
  scenes_[21] = std::make_unique<PBR_Scene>();
  scenes_[22] = std::make_unique<SSAO_Scene>();
  scenes_[23] = std::make_unique<PB_BloomScene>();
  scenes_[0] = std::make_unique<FinalScene>();

  scenes_[current_scene_idx_]->Begin();
}

void SceneManager::ChangeScene(int scene_index) {
  scenes_[current_scene_idx_]->End();
  const auto last_scene_idx = scenes_.size() - 1;
  if (scene_index < 0) {
    scene_index = last_scene_idx;
  }
  else if (scene_index > last_scene_idx) {
    scene_index = 0;
  }
  current_scene_idx_ = scene_index;
  scenes_[current_scene_idx_]->Begin();
}

void SceneManager::End() { 
  for (auto& scene : scenes_) {
    scene->End();
  }
}
