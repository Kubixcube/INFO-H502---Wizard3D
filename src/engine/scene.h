#ifndef SCENE_H
#define SCENE_H
#include<vector>
#include "engine/object.h"
#include "particule.h"
#include "shader.h"
#include <reactphysics3d/reactphysics3d.h>
class Scene: public reactphysics3d::EventListener
{
private:
    std::string name;
    std::vector<std::shared_ptr<Object>> entities;
    void makeSkyBox();
    void makePlayer();
    void makeFloor();
    bool isPlayerColliding();
    reactphysics3d::Collider * initPhysics(Object &obj);
    reactphysics3d::PhysicsCommon physicsCommon;
    reactphysics3d::PhysicsWorld* world;
public:
    Scene() = default;
    Scene(std::string);
    Object skybox;
    std::shared_ptr<Object> floor, player;
    Projectile fireball;
    ExplosionLight flash;
    std::vector<Particle> particles{400};
    size_t particleCursor{0};
    // TODO: deal with static/dynamic in another way
//    void drawEntityById(std::string ID);
    std::shared_ptr<Object> addEntity(const std::shared_ptr<Object>& obj,float mass, bool isStatic);
    void spawnProjectile(glm::vec3 position, glm::vec3 direction);
    void despawnProjectile();
    std::shared_ptr<Object> spawnIceWall(const glm::vec3& playerPos, const glm::vec3& aimFwd);
    void drawParticles(Shader& shader);
    void drawFireBall(Shader& shader);
    void movePlayer(const glm::vec3& movement);
    void update(float deltaTime);
    std::vector<std::shared_ptr<Object>>getEntities() const {return entities;}
    void onContact(const CollisionCallback::CallbackData& callbackData) override;
    void removeEntity(const std::shared_ptr<Object>& obj);


    ~Scene();

    void prepareFireball();

    void updateObjectModel(std::shared_ptr<Object> &entity) const;
};
#endif