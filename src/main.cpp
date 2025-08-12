
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

static int SCR_WIDTH=1280, SCR_HEIGHT=720;
Camera camera(glm::vec3(0.0f,1.5f,5.0f));
float lastX=SCR_WIDTH*0.5f, lastY=SCR_HEIGHT*0.5f; bool firstMouse=true;
bool useThirdPerson=true; glm::vec3 playerPos(0.0f); float orbitYaw=0.0f, orbitPitch=15.0f, camDist=5.0f;
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
    float speed=3.0f*dt;
    if(glfwGetKey(win,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(win,true);
    if(glfwGetKey(win,GLFW_KEY_C)==GLFW_PRESS) useThirdPerson=true;
    if(glfwGetKey(win,GLFW_KEY_V)==GLFW_PRESS) useThirdPerson=false;
    if(useThirdPerson){
        float yR=glm::radians(orbitYaw); glm::vec3 f=glm::normalize(glm::vec3(cos(yR),0,sin(yR)));
        glm::vec3 r=glm::normalize(glm::cross(f,glm::vec3(0,1,0)));
        if(glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS) playerPos+=f*speed;
        if(glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS) playerPos-=f*speed;
        if(glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS) playerPos-=r*speed;
        if(glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS) playerPos+=r*speed;
    } else {
        if(glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS) camera.ProcessKeyboard(FORWARD,dt);
        if(glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS) camera.ProcessKeyboard(BACKWARD,dt);
        if(glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS) camera.ProcessKeyboard(LEFT,dt);
        if(glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS) camera.ProcessKeyboard(RIGHT,dt);
        if(glfwGetKey(win,GLFW_KEY_SPACE)==GLFW_PRESS) camera.ProcessKeyboard(UP,dt);
        if(glfwGetKey(win,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) camera.ProcessKeyboard(DOWN,dt);
    }
}
GLuint loadTexture2D(const char* path){
    int w,h,c;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* d=stbi_load(path,&w,&h,&c,0);
    if(!d){ unsigned char white[3]={255,255,255}; GLuint t; glGenTextures(1,&t);
        glBindTexture(GL_TEXTURE_2D,t);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,1,1,0,GL_RGB,GL_UNSIGNED_BYTE,white);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        return t;
    }
    GLenum fmt=(c==4)?GL_RGBA:GL_RGB; GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,d);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(d);
    return t;
}

// --- Cubemap loader ---
GLuint loadCubemap(const std::vector<std::string>& faces) {
    int w,h,ch;
    stbi_set_flip_vertically_on_load(false);
    GLuint texID; glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
    for (unsigned int i=0; i<faces.size(); ++i) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &ch, 3);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                         GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            unsigned char fallback[3] = { 20u, 20u, 20u };
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                         GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fallback);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return texID;
}

// --- VAO skybox ---
void makeSkyboxVAO(GLuint &vao, GLuint &vbo){
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
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glBindVertexArray(0);
}

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

    GLuint skyVAO=0, skyVBO=0;
    makeSkyboxVAO(skyVAO, skyVBO);

    std::vector<std::string> faces = {
        "assets/cubemaps/sky/posx.jpg",  // +X
        "assets/cubemaps/sky/negx.jpg",   // -X
        "assets/cubemaps/sky/posy.jpg",    // +Y
        "assets/cubemaps/sky/negy.jpg", // -Y
        "assets/cubemaps/sky/posz.jpg",  // +Z
        "assets/cubemaps/sky/negz.jpg"    // -Z
    };
    GLuint cubemapTex = loadCubemap(faces);

    Shader lighting("src/shaders/lighting.vert","src/shaders/lighting.frag");

    Shader env("src/shaders/envmap.vert", "src/shaders/envmap.frag");
    float reflectMix = 0.6f;



    Object wizard("assets/models/wizard.obj","assets/textures/wizard_diffuse.png");
    Object cube  ("assets/models/cube.obj");
//    GLuint texWizard=loadTexture2D("assets/textures/wizard_diffuse.png");
//    GLuint texCube = loadTexture2D("assets/textures/container.jpg");
    glm::vec3 lightDir=glm::normalize(glm::vec3(-0.3f,-1.0f,-0.4f));
    glm::vec3 lightColor(1.0f), ambient(0.10f), specular(0.0f); float shininess=32.0f;
    cube.model=glm::translate(glm::mat4(1.0f), glm::vec3(0,0.5f,-2.0f));
    float last= (float) glfwGetTime();
    while(!glfwWindowShouldClose(win)){
        float t =(float)glfwGetTime();
        float dt = t-last;
        last=t;
        processInput(win,dt);
        glm::mat4 P= glm::perspective(glm::radians(camera.Zoom),(float)SCR_WIDTH/(float)SCR_HEIGHT,0.1f,100.0f);
        glm::mat4 V= useThirdPerson ? thirdPersonView(): camera.GetViewMatrix();
        wizard.model= glm::translate(glm::mat4(1.0f), playerPos);
        glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);
        glClearColor(0.05f,0.07f,0.09f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        lighting.use();
        lighting.setMat4("P",P); lighting.setMat4("V",V);
        glm::vec3 eye = useThirdPerson ? glm::vec3(glm::inverse(V)[3]) : camera.Position;
        lighting.setVec3("viewPos", eye);
        lighting.setVec3("lightDir", lightDir);
        lighting.setVec3("lightColor", lightColor);
        lighting.setVec3("ambientColor", ambient);
        lighting.setVec3("specularColor", specular);
        lighting.setFloat("shininess", shininess);

        env.use();
        env.setMat4("P", P);
        env.setMat4("V", V);
        env.setMat4("M", cube.model);
        env.setVec3("viewPos", eye);
        env.setFloat("mixFactor", reflectMix);
        env.setVec3("tint", glm::vec3(1.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
        glUniform1i(glGetUniformLocation(env.id(), "skybox"), 0);
        cube.draw();

        lighting.use();
        lighting.setMat4("P", P);
        lighting.setMat4("V", V);
        lighting.setMat4("M", wizard.model);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, texWizard);
        glUniform1i(glGetUniformLocation(lighting.id(),"diffuseMap"),0);
        wizard.draw();

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        sky.use();
        sky.setMat4("P", P);
        sky.setMat4("V", V);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
        glUniform1i(glGetUniformLocation(sky.id(), "skybox"), 0);

        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(win); glfwPollEvents();

    }
    glfwTerminate();
    return 0;
}
