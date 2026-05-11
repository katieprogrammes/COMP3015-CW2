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
#include "helper/sphere.h"

#include "helper/frustum.h"

#include <vector>
#include <string>

class SceneBasic_Uniform : public Scene
{
private:

    GLFWwindow* window;

    float rotSpeed;
    float tPrev;
    float angle;
    GLSLProgram prog, solidProg, uiProg, particleProg, skyboxProg;
    GLuint shadowFBO, pass1Index, pass2Index, depthTex;
    int shadowMapWidth, shadowMapHeight;
    glm::mat4 lightPV, shadowBias;
    int currentPass = 0;

    GLuint textVAO = 0;
    GLuint textVBO = 0;

    GLuint skyboxVAO = 0;
    GLuint skyboxVBO = 0;
    GLuint skyboxTexture = 0;

    GLuint noiseTex;
    GLuint treeTex = 0;

    glm::mat4 rotationMatrix;

    //particles
    GLuint particleArray[2];
    GLuint feedback[2];
    GLuint posBuf[2];
    GLuint velBuf[2];
    GLuint ageBuf[2];

    int drawBuf = 1;
    int nParticles = 800;
    float particleLifetime = 3.0f;
    float particleTime = 0.0f;
    float particleDeltaT = 0.0f;

    //objects
    Plane plane;
    Sphere fairySphere;
    std::unique_ptr<ObjMesh> tree;
    std::vector<glm::vec3> treesPosition;
    std::vector<float> treesScale;
    std::vector<float> treesRotation;
    std::vector<float> treeGreenTint;

    std::vector<std::unique_ptr<ObjMesh>> shrubMeshes;

    std::vector<glm::vec3> shrubPositions;
    std::vector<float> shrubScales;
    std::vector<float> shrubRotations;
    std::vector<float> shrubGreenTint;
    std::vector<int> shrubMeshIndex;
    
    int treeCount = 120;
    int shrubCount = 300;

    //fairy movement
    std::vector<glm::vec3> fairyOrbitCenters;
    std::vector<float> fairyAngleOffsets;

    float fairyOrbitRadius = 1.2f;
    float fairyOrbitSpeed = 1.2f;
    float fairyBobAmount = 0.35f;
    float fairyBobSpeed = 2.0f;

    //camera
    float yaw = -90.0f;
    float pitch = 0.0f;

    float lastX = 400.0f;
    float lastY = 300.0f;
    bool firstMouse = true;

    float deltaTime = 0.0f;

    glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, 6.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    Frustum lightFrustum;

    int samplesU, samplesV;
    int jitterMapSize;
    float radius;

    bool dustActive = false;
    glm::vec3 dustPosition = glm::vec3(0.0f);
    float dustTimer = 0.0f;

    //game logic
    std::vector<glm::vec3> fairyPositions;
    std::vector<bool> fairyCollected;
    std::vector<glm::vec3> evilFairyPositions;
    std::vector<glm::vec3> evilFairyOrbitCenters;
    std::vector<float> evilFairyAngleOffsets;

    int totalEvilFairies = 5;

    float corruptionTimer = 0.0f;
    float corruptionDuration = 2.0f;

    void updateEvilFairyOrbits(float t);
    void checkEvilFairyCollision();

    int score = 0;
    int totalFairies = 8;
    bool gameWon = false;

    void setMatrices();
    void compile();

    void setupFBO();
    void drawScene();
    void spitOutDepthBuffer();
    
    float jitter();
    void buildJitterTex();

    void initParticleBuffers();
    void renderFairyDust(const glm::vec3& fairyPos);
    void resetFairyDust();


    void initSkybox();
    GLuint loadCubemap(const std::vector<std::string>& faces);
    void renderSkybox();

    void updateFairyOrbits(float t);

    void initText();
    void renderText();
    void drawText(const std::string& text, float x, float y, float scale, const glm::vec4& color);

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
