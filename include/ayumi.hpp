#ifndef LOD_AYUMI_HPP_
#define LOD_AYUMI_HPP_

#include "oglwrap_config.hpp"
#include "oglwrap/glew.hpp"
#include "oglwrap/oglwrap.hpp"

#include "../engine/gameobject.hpp"
#include "../engine/mesh/animated_mesh_renderer.hpp"

#include "charmove.hpp"
#include "skybox.hpp"
#include "shadow.hpp"

extern const float GRAVITY;
/* 0 -> max quality
   2 -> max performance */
extern const int PERFORMANCE;

class Ayumi : public engine::GameObject {
  engine::AnimatedMeshRenderer mesh_;
  engine::Animation anim_;
  oglwrap::Program prog_, shadow_prog_;

  oglwrap::LazyUniform<glm::mat4> uProjectionMatrix_, uCameraMatrix_, uModelMatrix_, uBones_, uShadowCP_;
  oglwrap::LazyUniform<glm::mat4> shadow_uMCP_, shadow_uBones_;
  oglwrap::LazyUniform<glm::vec4> uSunData_;
  oglwrap::LazyUniform<int> uNumUsedShadowMaps_, uShadowSoftness_;

  bool attack2_, attack3_;

  CharacterMovement *charmove_;

  Skybox& skybox_;
  Shadow& shadow_;

public:
  Ayumi(Skybox& skybox, Shadow& shadow);
  engine::AnimatedMeshRenderer& getMesh();
  engine::Animation& getAnimation();
  void screenResized(const glm::mat4& projMat, GLuint, GLuint) override;
  void update(float time) override;
  void shadowRender(float time, const engine::Camera& cam) override;
  void render(float time, const engine::Camera& cam) override;
  void charmove(CharacterMovement& charmove) {
    charmove_ = &charmove;
    charmove_->setCanJumpCallback(std::bind(&Ayumi::canJump, this));
    charmove_->setCanFlipCallback(std::bind(&Ayumi::canFlip, this));
  }

private: // Callbacks
  CharacterMovement::CanDoCallback canJump;
  CharacterMovement::CanDoCallback canFlip;
  engine::Animation::AnimationEndedCallback animationEndedCallback;
};

#endif // LOD_AYUMI_HPP_
