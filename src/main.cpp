
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include "engine/shader.h"
#include "engine/camera.h"
#include "engine/object.h"
#include "engine/scene.h"
#include "engine/particule.h"
#include "PhysicsUtils.h"
#include <glm/gtx/norm.hpp>
#include <cmath>              

#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>



// --- ICE WALL helpers (prototypes) ---
glm::vec3 getAimForward(bool useThirdPerson, float orbitYaw, float orbitPitch, const Camera& camera);
std::shared_ptr<Object> spawnIceWall(Scene& scene, const glm::vec3& playerPos, const glm::vec3& fwd);
void removeIceWall(Scene& scene, std::shared_ptr<Object>& handle);

// ---- Planar reflection for Ice Wall ----
static GLuint gReflFBO = 0, gReflColor = 0, gReflDepth = 0;
static int    gReflW = 1024, gReflH = 1024;
static glm::mat4 gRefVP = glm::mat4(1.0f);


// Helpers "is finite" sûrs
static inline bool isFinite(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
static inline bool isFinite(const glm::quat& q) {
    return std::isfinite(q.x) && std::isfinite(q.y)
        && std::isfinite(q.z) && std::isfinite(q.w);
}

// Quad VAO pour billboard
GLuint particleVAO=0, particleVBO=0;
// billboard quad (2 triangles)
void makeParticleQuad(){
    if (particleVAO) return;
    float quad[] = {
        // x,y,   u,v
        -0.5f,-0.5f, 0.0f,0.0f,
         0.5f,-0.5f, 1.0f,0.0f,
         0.5f, 0.5f, 1.0f,1.0f,
        -0.5f,-0.5f, 0.0f,0.0f,
         0.5f, 0.5f, 1.0f,1.0f,
        -0.5f, 0.5f, 0.0f,1.0f
    };
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // inOffset
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // inUV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);
}

static int SCR_WIDTH=1280, SCR_HEIGHT=720;
Camera camera(glm::vec3(0.0f,1.5f,5.0f));
float lastX=SCR_WIDTH*0.5f, lastY=SCR_HEIGHT*0.5f; bool firstMouse=true;
bool useThirdPerson=true;
glm::vec3 playerPos(0.0f);
glm::vec3 playerRot(0.0f);


// --- ICE WALL (un seul à la fois) ---
static std::shared_ptr<Object> gIceWall = nullptr;

float orbitYaw=0.0f, orbitPitch=15.0f, camDist=5.0f;
glm::mat4 thirdPersonView(){
    float cy=cos(glm::radians(orbitYaw)), sy=sin(glm::radians(orbitYaw));
    float cp=cos(glm::radians(orbitPitch)), sp=sin(glm::radians(orbitPitch));
    glm::vec3 fwd=glm::normalize(glm::vec3(cy*cp, sp, sy*cp));
    glm::vec3 eye=playerPos+glm::vec3(0,1.6f,0)-fwd*camDist+glm::vec3(0, camDist*0.25f, 0);
    return glm::lookAt(eye, playerPos+glm::vec3(0,1.6f,0), glm::vec3(0,1,0));
}
void framebuffer_size_callback(GLFWwindow*,int w,int h){
    SCR_WIDTH=w;
    SCR_HEIGHT=h;
    glViewport(0,0,w,h);
}
void mouse_callback(GLFWwindow*, double xpos,double ypos){
    if(firstMouse){ lastX=(float)xpos; lastY=(float)ypos; firstMouse=false; }
    float xo=(float)xpos-lastX, yo=lastY-(float)ypos; lastX=(float)xpos; lastY=(float)ypos;
    if(useThirdPerson){
        orbitYaw+=xo*0.15f;
        orbitPitch+=yo*0.15f;
        orbitPitch=glm::clamp(orbitPitch,-10.0f,80.0f);
    }
    else camera.ProcessMouseMovement(xo, yo);
}
void scroll_callback(GLFWwindow*, double, double yoff){ camera.ProcessMouseScroll((float)yoff); }

void processInput(GLFWwindow* win,float dt, Scene& scene){
    float speed=5.0f*dt;
    glm::vec3 movementDir(0.0f);
    
    if(glfwGetKey(win,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(win,true);
    if(glfwGetKey(win,GLFW_KEY_C)==GLFW_PRESS) useThirdPerson=true;
    if(glfwGetKey(win,GLFW_KEY_V)==GLFW_PRESS) useThirdPerson=false;
    if(useThirdPerson){
        float yR=glm::radians(orbitYaw); glm::vec3 f=glm::normalize(glm::vec3(cos(yR),0,sin(yR)));
        glm::vec3 r=glm::normalize(glm::cross(f,glm::vec3(0,1,0)));
        if(glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS) movementDir+=f;
        if(glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS) movementDir-=f;
        if(glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS) movementDir-=r;
        if(glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS) movementDir+=r;

        if (glm::length(movementDir) > 0.001f) {
            movementDir = glm::normalize(movementDir);
            playerRot = movementDir; // Update facing direction only when moving
//            playerPos += movementDir * speed;
            scene.player->rotate(playerRot);
            scene.movePlayer(movementDir * dt);
        }
        

    }
    else {
        if(glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS) camera.ProcessKeyboard(FORWARD,dt);
        if(glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS) camera.ProcessKeyboard(BACKWARD,dt);
        if(glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS) camera.ProcessKeyboard(LEFT,dt);
        if(glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS) camera.ProcessKeyboard(RIGHT,dt);
        if(glfwGetKey(win,GLFW_KEY_SPACE)==GLFW_PRESS) camera.ProcessKeyboard(UP,dt);
        if(glfwGetKey(win,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) camera.ProcessKeyboard(DOWN,dt);
    }

    static bool prevClick=false;
    bool click = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    if (click && !prevClick && !scene.fireball.active){
        // origine & direction
        glm::mat4 Vnow = useThirdPerson ? thirdPersonView() : camera.GetViewMatrix();
        glm::vec3 eye  = glm::vec3(glm::inverse(Vnow)[3]);
        glm::vec3 fwd;
        if (useThirdPerson){
            float yR = glm::radians(orbitYaw);
            float cp = cos(glm::radians(orbitPitch));
            fwd = glm::normalize(glm::vec3(cos(yR)*cp, sin(glm::radians(orbitPitch)), sin(yR)*cp));
        } else {
            fwd = glm::normalize(camera.Front);
        }
        glm::vec3 muzzle = playerPos + 0.1f + glm::vec3(0,1.6f,0) + fwd*0.3f;
        scene.spawnProjectile(muzzle, fwd);
    }
    prevClick = click;

    // --- Clic droit : toggle mur de glace ---
    static bool prevR = false;
    bool rightClick = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    if (rightClick && !prevR) {
        if (!gIceWall) {
            glm::vec3 fwd = getAimForward(useThirdPerson, orbitYaw, orbitPitch, camera);
            gIceWall = scene.spawnIceWall(playerPos, fwd);
        } else {
            removeIceWall(scene, gIceWall);
        }
    }
    prevR = rightClick;
}

glm::vec3 getAimForward(bool useThirdPerson, float orbitYaw, float orbitPitch, const Camera& camera) {
    if (useThirdPerson) {
        float yR = glm::radians(orbitYaw);
        float pR = glm::radians(orbitPitch);
        float cp = cos(pR);
        glm::vec3 f = glm::normalize(glm::vec3(cos(yR)*cp, sin(pR), sin(yR)*cp));
        return f;
    } else {
        return glm::normalize(camera.Front);
    }
}

void removeIceWall(Scene& scene, std::shared_ptr<Object>& handle) {
    if (handle) {
        scene.removeEntity(handle);
        handle.reset();
    }
}

int main(){
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win=glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Wizard3D",NULL,NULL);
    if(!win){
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win,framebuffer_size_callback);
    glfwSetCursorPosCallback(win,mouse_callback);
    glfwSetScrollCallback(win,scroll_callback);
    glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);
    // shader loading
    Shader sky("src/shaders/skybox.vert", "src/shaders/skybox.frag");
    Shader lighting("src/shaders/lighting.vert","src/shaders/lighting.frag");
    Shader env("src/shaders/envmap.vert", "src/shaders/envmap.frag");
    Shader particleShader("src/shaders/particles.vert", "src/shaders/particles.frag");
    Shader ice("src/shaders/ice.vert","src/shaders/ice.frag");

    // FBO de réflexion plan
    glGenFramebuffers(1, &gReflFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gReflFBO);

    // couleur
    glGenTextures(1, &gReflColor);
    glBindTexture(GL_TEXTURE_2D, gReflColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gReflW, gReflH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // depth
    glGenRenderbuffers(1, &gReflDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, gReflDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gReflW, gReflH);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gReflColor, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gReflDepth);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    makeParticleQuad();
//    particles.resize(400); // pool simple
    srand((unsigned)time(nullptr));

    float reflectMix = 0.6f;
    Scene scene{"Firing range"};
    auto fallingCube = std::make_shared<Object>("assets/models/cube.obj", "assets/textures/container.jpg");
    fallingCube->translate({0.0f, 5.0f, 0.0f});
    scene.addEntity(fallingCube, 50.0f, false);
    auto cube = std::make_shared<Object> ("assets/models/cube.obj");
    glm::vec3 lightDir=glm::normalize(glm::vec3(-0.3f,-1.0f,-0.4f));
    glm::vec3 lightColor(1.0f), ambient(0.10f), specular(0.0f);
    float shininess = 32.0f;
    cube->model=glm::translate(glm::mat4(1.0f), glm::vec3(0,0.5f,-2.0f));
    // auto reflectCube = scene.addEntity(cube,0.0f, true);
    float last= (float) glfwGetTime();

auto renderReflection = [&](Scene& scene, const glm::mat4& P){
    if (!gIceWall) return;

    // --- sauvegarder viewport ---
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    // --- récupérer plan & caméra miroir (comme avant) ---
    const glm::vec3 planeP = glm::vec3(gIceWall->model[3]);
    glm::vec3 n = glm::normalize(glm::vec3(gIceWall->model[2]));
    glm::mat4 Vnow = useThirdPerson ? thirdPersonView() : camera.GetViewMatrix();
    glm::vec3 eye  = useThirdPerson ? glm::vec3(glm::inverse(Vnow)[3]) : camera.Position;
    glm::vec3 fwd  = useThirdPerson ? glm::normalize((playerPos+glm::vec3(0,1.6f,0)) - eye)
                                    : glm::normalize(camera.Front);
    glm::vec3 up   = useThirdPerson ? glm::vec3(0,1,0) : glm::normalize(camera.Up);

    auto reflectPoint = [&](glm::vec3 p){ float d = glm::dot(n, p - planeP); return p - 2.0f*d*n; };
    auto reflectDir   = [&](glm::vec3 v){ return v - 2.0f*glm::dot(n, v)*n; };

    glm::vec3 rEye = reflectPoint(eye);
    glm::vec3 rFwd = glm::normalize(reflectDir(fwd));
    glm::vec3 rUp  = glm::normalize(reflectDir(up));
    glm::mat4 Vref = glm::lookAt(rEye, rEye + rFwd, rUp);
    gRefVP = P * Vref;

    // --- binder FBO et clear ---
    glBindFramebuffer(GL_FRAMEBUFFER, gReflFBO);
    glViewport(0,0,gReflW,gReflH);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // IMPORTANT : pendant le pass miroir, le winding est inversé
    glFrontFace(GL_CW);

    // 1) SKYBOX (toujours en premier)
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    sky.use();
    sky.setMat4("P", P);
    sky.setMat4("V", Vref);
    scene.skybox.texture.map();
    glUniform1i(glGetUniformLocation(sky.id(), "skybox"), 0);
    scene.skybox.draw();
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // 2) OPAQUES (sol, entités, joueur) — SANS le mur
    lighting.use();
    lighting.setMat4("P", P);
    lighting.setMat4("V", Vref);
    lighting.setVec3("viewPos", rEye);
    lighting.setVec3("lightDir", lightDir);
    lighting.setVec3("lightColor", lightColor);
    lighting.setVec3("ambientColor", ambient);
    lighting.setVec3("specularColor", specular);
    lighting.setFloat("shininess", shininess);

    // floor
    lighting.setMat4("M", scene.floor->model);
    lighting.setVec2("texTiling", scene.floor->texture.tiling);
    scene.floor->texture.map();
    scene.floor->draw();

    // entities
    for (const auto& e : scene.getEntities()){
        if (gIceWall && e.get() == gIceWall.get()) continue; // no self-reflection
        lighting.setMat4("M", e->model);
        e->texture.map();
        e->draw();
    }
    // player
    lighting.setMat4("M", scene.player->model);
    lighting.setVec2("texTiling",scene.player->texture.tiling);
    scene.player->texture.map();
    glUniform1i(glGetUniformLocation(lighting.id(), "diffuseMap"), 0);
    scene.player->draw();

    // 3) PARTICULES (boule de feu + flash + pool) — APRÈS opaques
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    particleShader.use();
    particleShader.setMat4("P", P);
    particleShader.setMat4("V", Vref);
    glBindVertexArray(particleVAO);

    // flash disque
    if (scene.flash.blastTimer > 0.0f) {
        float t = 1.0f - (scene.flash.blastTimer / scene.flash.blastDuration);
        float size = 3.0f + 7.0f * t;
        particleShader.setVec3("center", scene.flash.pos);
        particleShader.setFloat("size",  size);
        particleShader.setVec4("color",  glm::vec4(1.0f, 0.6f, 0.15f, 0.45f));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    // cœur + particules (tes helpers)
    scene.drawFireBall(particleShader);
    scene.drawParticles(particleShader);

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // --- restaurer front face, unbind FBO ---
    glFrontFace(GL_CCW);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Générer mipmaps (roughness)
    glBindTexture(GL_TEXTURE_2D, gReflColor);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // --- restaurer viewport fenêtre ---
    glViewport(vp[0], vp[1], vp[2], vp[3]);
};

    std::shared_ptr<Object> target = std::make_shared<Object>("assets/models/target_dummy.obj", "assets/textures/target.png");
    float targetScale = 0.3f;
    target->translate({2.0f,0.0f,2.0f});
    target->scale({targetScale,targetScale,targetScale});
    scene.addEntity(target, 0, true);
    while(!glfwWindowShouldClose(win)){
        float t = (float) glfwGetTime();
        float dt = t-last;
        last=t;
        processInput(win,dt, scene);
        // wizard movement
//        scene.player->translate(playerPos);


        scene.update(dt);

        glm::mat4 P= glm::perspective(glm::radians(camera.Zoom),(float)SCR_WIDTH/(float)SCR_HEIGHT,0.1f,100.0f);
        glm::mat4 V= useThirdPerson ? thirdPersonView(): camera.GetViewMatrix();
        if (scene.player && scene.player->body) {
            playerPos = toGLM(scene.player->body->getTransform().getPosition());
        }
        glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);
        glClearColor(0.05f,0.07f,0.09f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        renderReflection(scene, P);

        lighting.use();
        lighting.setMat4("P",P);
        lighting.setMat4("V",V);
        glm::vec3 eye = useThirdPerson ? glm::vec3(glm::inverse(V)[3]) : camera.Position;
        lighting.setVec3("viewPos", eye);
        lighting.setVec3("lightDir", lightDir);
        lighting.setVec3("lightColor", lightColor);
        lighting.setVec3("ambientColor", ambient);
        lighting.setVec3("specularColor", specular);
        lighting.setFloat("shininess", shininess);
        lighting.setMat4("M", scene.floor->model);
        lighting.setVec2("texTiling", scene.floor->texture.tiling);
        scene.floor->texture.map();
        scene.floor->draw();

        for (const auto& entity : scene.getEntities()) {
            if (gIceWall && entity.get() == gIceWall.get()) continue;
            lighting.setMat4("M", entity->model);
            lighting.setVec2("texTiling",entity->texture.tiling);
            entity->texture.map();
            entity->draw();
        } 

        // --- Mur de glace
        if (gIceWall) {
            ice.use();
            ice.setMat4("P", P);
            ice.setMat4("V", V);
            ice.setVec3("viewPos", eye);

            // paramètres "glace"
            ice.setFloat("roughness", 0.10f);                 // 0=miroir, 1=flou
            ice.setVec3("tint", glm::vec3(0.50f, 0.50f, 1.0f));
            // ice.setFloat("mixEnv", 0.1f);

            // bind cubemap (unit 0) + texture reflet (unit 1)
            scene.skybox.texture.map();                       // unit 0
            glUniform1i(glGetUniformLocation(ice.id(), "skybox"), 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gReflColor);
            glUniform1i(glGetUniformLocation(ice.id(), "uReflectTex"), 1);

            // matrice VP du pass reflet
            ice.setMat4("uRefVP", gRefVP);

            // dessiner le mur
            ice.setMat4("M", gIceWall->model);
            gIceWall->draw();

            glActiveTexture(GL_TEXTURE0); // reset
        }

        lighting.use();
        lighting.setMat4("P", P);
        lighting.setMat4("V", V);
        lighting.setMat4("M", scene.player->model);
        lighting.setVec2("texTiling", scene.player->texture.tiling);
        scene.player->texture.map();
        glUniform1i(glGetUniformLocation(lighting.id(),"diffuseMap"),0);
        scene.player->draw();

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
//        glDisable(GL_CULL_FACE);
        sky.use();
        sky.setMat4("P", P);
        sky.setMat4("V", V);
        scene.skybox.texture.map();
        glUniform1i(glGetUniformLocation(sky.id(), "skybox"), 0);
        scene.skybox.draw();
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // --- point lights dynamiques (fireball / explosion) ---
        int plc = 0;
        glm::vec3 PLpos[4]; glm::vec3 PLcol[4];
        float PLint[4]; float PLrad[4];
        // lumière portée par la boule (si tu veux un halo pendant le vol)
        if (scene.fireball.active && plc < 4){
            PLpos[plc] = scene.fireball.getCurrentPos();
            PLcol[plc] = glm::vec3(1.0f, 0.6f, 0.2f);
            PLint[plc] = 1.5f;
            PLrad[plc] = 3.0f;
            plc++;
        }

        // flash d'explosion
        if (scene.flash.active && plc < 4){
            PLpos[plc] = scene.flash.pos;
            PLcol[plc] = glm::vec3(1.0f, 0.5f, 0.1f);
            PLint[plc] = scene.flash.intensity;
            PLrad[plc] = scene.flash.radius;
            plc++;
        }
        // push dans le shader d'éclairage
        lighting.use();
        glUniform1i(glGetUniformLocation(lighting.id(), "pointLightCount"), plc);
        if (plc > 0){
            glUniform3fv(glGetUniformLocation(lighting.id(),"pointLightPos"),       plc, &PLpos[0].x);
            glUniform3fv(glGetUniformLocation(lighting.id(),"pointLightColor"),     plc, &PLcol[0].x);
            glUniform1fv(glGetUniformLocation(lighting.id(),"pointLightIntensity"), plc, PLint);
            glUniform1fv(glGetUniformLocation(lighting.id(),"pointLightRadius"),plc, PLrad);
        }

        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // --- Particles & fireball (additive) ---
        glDisable(GL_CULL_FACE);                    
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
        glDepthMask(GL_FALSE);             // éviter d'écrire dans le depth

        particleShader.use();
        particleShader.setMat4("P", P);
        particleShader.setMat4("V", V);
        glBindVertexArray(particleVAO);

        // === FLASH SPRITE D'EXPLOSION (gros disque qui grossit puis s'éteint) ===
        if (scene.flash.blastTimer > 0.0f) {
            // t : 0 → 1 pendant la durée du flash
            float t = 1.0f - (scene.flash.blastTimer / scene.flash.blastDuration);

            // Taille qui part grand et croit vite (ajuste si besoin)
            float size = 3.0f + 7.0f * t;

            particleShader.setVec3("center", scene.flash.pos);
            particleShader.setFloat("size",  size);
            particleShader.setVec4("color",  glm::vec4(1.0f, 0.6f, 0.15f, 0.45f)); // orange clair

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // fireball comme un gros billboard
        scene.drawFireBall(particleShader);

        // particules
        scene.drawParticles(particleShader);

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glfwSwapBuffers(win); glfwPollEvents();

    }
    glfwTerminate();
    return 0;
}