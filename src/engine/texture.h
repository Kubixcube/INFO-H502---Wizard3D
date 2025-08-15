#ifndef WIZARD3D_TEXTURE_H
#define WIZARD3D_TEXTURE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>
#include <vector>
#include "stb_image.h"
class Texture {
    enum class Type {
        SIMPLE_TEX,
        CUBE,
    };
public:
    GLuint ID;
    Type type;
    glm::vec2 tiling{1.0f};
    Texture() = default;
    Texture(std::vector<std::string> faces);
    Texture(const char* path);
    void map() const;

    GLuint id() const { return ID; }  // ou getID()

private:
    int width, height, channels;
};

#endif //WIZARD3D_TEXTURE_H
