#include "scene.h"
#include "PhysicsUtils.h"

Scene::Scene(std::string n) : name(n) {
    world = physicsCommon.createPhysicsWorld();
    world->setGravity(rp3d::Vector3(0, -GRAVITY, 0));
    world->setEventListener(this);
    makePlayer();
    makeFloor();
    makeSkyBox();
    prepareFireball();
}

std::shared_ptr<Object> Scene::addEntity(const std::shared_ptr<Object>& obj,float mass, bool isStatic) {
    // Get initial position and orientation from the object's model matrix
    reactphysics3d::Collider *collider =  initPhysics(*obj);
    // Check if it's a dynamic or static object
    // dynamic has mass and material properties
    if (!isStatic) {
       obj->body->setType(reactphysics3d::BodyType::DYNAMIC);
        reactphysics3d::Material& material = collider->getMaterial();
        material.setBounciness(0.0f);
        material.setFrictionCoefficient(0.7f);
       obj->body->setMass(mass);
    }
    else {
       obj->body->setType(reactphysics3d::BodyType::STATIC);
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
    obj.body->setUserData(&obj);
    // Create the collision shape from the half-extents
    std::cout << obj.halfExtents.x << " | " << obj.halfExtents.y << " | " << obj.halfExtents.z << std::endl;
    reactphysics3d::BoxShape* boxShape = physicsCommon.createBoxShape(toReactPhysics3d(obj.halfExtents));
    reactphysics3d::Collider* collider = obj.body->addCollider(boxShape, reactphysics3d::Transform::identity());
    return collider;
}

void Scene::update(float deltaTime) {
    if (fireball.active) {
        const reactphysics3d::Vector3 fireballYVelocity(0.0f, -2.0f, 0.0f);
        float mass = fireball.body->getMass();
        reactphysics3d::Vector3 customGravityForce = fireball.body->getMass() * fireballYVelocity;
        // Applying the force to freball's center of mass
        fireball.body->applyWorldForceAtCenterOfMass(customGravityForce);
        // Smoke particule logic
        fireball.smokeTimer -= deltaTime;
        if (fireball.smokeTimer <= 0.0f) {
            fireball.smokeTimer = 0.05f;

            constexpr int SMOKE_COUNT = 4;
            for (int i = 0; i < SMOKE_COUNT; ++i) {
                size_t idx = (particleCursor + i) % particles.size();
                Particle& p = particles[idx];
                p.alive = true;
                p.pos   = fireball.getCurrentPos();
                glm::vec3 fbVel = toGLM(fireball.body->getLinearVelocity());
                p.vel   = -0.2f * glm::normalize(fbVel) + glm::vec3(randf(-0.5f,0.5f), randf(-0.2f,0.6f), randf(-0.5f,0.5f)) * 0.5f;
                p.life  = randf(0.25f, 0.45f);
                p.maxLife = p.life;
                p.size0 = p.size = randf(0.18f, 0.28f);
                p.color = glm::vec4(1.0f, randf(0.3f,0.6f), 0.0f, 0.9f);
            }
            particleCursor = (particleCursor + SMOKE_COUNT) % particles.size();
        }
    }
    // flash logic
    if (flash.active) {
        flash.intensity -= flash.decay * deltaTime;
        if (flash.intensity <= 0) {
            flash.active = false; // Deactivate the light part of the flash
        }
        if (flash.blastTimer > 0.0f) {
            flash.blastTimer -= deltaTime;
        }
    }

    // update particules
    for (auto& p : particles){
        if (p.alive) {
            p.life -= deltaTime;
            if (p.life <= 0.0f) { p.alive = false; continue; }

            float t = 1.0f - (p.life / p.maxLife); // 0 au début → 1 à la fin
            p.size  = p.size0 * (1.0f + 1.8f * t); // grossit jusqu’à ~2.8x
            p.color.a = std::max(0.0f, (1.0f - t) * 0.95f); // fade-out

            // expansion “sphérique” plus lisible
            p.pos  += p.vel * deltaTime;
            p.vel  += glm::vec3(0.0f, -9.8f, 0.0f) * 0.15f * deltaTime; // gravité légère
            p.vel  *= (1.0f - 0.4f * deltaTime); // “air drag” pour figer la forme
        }
    }

    // flash light decay
    if (flash.active){
        flash.intensity -= flash.decay * deltaTime;
        if (flash.intensity <= 0) flash.active=false;
    }

    world->update(deltaTime);

    for (auto& entity : entities) {
        if (entity->body && entity->body->getType() == reactphysics3d::BodyType::DYNAMIC) {
            const reactphysics3d::Transform& physTransform = entity->body->getTransform();
            glm::vec3 pos = toGLM(physTransform.getPosition());
            glm::quat rot = toGLM(physTransform.getOrientation());

            entity->model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
        }
    }
}

Scene::~Scene() {
    if (world) {
        physicsCommon.destroyPhysicsWorld(world);
    }
}

void Scene::makeSkyBox() {
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

void Scene::prepareFireball() {
    fireball.active = false;
    reactphysics3d::Transform transform(reactphysics3d::Vector3(0,0,0), reactphysics3d::Quaternion::identity());
    fireball.body = world->createRigidBody(transform);
    fireball.body->setType(reactphysics3d::BodyType::DYNAMIC);
    // fireball have their vertical vel
    fireball.body->setMass(10000.0f); // large to interact with any object
    fireball.body->enableGravity(false);
    // Fireball radius
    reactphysics3d::SphereShape* sphereShape = physicsCommon.createSphereShape(fireball.size);
    fireball.body->addCollider(sphereShape, reactphysics3d::Transform::identity());
    fireball.body->setUserData(&fireball);
    fireball.body->setIsActive(false);
}

void Scene::spawnProjectile(glm::vec3 position, glm::vec3 direction) {
    if (fireball.active) return; // only 1 fireball active
    fireball.active = true;
    fireball.lifetime = 5.0f; // reseting lifetime
    // set pos in phys engine
    reactphysics3d::Transform transform(toReactPhysics3d(position), reactphysics3d::Quaternion::identity());
    fireball.body->setTransform(transform);
    glm::vec3 initialVelocity = direction * fireball.speed;
    fireball.body->setLinearVelocity(toReactPhysics3d(initialVelocity));

    // making it's not spinning from a previous collision
    fireball.body->setAngularVelocity(reactphysics3d::Vector3(0, 0, 0));
    fireball.body->setIsActive(true);
}

void Scene::despawnProjectile() {
    if (!fireball.active) return;
    fireball.active = false;
    fireball.body->setIsActive(false);
    fireball.body->setAngularVelocity(reactphysics3d::Vector3(0, 0, 0));
}

void Scene::onContact(const reactphysics3d::CollisionCallback::CallbackData &callbackData) {
    // event listener override to handle fireballs
    EventListener::onContact(callbackData);
    for (unsigned p = 0; p < callbackData.getNbContactPairs(); p++) {
        CollisionCallback::ContactPair contactPair = callbackData.getContactPair(p);
        void* entity1 = contactPair.getBody1()->getUserData();
        void* entity2 = contactPair.getBody2()->getUserData();
        bool collided = false;
        // if the collision is the one with the fireball
        if (entity1 == &fireball || entity2 == &fireball) {
            if (!fireball.active) continue;
            collided = true;
            flash.active    = true;
            flash.pos       = fireball.getCurrentPos();
            flash.radius    = 18.0f;
            flash.intensity = 14.0f;
            flash.decay     = 7.0f;
            flash.blastTimer = flash.blastDuration;
//            flash.blastPos   = fireball.pos;
            // à la place du for (int i=0;i<140;i++){ ... find_if ... }
            constexpr int EXP_COUNT = 50000;
            for (int i = 0; i < EXP_COUNT; ++i) {
                Particle& p = particles[(particleCursor + i) % particles.size()];
                p.alive = true;
                p.pos   = fireball.getCurrentPos() + glm::vec3(0, 0.02f, 0);
                glm::vec3 dir(randf(-1,1), randf(0.0f,1.0f), randf(-1,1));
                if (glm::length(dir) < 1e-4f) dir = glm::vec3(0,1,0);
                dir = glm::normalize(dir);
                p.vel   = dir * randf(5.0f, 11.0f);
                p.life  = randf(0.9f, 1.4f);
                p.size  = randf(0.28f, 0.45f);
                p.maxLife = p.life;
                p.size0   = p.size;
                p.color = glm::vec4(1.0f, randf(0.25f,0.55f), 0.0f, 0.92f);
            }
            particleCursor = (particleCursor + EXP_COUNT) % particles.size();
            std::cout << "Fireball collision detected!" << std::endl;
            despawnProjectile();
        }
        if(!collided) {
            // pas de collision : avancer + fumée

        }
    }
}

void Scene::drawParticles(Shader &shader) {
    for (const auto& p : particles){
        if (!p.alive) continue;
        shader.setVec3("center", p.pos);
        shader.setFloat("size", p.size);
        shader.setVec4("color", p.color);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void Scene::drawFireBall(Shader &shader) {
    if ( fireball.active){
        shader.setVec3("center", fireball.getCurrentPos());
        shader.setFloat("size", 0.45f);
        shader.setVec4("color", glm::vec4(1.0f, 0.4f, 0.05f, 0.9f));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}
