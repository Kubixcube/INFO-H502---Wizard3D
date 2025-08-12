#ifndef SCENE_H
#define SCENE_H
#include<vector>
#include "Object.h"
#include <reactphysics3d/reactphysics3d.h>
class Scene
{
private:
    std::string name;
    std::vector<Object> entities;
    reactphysics3d::PhysicsCommon physicsCommon;
    reactphysics3d::PhysicsWorld* world;
public:
    Scene() = default;
    Scene(std::string);
    // TODO: deal with static/dynamic in another way
    Object& addEntity(Object obj,int mass, bool isStatic);
    void update(float deltaTime);
    std::vector<Object> getEntities() const {return entities;}
    ~Scene();
};
#endif