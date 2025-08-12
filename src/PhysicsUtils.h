#ifndef PHYSICS_UTILS_H
#define PHYSICS_UTILS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <reactphysics3d/reactphysics3d.h>
#define GRAVITY 9.80665f

// GLM to ReactPhysics3D
inline rp3d::Vector3 toReactPhysics3d(const glm::vec3& v) {
    return rp3d::Vector3(v.x, v.y, v.z);
}

inline rp3d::Quaternion toReactPhysics3d(const glm::quat& q) {
    return rp3d::Quaternion(q.x, q.y, q.z, q.w);
}

// ReactPhysics3D to GLM
inline glm::vec3 toGLM(const rp3d::Vector3& v) {
    return glm::vec3(v.x, v.y, v.z);
}

inline glm::quat toGLM(const rp3d::Quaternion& q) {
    //parameters: w, x, y, z for glm::quat
    return glm::quat(q.w, q.x, q.y, q.z);
}

#endif