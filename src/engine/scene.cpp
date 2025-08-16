#include "scene.h"
#include "PhysicsUtils.h"
#include <cmath>
#include <glm/gtx/norm.hpp>



Scene::Scene(std::string n) : name(n) {
    world = physicsCommon.createPhysicsWorld();
    world->setGravity(rp3d::Vector3(0, -GRAVITY, 0));
    world->setEventListener(this);
    makePlayer();
    makeFloor();
    makeSkyBox();
    prepareFireball();
}

// Helpers "is finite" sûrs
static inline bool isFinite(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
static inline bool isFinite(const glm::quat& q) {
    return std::isfinite(q.x) && std::isfinite(q.y)
        && std::isfinite(q.z) && std::isfinite(q.w);
}



static inline glm::quat orthoQuatFromModel(const glm::mat4& M) {
    glm::vec3 c0 = glm::vec3(M[0]);
    glm::vec3 c1 = glm::vec3(M[1]);
    glm::vec3 c2 = glm::vec3(M[2]);
    float sx = glm::length(c0); if (sx < 1e-8f) sx = 1.0f;
    float sy = glm::length(c1); if (sy < 1e-8f) sy = 1.0f;
    float sz = glm::length(c2); if (sz < 1e-8f) sz = 1.0f;
    glm::mat3 R;
    R[0] = c0 / sx;
    R[1] = c1 / sy;
    R[2] = c2 / sz;
    glm::quat q = glm::normalize(glm::quat_cast(R));
    return q;
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

reactphysics3d::Collider* Scene::initPhysics(Object& obj) {
    glm::vec3 position = glm::vec3(obj.model[3]);

    // Orthonormalise la rotation et retire le scale avant de caster en quat
    glm::quat orientation = orthoQuatFromModel(obj.model);   // << utilise le helper déjà présent

    if (!isFinite(position)) position = glm::vec3(0.0f);
    if (!isFinite(orientation)) orientation = glm::quat(); // identité

    reactphysics3d::Transform transform(toReactPhysics3d(position),
                                        toReactPhysics3d(orientation));

    obj.body = world->createRigidBody(transform);   // ✨ transform toujours valide
    obj.body->setUserData(&obj);
    reactphysics3d::Transform colliderOffset = reactphysics3d::Transform::identity();
    colliderOffset.setPosition(toReactPhysics3d(obj.aabbCenter));
    // Collision shape
    // Assure-toi que halfExtents a été défini AVANT d'appeler addEntity/spawn
    reactphysics3d::BoxShape* boxShape =
        physicsCommon.createBoxShape(toReactPhysics3d(obj.halfExtents));
    reactphysics3d::Collider* collider =
        obj.body->addCollider(boxShape, colliderOffset);

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
        fireball.lifetime -= deltaTime;
        if (fireball.lifetime <= 0) {
            std::cout << "End of fireball lifetime" << std::endl;
            despawnProjectile();
        }
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
            updateObjectModel(entity);
        }
    }
    // player update
    updateObjectModel(player);

}

void Scene::updateObjectModel(std::shared_ptr<Object> &entity) const {
    const reactphysics3d::Transform& physTransform = entity->body->getTransform();
    glm::vec3 pos = toGLM(physTransform.getPosition());
    glm::quat rot = toGLM(physTransform.getOrientation());
    entity->model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
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
            "assets/cubemaps/wall/posx.jpg",  // +X
            "assets/cubemaps/wall/negx.jpg",   // -X
            "assets/cubemaps/wall/posy.jpg",    // +Y
            "assets/cubemaps/wall/negy.jpg", // -Y
            "assets/cubemaps/wall/posz.jpg",  // +Z
            "assets/cubemaps/wall/negz.jpg"    // -Z
    };
    skybox.bindTexture(faces);
}

void Scene::removeEntity(const std::shared_ptr<Object>& obj) {
    if (!obj) return;
    for (size_t i = 0; i < entities.size(); ++i) {
        // entities est un vector<shared_ptr<Object>>
        if (entities[i].get() == obj.get()) {
            // Détruire la physique si présente
            if (entities[i]->body) {
                world->destroyRigidBody(entities[i]->body);
                entities[i]->body = nullptr;
            }
            // (optionnel si tu gères shapes/colliders)
            // if (entities[i]->collider && entities[i]->body) entities[i]->body->removeCollider(entities[i]->collider);
            // if (entities[i]->shape) physicsCommon.destroyCollisionShape(entities[i]->shape);

            // Supprimer de la liste (swap-erase)
            if (i != entities.size() - 1) std::swap(entities[i], entities.back());
            entities.pop_back();
            return;
        }
    }
}

//void Scene::makePlayer() {
//    player = std::make_shared<Object>("assets/models/wizard.obj","assets/textures/wizard_diffuse.png");
//    player->speed = 5.0f;
//    initPhysics(*player);
//    player->body->setType(reactphysics3d::BodyType::KINEMATIC);
//}
void Scene::makePlayer() {
    player = std::make_shared<Object>("assets/models/wizard.obj","assets/textures/wizard_diffuse.png");
    player->speed = 5.0f;

    //  calculate capsule dimensions based on the model's halfExtents
    float capsuleRadius = std::max(player->halfExtents.x, player->halfExtents.z);
    float totalHeight = player->halfExtents.y * 2.0f;

    //  "height" is only the cylindrical part
    //  we subtract the two half-spheres (caps) from the total height.
    float capsuleHeight = totalHeight - (2.0f * capsuleRadius);

    // check to ensure the height is not negative if the model is wider than it is tall.
    if (capsuleHeight < 0) {
        capsuleHeight = 0;
    }

    reactphysics3d::Transform transform(reactphysics3d::Vector3(0,0,0), reactphysics3d::Quaternion::identity());
    player->body = world->createRigidBody(transform);
    player->body->setUserData(player.get());
    player->body->setType(reactphysics3d::BodyType::KINEMATIC);

    reactphysics3d::CapsuleShape* capsuleShape = physicsCommon.createCapsuleShape(capsuleRadius * 0.75, capsuleHeight);
    reactphysics3d::Transform colliderOffset = reactphysics3d::Transform::identity();
    colliderOffset.setPosition(toReactPhysics3d(player->aabbCenter));

    // adding the collider to the body using the offset transform
    player->body->addCollider(capsuleShape, colliderOffset);
//    player->body->addCollider(capsuleShape, reactphysics3d::Transform::identity());
}
void Scene::makeFloor() {
    floor = std::make_shared<Object>("assets/models/plane.obj", "assets/textures/grass.jpg");
    float newSize = 200.0f;
    floor->scale({newSize,1.0f,newSize});
    floor->texture.tiling = glm::vec2 {newSize* 0.1f};
    initPhysics(*floor);
    floor->body->setType(reactphysics3d::BodyType::STATIC);
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
        // if the collision is the one with the fireball
        if (entity1 == &fireball || entity2 == &fireball) {
            if (entity1 == player.get() || entity2 == player.get()) continue;
            if (!fireball.active) continue;
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

// Nécessite <glm/gtx/norm.hpp> déjà inclus avec length2
std::shared_ptr<Object> Scene::spawnIceWall(const glm::vec3 &playerPos, const glm::vec3 &aimFwd) {
    // Dimensions
    const float WIDTH  = 10.f;
    const float HEIGHT = 6.0f;
    const float THICK  = 1.0f;   // un peu plus épais côté physique = plus stable
    float capsuleRadius = std::max(player->halfExtents.x, player->halfExtents.z);
    float finalRadius = capsuleRadius * 0.75;
//    const float DIST   = 2.6f;    // distance devant le joueur
    const float DIST = finalRadius + (THICK * 0.5f) + 7.0f;
    // 1) Direction horizontale (projetée au sol) -> mur perpendiculaire à cette direction
//    glm::vec3 f = aimFwd;
    glm::vec3 fHoriz = glm::vec3(aimFwd.x, 0.0f, aimFwd.z);
    if (glm::length2(fHoriz) < 1e-12f) fHoriz = glm::vec3(0,0,-1); // fallback si on vise pile vertical
    fHoriz = glm::normalize(fHoriz);

    // 2) Base orthonormale "verticale" : up = Y, normal du mur = fHoriz
    const glm::vec3 up(0,1,0);
    glm::vec3 right = glm::normalize(glm::cross(fHoriz, up)); // X local du mur

    // 3) Centre du mur : posé au sol, à mi-hauteur, et DIST devant le joueur (sur XZ)
    // Hauteur (y) du dessus du floor (supposé axis-aligned)
//    glm::vec3 floorT = glm::vec3(floor->model[3]);
    const float groundY = floor->model[3].y;
    glm::vec3 center = glm::vec3(playerPos.x, groundY + HEIGHT * 0.5f, playerPos.z) + fHoriz * DIST;

    // 4) Matrice TR **sans scale** (pour la PHYSIQUE)
    glm::mat4 M(1.0f);
    M[0] = glm::vec4(right,  0.0f);
    M[1] = glm::vec4(up,     0.0f);
    M[2] = glm::vec4(fHoriz, 0.0f);  // Z local = normale du mur (horizontale)
    M[3] = glm::vec4(center, 1.0f);

    // 5) Création de l'objet : TR propre pour la physique, halfExtents définis AVANT addEntity
    auto wall = std::make_shared<Object>("assets/models/cube.obj");
    wall->model       = M;                                             // TR sans scale pour RP3D
    wall->halfExtents = glm::vec3(WIDTH*0.5f, HEIGHT*0.5f, THICK*0.5f); // taille collision

    // 6) Ajout à la scène en STATIQUE (0.0f), la physique lit le TR propre
    auto handle = addEntity(wall, 0.0f, true);
    handle->scale(glm::vec3(WIDTH, HEIGHT, THICK), false);
    // 8) Option : neutraliser toute texture 2D héritée (évite "container")
    handle->texture = Texture(); // id=0 → rendu via shader ice uniquement
    return handle;
}

void Scene::movePlayer(const glm::vec3 &movement) {
    // player moves '1 unit' at the time so we don't need keep velocity
    if (!player || !player->body) return;
    glm::vec3 frameMove = movement * player->speed;
    reactphysics3d::Transform currentTransform = player->body->getTransform();
    glm::vec3 currentPosition = toGLM(currentTransform.getPosition());
    glm::vec3 newPosition = currentPosition + frameMove;
    // to avoid getting stuck in a static shape, we need to revert  to old pos
    glm::vec3 newPositionX = currentPosition + glm::vec3{frameMove.x, 0.0f,0.0f};
    //  new transform
    reactphysics3d::Transform newTransform = currentTransform;
    reactphysics3d::Transform newTransformX = currentTransform;
    newTransformX.setPosition(toReactPhysics3d(newPositionX));
    newTransform.setPosition(toReactPhysics3d(newPosition));

    // move the body to the new spot for collision check
    player->body->setTransform(newTransformX);
//    player->body->setTransform(newTransform);

    // if the player's body is  overlapping with any other object on X
    if (isPlayerColliding()) {
        // colliding so we need to revert the transform to the original position.
        player->body->setTransform(currentTransform);
    }
    reactphysics3d::Transform currentTransformZ = player->body->getTransform();
    glm::vec3 currentPositionZ = toGLM(currentTransformZ.getPosition());
    glm::vec3 newPositionZ = currentPositionZ + glm::vec3(0.0f, 0.0f, frameMove.z);
    reactphysics3d::Transform newTransformZ = currentTransformZ;
    newTransformZ.setPosition(toReactPhysics3d(newPositionZ));
    player->body->setTransform(newTransformZ);

    if (isPlayerColliding()) {
        // collision on Z-axis, revert to the state before the Z-move
        player->body->setTransform(currentTransformZ);
    }
}

bool Scene::isPlayerColliding() {
    if (!player || !player->body) return false;
    unsigned int numBodies = world->getNbRigidBodies();
    // test all bodies to find if player collides
    for (int i = 0; i < numBodies; ++i) {
        reactphysics3d::RigidBody* otherBody = world->getRigidBody(i);
        if (otherBody == player->body) continue;
        if (otherBody == floor->body) continue;
        if (otherBody->getType() == reactphysics3d::BodyType::DYNAMIC) {
//            std::cout << "Wizard collides with dynamic object!" << std::endl;
            continue;
        };
        // if player collides
        if (world->testOverlap(player->body, otherBody)) {
            return true;
        }
    }
    return false;
}
