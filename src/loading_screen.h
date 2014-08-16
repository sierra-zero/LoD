// Copyright (c) 2014, Tamas Csala

#ifndef LOD_LOADING_SCREEN_H_
#define LOD_LOADING_SCREEN_H_

#include "engine/oglwrap_config.h"
#include "oglwrap/shader.h"
#include "oglwrap/uniform.h"
#include "oglwrap/shapes/rectangle_shape.h"
#include "oglwrap/textures/texture2D.h"
#include "oglwrap/smart_enums.h"

#include "engine/game_object.h"

class LoadingScreen {
  gl::Texture2D tex_;
  gl::RectangleShape rect_;

  gl::Program prog_;

public:
  LoadingScreen()
      : rect_({gl::RectangleShape::kPosition, gl::RectangleShape::kTexCoord}) {
    gl::VertexShader vs("loading.vert");
    gl::FragmentShader fs("loading.frag");
    (prog_ << vs << fs).link();
    gl::Use(prog_);

    gl::BoundTexture2D tex{tex_};
    tex.loadTexture("textures/loading.png");
    tex.minFilter(gl::kLinear);
    tex.magFilter(gl::kLinear);

    gl::UniformSampler(prog_, "uTex").set(0);

    prog_.validate();

    (prog_ | "aPosition").bindLocation(rect_.kPosition);
    (prog_ | "aTexCoord").bindLocation(rect_.kTexCoord);
  }

  void render() {
    gl::Use(prog_);
    glActiveTexture(0);
    gl::BoundTexture2D tex{tex_};

    gl::TemporarySet capabilies{{{gl::kCullFace, false},
                                 {gl::kDepthTest, false}}};

    rect_.render();
  }
};

#endif  // LOD_LOADING_SCREEN_H_
