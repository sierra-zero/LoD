/** @file charmove.hpp
    @brief Implements features related to character movement.
*/

#ifndef LOD_CHARMOVE_HPP_
#define LOD_CHARMOVE_HPP_

#include "oglwrap/utils/camera.hpp"

extern const float GRAVITY;

namespace oglwrap {

class CharacterMovement {
  glm::vec3 pos;

  // Current and destination rotation angles.
  double currRot, destRot;

  // Moving speed per second in OpenGL units.
  float rotSpeed, vertSpeed;

  glm::vec2 horizSpeed;

  bool walking, jumping, transition;

public:
  CharacterMovement(glm::vec3 pos = glm::vec3(),
                    float rotationSpeed_PerSec = 180.0f)
    : pos(pos)
    , currRot(0)
    , destRot(0)
    , rotSpeed(rotationSpeed_PerSec)
    , vertSpeed(0)
    , walking(false)
    , jumping(false)
    , transition(false)
  { }

  void update(const Camera& cam, glm::vec2 character_offset) {
    using namespace glm;

    static sf::Clock clock;
    static float prevTime = 0;
    float time = clock.getElapsedTime().asSeconds();
    float dt =  time - prevTime;
    prevTime = time;

    ivec2 moveDir; // up and right is positive
    bool w = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
    bool a = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
    bool s = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
    bool d = sf::Keyboard::isKeyPressed(sf::Keyboard::D);

    if(w && !s) {
      moveDir.y = 1;
    } else if(s && !w) {
      moveDir.y = -1;
    }

    if(d && !a) {
      moveDir.x = 1;
    } else if(a && !d) {
      moveDir.x = -1;
    }

    static ivec2 lastMoveDir;
    bool lastWalking = walking;
    walking = moveDir.x || moveDir.y;
    transition = transition || (walking != lastWalking) || (lastMoveDir != moveDir);
    lastMoveDir = moveDir;

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !jumping) {
      jumping = true;
      vertSpeed = 0.5f;
    }


    if(walking) {
      double cameraRot = -cam.getRoty(); // -z is forward
      double moveRot = atan2(moveDir.y, moveDir.x); // +y is forward
      destRot = ToDegree(cameraRot + moveRot);
      destRot = fmod(destRot, 360);

      double diff = destRot - currRot;
      double sign = diff / fabs(diff);

      // Take the shorter path.
      while(fabs(diff) > 180) {
        destRot -= sign * 360;

        diff = destRot - currRot;
        sign = diff / fabs(diff);
      }

      if(transition) {
        if(fabs(diff) > rotSpeed / 20.0f) {
          currRot += sign * dt * rotSpeed;
          currRot = fmod(currRot, 360);
        } else {
          transition = false;
        }
      } else {
        currRot = fmod(destRot, 360);
      }
    }

    mat3 transformation = mat3(rotate(mat4(), (float)fmod(currRot, 360), vec3(0,1,0)));

    pos += transformation * vec3(character_offset.x, 0, character_offset.y);
    if(jumping) {
      pos += transformation * vec3(0, 0, 10.0f * dt);
    }
  }

  void updateHeight(float groundHeight) {
    static sf::Clock clock;
    static float prevTime = 0;
    float time = clock.getElapsedTime().asSeconds();
    float dt =  time - prevTime;
    prevTime = time;

    const float diff = groundHeight - pos.y;
    if(diff >= 0 && jumping && vertSpeed < 0) {
      jumping = false;
      pos.y = groundHeight;
      return;
    }
    if(!jumping) {
      const float offs = std::max(fabs(diff / 2.0f), 0.05) * dt * 20.0f;
      if(fabs(diff) > offs) {
        pos.y += diff / fabs(diff) * offs;
      }
    }
    if(jumping) {
      if(diff > 0) {
        pos.y += std::max(diff, vertSpeed) * dt * 30.0f;
      } else {
        pos.y += vertSpeed * dt * 30.0f;
      }
      vertSpeed -= dt * GRAVITY;
    }
  }

  bool is_jumping() const {
    return jumping;
  }

  bool is_jumping_rise() const {
    return jumping && vertSpeed > 0;
  }

  bool is_jumping_fall() const {
    return jumping && vertSpeed < 0;
  }

  bool isWalking() const {
    return walking;
  }

  glm::mat4 getModelMatrix() const {
    glm::mat4 rot = glm::rotate(glm::mat4(), (float)fmod(currRot, 360), glm::vec3(0,1,0));
    // The matrix's last column is responsible for the translation.
    rot[3] = glm::vec4(pos, 1.0);
    return rot;
  }

  glm::vec3 getPos() const {
    return pos;
  }
};

}

#endif // LOD_CHARMOVE_HPP_
