#include "final_scene.h"
#include "engine.h"
#include "file_utility.h"

#include <iostream>
#include <random>

// TODO 02.02:
// leo_magnus dans la sc�ne.

// TODO
// faire des shadows maps.
// // si cascaded shadow map, faire un lerp entre la shadow map d'avant et celle actuel
// (si c'est pas la 1�re shadow map) -> ca permet des transitions moins trash.

void FinalScene::Begin() {
  CreatePipelines();

  CreateMeshes();
  CreateModels();
  CreateMaterials();

  CreateFrameBuffers();

  CreateHdrCubemap();
  CreateIrradianceCubeMap();
  CreatePrefilterCubeMap();
  CreateBrdfLut();

  // Important to call glViewport with the screen dimension after the creation
  // of the different IBL pre-computed textures.
  const auto screen_size = Engine::window_size();
  glViewport(0, 0, screen_size.x, screen_size.y);

  CreateSsaoData();

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void FinalScene::End() {
  DestroyPipelines();

  DestroyMeshes();
  DestroyModels();
  DestroyMaterials();

  DestroyIblPreComputedCubeMaps();

  DestroyFrameBuffers();

  camera_.End();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void FinalScene::Update(float dt) {
  const auto window_aspect = Engine::window_aspect();

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = camera_.CalculateProjectionMatrix(window_aspect);
  camera_frustum_ = camera_.CalculateFrustum(window_aspect);

  // Draw the geometry and color data in the G-Buffer.
  ApplyGeometryPass();

  // Calculate ambient occlusion based on the G-Buffer data.
  // Draw result in the SSAO frame buffers.
  ApplySsaoPass();

  // Depth only pass rendered from the directional light point of view.
  ApplyShadowMappingPass();

  // Calculate lighting per pixel on deferred shading based on the G-Buffer data.
  // Draw the result in the HDR color buffer.
  ApplyDeferredPbrLightingPass();

  // Draw the objects that are not impacted by light in front shading.
  // Draw the result in the HDR color buffer.
  ApplyFrontShadingPass();

  // Take the most brightness part of the scene to apply a bloom algorithm on it.
  // Draw the result in the bloom frame buffer.
  ApplyBloomPass();

  // Apply tone mapping and gamma correction to the HDR color buffer.
  // Draw the result in the default framebuffer.
  ApplyHdrPass();
}

void FinalScene::OnEvent(const SDL_Event& event) { 
  camera_.OnEvent(event);

  switch (event.type) {
  case SDL_KEYDOWN:
    switch (event.key.keysym.scancode) {
      case SDL_SCANCODE_X:
        break;
      case SDL_SCANCODE_C:
        break;
      default:
        break;
    }
    break;
  case SDL_WINDOWEVENT: {
    switch (event.window.event) {
      case SDL_WINDOWEVENT_RESIZED: {
        const auto new_size = glm::uvec2(event.window.data1, event.window.data2);
        g_buffer_.Resize(new_size);
        ssao_fbo_.Resize(new_size);
        ssao_blur_fbo_.Resize(new_size);
        hdr_fbo_.Resize(new_size);
        break;
      }
      default:
        break;
    }
    break;
  }
  default:
    break;
  }
}

void FinalScene::CreatePipelines() noexcept {
  equirect_to_cubemap_pipe_.Begin(
      "data/shaders/final_scene/transform/local_transform.vert",
      "data/shaders/final_scene/hdr/equirectangular_to_cubemap.frag");

  irradiance_pipeline_.Begin(
      "data/shaders/final_scene/transform/local_transform.vert",
      "data/shaders/final_scene/pbr/irradiance_convultion.frag");

  prefilter_pipeline_.Begin(
      "data/shaders/final_scene/transform/local_transform.vert",
      "data/shaders/final_scene/pbr/prefilter.frag");

  brdf_pipeline_.Begin("data/shaders/final_scene/pbr/brdf.vert",
                       "data/shaders/final_scene/pbr/brdf.frag");

  geometry_pipeline_.Begin("data/shaders/final_scene/pbr/pbr_g_buffer.vert",
                           "data/shaders/final_scene/pbr/pbr_g_buffer.frag");

  three_channels_geometry_pipe_.Begin("data/shaders/final_scene/pbr/pbr_g_buffer.vert",
                                      "data/shaders/final_scene/pbr/3_channels_pbr_g_buffer.frag");

  instanced_geometry_pipeline_.Begin("data/shaders/final_scene/pbr/instanced_pbr_g_buffer.vert",
                                     "data/shaders/final_scene/pbr/pbr_g_buffer.frag");

  ssao_pipeline_.Begin("data/shaders/final_scene/transform/screen_transform.vert",
                       "data/shaders/final_scene/ssao/ssao.frag");
  ssao_blur_pipeline_.Begin("data/shaders/final_scene/transform/screen_transform.vert",
                            "data/shaders/final_scene/ssao/ssao_blur.frag");

  shadow_mapping_pipe_.Begin("data/shaders/final_scene/shadow/simple_depth.vert",
                             "data/shaders/final_scene/shadow/simple_depth.frag");

  instanced_shadow_mapping_pipe_.Begin("data/shaders/final_scene/shadow/instanced_simple_depth.vert",
                                       "data/shaders/final_scene/shadow/simple_depth.frag");

  pbr_lighting_pipeline_.Begin("data/shaders/final_scene/transform/screen_transform.vert",
                               "data/shaders/final_scene/pbr/deferred_pbr.frag");

  debug_lights_pipeline_.Begin("data/shaders/final_scene/transform/transform.vert",
                               "data/shaders/final_scene/visual_debug/light_debug.frag");

  cubemap_pipeline_.Begin("data/shaders/final_scene/hdr/hdr_cubemap.vert",
                          "data/shaders/final_scene/hdr/hdr_cubemap.frag");
  
  down_sample_pipeline_.Begin("data/shaders/final_scene/transform/screen_transform.vert",
                              "data/shaders/final_scene/bloom/down_sample.frag");
  up_sample_pipeline_.Begin("data/shaders/final_scene/transform/screen_transform.vert",
                            "data/shaders/final_scene/bloom/up_sample.frag");

  bloom_hdr_pipeline_.Begin("data/shaders/final_scene/transform/screen_transform.vert",
                      "data/shaders/final_scene/hdr/hdr.frag");

  // Setup the Sampler2D uniforms.
  // -----------------------------
  geometry_pipeline_.Bind();
  geometry_pipeline_.SetInt("material.albedo_map", 0);
  geometry_pipeline_.SetInt("material.normal_map", 1);
  geometry_pipeline_.SetInt("material.metallic_map", 2);
  geometry_pipeline_.SetInt("material.roughness_map", 3);
  geometry_pipeline_.SetInt("material.ao_map", 4);

  three_channels_geometry_pipe_.Bind();
  three_channels_geometry_pipe_.SetInt("material.albedo_map", 0);
  three_channels_geometry_pipe_.SetInt("material.normal_map", 1);
  three_channels_geometry_pipe_.SetInt("material.ao_metallic_roughness_map", 2);

  instanced_geometry_pipeline_.Bind();
  instanced_geometry_pipeline_.SetInt("material.albedo_map", 0);
  instanced_geometry_pipeline_.SetInt("material.normal_map", 1);
  instanced_geometry_pipeline_.SetInt("material.metallic_map", 2);
  instanced_geometry_pipeline_.SetInt("material.roughness_map", 3);
  instanced_geometry_pipeline_.SetInt("material.ao_map", 4);

  ssao_pipeline_.Bind();
  ssao_pipeline_.SetInt("gViewPositionMetallic", 0);
  ssao_pipeline_.SetInt("gViewNormalRoughness", 1);
  ssao_pipeline_.SetInt("texNoise", 2);
  ssao_pipeline_.SetFloat("radius", kSsaoRadius);
  ssao_pipeline_.SetFloat("biais", kSsaoBiais);

  ssao_blur_pipeline_.Bind();
  ssao_blur_pipeline_.SetInt("ssaoInput", 0);

  pbr_lighting_pipeline_.Bind();
  pbr_lighting_pipeline_.SetInt("irradianceMap", 0);
  pbr_lighting_pipeline_.SetInt("prefilterMap", 1);
  pbr_lighting_pipeline_.SetInt("brdfLUT", 2);
  pbr_lighting_pipeline_.SetInt("gViewPositionMetallic", 3);
  pbr_lighting_pipeline_.SetInt("gViewNormalRoughness", 4);
  pbr_lighting_pipeline_.SetInt("gAlbedoAmbientOcclusion", 5);
  pbr_lighting_pipeline_.SetInt("ssao", 6);
  pbr_lighting_pipeline_.SetInt("shadowMap", 7);
  pbr_lighting_pipeline_.SetFloat("combined_ao_factor", kCombiendAoFactor);

  cubemap_pipeline_.Bind();
  cubemap_pipeline_.SetInt("environmentMap", 0);

  bloom_hdr_pipeline_.Bind();
  bloom_hdr_pipeline_.SetInt("hdrBuffer", 0);
  bloom_hdr_pipeline_.SetInt("bloomBlur", 1);
}

void FinalScene::CreateHdrCubemap() noexcept {
    const auto screen_size = Engine::window_size();

    const auto equirectangular_map = LoadHDR_Texture("data/textures/hdr/chinese_garden_4k.hdr",
                                                     GL_CLAMP_TO_EDGE, GL_LINEAR);

    glGenTextures(1, &env_cubemap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
    for (GLuint i = 0; i < 6; i++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                 kSkyboxResolution, kSkyboxResolution, 0, GL_RGB, GL_FLOAT, NULL);
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
    glBindTexture(GL_TEXTURE_2D, equirectangular_map);

    capture_fbo_.Bind();
    capture_fbo_.Resize(glm::uvec2(kSkyboxResolution));
    glViewport(0, 0, kSkyboxResolution, kSkyboxResolution);


    for (GLuint i = 0; i < 6; i++) {
      equirect_to_cubemap_pipe_.SetMatrix4("transform.view", capture_views_[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env_cubemap_, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      renderer_.DrawMesh(cubemap_mesh_);
    }

    capture_fbo_.UnBind();

    // Then generate mipmaps.
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glDeleteTextures(1, &equirectangular_map);
}

void FinalScene::CreateIrradianceCubeMap() noexcept {
  glGenTextures(1, &irradiance_cubemap_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_);

  for (GLuint i = 0; i < 6; i++) {
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
              kIrradianceMapResolution, kIrradianceMapResolution, 0,
              GL_RGB, GL_FLOAT, NULL);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  capture_fbo_.Bind();
  capture_fbo_.Resize(glm::uvec2(kIrradianceMapResolution));

  irradiance_pipeline_.Bind();
  irradiance_pipeline_.SetInt("environmentMap", 0);
  irradiance_pipeline_.SetMatrix4("transform.projection", capture_projection_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);

  glViewport(0, 0, kIrradianceMapResolution, kIrradianceMapResolution);  
  capture_fbo_.Bind();

  for (GLuint i = 0; i < 6; i++) {
    irradiance_pipeline_.SetMatrix4("transform.view", capture_views_[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                           irradiance_cubemap_, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    renderer_.DrawMesh(cubemap_mesh_);
  }

  capture_fbo_.UnBind();
}

void FinalScene::CreatePrefilterCubeMap() noexcept {
  glGenTextures(1, &prefilter_cubemap_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_cubemap_);

  for (GLuint i = 0; i < 6; i++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                   kPrefilterMapResolution, kPrefilterMapResolution, 0, GL_RGB,
                   GL_FLOAT, NULL);
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

  capture_fbo_.Bind();

  GLuint maxMipLevels = 5;
  for (GLuint mip = 0; mip < maxMipLevels; mip++) {
      // reisze framebuffer according to mip-level size.
      GLuint mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
      GLuint mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

      capture_fbo_.Resize(glm::uvec2(mipWidth, mipHeight));
      glViewport(0, 0, mipWidth, mipHeight);

      float roughness = (float)mip / (float)(maxMipLevels - 1);
      prefilter_pipeline_.SetFloat("roughness", roughness);

      for (GLuint i = 0; i < 6; i++) {
        prefilter_pipeline_.SetMatrix4("transform.view", capture_views_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter_cubemap_,
                               mip);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        renderer_.DrawMesh(cubemap_mesh_);
      }
  }

  capture_fbo_.UnBind();
}

void FinalScene::CreateBrdfLut() noexcept {
  glGenTextures(1, &brdf_lut_);

  // pre-allocate enough memory for the LUT texture.
  glBindTexture(GL_TEXTURE_2D, brdf_lut_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, kBrdfLutResolution,
               kBrdfLutResolution, 0, GL_RG, GL_FLOAT, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  capture_fbo_.Bind();
  capture_fbo_.Resize(glm::uvec2(kBrdfLutResolution));
  glViewport(0, 0, kBrdfLutResolution, kBrdfLutResolution);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         brdf_lut_, 0);

  brdf_pipeline_.Bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
  renderer_.DrawMesh(screen_quad_);

  capture_fbo_.UnBind();
}

void FinalScene::CreateFrameBuffers() noexcept {
  const auto screen_size = Engine::window_size();

  // IBL capture texture data framebuffer.
  // -------------------------------------
  DepthStencilAttachment capture_depth_stencil_attach(GL_DEPTH_COMPONENT24,
                                                      GL_DEPTH_ATTACHMENT);
  FrameBufferSpecification capture_specification;
  capture_specification.SetSize(glm::uvec2(kSkyboxResolution, kSkyboxResolution));
  capture_specification.SetDepthStencilAttachment(capture_depth_stencil_attach);
  // Color attachments are apart of the framebuffer in order to send them
  // easily to the pbr shaders.
  capture_fbo_.Create(capture_specification);
  capture_fbo_.Bind();
  // Configure a basic color attachment.
  constexpr GLenum buf = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(static_cast<GLsizei>(1), &buf);
  capture_fbo_.UnBind();

  // Configure G-Buffer Framebuffer.
  // -------------------------------
  ColorAttachment g_pos_metallic_attachment(GL_RGBA16F, GL_RGBA, GL_NEAREST,
                                            GL_CLAMP_TO_EDGE);
  ColorAttachment g_normal_roughness_attachment(GL_RGBA16F, GL_RGBA, GL_NEAREST,
                                                GL_CLAMP_TO_EDGE);
  ColorAttachment g_albedo_ao_attachment(GL_RGBA, GL_RGBA, GL_NEAREST,
                                         GL_CLAMP_TO_EDGE);

  DepthStencilAttachment g_depth_stencil_attachment(GL_DEPTH_COMPONENT24,
                                                    GL_DEPTH_ATTACHMENT);

  FrameBufferSpecification g_buffer_specification;
  g_buffer_specification.SetSize(screen_size);
  g_buffer_specification.PushColorAttachment(g_pos_metallic_attachment);
  g_buffer_specification.PushColorAttachment(g_normal_roughness_attachment);
  g_buffer_specification.PushColorAttachment(g_albedo_ao_attachment);
  g_buffer_specification.SetDepthStencilAttachment(g_depth_stencil_attachment);

  g_buffer_.Create(g_buffer_specification);

  // SSAO framebuffers.
  // ------------------
  ColorAttachment ssao_color_attach(GL_RED, GL_RED, GL_NEAREST,
                                    GL_CLAMP_TO_EDGE);
  FrameBufferSpecification ssao_specification;
  ssao_specification.SetSize(screen_size);
  ssao_specification.PushColorAttachment(ssao_color_attach);

  ssao_fbo_.Create(ssao_specification);

  ColorAttachment ssao_blur_color_attach(GL_RED, GL_RED, GL_NEAREST,
                                         GL_CLAMP_TO_EDGE);
  FrameBufferSpecification ssao_blur_specification;
  ssao_blur_specification.SetSize(screen_size);
  ssao_blur_specification.PushColorAttachment(ssao_blur_color_attach);

  ssao_blur_fbo_.Create(ssao_blur_specification);

  // Shadow map Framebuffer.
  // -----------------------
  glGenFramebuffers(1, &shadow_map_fbo_);
  // create depth texture
  glGenTextures(1, &shadow_map_);
  glBindTexture(GL_TEXTURE_2D, shadow_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, kShadowMapWidth_,
               kShadowMapHeight_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  constexpr std::array<float, 4> border_colors_ = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                   border_colors_.data());

  // attach depth texture as FBO's depth buffer
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         shadow_map_, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Bloom Framebuffer.
  // ------------------
  bool status = bloom_fbo_.Init(screen_size.x, screen_size.y, kBloomMipsCount_);
  if (!status) {
      std::cerr << "Failed to initialize bloom FBO - cannot create bloom renderer!\n";
  }

  // HDR framebuffer.
  // ----------------

  // 1st attachment stores all pixels on screen.
  ColorAttachment hdr_color_attachment(GL_RGBA16F, GL_RGBA, GL_LINEAR,
                                       GL_CLAMP_TO_EDGE);
  // 2nd attachment stores all bright pixels (pixels with a value greater than 1.0).
  ColorAttachment bright_color_attachment(GL_RGBA16F, GL_RGBA, GL_LINEAR,
                                          GL_CLAMP_TO_EDGE);
  DepthStencilAttachment hdr_depth_stencil_attachment(GL_DEPTH_COMPONENT24,
                                                      GL_DEPTH_ATTACHMENT);
  FrameBufferSpecification hdr_specification;
  hdr_specification.SetSize(screen_size);
  hdr_specification.PushColorAttachment(hdr_color_attachment);
  hdr_specification.PushColorAttachment(bright_color_attachment);
  hdr_specification.SetDepthStencilAttachment(hdr_depth_stencil_attachment);

  hdr_fbo_.Create(hdr_specification);
}

void FinalScene::CreateSsaoData() noexcept {
  // generate sample kernel
  // ----------------------
  std::uniform_real_distribution<GLfloat> random_floats(0.0, 1.0);
  // generates random floats between 0.0 and 1.0
  std::default_random_engine generator;

  for (GLuint i = 0; i < kSsaoKernelSampleCount_; i++) {
      glm::vec3 sample(random_floats(generator) * 2.0 - 1.0,
                       random_floats(generator) * 2.0 - 1.0,
                       random_floats(generator));
      sample = glm::normalize(sample);
      sample *= random_floats(generator);
      float scale = float(i) / kSsaoKernelSampleCount_;

      // scale samples s.t. they're more aligned to center of kernel
      // scale = lerp(0.1f, 1.0f, scale * scale);
      scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
      sample *= scale;
      ssao_kernel_[i] = sample;
  }

  static constexpr auto kSsaoNoiseDimensionXY =
      kSsaoNoiseDimensionX_ * kSsaoNoiseDimensionY_;

  std::array<glm::vec3, kSsaoNoiseDimensionXY> ssao_noise{};
  for (GLuint i = 0; i < kSsaoNoiseDimensionXY; i++) {
      // As the sample kernel is oriented along the positive z direction in
      // tangent space, we leave the z component at 0.0 so we rotate around the
      // z axis.
      glm::vec3 noise(random_floats(generator) * 2.0 - 1.0,
                      random_floats(generator) * 2.0 - 1.0, 0.0f);
      ssao_noise[i] = noise;
  }

  glGenTextures(1, &noise_texture_);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, kSsaoNoiseDimensionX_,
               kSsaoNoiseDimensionX_, 0, GL_RGB, GL_FLOAT, ssao_noise.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void FinalScene::CreateMeshes() noexcept {
  sphere_.CreateSphere();

  sphere_model_matrices_.reserve(kSphereCount_);
  visible_sphere_model_matrices_.reserve(kSphereCount_);

  for (int row = 0; row < kRowCount_; row++) {
    for (int col = 0; col < kColumnCount_; col++) {
      glm::mat4 model(1.0f);
      model = glm::translate(model, 
                             glm::vec3((col - (kColumnCount_ / 2)) * kSpacing_,
                             (row - (kRowCount_ / 2)) * kSpacing_, 0.0f));

      sphere_model_matrices_.push_back(model);
    }
  }

  sphere_.SetupModelMatrixBuffer(sphere_model_matrices_.data(),
                                 sphere_model_matrices_.size(), GL_DYNAMIC_DRAW);

  // Generate bounding sphere volume to test intersection with the camera frustum.
  sphere_.GenerateBoundingSphere(); 

  cube_.CreateCube();
  cube_.GenerateBoundingSphere();

  cubemap_mesh_.CreateCubeMap();
  screen_quad_.CreateScreenQuad();
}

void FinalScene::CreateModels() noexcept {
  leo_magnus_.Load("data/models/leo_magnus/leo_magnus.obj", true, false);
  sword_.Load("data/models/leo_magnus/sword.obj", true, false);
}

void FinalScene::CreateMaterials() noexcept {
  const auto albedo_map =
      LoadTexture("data/textures/pbr/rusted_iron/albedo.png", GL_CLAMP_TO_EDGE,
                  GL_LINEAR, true, false);

  const auto normal_map =
      LoadTexture("data/textures/pbr/rusted_iron/normal.png", GL_CLAMP_TO_EDGE,
                  GL_LINEAR, false, false);

  const auto metallic_map =
      LoadTexture("data/textures/pbr/rusted_iron/metallic.png",
                  GL_CLAMP_TO_EDGE, GL_LINEAR, false, false);

  const auto roughness_map =
      LoadTexture("data/textures/pbr/rusted_iron/roughness.png",
                  GL_CLAMP_TO_EDGE, GL_LINEAR, false, false);

  const auto ao_map = LoadTexture("data/textures/pbr/rusted_iron/ao.png",
                                  GL_CLAMP_TO_EDGE, GL_LINEAR, false, false);

  rusted_iron_mat_.Create(albedo_map, normal_map, metallic_map, roughness_map,
                          ao_map);

  const auto bamboo_albedo_map =
      LoadTexture("data/textures/pbr/bamboo/bamboo_albedo.png", GL_CLAMP_TO_EDGE,
                  GL_LINEAR, true, false);

  const auto bamboo_normal_map =
      LoadTexture("data/textures/pbr/bamboo/bamboo_normal.png", GL_CLAMP_TO_EDGE,
                  GL_LINEAR, false, false);

  const auto bamboo_metallic_map =
      LoadTexture("data/textures/pbr/bamboo/bamboo_metallic.png",
                  GL_CLAMP_TO_EDGE, GL_LINEAR, false, false);

  const auto bamboo_roughness_map =
      LoadTexture("data/textures/pbr/bamboo/bamboo_roughness.png",
                  GL_CLAMP_TO_EDGE, GL_LINEAR, false, false);

  const auto bamboo_ao_map =
      LoadTexture("data/textures/pbr/bamboo/bamboo_ao.png",
                                  GL_CLAMP_TO_EDGE, GL_LINEAR, false, false);

  bambo_mat.Create(bamboo_albedo_map, bamboo_normal_map,
                          bamboo_metallic_map, bamboo_roughness_map,
                          bamboo_ao_map);

  leo_magnus_textures_.resize(15, 0);

  // Low grosse armure.
  leo_magnus_textures_[0] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_grosse_armure_BaseColor.png", true, false);
  leo_magnus_textures_[1] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_grosse_armure_Normal.png", false, false);
  leo_magnus_textures_[2] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_grosse_armure_OcclusionRoughnessMetallic.png", false, false);

  // Low cape.
  leo_magnus_textures_[3] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_cape_BaseColor.png", true, false);
  leo_magnus_textures_[4] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_cape_Normal.png", false, false);
  leo_magnus_textures_[5] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_cape_OcclusionRoughnessMetallic.png", false, false);

  // Low tete.
  leo_magnus_textures_[6] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_tete_BaseColor.png", true,
      false);
  leo_magnus_textures_[7] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_tete_Normal.png", false,
      false);
  leo_magnus_textures_[8] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_tete_OcclusionRoughnessMetallic.png",
      false, false);

  // Low pilosit�.
  leo_magnus_textures_[9] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_pilosite_BaseColor.png", true,
      false);
  leo_magnus_textures_[10] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_pilosite_Normal.png", false,
      false);
  leo_magnus_textures_[11] = LoadTexture(
      "data/models/leo_magnus/"
      "leo_magnus_low_pilosite_OcclusionRoughnessMetallic.png",
      false, false);

  // Low petite armure.
  leo_magnus_textures_[12] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_petite_armure_BaseColor.png", true,
      false);
  leo_magnus_textures_[13] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_petite_armure_Normal.png", false,
      false);
  leo_magnus_textures_[14] = LoadTexture(
      "data/models/leo_magnus/leo_magnus_low_petite_armure_OcclusionRoughnessMetallic.png",
      false, false);

  sword_textures_.resize(3, 0);

  sword_textures_[0] =
      LoadTexture("data/models/leo_magnus/epee_low_1001_BaseColor.png", true, false);
  sword_textures_[1] = LoadTexture(
      "data/models/leo_magnus/epee_low_1001_Normal.png", false, false);
  sword_textures_[2] = LoadTexture(
      "data/models/leo_magnus/epee_low_1001_OcclusionRoughnessMetallic.png", false, false);
}

void FinalScene::ApplyGeometryPass() noexcept {
  // Render all geometric/color data to g-buffer
  g_buffer_.Bind();
  glClearColor(0.0, 0.0, 0.0, 1.0); // keep it black so it doesn't leak into g-buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // Draw single meshes.
  // -------------------
  geometry_pipeline_.Bind();

  geometry_pipeline_.SetMatrix4("transform.projection", projection_);
  geometry_pipeline_.SetMatrix4("transform.view", view_);

  DrawObjectGeometry(GeometryPipelineType::kDeferredShading);

  // Draw instanced meshes.
  // ----------------------
  instanced_geometry_pipeline_.Bind();

  instanced_geometry_pipeline_.SetMatrix4("transform.projection", projection_);
  instanced_geometry_pipeline_.SetMatrix4("transform.view", view_);

  DrawInstancedObjectGeometry(GeometryPipelineType::kDeferredShading);

  g_buffer_.UnBind();
}

void FinalScene::ApplySsaoPass() noexcept {
  const auto screen_size = Engine::window_size();
  // SSAO texture creation.
  // ----------------------
  ssao_fbo_.Bind();
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_CULL_FACE);
  ssao_pipeline_.Bind();
  // Send kernel + rotation
  for (unsigned int i = 0; i < kSsaoKernelSampleCount_; ++i) {
      ssao_pipeline_.SetVec3("samples[" + std::to_string(i) + "]",
                             ssao_kernel_[i]);
  }

  ssao_pipeline_.SetMatrix4("projection", projection_);
  ssao_pipeline_.SetVec2("noiseScale", glm::vec2(screen_size.x / kSsaoNoiseDimensionX_, 
                                                 screen_size.y / kSsaoNoiseDimensionY_));

  glActiveTexture(GL_TEXTURE0);
  g_buffer_.BindColorBuffer(0);  // position-metallic map.
  glActiveTexture(GL_TEXTURE1);
  g_buffer_.BindColorBuffer(1);  // normal-roughness map.
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);

  renderer_.DrawMesh(screen_quad_);

  // SSAO blur.
  // ----------
  ssao_blur_fbo_.Bind();
  glClear(GL_COLOR_BUFFER_BIT);
  ssao_blur_pipeline_.Bind();

  glActiveTexture(GL_TEXTURE0);
  ssao_fbo_.BindColorBuffer(0);

  renderer_.DrawMesh(screen_quad_);
}

void FinalScene::ApplyShadowMappingPass() noexcept {
  // render scene from light's point of view
  glViewport(0, 0, kShadowMapWidth_, kShadowMapHeight_);
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo_);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  light_dir_ = glm::normalize(glm::vec3(0) - light_pos_);

  glm::mat4 lightProjection, lightView;
  float near_plane = 1.0f, far_plane = 100.f;
  float width = 20.f, height = 20.f;
  lightProjection =
      glm::ortho(-width, width, -height, height, near_plane, far_plane);
  lightView = glm::lookAt(light_pos_, light_pos_ + light_dir_,
                          glm::vec3(0.0, 1.0, 0.0));
  light_space_matrix_ = lightProjection * lightView;

  // Draw single meshes.
  // -------------------
  shadow_mapping_pipe_.Bind();
  shadow_mapping_pipe_.SetMatrix4("lightSpaceMatrix", light_space_matrix_);

  DrawObjectGeometry(GeometryPipelineType::kShadowMapping);

  // Draw instanced meshes.
  // ----------------------
  instanced_shadow_mapping_pipe_.Bind();
  instanced_shadow_mapping_pipe_.SetMatrix4("lightSpaceMatrix", light_space_matrix_);

  DrawInstancedObjectGeometry(GeometryPipelineType::kShadowMapping);

  const auto window_size = Engine::window_size();
  glViewport(0, 0, window_size.x, window_size.y);
}

void FinalScene::ApplyDeferredPbrLightingPass() noexcept {
  // Draw the scene in the hdr framebuffer with PBR lighting.
  // --------------------------------------------------------
  hdr_fbo_.Bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  pbr_lighting_pipeline_.Bind();

  pbr_lighting_pipeline_.SetVec3("viewPos", camera_.position());
  pbr_lighting_pipeline_.SetMatrix4("inverseViewMatrix", glm::inverse(view_));

  // Bind pre-computed IBL data.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_cubemap_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, brdf_lut_);

  glActiveTexture(GL_TEXTURE3);
  g_buffer_.BindColorBuffer(0); // position-metallic map.
  glActiveTexture(GL_TEXTURE4);
  g_buffer_.BindColorBuffer(1); // normal-roughness map.
  glActiveTexture(GL_TEXTURE5);
  g_buffer_.BindColorBuffer(2); // albedo-ao map.
  glActiveTexture(GL_TEXTURE6);
  ssao_blur_fbo_.BindColorBuffer(0); // blured ssao map.

  glActiveTexture(GL_TEXTURE7);
  glBindTexture(GL_TEXTURE_2D, shadow_map_);

  const auto light_view_dir = glm::vec3(view_ * glm::vec4(light_dir_, 1.0f));
  pbr_lighting_pipeline_.SetVec3("directional_light.world_direction", light_dir_);
  pbr_lighting_pipeline_.SetVec3("directional_light.color", light_color);
  pbr_lighting_pipeline_.SetMatrix4("lightSpaceMatrix", light_space_matrix_);

  // Set light unfiorms.
  for (unsigned int i = 0; i < kLightCount; i++) {
    // Transform light positions in view space because of the SSAO which 
    // needs view space.
    //const auto light_view_pos = glm::vec3(view_ * glm::vec4(light_positions_[i], 1.0));

    pbr_lighting_pipeline_.SetVec3("lightPositions[" + std::to_string(i) + "]",
                                   light_positions_[i]);
    pbr_lighting_pipeline_.SetVec3("lightColors[" + std::to_string(i) + "]",
                                    light_colors_[i]);
  }

  glDisable(GL_CULL_FACE);
  renderer_.DrawMesh(screen_quad_);
}

void FinalScene::ApplyFrontShadingPass() noexcept {
  const auto screen_size = Engine::window_size();

  g_buffer_.BindRead();
  hdr_fbo_.BindDraw();
  glBlitFramebuffer(0, 0, screen_size.x, screen_size.y, 0, 0, screen_size.x,
                    screen_size.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  hdr_fbo_.Bind();

  debug_lights_pipeline_.Bind();

  debug_lights_pipeline_.SetMatrix4("transform.view", view_);
  debug_lights_pipeline_.SetMatrix4("transform.projection", projection_);

  for (unsigned int i = 0; i < kLightCount; i++) {
    model_ = glm::mat4(1.0f);
    model_ = glm::translate(model_, light_positions_[i]);
    model_ = glm::scale(model_, glm::vec3(0.5f));
    debug_lights_pipeline_.SetMatrix4("transform.model", model_);

    debug_lights_pipeline_.SetVec3("lightColor", light_colors_[i]);

    renderer_.DrawMesh(sphere_, GL_TRIANGLE_STRIP);
  }

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, light_pos_);
  model_ = glm::scale(model_, glm::vec3(1.f));
  debug_lights_pipeline_.SetMatrix4("transform.model", model_);

  debug_lights_pipeline_.SetVec3("lightColor", glm::vec3(1.f, 0.f, 0.f));

  renderer_.DrawMesh(sphere_, GL_TRIANGLE_STRIP);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);  // Cull skybox front faces because we are inside the cubemap.

  // Render skybox as last to prevent overdraw.
  cubemap_pipeline_.Bind();
  cubemap_pipeline_.SetMatrix4("transform.view", view_);
  cubemap_pipeline_.SetMatrix4("transform.projection", projection_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
  renderer_.DrawMesh(cubemap_mesh_);

  hdr_fbo_.UnBind();
}

void FinalScene::ApplyBloomPass() noexcept {
  const auto window_size = Engine::window_size();

  bloom_fbo_.Bind();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  down_sample_pipeline_.Bind();

  const auto& mip_chain = bloom_fbo_.mip_chain();

  down_sample_pipeline_.SetVec2("srcResolution", window_size);

  // Bind srcTexture (HDR color buffer) as initial texture input
  glActiveTexture(GL_TEXTURE0);
  hdr_fbo_.BindColorBuffer(1);

  // Progressively downsample through the mip chain.
  for (int i = 0; i < mip_chain.size(); i++) {
    const BloomMip& mip = mip_chain[i];

    glViewport(0, 0, mip.size.x, mip.size.y);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           mip.texture, 0);

    // Render screen-filled quad of resolution of current mip
    renderer_.DrawMesh(screen_quad_);

    // Set current mip resolution as srcResolution for next iteration
    down_sample_pipeline_.SetVec2("srcResolution", mip.size);
    // Set current mip as texture input for next iteration
    glBindTexture(GL_TEXTURE_2D, mip.texture);
  }

  up_sample_pipeline_.Bind();
  up_sample_pipeline_.SetFloat("filterRadius", kbloomFilterRadius_);

  // Enable additive blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);

  for (int i = mip_chain.size() - 1; i > 0; i--) {
    const BloomMip& mip = mip_chain[i];
    const BloomMip& nextMip = mip_chain[i - 1];

    // Bind viewport and texture from where to read
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mip.texture);

    // Set framebuffer render target (we write to this texture)
    glViewport(0, 0, nextMip.size.x, nextMip.size.y);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           nextMip.texture, 0);

    // Render screen-filled quad of resolution of current mip
    renderer_.DrawMesh(screen_quad_);
  }

  // Disable additive blending
  // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Restore if this was default
  glDisable(GL_BLEND);
}

void FinalScene::ApplyHdrPass() noexcept {
  const auto window_size = Engine::window_size();
  glViewport(0, 0, window_size.x, window_size.y);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glClear(GL_COLOR_BUFFER_BIT);

  bloom_hdr_pipeline_.Bind();

  glActiveTexture(GL_TEXTURE0);
  hdr_fbo_.BindColorBuffer(0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, bloom_fbo_.mip_chain()[0].texture);

  bloom_hdr_pipeline_.SetFloat("bloomStrength", kBloomStrength_);

  renderer_.DrawMesh(screen_quad_);
}

void FinalScene::DrawObjectGeometry(GeometryPipelineType geometry_type) noexcept {
  Pipeline* current_pipeline = nullptr;

  bool is_deferred_pipeline_ = false;

  switch (geometry_type) {
    case GeometryPipelineType::kDeferredShading:
      current_pipeline = &geometry_pipeline_;
      is_deferred_pipeline_ = true;
      break;
    case GeometryPipelineType::kShadowMapping:
      current_pipeline = &shadow_mapping_pipe_;
      is_deferred_pipeline_ = false;
      break;
    default:
      return;
  }

  bambo_mat.Bind(GL_TEXTURE0);

  // Render ground cube.
  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0, -10, 0));
  model_ = glm::scale(model_, glm::vec3(20.f, 0.5f, 20.f));

  if (is_deferred_pipeline_) {
    if (cube_.bounding_sphere().IsOnFrustum(camera_frustum_, model_)) {
      current_pipeline->SetMatrix4("transform.model", model_);

      current_pipeline->SetMatrix4("viewNormalMatrix",
          glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

      renderer_.DrawMesh(cube_);
    }
  } 
  else {
    current_pipeline->SetMatrix4("transform.model", model_);

    renderer_.DrawMesh(cube_);
  }

  if (is_deferred_pipeline_) {
    three_channels_geometry_pipe_.Bind();
    three_channels_geometry_pipe_.SetMatrix4("transform.view", view_);
    three_channels_geometry_pipe_.SetMatrix4("transform.projection",
                                             projection_);
    current_pipeline = &three_channels_geometry_pipe_;
  }

  // Render Leo Magnus.
  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0, -2.5f, 6));
  model_ = glm::scale(model_, glm::vec3(40.f));

  current_pipeline->SetMatrix4("transform.model", model_);
  
  current_pipeline->SetMatrix4("viewNormalMatrix",
      glm::mat4(glm::transpose(glm::inverse(view_ * model_))));
  
  renderer_.DrawModelWithMaterials(leo_magnus_, leo_magnus_textures_, 3);

  // Render Leo Magnus'sword.
  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0.875f, -2.5f, 7.725f));
  model_ = glm::scale(model_, glm::vec3(40.f));

  current_pipeline->SetMatrix4("transform.model", model_);

  current_pipeline->SetMatrix4("viewNormalMatrix",
      glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

  renderer_.DrawModelWithMaterials(sword_, sword_textures_, 0);

  current_pipeline = nullptr;
}

void FinalScene::DrawInstancedObjectGeometry(GeometryPipelineType geometry_type)
    noexcept {
  rusted_iron_mat_.Bind(GL_TEXTURE0);

  const auto is_deferred_pipeline =
      geometry_type == GeometryPipelineType::kDeferredShading;

  if (is_deferred_pipeline) {
    visible_sphere_model_matrices_.clear();

    for (const auto& sphere_model : sphere_model_matrices_) {
      if (sphere_.bounding_sphere().IsOnFrustum(camera_frustum_,
                                                sphere_model)) 
      {
         visible_sphere_model_matrices_.push_back(sphere_model);
      }
    }
  }

  const auto& buffer_data = is_deferred_pipeline 
                               ? visible_sphere_model_matrices_
                               : sphere_model_matrices_;

  sphere_.SetModelMatrixBufferSubData(buffer_data.data(), buffer_data.size());

  const auto sphere_count = is_deferred_pipeline
                                ? visible_sphere_model_matrices_.size()
                                : kSphereCount_;

  // Render spheres.
  renderer_.DrawInstancedMesh(sphere_, sphere_count, GL_TRIANGLE_STRIP);
}

void FinalScene::DestroyPipelines() noexcept {
  equirect_to_cubemap_pipe_.End();
  irradiance_pipeline_.End();
  prefilter_pipeline_.End();
  brdf_pipeline_.End();

  geometry_pipeline_.End();
  instanced_geometry_pipeline_.End();
  three_channels_geometry_pipe_.End();
  ssao_pipeline_.End();
  ssao_blur_pipeline_.End();
  shadow_mapping_pipe_.End();
  instanced_shadow_mapping_pipe_.End();

  pbr_lighting_pipeline_.End();
  debug_lights_pipeline_.End();
  cubemap_pipeline_.End();

  down_sample_pipeline_.End();
  up_sample_pipeline_.End();

  bloom_hdr_pipeline_.End();
}

void FinalScene::DestroyIblPreComputedCubeMaps() noexcept {
  glDeleteTextures(1, &env_cubemap_);
  glDeleteTextures(1, &irradiance_cubemap_);
  glDeleteTextures(1, &prefilter_cubemap_);
  glDeleteTextures(1, &brdf_lut_);
}

void FinalScene::DestroyFrameBuffers() noexcept {
  capture_fbo_.Destroy();
  g_buffer_.Destroy();
  ssao_fbo_.Destroy();
  ssao_blur_fbo_.Destroy();
  bloom_fbo_.Destroy();
  hdr_fbo_.Destroy();
}

void FinalScene::DestroyMeshes() noexcept {
  sphere_.Destroy();
  cube_.Destroy();
  cubemap_mesh_.Destroy();
  screen_quad_.Destroy();
}

void FinalScene::DestroyModels() noexcept { 
  leo_magnus_.Destroy();
  sword_.Destroy();
}

void FinalScene::DestroyMaterials() noexcept { 
  rusted_iron_mat_.Destroy();
  bambo_mat.Destroy();
}
