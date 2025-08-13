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
    void makeSkyBox2();
    void makePlayer();
    reactphysics3d::PhysicsCommon physicsCommon;
    reactphysics3d::PhysicsWorld* world;
public:
    Scene() = default;
    Scene(std::string);
    Object skybox;
    Object floor, player;
    // TODO: deal with static/dynamic in another way
//    void drawEntityById(std::string ID);
    Object& addEntity(Object obj, float mass , bool isStatic);
    void update(float deltaTime);
    std::vector<Object> getEntities() const {return entities;}
    ~Scene();


};
#endif