
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


#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

// ---- Projectile & particules ----
struct Fireball {
    bool active=false;
    glm::vec3 pos{0};
    glm::vec3 vel{0};
    float radius=0.30f;
    float t=0.00f;
};

struct Particle {
    bool alive=false;
    glm::vec3 pos{0}, vel{0};
    float life=0.0f, maxLife=0.0f;
    float size=0.0f, size0=0.0f;   // <- nouveaux champs
    glm::vec4 color{1,0.5,0,1}; // orange
};

struct ExplosionLight {
    bool active=false;
    glm::vec3 pos{0};
    float intensity=0.0f;
    float radius=0.0f;
    float decay=0.0f; // /sec
};

// Globals
Fireball fireball;
ExplosionLight flash;
std::vector<Particle> particles;

// === Flash sprite d'explosion ===
static float     blastTimer = 0.0f;      // (s)
static const float blastDuration = 0.18f;
static glm::vec3 blastPos = glm::vec3(0.0f);

// Quad VAO pour billboard
GLuint particleVAO=0, particleVBO=0;

static float randf(float a, float b){ return a + (b-a) * (float(rand())/float(RAND_MAX)); }

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

// AABB du cube à partir de son modèle (unit cube [-0.5,0.5]^3)
static void aabbFromModel(const glm::mat4& M, glm::vec3& mn, glm::vec3& mx){
    glm::vec3 c[8] = {
        {-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,0.5f,-0.5f},{-0.5f,0.5f,-0.5f},
        {-0.5f,-0.5f, 0.5f},{0.5f,-0.5f, 0.5f},{0.5f,0.5f, 0.5f},{-0.5f,0.5f, 0.5f}
    };
    mn = glm::vec3( 1e9f); mx = glm::vec3(-1e9f);
    for (int i=0;i<8;++i){
        glm::vec3 w = glm::vec3(M * glm::vec4(c[i],1));
        mn = glm::min(mn, w);
        mx = glm::max(mx, w);
    }
}

static bool sphereAABB(const glm::vec3& p, float r, const glm::vec3& mn, const glm::vec3& mx){
    glm::vec3 q = glm::clamp(p, mn, mx);
    glm::vec3 d = p - q;
    return glm::dot(d,d) <= r*r;
}


static int SCR_WIDTH=1280, SCR_HEIGHT=720;
Camera camera(glm::vec3(0.0f,1.5f,5.0f));
float lastX=SCR_WIDTH*0.5f, lastY=SCR_HEIGHT*0.5f; bool firstMouse=true;
bool useThirdPerson=true;
glm::vec3 playerPos(0.0f);
glm::vec3 playerRot(0.0f);

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
void processInput(GLFWwindow* win,float dt){
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
            playerPos += movementDir * speed;
        }
        

    } else {
        if(glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS) camera.ProcessKeyboard(FORWARD,dt);
        if(glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS) camera.ProcessKeyboard(BACKWARD,dt);
        if(glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS) camera.ProcessKeyboard(LEFT,dt);
        if(glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS) camera.ProcessKeyboard(RIGHT,dt);
        if(glfwGetKey(win,GLFW_KEY_SPACE)==GLFW_PRESS) camera.ProcessKeyboard(UP,dt);
        if(glfwGetKey(win,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) camera.ProcessKeyboard(DOWN,dt);
    }

    static bool prevClick=false;
    bool click = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    if (click && !prevClick && !fireball.active){
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
        glm::vec3 muzzle = playerPos + glm::vec3(0,1.6f,0) + fwd*0.3f;

        fireball.active = true;
        fireball.pos = muzzle;
        fireball.vel = fwd * 10.0f; // vitesse
        fireball.t   = 0.0f;
    }
    prevClick = click;

}
//GLuint loadTexture2D(const char* path){
//    int w,h,c;
//    stbi_set_flip_vertically_on_load(true);
//    unsigned char* d=stbi_load(path,&w,&h,&c,0);
//    if(!d){ unsigned char white[3]={255,255,255}; GLuint t; glGenTextures(1,&t);
//        glBindTexture(GL_TEXTURE_2D,t);
//        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,1,1,0,GL_RGB,GL_UNSIGNED_BYTE,white);
//        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
//        glGenerateMipmap(GL_TEXTURE_2D);
//        return t;
//    }
//    GLenum fmt=(c==4)?GL_RGBA:GL_RGB; GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
//    glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,d);
//    glGenerateMipmap(GL_TEXTURE_2D);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
//    stbi_image_free(d);
//    return t;
//}

//// --- Cubemap loader ---
//GLuint loadCubemap(const std::vector<std::string>& faces) {
//    int w,h,ch;
//    stbi_set_flip_vertically_on_load(false);
//    GLuint texID;
//    glGenTextures(1, &texID);
//    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
//    for (unsigned int i=0; i<faces.size(); ++i) {
//        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &ch, 3);
//        if (data) {
//            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
//                         GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
//            stbi_image_free(data);
//        } else {
//            unsigned char fallback[3] = { 20u, 20u, 20u };
//            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
//                         GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fallback);
//        }
//    }
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//    return texID;
//}

// --- VAO skybox ---
//void makeSkyboxVAO(GLuint &vao, GLuint &vbo){
//    static const float skyboxVertices[] = {
//        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
//         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
//        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
//        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
//         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
//         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
//        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
//         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
//        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
//         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
//        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
//         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
//    };
//    glGenVertexArrays(1, &vao);
//    glGenBuffers(1, &vbo);
//    glBindVertexArray(vao);
//    glBindBuffer(GL_ARRAY_BUFFER, vbo);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
//    glBindVertexArray(0);
//}

int main(){
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win=glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Wizard3D",NULL,NULL); if(!win){ glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win,framebuffer_size_callback);
    glfwSetCursorPosCallback(win,mouse_callback);
    glfwSetScrollCallback(win,scroll_callback);
    glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    Shader sky("src/shaders/skybox.vert", "src/shaders/skybox.frag");

   // GLuint skyVAO=0, skyVBO=0;
    // makeSkyboxVAO(skyVAO, skyVBO);

    std::vector<std::string> faces = {
        "assets/cubemaps/sky/posx.jpg",  // +X
        "assets/cubemaps/sky/negx.jpg",   // -X
        "assets/cubemaps/sky/posy.jpg",    // +Y
        "assets/cubemaps/sky/negy.jpg", // -Y
        "assets/cubemaps/sky/posz.jpg",  // +Z
        "assets/cubemaps/sky/negz.jpg"    // -Z
    };
    // GLuint cubemapTex = loadCubemap(faces);

    Shader lighting("src/shaders/lighting.vert","src/shaders/lighting.frag");

    Shader env("src/shaders/envmap.vert", "src/shaders/envmap.frag");

    Shader particleShader("src/shaders/particles.vert", "src/shaders/particles.frag");
    makeParticleQuad();
    particles.resize(400); // pool simple
    srand((unsigned)time(nullptr));

    float reflectMix = 0.6f;
    Scene scene{"Firing range"};
//    scene.addEntity(floor, 0.0f, true);
    Object fallingCube{"assets/models/cube.obj", "assets/textures/container.jpg"};
    fallingCube.translate({0.0f, 5.0f, 0.0f});
    scene.addEntity(fallingCube, 50.0f, false);
//    Object wizard("assets/models/wizard.obj","assets/textures/wizard_diffuse.png");
//    wizard = scene.addEntity(wizard, 150.0f, false);
    Object cube  ("assets/models/cube.obj");
    glm::vec3 lightDir=glm::normalize(glm::vec3(-0.3f,-1.0f,-0.4f));
    glm::vec3 lightColor(1.0f), ambient(0.10f), specular(0.0f); float shininess=32.0f;
    cube.model=glm::translate(glm::mat4(1.0f), glm::vec3(0,0.5f,-2.0f));
    float last= (float) glfwGetTime();
    while(!glfwWindowShouldClose(win)){
        float t =(float)glfwGetTime();
        float dt = t-last;
        last=t;
        processInput(win,dt);
        // wizard movement
        scene.player.translate(playerPos);
        scene.player.rotate(playerRot);

        scene.update(dt);





        glm::mat4 P= glm::perspective(glm::radians(camera.Zoom),(float)SCR_WIDTH/(float)SCR_HEIGHT,0.1f,100.0f);
        glm::mat4 V= useThirdPerson ? thirdPersonView(): camera.GetViewMatrix();





        glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);
        glClearColor(0.05f,0.07f,0.09f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
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
        lighting.setMat4("M", scene.floor.model);
        scene.floor.texture.map();
        scene.floor.draw();
        for (const auto& entity : scene.getEntities()) {
            lighting.setMat4("M", entity.model);
            entity.texture.map();
            entity.draw();
        }
        env.use();
        env.setMat4("P", P);
        env.setMat4("V", V);
        env.setMat4("M", cube.model);
        env.setVec3("viewPos", eye);
        env.setFloat("mixFactor", reflectMix);
        env.setVec3("tint", glm::vec3(1.0f));
        scene.skybox.texture.map();
        glUniform1i(glGetUniformLocation(env.id(), "skybox"), 0);
        glFrontFace(GL_CW);
        cube.texture.map();
        cube.draw();
        glFrontFace(GL_CCW);
        lighting.use();
        lighting.setMat4("P", P);
        lighting.setMat4("V", V);
        lighting.setMat4("M", scene.player.model);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, texWizard);
//        wizard.texture.map();
        scene.player.texture.map();
        glUniform1i(glGetUniformLocation(lighting.id(),"diffuseMap"),0);
//        wizard.draw();
        scene.player.draw();




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



//        glEnable(GL_CULL_FACE);

        // === UPDATE FIREBALL & PARTICLES ===
        glm::vec3 gravity(0,-2.0f,0);

        if (fireball.active){
            fireball.t += dt;

            // 1) Intégration avec positions prev/next
            const glm::vec3 prev = fireball.pos;
            fireball.vel += gravity * dt;
            const glm::vec3 next = prev + fireball.vel * dt;
            const float r = fireball.radius;

            // 2) Collision sol (y=0) - "swept" anti-tunneling
            const float y0 = prev.y - r;
            const float y1 = next.y - r;
            const bool groundHit = (y0 > 0.0f && y1 <= 0.0f) || (y0 <= 0.0f && y1 <= 0.0f);

            // 3) (optionnel) Collision AABB cube démo
            bool aabbHit = false;
            {
                glm::vec3 mn, mx; aabbFromModel(cube.model, mn, mx);
                aabbHit = sphereAABB(next, r, mn, mx);
            }

            if (groundHit || aabbHit){
                // temps d'impact le long du segment prev->next (pour le sol)
                float tHit = 1.0f;
                if (groundHit){
                    const float denom = (y0 - y1);
                    tHit = denom != 0.0f ? (y0 - 0.0f) / denom : 0.0f;
                    tHit = glm::clamp(tHit, 0.0f, 1.0f);
                }
                fireball.pos = prev + tHit * (next - prev);
                if (fireball.pos.y < r) fireball.pos.y = r; // poser sur le plan

                // EXPLOSION (flash + burst)
                flash.active    = true;
                flash.pos       = fireball.pos;
                flash.radius    = 18.0f;   
                flash.intensity = 14.0f;   
                flash.decay     = 7.0f;    

                blastTimer = blastDuration;
                blastPos   = fireball.pos;


                // en global (une seule fois)
                static size_t particleCursor = 0;

                // à la place du for (int i=0;i<140;i++){ ... find_if ... }
                constexpr int EXP_COUNT = 50000;
                for (int i = 0; i < EXP_COUNT; ++i) {
                    Particle& p = particles[(particleCursor + i) % particles.size()];
                    p.alive = true;
                    p.pos   = fireball.pos + glm::vec3(0, 0.02f, 0);
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

                // puis désactive le projectile
                fireball.active = false;
                fireball.vel = glm::vec3(0.0f);
            } else {
                // pas de collision : avancer + fumée
                fireball.pos = next;

                for (int i=0;i<4;i++){
                    auto it = std::find_if(particles.begin(), particles.end(), [](const Particle& p){return !p.alive;});
                    if (it != particles.end()){
                        it->alive = true;
                        it->pos   = fireball.pos;
                        it->vel   = -0.2f*glm::normalize(fireball.vel) + glm::vec3(randf(-0.5f,0.5f), randf(-0.2f,0.6f), randf(-0.5f,0.5f))*0.5f;
                        it->life  = randf(0.25f, 0.45f);
                        it->size  = randf(0.18f, 0.28f);
                        it->color = glm::vec4(1.0f, randf(0.3f,0.6f), 0.0f, 0.9f);
                    }
                }
            }
        }



        // update particules
        for (auto& p : particles){
            if (p.alive) {
                p.life -= dt;
                if (p.life <= 0.0f) { p.alive = false; continue; }

                float t = 1.0f - (p.life / p.maxLife); // 0 au début → 1 à la fin
                p.size  = p.size0 * (1.0f + 1.8f * t); // grossit jusqu’à ~2.8x
                p.color.a = std::max(0.0f, (1.0f - t) * 0.95f); // fade-out

                // expansion “sphérique” plus lisible
                p.pos  += p.vel * dt;
                p.vel  += glm::vec3(0.0f, -9.8f, 0.0f) * 0.15f * dt; // gravité légère
                p.vel  *= (1.0f - 0.4f * dt); // “air drag” pour figer la forme
            }
        }

        // flash light decay
        if (flash.active){
            flash.intensity -= flash.decay * dt;
            if (flash.intensity <= 0) flash.active=false;
        }

        // --- point lights dynamiques (fireball / explosion) ---
        int plc = 0;
        glm::vec3 PLpos[4]; glm::vec3 PLcol[4];
        float PLint[4]; float PLrad[4];

        // lumière portée par la boule (si tu veux un halo pendant le vol)
        if (fireball.active && plc < 4){
            PLpos[plc] = fireball.pos;
            PLcol[plc] = glm::vec3(1.0f, 0.6f, 0.2f);
            PLint[plc] = 1.5f;
            PLrad[plc] = 3.0f;
            plc++;
        }

        // flash d'explosion
        if (flash.active && plc < 4){
            PLpos[plc] = flash.pos;
            PLcol[plc] = glm::vec3(1.0f, 0.5f, 0.1f);
            PLint[plc] = flash.intensity;
            PLrad[plc] = flash.radius;
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
        if (blastTimer > 0.0f) {
            // t : 0 → 1 pendant la durée du flash
            float t = 1.0f - (blastTimer / blastDuration);

            // Taille qui part grand et croit vite (ajuste si besoin)
            float size = 3.0f + 7.0f * t;

            particleShader.setVec3("center", blastPos);
            particleShader.setFloat("size",  size);
            particleShader.setVec4("color",  glm::vec4(1.0f, 0.6f, 0.15f, 0.45f)); // orange clair

            glDrawArrays(GL_TRIANGLES, 0, 6);

            // décrémente le timer (tu peux le faire ici ou dans l'update)
            blastTimer -= dt;
        }

        // fireball comme un gros billboard
        if (fireball.active){
            particleShader.setVec3("center", fireball.pos);
            particleShader.setFloat("size", 0.45f);
            particleShader.setVec4("color", glm::vec4(1.0f, 0.4f, 0.05f, 0.9f));
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // particules
        for (const auto& p : particles){
            if (!p.alive) continue;
            particleShader.setVec3("center", p.pos);
            particleShader.setFloat("size", p.size);
            particleShader.setVec4("color", p.color);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);



        glfwSwapBuffers(win); glfwPollEvents();

    }
    glfwTerminate();
    return 0;
}


