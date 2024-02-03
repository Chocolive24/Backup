#include "pbr_scene.h"
#include "engine.h"
#include "file_utility.h"

#include <iostream>

// NOTE:
// PBR requires all inputs to be linear. (HDR obligatory + gamma correction 
// at the last fragment pass).

void PBR_Scene::Begin() {
  pbr_pipeline_.Begin("data/shaders/transform.vert",
                      "data/shaders/pbr/pbr.frag");

  textured_pbr_pipeline_.Begin("data/shaders/pbr/textured_pbr.vert",
                               "data/shaders/pbr/textured_pbr.frag");

  equirect_to_cubemap_pipe_.Begin("data/shaders/local_transform.vert",
                                  "data/shaders/hdr/equirectangular_to_cubemap.frag");

  cubemap_pipeline_.Begin("data/shaders/hdr/hdr_cubemap.vert",
                          "data/shaders/hdr/hdr_cubemap.frag");

  irradiance_pipeline_.Begin("data/shaders/local_transform.vert",
                             "data/shaders/pbr/irradiance_convultion.frag");

  prefilter_pipeline_.Begin("data/shaders/local_transform.vert",
                            "data/shaders/pbr/prefilter.frag");

  brdf_pipeline_.Begin("data/shaders/pbr/brdf.vert",
                       "data/shaders/pbr/brdf.frag");

  hdr_pipeline_.Begin("data/shaders/screen_texture.vert",
                      "data/shaders/hdr/hdr.frag");

  current_pipeline_ = &pbr_pipeline_;

  sphere_.CreateSphere();
  cube_.CreateCube();
  cubemap_mesh_.CreateCubeMap();
  screen_quad_.CreateScreenQuad();

  CreateHDR_Cubemap();
  CreateIrradianceCubeMap();
  CreatePrefilterCubeMap();
  CreateBRDF_LUT_Texture();

  // Important to call glViewport with the screen dimension after the creation
  // of the different IBL pre-computed textures.
  const auto screen_size = Engine::window_size();
  glViewport(0, 0, screen_size.x, screen_size.y);

  const auto albedo_map =
      LoadTexture("data/textures/pbr/rusted_iron/albedo.png", GL_CLAMP_TO_EDGE,
                  GL_LINEAR, true, false);

  const auto normal_map =
      LoadTexture("data/textures/pbr/rusted_iron/normal.png", GL_CLAMP_TO_EDGE,
                  GL_LINEAR, false, false);

  const auto metallic_map =
      LoadTexture("data/textures/pbr/rusted_iron/metallic.png",
                  GL_CLAMP_TO_EDGE, GL_LINEAR, true, false);

  const auto roughness_map =
      LoadTexture("data/textures/pbr/rusted_iron/roughness.png",
                  GL_CLAMP_TO_EDGE, GL_LINEAR, true, false);

  const auto ao_map =
      LoadTexture("data/textures/pbr/rusted_iron/ao.png", GL_CLAMP_TO_EDGE,
                  GL_LINEAR, false, false);

  rusted_iron_mat_.Create(albedo_map, normal_map, metallic_map, roughness_map,
                          ao_map);

  pbr_pipeline_.Bind();
  pbr_pipeline_.SetInt("irradianceMap", 0);
  pbr_pipeline_.SetInt("prefilterMap", 1);
  pbr_pipeline_.SetInt("brdfLUT", 2);

  pbr_pipeline_.SetVec3("material.albedo", glm::vec3(0.5f, 0.0f, 0.0f));
  pbr_pipeline_.SetFloat("material.ao", 1.0f);

  textured_pbr_pipeline_.Bind();
  textured_pbr_pipeline_.SetInt("irradianceMap", 0);
  textured_pbr_pipeline_.SetInt("prefilterMap", 1);
  textured_pbr_pipeline_.SetInt("brdfLUT", 2);
  textured_pbr_pipeline_.SetInt("material.albedo_map", 3);
  textured_pbr_pipeline_.SetInt("material.normal_map", 4);
  textured_pbr_pipeline_.SetInt("material.metallic_map", 5);
  textured_pbr_pipeline_.SetInt("material.roughness_map", 6);
  textured_pbr_pipeline_.SetInt("material.ao_map", 7);

  cubemap_pipeline_.Bind();
  cubemap_pipeline_.SetInt("environmentMap", 0);

  // Generate HDR framebuffer
  glGenFramebuffers(1, &hdr_fbo_);
  glGenTextures(1, &hdr_color_buffer_);

  // Bind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);

  // Bind the texture
  glBindTexture(GL_TEXTURE_2D, hdr_color_buffer_);

  // Specify texture storage and parameters
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_size.x, screen_size.y, 0,
               GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Attach texture to framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         hdr_color_buffer_, 0);

  // Create depth buffer (renderbuffer)
  glGenRenderbuffers(1, &depth_rbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screen_size.x,
                        screen_size.y);

  // Attach renderbuffer to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depth_rbo_);

  // Check framebuffer completeness
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Framebuffer not complete!" << std::endl;
  }

  // Unbind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void PBR_Scene::End() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  pbr_pipeline_.End();
  textured_pbr_pipeline_.End();
  equirect_to_cubemap_pipe_.End();
  cubemap_pipeline_.End();
  irradiance_pipeline_.End();
  prefilter_pipeline_.End();
  brdf_pipeline_.End();
  hdr_pipeline_.End();
  current_pipeline_ = nullptr;

  camera_.End();
  backpack_.Destroy();
  sphere_.Destroy();
  cube_.Destroy();
  cubemap_mesh_.Destroy();
  screen_quad_.Destroy();

  glDeleteTextures(1, &env_cubemap_);
  glDeleteTextures(1, &irradiance_cubemap_);
  glDeleteTextures(1, &prefilter_cubemap_);
  glDeleteTextures(1, &brdf_lut_);

  rusted_iron_mat_.Destroy();

  glDeleteTextures(1, &equirectangular_map_);
}

void PBR_Scene::Update(float dt) {
  static float time = 0.f;
  time += dt;

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = camera_.CalculateProjectionMatrix(Engine::window_aspect());

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  current_pipeline_->Bind();

  current_pipeline_->SetMatrix4("transform.projection", projection_);
  current_pipeline_->SetMatrix4("transform.view", view_);

  current_pipeline_->SetVec3("viewPos", camera_.position());

  // Bind pre-computed IBL data.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_cubemap_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, brdf_lut_);

  // Bind material.
  rusted_iron_mat_.Bind(GL_TEXTURE3);

  // Render light source (simply re-render sphere at light positions)
  // this looks a bit off as we use the same shader, but it'll make their
  // positions obvious and keeps the codeprint small.
  for (unsigned int i = 0; i < kLightCount; i++) {
    current_pipeline_->SetVec3("lightPositions[" + std::to_string(i) + "]", light_positions[i]);
    current_pipeline_->SetVec3("lightColors[" + std::to_string(i) + "]",
                          light_colors[i]);

    //model_ = glm::mat4(1.0f);
    //model_ = glm::translate(model_, light_positions[i]);
    //model_ = glm::scale(model_, glm::vec3(0.5f));
    //current_pipeline_->SetMatrix4("model", model_);
    //current_pipeline_->SetMatrix4(
    //    "normalMatrix",
    //    glm::mat4(glm::transpose(glm::inverse(glm::mat3(model_)))));

    //current_pipeline_->DrawMesh(sphere_, GL_TRIANGLE_STRIP);
  }

  // Render rows*column number of spheres with varying metallic/roughness values
  // scaled by rows and columns respectively
  for (int row = 0; row < kRowCount; row++) {
    current_pipeline_->SetFloat("material.metallic", static_cast<float>(row) / 
                                                static_cast<float>(kRowCount));
    for (int col = 0; col < kColumnCount; ++col) {
      // we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces
      // (roughness of 0.0) tend to look a bit off on direct lighting.
      current_pipeline_->SetFloat("material.roughness",
                      glm::clamp(static_cast<float>(col) / 
                                 static_cast<float>(kColumnCount), 0.05f, 1.0f));

      model_ = glm::mat4(1.0f);
      model_ = glm::translate(model_,
                              glm::vec3((col - (kColumnCount / 2)) * kSpacing,
                                        (row - (kRowCount / 2)) * kSpacing, 0.0f));
      current_pipeline_->SetMatrix4("transform.model", model_);
      current_pipeline_->SetMatrix4("normalMatrix",
                     glm::mat4(glm::transpose(glm::inverse(glm::mat3(model_)))));

      renderer_.DrawMesh(sphere_, GL_TRIANGLE_STRIP);
    }
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT); // We see the back faces of the cubemap, so cull front faces.

  // render skybox (render as last to prevent overdraw)
  cubemap_pipeline_.Bind();
  cubemap_pipeline_.SetMatrix4("transform.view", view_);
  cubemap_pipeline_.SetMatrix4("transform.projection", projection_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
  //glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_); // display irradiance map.
  //glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_cubemap_); // display prefilter map.
  renderer_.DrawMesh(cubemap_mesh_);
}

void PBR_Scene::OnEvent(const SDL_Event& event) { 
    camera_.OnEvent(event);

    switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_X:
          current_pipeline_ = &pbr_pipeline_;
          break;
        case SDL_SCANCODE_C:
          current_pipeline_ = &textured_pbr_pipeline_;
          break;
        default:
          break;
      }
      break;

    default:
      break;
    }
}

void PBR_Scene::CreateHDR_Cubemap() {
    const auto screen_size = Engine::window_size();

    glGenFramebuffers(1, &capture_fbo_);
    glGenRenderbuffers(1, &capture_rbo_);

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512,
                          512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, capture_rbo_);

    equirectangular_map_ = LoadHDR_Texture("data/textures/hdr/chinese_garden_2k.hdr",
                                           GL_CLAMP_TO_EDGE, GL_LINEAR);

    glGenTextures(1, &env_cubemap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
    for (GLuint i = 0; i < 6; i++) {
      // note that we store each face with 16 bit floating point values
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    equirect_to_cubemap_pipe_.Bind();
    equirect_to_cubemap_pipe_.SetInt("equirectangularMap", 0);
    equirect_to_cubemap_pipe_.SetMatrix4("transform.projection",
                                         capture_projection_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, equirectangular_map_);

    glViewport(0, 0, 512, 512);
    // don't forget to configure the viewport to the
    // capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    for (GLuint i = 0; i < 6; i++) {
      equirect_to_cubemap_pipe_.SetMatrix4("transform.view", capture_views_[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env_cubemap_, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      equirect_to_cubemap_pipe_.DrawMesh(cubemap_mesh_);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // then generate mipmaps
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void PBR_Scene::CreateIrradianceCubeMap() {
    constexpr int kResolution = 32;

    glGenTextures(1, &irradiance_cubemap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_);

    for (GLuint i = 0; i < 6; i++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, kResolution,
                 kResolution, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kResolution,
                          kResolution);

    irradiance_pipeline_.Bind();
    irradiance_pipeline_.SetInt("environmentMap", 0);
    irradiance_pipeline_.SetMatrix4("transform.projection", capture_projection_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);

    // don't forget to configure the viewport to the
    // capture dimensions.
    glViewport(0, 0, kResolution, kResolution);  

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    for (GLuint i = 0; i < 6; i++) {
      irradiance_pipeline_.SetMatrix4("transform.view", capture_views_[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                             irradiance_cubemap_, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      irradiance_pipeline_.DrawMesh(cubemap_mesh_);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PBR_Scene::CreatePrefilterCubeMap() {
  constexpr int kResolution = 128;

  glGenTextures(1, &prefilter_cubemap_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_cubemap_);

  for (GLuint i = 0; i < 6; i++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                   kResolution, kResolution, 0, GL_RGB, GL_FLOAT, nullptr);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  prefilter_pipeline_.Bind();
  prefilter_pipeline_.SetInt("environmentMap", 0);
  prefilter_pipeline_.SetMatrix4("transform.projection", capture_projection_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);

  glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);

  GLuint maxMipLevels = 5;
  for (GLuint mip = 0; mip < maxMipLevels; mip++) {
      // reisze framebuffer according to mip-level size.
      GLuint mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
      GLuint mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

      glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo_);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth,
                            mipHeight);
      glViewport(0, 0, mipWidth, mipHeight);

      float roughness = (float)mip / (float)(maxMipLevels - 1);
      prefilter_pipeline_.SetFloat("roughness", roughness);

      for (GLuint i = 0; i < 6; i++) {
        prefilter_pipeline_.SetMatrix4("transform.view", capture_views_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter_cubemap_,
                               mip);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        prefilter_pipeline_.DrawMesh(cubemap_mesh_);
      }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PBR_Scene::CreateBRDF_LUT_Texture() {
  glGenTextures(1, &brdf_lut_);

  // pre-allocate enough memory for the LUT texture.
  glBindTexture(GL_TEXTURE_2D, brdf_lut_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         brdf_lut_, 0);

  glViewport(0, 0, 512, 512);
  brdf_pipeline_.Bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
  brdf_pipeline_.DrawMesh(screen_quad_);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
