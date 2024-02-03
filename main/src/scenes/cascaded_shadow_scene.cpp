#include "cascaded_shadow_scene.h"
#include "engine.h"

#include <iostream>

void CascadedShadowScene::Begin() {
  ground_pipeline_.Begin("data/shaders/shadow/transform_cascaded_shadow.vert",
                        "data/shaders/shadow/cascaded_shadow.frag");
  backpack_pipeline_.Begin("data/shaders/shadow/tang_transform_casc_shadow.vert",
                        "data/shaders/shadow/tangent_cascaded_shadow.frag");
  hdr_pipeline_.Begin("data/shaders/screen_texture.vert",
                      "data/shaders/hdr/hdr.frag");
  shadow_mapping_pipe_.Begin("data/shaders/shadow/simple_depth.vert",
                            "data/shaders/shadow/simple_depth.frag");
  shadow_map_debug_pipe_.Begin("data/shaders/screen_texture.vert",
                               "data/shaders/shadow/depth_map_rendering.frag");

  constexpr bool hdr = true;

  cube_.CreateCube();
  screen_quad_.CreateScreenQuad();

  wood_map_ = LoadTexture("data/textures/wood.png", GL_CLAMP_TO_EDGE, 
                          GL_LINEAR, hdr);
  backpack_.Load("data/models/backpack/backpack.obj", hdr, false);

  const auto window_size = Engine::window_size();

  // Generate HDR framebuffer
  glGenFramebuffers(1, &hdr_fbo_);
  glGenTextures(1, &hdr_color_buffer_);

  // Bind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);

  // Create the two color buffers.
  // Bind the texture
  glBindTexture(GL_TEXTURE_2D, hdr_color_buffer_);
  
  // Specify texture storage and parameters
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_size.x, window_size.y, 0,
                  GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  // Attach texture to framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                        GL_TEXTURE_2D, hdr_color_buffer_, 0);

  // Create depth buffer (renderbuffer)
  glGenRenderbuffers(1, &depth_rbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_size.x,
                        window_size.y);

  // Attach renderbuffer to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depth_rbo_);

  // Check framebuffer completeness
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Framebuffer not complete!" << std::endl;
  }

  // Unbind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);


  // configure depth map FBO
  // -----------------------
  glGenFramebuffers(1, &depth_map_fbo_);
  // create depth texture
  glGenTextures(depth_maps_.size(), depth_maps_.data());

  for (int i = 0; i < depth_maps_.size(); i++) {
    glBindTexture(GL_TEXTURE_2D, depth_maps_[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, kShadowMapWidth_,
                 kShadowMapHeight_, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT,  NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    static constexpr std::array<float, 4> border_colors_ = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                     border_colors_.data());
  }

  // attach depth texture as FBO's depth buffer
  glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depth_maps_[0], 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glEnable(GL_DEPTH_TEST);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));
}

void CascadedShadowScene::End() {
  // Unload program/pipeline
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  Engine::set_clear_color(glm::vec3(0.f));

  camera_.End();

  ground_pipeline_.End();
  backpack_pipeline_.End();
  hdr_pipeline_.End();
  shadow_mapping_pipe_.End();
  shadow_map_debug_pipe_.End();

  backpack_.Destroy();
  cube_.Destroy();
  screen_quad_.Destroy();

  glDeleteTextures(1, &wood_map_);
}

void CascadedShadowScene::Update(float dt) {
  const auto window_size = Engine::window_size();
  const auto aspect = Engine::window_aspect();

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = camera_.CalculateProjectionMatrix(aspect);
  const auto& frustum = camera_.CalculateFrustum(aspect);
  const auto camera_near = camera_.near();
  const auto camera_far = camera_.far();

  // 1. render depth of scene to texture (from light's perspective)
  // --------------------------------------------------------------
  // render scene from light's point of view
  glViewport(0, 0, kShadowMapWidth_, kShadowMapHeight_);
  glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(shadow_map_cull_face);

  ShadowMappingPass(camera_near, camera_far);

  // Render scene with shadow calculation in HDR color buffer.
  glViewport(0, 0, window_size.x, window_size.y);
  glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  backpack_pipeline_.Bind();

  // Common vert shader uniforms.
  backpack_pipeline_.SetMatrix4("transform.view", view_);
  backpack_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(-2.0f, 1.75f, -3.0));
  model_ = glm::scale(model_, glm::vec3(1.f));
  backpack_pipeline_.SetMatrix4("transform.model", model_);
  backpack_pipeline_.SetMatrix4("normalMatrix",
                              glm::mat4(glm::transpose(glm::inverse(model_))));

  // Normal mapping uniforms.
  backpack_pipeline_.SetVec3("viewPos", camera_.position());
  backpack_pipeline_.SetVec3("lightDir", light_dir_);

  // Shadow uniforms.
  for (int i = 0; i < light_space_matrices_.size(); i++) {
    backpack_pipeline_.SetMatrix4(
        "lightSpaceMatrix[" + std::to_string(i) + "]", light_space_matrices_[i]);

    // Bind the shadow map.
    glActiveTexture(GL_TEXTURE3 + i);
    glBindTexture(GL_TEXTURE_2D, depth_maps_[i]);
    backpack_pipeline_.SetInt("shadowMap[" + std::to_string(i) + "].data", 3 + i);
  }
  
  backpack_pipeline_.SetFloat("maxNearCascade",
                                  camera_.far() * cascaded_near_ratio_);
  backpack_pipeline_.SetFloat("maxMiddleCascade",
                                  camera_.far() * cascaded_mid_ratio_);
  backpack_pipeline_.SetFloat("farPlane",
                                  camera_.far());

  backpack_pipeline_.SetBool("debug_color", debug_color);

  backpack_pipeline_.DrawModel(backpack_);

  ground_pipeline_.Bind();

  ground_pipeline_.SetMatrix4("transform.view", view_);
  ground_pipeline_.SetMatrix4("transform.projection", projection_);

  ground_pipeline_.SetVec3("viewPos", camera_.position());
  ground_pipeline_.SetVec3("lightDir", light_dir_);

  // Shadow uniforms.
  for (int i = 0; i < light_space_matrices_.size(); i++) {
    ground_pipeline_.SetMatrix4(
        "lightSpaceMatrix[" + std::to_string(i) + "]",
        light_space_matrices_[i]);

    // Bind the shadow map.
    glActiveTexture(GL_TEXTURE1 + i);
    glBindTexture(GL_TEXTURE_2D, depth_maps_[i]);
    ground_pipeline_.SetInt("shadowMap[" + std::to_string(i) + "].data", 1 + i);
  }

  ground_pipeline_.SetFloat("maxNearCascade",
      camera_.far() * cascaded_near_ratio_);
  ground_pipeline_.SetFloat("maxMiddleCascade",
      camera_.far() * cascaded_mid_ratio_);
  ground_pipeline_.SetFloat("farPlane", camera_.far());

  ground_pipeline_.SetBool("debug_color", debug_color);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, wood_map_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0.f, -0.5f, 0.f));
  model_ = glm::scale(model_, glm::vec3(25.f, 1.f, 25.f));
  model_ = glm::rotate(model_, glm::radians(90.f), glm::vec3(1, 0, 0));
  ground_pipeline_.SetMatrix4("transform.model", model_);

  ground_pipeline_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  ground_pipeline_.DrawMesh(cube_);

  // Cubes.
  // -----------------------------------------------

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0.0f, 1.5f, 0.0));
  ground_pipeline_.SetMatrix4("transform.model", model_);

  ground_pipeline_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  ground_pipeline_.DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(2.0f, 0.5f, 1.0));
  model_ = glm::scale(model_, glm::vec3(0.5f));
  ground_pipeline_.SetMatrix4("transform.model", model_);

  ground_pipeline_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  ground_pipeline_.DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(-1.0f, 1.0f, 2.0));
  model_ = glm::rotate(model_, glm::radians(60.0f),
                       glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
  model_ = glm::scale(model_, glm::vec3(0.5));
  ground_pipeline_.SetMatrix4("transform.model", model_);

  ground_pipeline_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  ground_pipeline_.DrawMesh(cube_);

  // Render the scene with HDR and gamma correction:
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  hdr_pipeline_.Bind();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, hdr_color_buffer_);

  hdr_pipeline_.SetFloat("exposure", 1.f);

  hdr_pipeline_.DrawMesh(screen_quad_);

  // render Depth map to quad for visual debugging
  // ---------------------------------------------

  if (debug_depth_map_) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    shadow_map_debug_pipe_.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth_maps_[debug_depth_map_idx_]);

    shadow_map_debug_pipe_.DrawMesh(screen_quad_);
  }
}

void CascadedShadowScene::ShadowMappingPass(const float& camera_near,
                                            const float& camera_far) {
  for (int i = 0; i < depth_maps_.size(); i++) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depth_maps_[i], 0);
    glClear(GL_DEPTH_BUFFER_BIT);

    float near_ratio = 0.f, far_ratio = 0.f;

    switch (i) {
      case 0:
        near_ratio = camera_near;
        far_ratio = camera_far * cascaded_near_ratio_;
        break;
      case 1:
        near_ratio = camera_far * cascaded_near_ratio_;
        far_ratio = camera_far * cascaded_mid_ratio_;
        break;
      case 2:
        near_ratio = camera_far * cascaded_mid_ratio_;
        far_ratio = camera_far;
        break;
    }

    glm::mat4 light_space_matrix =
        CalculateLightSpaceMatrix(near_ratio, far_ratio);
    light_space_matrices_[i] = light_space_matrix;

    shadow_mapping_pipe_.Bind();
    shadow_mapping_pipe_.SetMatrix4("lightSpaceMatrix", light_space_matrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wood_map_);

    model_ = glm::mat4(1.f);
    model_ = glm::translate(model_, glm::vec3(0.f, -0.5f, 0.f));
    model_ = glm::scale(model_, glm::vec3(25.f, 1.f, 25.f));
    model_ = glm::rotate(model_, glm::radians(90.f), glm::vec3(1, 0, 0));
    shadow_mapping_pipe_.SetMatrix4("transform.model", model_);

    shadow_mapping_pipe_.SetMatrix4(
        "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

    shadow_mapping_pipe_.DrawMesh(cube_);

    shadow_mapping_pipe_.SetMatrix4("transform.view", view_);
    shadow_mapping_pipe_.SetMatrix4("transform.projection", projection_);

    model_ = glm::mat4(1.f);
    model_ = glm::translate(model_, glm::vec3(-2.0f, 1.75f, -3.0));
    model_ = glm::scale(model_, glm::vec3(1.f));
    shadow_mapping_pipe_.SetMatrix4("transform.model", model_);
    shadow_mapping_pipe_.SetMatrix4(
        "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

    shadow_mapping_pipe_.DrawModel(backpack_);

    // Cubes.
    // -----------------------------------------------

    model_ = glm::mat4(1.f);
    model_ = glm::translate(model_, glm::vec3(0.0f, 1.5f, 0.0));
    shadow_mapping_pipe_.SetMatrix4("transform.model", model_);

    shadow_mapping_pipe_.SetMatrix4(
          "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

    shadow_mapping_pipe_.DrawMesh(cube_);

    model_ = glm::mat4(1.0f);
    model_ = glm::translate(model_, glm::vec3(2.0f, 0.5f, 1.0));
    model_ = glm::scale(model_, glm::vec3(0.5f));
    shadow_mapping_pipe_.SetMatrix4("transform.model", model_);

    shadow_mapping_pipe_.SetMatrix4(
          "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

    shadow_mapping_pipe_.DrawMesh(cube_);

    model_ = glm::mat4(1.0f);
    model_ = glm::translate(model_, glm::vec3(-1.0f, 1.0f, 2.0));
    model_ = glm::rotate(model_, glm::radians(60.0f),
                         glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model_ = glm::scale(model_, glm::vec3(0.5));
    shadow_mapping_pipe_.SetMatrix4("transform.model", model_);

    shadow_mapping_pipe_.SetMatrix4(
          "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

    shadow_mapping_pipe_.DrawMesh(cube_);
  }
}

glm::mat4 CascadedShadowScene::CalculateLightSpaceMatrix(float near,
                                                         float far) {
  const auto proj = glm::perspective(
      glm::radians(camera_.fov()), Engine::window_aspect(), near, far);

  const auto corners = GetFrustumCornersWorldSpace(proj, view_);

  glm::vec3 center = glm::vec3(0, 0, 0);
  for (const auto& v : corners) {
    center += glm::vec3(v);
  }
  center /= corners.size();

  const auto light_view =
      glm::lookAt(center -light_dir_, center,
                  glm::vec3(0.0f, 1.0f, 0.0f));

  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();
  float minZ = std::numeric_limits<float>::max();
  float maxZ = std::numeric_limits<float>::lowest();

  for (const auto& v : corners) {
    const auto trf = light_view * v;
    minX = std::min(minX, trf.x);
    maxX = std::max(maxX, trf.x);
    minY = std::min(minY, trf.y);
    maxY = std::max(maxY, trf.y);
    minZ = std::min(minZ, trf.z);
    maxZ = std::max(maxZ, trf.z);
  }

   float midLenX = (maxX - minX) / 2.0f;
   float midLenY = (maxY - minY) / 2.0f;
   float midLenZ = (maxZ - minZ) / 2.0f;
   
   const float midX = minX + midLenX;
   const float midY = minY + midLenY;
   const float midZ = minZ + midLenZ;
   
   constexpr float zMultiplier = 1.5f;
   constexpr float xyMargin = 1.1f;
   midLenX *= xyMargin;
   midLenY *= xyMargin;
   midLenZ *= zMultiplier;
   
   const glm::vec3 minLightProj{midX - midLenX, midY - midLenY, midZ - midLenZ};
   const glm::vec3 maxLightProj{midX + midLenX, midY + midLenY, midZ + midLenZ};

   const auto light_projection = glm::ortho(
        minLightProj.x,
		maxLightProj.x,
		minLightProj.y,
		maxLightProj.y,
		minLightProj.z,
		maxLightProj.z
   );

   // z_mult is a variable that pull back the near plan and push away the far
  // plane. We use it because geometry outside the frustum can possibly
  // generates shadows inside. Tune this parameter according to the scene.
  //constexpr float z_mult = 1.f;

  //if (minZ < 0) {
  //  minZ *= z_mult;
  //} else {
  //  minZ /= z_mult;
  //}

  //if (maxZ < 0) {
  //  maxZ /= z_mult;
  //} else {
  //  maxZ *= z_mult;
  //}

  //const auto light_projection = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);

   return light_projection * light_view;
}

std::vector<glm::vec4> CascadedShadowScene::GetFrustumCornersWorldSpace(
    const glm::mat4& proj, const glm::mat4& view) {
  const auto inv = glm::inverse(proj * view);

  std::vector<glm::vec4> frustumCorners;
  frustumCorners.reserve(8);
  for (unsigned int x = 0; x < 2; ++x) {
    for (unsigned int y = 0; y < 2; ++y) {
      for (unsigned int z = 0; z < 2; ++z) {
        const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f,
                                             2.0f * z - 1.0f, 1.0f);
        frustumCorners.push_back(pt / pt.w);
      }
    }
  }

  return frustumCorners;
}

void CascadedShadowScene::OnEvent(const SDL_Event& event) { 
    camera_.OnEvent(event);

    switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_E:
          debug_depth_map_ = !debug_depth_map_;
          break;
        case SDL_SCANCODE_R:
          debug_color = !debug_color;
          break;
        case SDL_SCANCODE_1:
          debug_depth_map_idx_ = 0;
          break;
        case SDL_SCANCODE_2:
          debug_depth_map_idx_ = 1;
          break;
        case SDL_SCANCODE_3:
          debug_depth_map_idx_ = 2;
          break;
        case SDL_SCANCODE_C:
          shadow_map_cull_face = shadow_map_cull_face == GL_BACK ? GL_FRONT : GL_BACK;
          break;
      }
      break;

    default:
      break;
    }
}
