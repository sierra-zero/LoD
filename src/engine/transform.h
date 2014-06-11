// Copyright (c) 2014, Tamas Csala

#include <vector>
#include <algorithm>

#define GLM_FORCE_RADIANS
#include "../oglwrap/glm/glm/glm.hpp"
#include "../oglwrap/glm/glm/gtc/type_ptr.hpp"
#include "../oglwrap/glm/glm/gtc/matrix_transform.hpp"

#ifndef ENGINE_TRANSFORM_H_
#define ENGINE_TRANSFORM_H_

namespace engine {

template<typename T, glm::precision P = glm::precision::highp>
class Transformation {

  using vec3 = glm::detail::tvec3<T, P>;
  using vec4 = glm::detail::tvec4<T, P>;
  using mat3 = glm::detail::tmat3x3<T, P>;
  using mat4 = glm::detail::tmat4x4<T, P>;
  using quat = glm::detail::tquat<T, P>;

  Transformation* parent_;
  std::vector<Transformation*> children_;
  vec3 pos_, scale_;
  quat rot_;

public:

  Transformation()
      : parent_(nullptr)
      , scale_(1, 1, 1)
  { }

  virtual ~Transformation() {}

  void addChild(Transformation& t) {
    t.parent_ = this;
    children_.push_back(&t);
  }

  void removeChild(Transformation& t) {
    auto pos = std::find(children_.begin(), children_.end(), &t);
    if (pos != children_.end()) {
      children_.erase(pos);
    }
  }

  Transformation* getParent() const {
    return parent_;
  }

  const std::vector<Transformation*>& getChildren() const {
    return children_;
  }

  class WorldSpacePositionProxy : public vec3 {
    Transformation& tf;
  public:
    WorldSpacePositionProxy(Transformation& tf)
      : vec3(tf.pos()), tf(tf) { }

    ~WorldSpacePositionProxy() { tf.set_pos(*this); }
  };

  WorldSpacePositionProxy pos_proxy() {
    return WorldSpacePositionProxy(*this);
  }

  virtual const vec3 pos() const {
    if (parent_) {
      return vec3(
        parent_->localToWorldMatrix() * vec4(pos_, 1)
      );
    } else {
      return pos_;
    }
  }

  virtual void set_pos(const vec3& new_pos) {
    if (parent_) {
      pos_ = vec3(parent_->worldToLocalMatrix() * vec4(new_pos - parent_->pos(), 0));
    } else {
      pos_ = new_pos;
    }
  }

  vec3& local_pos() {
    return pos_;
  }

  const vec3& local_pos() const {
    return pos_;
  }

  void set_local_pos(const vec3& new_pos) {
    pos_ = new_pos;
  }

  class WorldSpaceScaleProxy : public vec3 {
    Transformation& tf;
  public:
    WorldSpaceScaleProxy(Transformation& tf)
      : vec3(tf.scale()), tf(tf) { }

    ~WorldSpaceScaleProxy() { tf.set_scale(*this); }
  };

  WorldSpaceScaleProxy scale_proxy() {
    return WorldSpaceScaleProxy(*this);
  }

  virtual const vec3 scale() const {
    if (parent_) {
      return mat3(parent_->localToWorldMatrix()) * scale_;
    } else {
      return scale_;
    }
  }

  virtual void set_scale(const vec3& new_scale) {
    if (parent_) {
      scale_ = mat3(parent_->worldToLocalMatrix()) * new_scale;
    } else {
      scale_ = new_scale;
    }
  }

  vec3& local_scale() {
    return scale_;
  }

  const vec3& local_scale() const {
    return scale_;
  }

  void set_local_scale(const vec3& new_scale) {
    scale_ = new_scale;
  }

  class WorldSpaceRotationProxy : public quat {
    Transformation& tf;
  public:
    WorldSpaceRotationProxy(Transformation& tf)
      : quat(tf.rot()), tf(tf) { }

    ~WorldSpaceRotationProxy() { tf.set_rot(*this); }
  };

  WorldSpaceRotationProxy rot_proxy() {
    return WorldSpaceRotationProxy(*this);
  }

  virtual const quat rot() const {
    if (parent_) {
      return parent_->rot() * rot_;
    } else {
      return rot_;
    }
  }

  virtual void set_rot(const quat& new_rot) {
    if (parent_) {
      rot_ = glm::inverse(parent_->rot()) * new_rot;
    } else {
      rot_ = new_rot;
    }
  }

  quat& local_rot() {
    return rot_;
  }

  const quat& local_rot() const {
    return rot_;
  }

  void set_local_rot(const quat& new_rot) {
    return rot_ = new_rot;
  }

  // Sets the rotation, so that 'local_space_vec' in local space will be
  // equivalent to 'world_space_vec' in world space.
  virtual void set_rot(const vec3& local_space_vec, const vec3& world_space_vec) {
    vec3 local = glm::normalize(local_space_vec);
    vec3 world = glm::normalize(world_space_vec);

    // Rotate around the vector, that is orthogonal to both.
    vec3 axis = glm::cross(local, world);

    // If they are not parallel
    if (glm::length(axis) > 1e-3) {
      // Dot gives us the cosine of their angle
      T cosangle = glm::dot(local, world);
      // We need the angle in degrees
      T angle = std::acos(cosangle);
      // Rotate with the calced values
      set_rot(glm::quat_cast(glm::rotate(mat4(), angle, axis)));
    } else {
      // If they are parallel, we only have to care about the case
      // when they go the opposite direction
      if (glm::dot(local, world) < 0) {
        // Check if local is parallel to the X axis
        if (fabs(glm::dot(local, vec3(1, 0, 0))) > 1e-3) {
          // If not, we can use it, to generate the axis to rotate around
          vec3 axis = glm::cross(vec3(1, 0, 0), local);
          set_rot(glm::quat_cast(glm::rotate(mat4(), T{M_PI}, axis)));
        } else {
          // Else we can use the Y axis for the same purpose
          vec3 axis = glm::cross(vec3(0, 1, 0), local);
          set_rot(glm::quat_cast(glm::rotate(mat4(), T{M_PI}, axis)));
        }
      } else {
        set_rot(quat{});
      }
    }
  }

  vec3 localRotateAndScale() const {
    return scale_ * rot_;
  }

  vec3 rotateAndScale() const {
    if (parent_) {
      return parent_->rotateAndScale() * scale_ * rot_;

      // Same as:
      // return mat3(localToWorldMatrix()) * vec3(1, 1, 1);
    } else {
      return scale_ * rot_;
    }
  }

  virtual vec3 forward() const {
    return glm::normalize(rot() * vec3(0, 0, -1));
  }

  virtual void set_forward(const vec3& new_fwd) {
    set_rot(vec3(0, 0, -1), new_fwd);
  }

  virtual vec3 up() const {
    return glm::normalize(rot() * vec3(0, 1, 0));
  }

  virtual void set_up(const vec3& new_up) {
     set_rot(vec3(0, 1, 0), new_up);
  }

  virtual vec3 right() const {
    return glm::normalize(rot() * vec3(1, 0, 0));
  }

  virtual void set_right(const vec3& new_right) {
    set_rot(vec3(1, 0, 0), new_right);
  }

  mat4 worldToLocalMatrix() const {
    return glm::inverse(localToWorldMatrix());
  }

  virtual mat4 localToWorldMatrix() const {
    mat4 local_transf = glm::scale(glm::mat4_cast(rot_), scale_);
    local_transf[3] = vec4(pos_, 1);

    if (parent_) {
      return parent_->localToWorldMatrix() * local_transf;
    } else {
      return local_transf;
    }
  }

  // To help the users a little
  mat4 matrix() const {
    return localToWorldMatrix();
  }

  operator mat4() const {
    return localToWorldMatrix();
  }
};

using Transform = Transformation<float, glm::precision::highp>;

}

#endif
