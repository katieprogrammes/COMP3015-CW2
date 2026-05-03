#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include <glad/glad.h>
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "helper/objmesh.h"
#include "helper/plane.h"

class SceneBasic_Uniform : public Scene
{
private:
    float rotSpeed;
    float tPrev;
    float angle;
    GLSLProgram prog;
    glm::mat4 rotationMatrix;

    Plane plane;

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
