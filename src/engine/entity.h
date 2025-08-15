#ifndef WIZARD3D_ENTITY_H
#define WIZARD3D_ENTITY_H

#include "reactphysics3d/body/RigidBody.h"
#include "PhysicsUtils.h"

class Entity {
public:
    float speed{0};
    reactphysics3d::RigidBody* body = nullptr;
    virtual ~Entity() = default;
};
#endif //WIZARD3D_ENTITY_H
