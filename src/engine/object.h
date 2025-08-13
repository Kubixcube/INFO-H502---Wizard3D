#ifndef OBJECT_H
#define OBJECT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include "texture.h"
#include "reactphysics3d/body/RigidBody.h"

class Object{
public:
    GLuint VAO{0}, VBO{0};
    GLsizei numVertices{0};
    glm::mat4 model{1.0f};
    // for rep3d collision
    // Axis-aligned bounding boxes
    // https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
    glm::vec3 halfExtents;
    glm::vec3 aabbCenter;
    reactphysics3d::RigidBody* body = nullptr;
    Texture texture;
    Object() = default;
    explicit Object(const char* path){ load(path); }

    template<class T>
    explicit Object(const char* objPath, T texPath) : Object(objPath){ bindTexture(texPath); }

    template<class T>
    void bindTexture(T path) {
     texture = Texture{path};
    }
    static bool parseFaceItem(const std::string& s, int& vi, int& ti, int& ni) {
        vi = ti = ni = 0;
        // Try v/t/n
        if (std::sscanf(s.c_str(), "%d/%d/%d", &vi, &ti, &ni) == 3) return true;
        // Try v//n
        if (std::sscanf(s.c_str(), "%d//%d", &vi, &ni) == 2) { ti = 0; return true; }
        // Try v/t
        if (std::sscanf(s.c_str(), "%d/%d", &vi, &ti) == 2) { ni = 0; return true; }
        // Try v
        if (std::sscanf(s.c_str(), "%d", &vi) == 1) { ti = 0; ni = 0; return true; }
        return false;
    }

    void load(const char* path){
        std::vector<glm::vec3> P,N;
        std::vector<glm::vec2> T;
        std::vector<float> buf;

        std::ifstream in(path);
        if(!in.is_open()){
            std::cerr << "Failed to open OBJ: " << path << std::endl;
            return;
        }

        std::string line;
        while(std::getline(in, line)){
            if(line.empty() || line[0]=='#') continue;
            std::istringstream ss(line);
            std::string tag; ss >> tag;

            if(tag=="v"){ glm::vec3 p; ss >> p.x >> p.y >> p.z; P.push_back(p); }
            else if(tag=="vt"){ glm::vec2 u; ss >> u.x >> u.y; T.push_back(u); }
            else if(tag=="vn"){ glm::vec3 n; ss >> n.x >> n.y >> n.z; N.push_back(n); }
            else if(tag=="f"){
                // Collecte tous les sommets de la face
                std::vector<std::tuple<int,int,int>> face;
                std::string vtn;
                while (ss >> vtn) {
                    int vi,ti,ni;
                    if (parseFaceItem(vtn, vi, ti, ni)) {
                        face.emplace_back(vi, ti, ni);
                    }
                }
                if (face.size() < 3) continue;
                // Triangulation en éventail: (0, i, i+1)
                for (size_t i = 1; i + 1 < face.size(); ++i) {
                    size_t idx[3] = {0, i, i+1};
                    for (int k=0;k<3;k++){
                        auto [vi, ti, ni] = face[idx[k]];
                        const glm::vec3& p = P.at(vi - 1);
                        const glm::vec2  u = (ti>0 ? T.at(ti - 1) : glm::vec2(0.0f));
                        const glm::vec3  n = (ni>0 ? N.at(ni - 1) : glm::vec3(0.0f,1.0f,0.0f));
                        buf.insert(buf.end(), {p.x,p.y,p.z, n.x,n.y,n.z, u.x,u.y});
                    }
                }
            }
        }
        // AABB and Half-Extents Calculation
        if (!P.empty()) {
//            glm::vec3 minBounds = P[0];
//            glm::vec3 maxBounds = P[0];
            glm::vec3 minBounds(std::numeric_limits<float>::max());
            glm::vec3 maxBounds(-std::numeric_limits<float>::max());
            for (const auto& pos : P) {
                minBounds.x = std::min(minBounds.x, pos.x);
                minBounds.y = std::min(minBounds.y, pos.y);
                minBounds.z = std::min(minBounds.z, pos.z);

                maxBounds.x = std::max(maxBounds.x, pos.x);
                maxBounds.y = std::max(maxBounds.y, pos.y);
                maxBounds.z = std::max(maxBounds.z, pos.z);
            }
            halfExtents = (maxBounds - minBounds) * 0.5f;
            halfExtents.x = halfExtents.x != 0 ? halfExtents.x : 0.1f;
            halfExtents.y = halfExtents.y != 0 ? halfExtents.y : 0.1f;
            halfExtents.z = halfExtents.z != 0 ? halfExtents.z : 0.1f;
            std::cout << "[OBJ] " << path << ": " << halfExtents.x << " | " << halfExtents.y << " | " << halfExtents.z << std::endl;
            aabbCenter =  0.5f * (minBounds + maxBounds);;
        }
        else {
            // Fallback for an empty or invalid model
            std::cout << "Invalid model" << std::endl;
            this->halfExtents = glm::vec3(0.5f);
            this->aabbCenter = glm::vec3(0.0f);
        }

        numVertices = (GLsizei)(buf.size() / 8);
        std::cout << "[OBJ] " << path << " -> vertices: " << numVertices << std::endl;

        if (numVertices == 0) {
            std::cerr << "[OBJ] WARNING: no vertices parsed, nothing will be drawn.\n";
        }

        if(!VAO) glGenVertexArrays(1, &VAO);
        if(!VBO) glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(float), buf.data(), GL_STATIC_DRAW);

        GLsizei stride = 8 * sizeof(float);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*) 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,(void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

    }
    void translate(glm::vec3 pos){
        model = glm::translate(glm::mat4(1.0f), pos);
        if (body) {
            // get the current transform from the physics body.
            reactphysics3d::Transform currentTransform = body->getTransform();
            // set the position of the transform to the one from your input.
            currentTransform.setPosition(reactphysics3d::Vector3(pos.x, pos.y, pos.z));
            // set the new transform on the rigid body.
            body->setTransform(currentTransform);
        }
    }

    void scale(glm::vec3 scaling) {
        model = glm::scale(model, scaling);
        halfExtents *= scaling;
    }
    void draw() const {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, numVertices);
        glBindVertexArray(0);
    }
};

#endif
