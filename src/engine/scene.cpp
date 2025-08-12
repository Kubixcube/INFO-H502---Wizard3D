#include "scene.h"
#include "PhysicsUtils.h"

Scene::Scene(std::string n) : name(n) {
    world = physicsCommon.createPhysicsWorld();
    world->setGravity(rp3d::Vector3(0, -GRAVITY, 0));
}

Object& Scene::addEntity(Object obj,int mass, bool isStatic) {
    // Get initial position and orientation from the object's model matrix
    std::cout << "test" << std::endl;
    glm::vec3 position = glm::vec3(obj.model[3]);
    std::cout << "test 2" << std::endl;
    glm::quat orientation = glm::quat_cast(obj.model);
    std::cout << "test 3" << std::endl;
    reactphysics3d::Transform transform(toReactPhysics3d(position), toReactPhysics3d(orientation));
    std::cout << "test 4" << std::endl;
    // Create a rigid body in the physics world
    obj.body = world->createRigidBody(transform);
    std::cout << "test 5" << std::endl;
    // Create the collision shape from the half-extents
    reactphysics3d::BoxShape* boxShape = physicsCommon.createBoxShape(toReactPhysics3d(obj.halfExtents));
    reactphysics3d::Collider* collider = obj.body->addCollider(boxShape, reactphysics3d::Transform::identity());

    // Check if it's a dynamic or static object
    // dynamic has mass and material properties
    if (!isStatic) {
        obj.body->setType(reactphysics3d::BodyType::DYNAMIC);
        reactphysics3d::Material& material = collider->getMaterial();
        material.setBounciness(0.2f);
        material.setFrictionCoefficient(0.7f);
        obj.body->setMass(mass);
    }
    else {
        obj.body->setType(reactphysics3d::BodyType::STATIC);
    }
    entities.push_back(obj);
    return entities.back();
}

void Scene::update(float deltaTime) {
    world->update(deltaTime);

    for (auto& entity : entities) {
        if (entity.body && entity.body->getType() == reactphysics3d::BodyType::DYNAMIC) {
            const reactphysics3d::Transform& physTransform = entity.body->getTransform();
            glm::vec3 pos = toGLM(physTransform.getPosition());
            glm::quat rot = toGLM(physTransform.getOrientation());

            entity.model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
        }
    }
}

Scene::~Scene() {
    if (world) {
        physicsCommon.destroyPhysicsWorld(world);
    }
}
