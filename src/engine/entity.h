#ifndef WIZARD3D_ENTITY_H
#define WIZARD3D_ENTITY_H

#include "reactphysics3d/body/RigidBody.h"

class Entity {
public:
    reactphysics3d::RigidBody* body = nullptr;
    virtual ~Entity() = default;
};
#endif //WIZARD3D_ENTITY_H
