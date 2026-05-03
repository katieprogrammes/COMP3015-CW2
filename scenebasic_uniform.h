#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include <glad/glad.h>
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "helper/objmesh.h"
#include "helper/plane.h"

class SceneBasic_Uniform : public Scene
{
private:

    GLFWwindow* window;

    float rotSpeed;
    float tPrev;
    float angle;
    GLSLProgram prog;
    glm::mat4 rotationMatrix;

    //objects
    Plane plane;
    std::unique_ptr<ObjMesh> tree;
    std::vector<glm::vec3> treesPosition;
    std::vector<float> treesScale;
    std::vector<float> treesRotation;
    std::vector<float> treeGreenTint;

    int treeCount = 40;

    //camera
    float yaw = -90.0f;
    float pitch = 0.0f;

    float lastX = 400.0f;
    float lastY = 300.0f;
    bool firstMouse = true;

    glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, 6.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);


    void compile();
    void setMatrices();


public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
