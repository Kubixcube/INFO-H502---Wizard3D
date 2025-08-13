#include "scene.h"
#include "PhysicsUtils.h"

Scene::Scene(std::string n) : name(n) {
    world = physicsCommon.createPhysicsWorld();
    world->setGravity(rp3d::Vector3(0, -GRAVITY, 0));
    makePlayer();
    makeFloor();
    makeSkyBox2();
}

Object& Scene::addEntity(Object obj,float mass, bool isStatic) {
    // Get initial position and orientation from the object's model matrix
    reactphysics3d::Collider *collider =  initPhysics(obj);
    // Check if it's a dynamic or static object
    // dynamic has mass and material properties
    if (!isStatic) {
        obj.body->setType(reactphysics3d::BodyType::DYNAMIC);
        reactphysics3d::Material& material = collider->getMaterial();
        material.setBounciness(0.0f);
        material.setFrictionCoefficient(0.7f);
        obj.body->setMass(mass);
    }
    else {
        obj.body->setType(reactphysics3d::BodyType::STATIC);
    }
    entities.push_back(obj);
    return entities.back();
}

reactphysics3d::Collider * Scene::initPhysics(Object &obj) {
    glm::vec3 position = glm::vec3(obj.model[3]);
    glm::mat4 rotationMatrix = obj.model;
    // normalizing matrix for non uniform scale objects
    rotationMatrix[0] = glm::normalize(rotationMatrix[0]);
    rotationMatrix[1] = glm::normalize(rotationMatrix[1]);
    rotationMatrix[2] = glm::normalize(rotationMatrix[2]);
    glm::quat orientation = glm::quat_cast(rotationMatrix);
    reactphysics3d::Transform transform(toReactPhysics3d(position), toReactPhysics3d(orientation));
    // Create a rigid body in the physics world
    obj.body = world->createRigidBody(transform);
    // Create the collision shape from the half-extents
    std::cout << obj.halfExtents.x << " | " << obj.halfExtents.y << " | " << obj.halfExtents.z << std::endl;
    reactphysics3d::BoxShape* boxShape = physicsCommon.createBoxShape(toReactPhysics3d(obj.halfExtents));
    reactphysics3d::Collider* collider = obj.body->addCollider(boxShape, reactphysics3d::Transform::identity());
    return collider;
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

void Scene::makeSkyBox2() {
    skybox = Object();

    static const float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };
    skybox.numVertices = 36;
    glGenVertexArrays(1, &skybox.VAO);
    glGenBuffers(1, &skybox.VBO);
    glBindVertexArray(skybox.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, skybox.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glBindVertexArray(0);
    std::vector<std::string> faces = {
            "assets/cubemaps/sky/posx.jpg",  // +X
            "assets/cubemaps/sky/negx.jpg",   // -X
            "assets/cubemaps/sky/posy.jpg",    // +Y
            "assets/cubemaps/sky/negy.jpg", // -Y
            "assets/cubemaps/sky/posz.jpg",  // +Z
            "assets/cubemaps/sky/negz.jpg"    // -Z
    };
    skybox.bindTexture(faces);
}

void Scene::makePlayer() {
    player = Object{"assets/models/wizard.obj","assets/textures/wizard_diffuse.png"};
    initPhysics(player);
    player.body->setType(reactphysics3d::BodyType::KINEMATIC);
}

void Scene::makeFloor() {
    floor = Object{"assets/models/plane.obj", "assets/textures/grass.jpg"};
    floor.scale({20.0f,1.0f,20.0f});
    initPhysics(floor);
    floor.body->setType(reactphysics3d::BodyType::STATIC);
}
