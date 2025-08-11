#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "shader.h"
#include "camera.h"
#include "../DynamicObject.h"
#include <iostream>
#include "../scene.h"

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f),
              glm::vec3(0.0f, 1.0f, 0.0f),
              -90.0f, 0.0f);

bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

float deltaTime = 0.0f; 
float lastFrame = 0.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // inversé car y va vers le bas

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}


void processInput(GLFWwindow *window) {
    float cameraSpeed = 2.5f * deltaTime;

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard('Z', deltaTime);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard('S', deltaTime);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard('Q', deltaTime);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard('D', deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
inline glm::vec3 calculateMinimumTranslationVector(const AABB& boxA, const AABB& boxB) {
    float overlapX = std::min(boxA.max.x - boxB.min.x, boxB.max.x - boxA.min.x);
    float overlapY = std::min(boxA.max.y - boxB.min.y, boxB.max.y - boxA.min.y);
    float overlapZ = std::min(boxA.max.z - boxB.min.z, boxB.max.z - boxA.min.z);

    glm::vec3 centerA = 0.5f * (boxA.min + boxA.max);
    glm::vec3 centerB = 0.5f * (boxB.min + boxB.max);

    if (overlapX <= overlapY && overlapX <= overlapZ)
        return glm::vec3((centerA.x < centerB.x) ? -overlapX : overlapX, 0, 0);

    if (overlapY <= overlapX && overlapY <= overlapZ)
        return glm::vec3(0, (centerA.y < centerB.y) ? -overlapY : overlapY, 0);

    return glm::vec3(0, 0, (centerA.z < centerB.z) ? -overlapZ : overlapZ);
}

inline void resolveObjectCollision(Object& objectA, Object& objectB) {
    AABB boundsA = objectA.worldAABB();
    AABB boundsB = objectB.worldAABB();

    if (!AABB::collisionCheck(boundsA, boundsB))
        return;

    glm::vec3 mtv = calculateMinimumTranslationVector(boundsA, boundsB);
    if (mtv == glm::vec3(0))
        return;

    DynamicObject* dynamicA = dynamic_cast<DynamicObject*>(&objectA);
    DynamicObject* dynamicB = dynamic_cast<DynamicObject*>(&objectB);
    glm::vec3 collisionNormal = glm::normalize(mtv);

    if (dynamicA && !dynamicB) {
        dynamicA->posistion += mtv;
        dynamicA->model = glm::translate(glm::mat4(1.0f), dynamicA->posistion);

        float velocityAlongNormalA = glm::dot(dynamicA->mouvement, collisionNormal);
        if (velocityAlongNormalA < 0.0f)
            dynamicA->mouvement -= velocityAlongNormalA * collisionNormal;
    }
    else if (!dynamicA && dynamicB) {
        dynamicB->posistion -= mtv;
        dynamicB->model = glm::translate(glm::mat4(1.0f), dynamicB->posistion);

        float velocityAlongNormalB = glm::dot(dynamicB->mouvement, collisionNormal);
        if (velocityAlongNormalB > 0.0f)
            dynamicB->mouvement -= velocityAlongNormalB * collisionNormal;
    }
    else if (dynamicA && dynamicB) {
        dynamicA->posistion += 0.5f * mtv;
        dynamicB->posistion -= 0.5f * mtv;
        dynamicA->model = glm::translate(glm::mat4(1.0f), dynamicA->posistion);
        dynamicB->model = glm::translate(glm::mat4(1.0f), dynamicB->posistion);

        float velocityAlongNormalA = glm::dot(dynamicA->mouvement, collisionNormal);
        if (velocityAlongNormalA < 0.0f)
            dynamicA->mouvement -= velocityAlongNormalA * collisionNormal;

        float velocityAlongNormalB = glm::dot(dynamicB->mouvement, collisionNormal);
        if (velocityAlongNormalB > 0.0f)
            dynamicB->mouvement -= velocityAlongNormalB * collisionNormal;
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lighting Cube", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    // create shader
    Shader shader("../src/shaders/light.vert", "../src/shaders/light.frag");
    shader.use();
    shader.setInt("texture1", 0);
    shader.setVec2("tiling", 1.0f, 1.0f); // default tiling (can change per object)
    // setting up the scene
    Scene scene("fire range");
    // load OBJ via Object class
    // dynamic objects
    auto cube = std::make_shared<DynamicObject>("../src/assets/objects/cube.obj", 5.0f);
    //DynamicObject cube("../src/assets/objects/cube.obj", 5.0f);
    cube->bindTexture("../src/assets/textures/container.jpg");
    cube->setPos(glm::vec3(0.0f, 100.0f, 0.0f));
    cube->makeObject(shader, true);
    scene.addNewDynamicEntity(cube);
    //static objects
    //cube.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f)); // default
    auto floor = std::make_shared<Object>("../src/assets/objects/plane.obj");
    floor->bindTexture("../src/assets/textures/green-fake-grass-background.jpg");
    floor->makeObject(shader, true);
    scene.addNewStaticEntity(floor);
    // --- main loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        cube->updatePos(deltaTime);
        resolveObjectCollision(*cube, *floor);
        processInput(window);
        shader.use();
        cube->model = glm::translate(glm::mat4(1.0f), cube->posistion);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        shader.setMat4("view", glm::value_ptr(view));
        shader.setMat4("projection", glm::value_ptr(proj));
        shader.setVec3("lightDir", -0.2f, -1.0f, -0.3f);
        shader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        shader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);

        floor->model = glm::mat4(1.0f);
        floor->model = glm::translate(floor->model, glm::vec3(0.0f, -1.0f, 0.0f));
        floor->model = glm::scale(floor->model, glm::vec3(10.0f, 1.0f, 10.0f));

        shader.setMat4("model", glm::value_ptr(floor->model));
        shader.setVec2("tiling", 8.0f, 8.0f);
        floor->draw();

        shader.setMat4("model", glm::value_ptr(cube->model));
        shader.setVec2("tiling", 4.0f, 4.0f);
        cube->draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup & terminate
    glfwTerminate();
    return 0;
}
