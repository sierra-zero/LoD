#include <vector>
#include <string>
#include <iostream>
#include <SFML/Window.hpp>

#include "oglwrap_config.hpp"
#include "oglwrap/glew.hpp"
#include "oglwrap/oglwrap.hpp"
#include "oglwrap/utils/camera.hpp"

#include "charmove.hpp"
#include "skybox.hpp"
#include "terrain.hpp"
#include "bloom.hpp"
#include "ayumi.hpp"
#include "tree.hpp"
#include "shadow.hpp"
#include "map.hpp"

using namespace oglwrap;
Context gl;

extern const float GRAVITY = 18.0f;
/* 0 -> max quality
   2 -> max performance */
extern const int PERFORMANCE = 1;
extern const float kFieldOfView = 60;
double ogl_version;
bool was_left_click = false;

void glInit() {
  gl.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  gl.ClearDepth(1.0f);
  gl.Enable(Capability::DepthTest);
  gl.Enable(Capability::DepthClamp);
  gl.CullFace(Face::Back);
}

void FpsDisplay(float time) {
  static float accum_time = 0.0f;
  static float last_call = time;
  float dt = time - last_call;
  last_call = time;
  static int calls = 0;

  calls++;
  accum_time += dt;
  if(accum_time > 1.0f) {
    std::cout << "FPS: " << calls << std::endl;
    accum_time = calls = 0;
  }
}

static sf::Clock dbg_clock;

static void PrintDebugText(const std::string& str) {
  std::cout << str << ": ";
}

static void PrintDebugTime() {
  std::cout << dbg_clock.getElapsedTime().asMilliseconds() << " ms" << std::endl;
  dbg_clock.restart();
}

int main() {
  PrintDebugText("Creating the OpenGL context");
  sf::Window window(
    sf::VideoMode(800, 600),
    "Land of Dreams",
    sf::Style::Default,
    sf::ContextSettings(32, 0, 0, 2, 1)
  );
  PrintDebugTime();

  sf::ContextSettings settings = window.getSettings();
  std::cout << " - Depth bits: " << settings.depthBits << std::endl;
  std::cout << " - Stencil bits: " << settings.stencilBits << std::endl;
  std::cout << " - Anti-aliasing level: " << settings.antialiasingLevel << std::endl;
  std::cout << " - OpenGL version: " << settings.majorVersion << "." << settings.minorVersion << std::endl;

  ogl_version = settings.majorVersion + settings.minorVersion/10.0;

  if(ogl_version < 2.1) {
    std::cout << "At least OpenGL version 2.1 is required to run this program" << std::endl;
    return -1;
  }

  // No V-sync needed because of multiple draw calls per frame.
  window.setFramerateLimit(60);
  window.setVerticalSyncEnabled(false);

  sf::Clock clock;
  bool fixMouse = false;

  GLenum err = glewInit();
  if(err != GLEW_OK) {
    std::cout << "GlewInit error: " << glewGetErrorString(err) << std::endl;
    return -1;
  }

  try {
      glInit();

      dbg_clock.restart();
      PrintDebugText("Initializing the skybox");
        Skybox skybox;
      PrintDebugTime();

      PrintDebugText("Initializing the resources for the bloom effect");
        BloomEffect bloom;
      PrintDebugTime();

      PrintDebugText("Initializing the terrain");
        Terrain terrain(skybox);
      PrintDebugTime();

      PrintDebugText("Initializing the shadow maps");
        Shadow shadow(PERFORMANCE < 2 ? 512 : 256, 64);
      PrintDebugTime();

      PrintDebugText("Initializing the trees");
        Tree tree(skybox, terrain);
      PrintDebugTime();

      PrintDebugText("Initializing the map");
        Map map(glm::vec2(terrain.w, terrain.h));
      PrintDebugTime();

      CharacterMovement charmove(glm::vec3(0, terrain.getScales().y * 13, 0));

      PrintDebugText("Initializing Ayumi");
        Ayumi ayumi(skybox, charmove);
      PrintDebugTime();

      ThirdPersonalCamera cam(
        ayumi.getMesh().bSphereCenter() + glm::vec3(ayumi.getMesh().bSphereRadius() * 2),
        ayumi.getMesh().bSphereCenter(),
        1.5f
      );

      std::cout << "\nStarting the main loop.\n" << std::endl;

      sf::Event event;
      bool running = true;
      while(running) {

        while(window.pollEvent(event)) {
          switch(event.type) {
            case sf::Event::Closed:
              running = false;
              break;
            case sf::Event::KeyPressed:
              if(event.key.code == sf::Keyboard::Escape) {
                running = false;
              } else if(event.key.code == sf::Keyboard::F11) {
                fixMouse = !fixMouse;
                window.setMouseCursorVisible(!fixMouse);
              } else if(event.key.code == sf::Keyboard::Space) {
                charmove.handleSpacePressed();
              } else if(event.key.code == sf::Keyboard::M) {
                map.toggle();
              }
              break;
            case sf::Event::Resized: {
                int width = event.size.width, height = event.size.height;

                gl.Viewport(width, height);
                bloom.resize(width, height);
                shadow.resize(width, height);

                auto projMat = glm::perspectiveFov<float>(
                  kFieldOfView, width, height, 0.5, 3000
                );

                ayumi.resize(projMat);
                skybox.resize(projMat);
                terrain.resize(projMat);
                tree.resize(projMat);
              }
              break;
            case sf::Event::MouseWheelMoved:
              cam.scrolling(event.mouseWheel.delta);
              break;
            case sf::Event::MouseButtonPressed:
              if(event.mouseButton.button == sf::Mouse::Left) {
                was_left_click = true;
              }
              break;
            default:
              break;
          }
        }

        gl.Clear().Color().Depth();

        // Updates
        cam.updateRotation(window, fixMouse);
        float time = clock.getElapsedTime().asSeconds();
        charmove.update(cam, ayumi.getMesh().offsetSinceLastFrame());
        cam.updateTarget(charmove.getPos() + ayumi.getMesh().bSphereCenter());
        auto scales = terrain.getScales();
        charmove.updateHeight(
          scales.y * terrain.getHeight(
            charmove.getPos().x / (double)scales.x,
            charmove.getPos().z / (double)scales.z
          )
        );
        ayumi.updateStatus(time, charmove);
        FpsDisplay(time);

        // Create shadow data
        shadow.begin(); {
          ayumi.shadowRender(time, shadow, charmove);
          tree.shadowRender(time, cam, shadow);
        } shadow.end();

        // Actual renders
        skybox.render(time, cam.cameraMatrix());
        terrain.render(time, cam, shadow);
        ayumi.render(time, cam, charmove, shadow);
        tree.render(time, cam);
        bloom.render();
        map.render(cam);

        window.display();

      }

      return 0;

    } catch(std::exception& err) {
      std::cerr << err.what();
    }

  return 1;
}
