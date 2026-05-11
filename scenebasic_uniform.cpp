#include "scenebasic_uniform.h"

#include <cstdio>
#include <cstdlib>

#include <sstream>
using std::string;

#include <iostream>
using std::cerr;
using std::endl;

#define STB_EASY_FONT_IMPLEMENTATION
#include "helper/stb_easy_font.h"
#include "helper/stb/stb_image.h"
#include "helper/glutils.h"
#include "helper/texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include "helper/particleutils.h"
#include "helper/noisetex.h"
#include "helper/skybox.h"

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;


float randomFloat(float min, float max)
{
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return min + r * (max - min);
}

SceneBasic_Uniform::SceneBasic_Uniform() : 
    tPrev(0), 
    plane(250.0f, 250.0f, 1, 1), 
    fairySphere(0.2f, 16, 16), 
    angle(90.0f), 
    rotSpeed(glm::pi<float>()/16.0f), 
    drawBuf(1), 
    nParticles(175),
    particleLifetime(6.0f),
    particleTime(0.0f),
    particleDeltaT(0.0f)
{
    shadowMapWidth = 1024;
    shadowMapHeight = 1024;

    samplesU = 4;
    samplesV = 8;
    jitterMapSize = 8;
    radius = 7.0f;

    treeMeshes.push_back(ObjMesh::load("media/Tree5.obj", true));
    treeMeshes.push_back(ObjMesh::load("media/Tree4.obj", true));
    treeMeshes.push_back(ObjMesh::load("media/Tree2.obj", true));
    treeMeshes.push_back(ObjMesh::load("media/Tree9.obj", true));
    treeMeshes.push_back(ObjMesh::load("media/Tree7.obj", true));

    shrubMeshes.push_back(ObjMesh::load("media/shrub1.obj", true));
    shrubMeshes.push_back(ObjMesh::load("media/shrub2.obj", true));
    shrubMeshes.push_back(ObjMesh::load("media/monstera.obj", true));
    shrubMeshes.push_back(ObjMesh::load("media/elephant.obj", true));
}

void SceneBasic_Uniform::initScene()
{
    compile();
    
    glClearColor(0.0, 0.0, 0.1, 1.0); //setup background colour
    glEnable(GL_DEPTH_TEST);

    initSkybox();

    initText();

    view = glm::lookAt(vec3(0.0f, 4.0f, 6.0f), vec3(0.0f, 2.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = mat4(1.0f);

    angle = 0.0f;

    setupFBO();
    buildJitterTex();
    noiseTex = NoiseTex::generatePeriodic2DTex(4.0f, 0.5f, 256, 256);

    glEnable(GL_BLEND);

    glActiveTexture(GL_TEXTURE4);
    ParticleUtils::createRandomTex1D(nParticles * 3);

    glActiveTexture(GL_TEXTURE5);
    treeTex = Texture::loadTexture("media/texture/Linden.png");
    glBindTexture(GL_TEXTURE_2D, treeTex);

    groundTex = Texture::loadTexture("media/texture/forestFloor.png");
    glBindTexture(GL_TEXTURE_2D, groundTex);

    //Making sure texture can repeat
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    initParticleBuffers();

    particleProg.use();
    particleProg.setUniform("RandomTex", 4);
    particleProg.setUniform("ParticleLifetime", particleLifetime);
    particleProg.setUniform("ParticleSize", 0.05f);
    particleProg.setUniform("Accel", vec3(0.0f, -0.5f, 0.0f));

    GLuint programHandle = prog.getHandle();
    pass1Index = glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "recordDepth");
    pass2Index = glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "shadeWithShadow");

    shadowBias = mat4(vec4(0.5f, 0.0f, 0.0f, 0.0f),
        vec4(0.0f, 0.5f, 0.0f, 0.0f),
        vec4(0.0f, 0.0f, 0.5f, 0.0f),
        vec4(0.5f, 0.5f, 0.5f, 1.0f)

    );

    vec3 lightPos = vec3(0.0f, 55.0f, 20.0f);
    vec3 lightTarget = vec3(0.0f, 0.0f, 18.0f);
    lightFrustum.orient(lightPos, lightTarget, vec3(0.0f, 1.0f, 0.0f));
    lightFrustum.setPerspective(70.0f, 1.0f, 1.0f, 140.0f);

    lightPV = shadowBias * lightFrustum.getProjectionMatrix() * lightFrustum.getViewMatrix();


    prog.use(); 
    prog.setUniform("Light.Intensity", vec3(0.85f));
    prog.setUniform("Light.L", vec3(0.9f));
    prog.setUniform("Light.La", vec3(0.5f));
    prog.setUniform("ShadowMap", 0);
    prog.setUniform("OffsetTex", 1);
    prog.setUniform("DiffuseTex", 5);
    prog.setUniform("Radius", radius / 512.0f);
    prog.setUniform("OffsetTexSize", vec3(jitterMapSize, jitterMapSize, samplesU * samplesV / 2.0f));

    window = glfwGetCurrentContext();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //Randomising tree placement

    for (int i = 0; i < treeCount; i++)
    {
        glm::vec3 treePos;
        bool validTreePosition = false;
        int attempts = 0;

        while (!validTreePosition && attempts < 100)
        {
            attempts++;

            float x = (rand() % 100 - 50); //randomise placement
            float z = (rand() % 100 - 50);

            treePos = glm::vec3(x, 2.25f, z);

            validTreePosition = true;

            //Spawn away from each other
            for (int j = 0; j < treesPosition.size(); j++)
            {
                glm::vec2 newTreeXZ(treePos.x, treePos.z);
                glm::vec2 existingTreeXZ(treesPosition[j].x, treesPosition[j].z);

                if (glm::distance(newTreeXZ, existingTreeXZ) < 3.0f)
                {
                    validTreePosition = false;
                    break;
                }
            }
        }

        treesPosition.push_back(treePos);
        treesScale.push_back(1.8f + (rand() % 40) * 0.01f);
        treesRotation.push_back((rand() % 360));

        float tint = 0.6f + (rand() % 40) / 100.0f;
        treeGreenTint.push_back(tint);

        int meshChoice = rand() % treeMeshes.size();
        treeMeshIndex.push_back(meshChoice);
    }

    //Tree border around the playable area
    float borderLimit = 46.0f;      //slightly outside player clamp
    float spacing = 4.0f;           //smaller = denser border

    for (float x = -borderLimit; x <= borderLimit; x += spacing)
    {
        //Back edge
        treesPosition.push_back(glm::vec3(x, 2.25f, -borderLimit));
        treesScale.push_back(randomFloat(2.2f, 3.0f));
        treesRotation.push_back(randomFloat(0.0f, 360.0f));
        treeGreenTint.push_back(randomFloat(0.55f, 0.85f));
        treeMeshIndex.push_back(rand() % treeMeshes.size());

        //Front edge
        treesPosition.push_back(glm::vec3(x, 2.25f, borderLimit));
        treesScale.push_back(randomFloat(2.2f, 3.0f));
        treesRotation.push_back(randomFloat(0.0f, 360.0f));
        treeGreenTint.push_back(randomFloat(0.55f, 0.85f));
        treeMeshIndex.push_back(rand() % treeMeshes.size());
    }

    for (float z = -borderLimit; z <= borderLimit; z += spacing)
    {
        //Left edge
        treesPosition.push_back(glm::vec3(-borderLimit, 2.25f, z));
        treesScale.push_back(randomFloat(2.2f, 3.0f));
        treesRotation.push_back(randomFloat(0.0f, 360.0f));
        treeGreenTint.push_back(randomFloat(0.55f, 0.85f));
        treeMeshIndex.push_back(rand() % treeMeshes.size());

        //Right edge
        treesPosition.push_back(glm::vec3(borderLimit, 2.25f, z));
        treesScale.push_back(randomFloat(2.2f, 3.0f));
        treesRotation.push_back(randomFloat(0.0f, 360.0f));
        treeGreenTint.push_back(randomFloat(0.55f, 0.85f));
        treeMeshIndex.push_back(rand() % treeMeshes.size());
    }

    // Randomising shrub placement
    for (int i = 0; i < shrubCount; i++)
    {
        glm::vec3 shrubPos;
        bool validShrubPosition = false;
        int attempts = 0;

        while (!validShrubPosition && attempts < 100)
        {
            attempts++;

            float x = randomFloat(-30.0f, 30.0f);
            float z = randomFloat(-30.0f, 30.0f);

            shrubPos = glm::vec3(x, 1.0f, z);

            validShrubPosition = true;

            //Plants away from trees to stop clipping
            for (int j = 0; j < treesPosition.size(); j++)
            {
                glm::vec2 shrubXZ(shrubPos.x, shrubPos.z);
                glm::vec2 treeXZ(treesPosition[j].x, treesPosition[j].z);

                if (glm::distance(shrubXZ, treeXZ) < 2.0f)
                {
                    validShrubPosition = false;
                    break;
                }
            }
        }

        shrubPositions.push_back(shrubPos);
        shrubScales.push_back(randomFloat(3.0f, 5.0f));
        shrubRotations.push_back(randomFloat(0.0f, 360.0f));

        float tint = randomFloat(0.65f, 1.0f);
        shrubGreenTint.push_back(tint);

        int meshChoice = rand() % shrubMeshes.size();
        shrubMeshIndex.push_back(meshChoice);
    }

    //Good Fairy placement
    totalFairies = 12;

    fairyPositions.clear();
    fairyCollected.clear();
    fairyOrbitCenters.clear();
    fairyAngleOffsets.clear();


    //Good fairy placement
    for (int i = 0; i < totalFairies; i++)
    {
        glm::vec3 spawnCenter;
        bool validSpawn = false;
        int attempts = 0;

        while (!validSpawn && attempts < 100)
        {
            attempts++;

            //Random spawn area
            spawnCenter.x = randomFloat(-34.0f, 34.0f);
            spawnCenter.y = 4.0f;
            spawnCenter.z = randomFloat(-34.0f, 28.0f);

            glm::vec2 spawnXZ(spawnCenter.x, spawnCenter.z);
            glm::vec2 playerStartXZ(cameraPos.x, cameraPos.z);

            validSpawn = true;

            //Spawn away from player
            if (glm::distance(spawnXZ, playerStartXZ) < 4.0f)
            {
                validSpawn = false;
            }

            //Spawn away from each other
            for (int j = 0; j < fairyOrbitCenters.size(); j++)
            {
                glm::vec2 otherXZ(
                    fairyOrbitCenters[j].x,
                    fairyOrbitCenters[j].z
                );

                if (glm::distance(spawnXZ, otherXZ) < 6.0f)
                {
                    validSpawn = false;
                    break;
                }
            }
        }

        float angle = randomFloat(0.0f, glm::two_pi<float>());

        fairyOrbitCenters.push_back(spawnCenter);
        fairyAngleOffsets.push_back(angle);

        glm::mat4 orbit = glm::mat4(1.0f);
        orbit = glm::translate(orbit, spawnCenter);
        orbit = glm::rotate(orbit, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        orbit = glm::translate(orbit, glm::vec3(fairyOrbitRadius, 0.0f, 0.0f));

        glm::vec4 worldPos = orbit * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        fairyPositions.push_back(glm::vec3(worldPos));
        fairyCollected.push_back(false);
    }

    //Evil fairy placement
    totalEvilFairies = 6;

    evilFairyPositions.clear();
    evilFairyOrbitCenters.clear();
    evilFairyAngleOffsets.clear();

    for (int i = 0; i < totalEvilFairies; i++)
    {
        glm::vec3 spawnCenter;
        bool validSpawn = false;

        while (!validSpawn)
        {
            spawnCenter.x = randomFloat(-34.0f, 34.0f);
            spawnCenter.y = 4.0f;
            spawnCenter.z = randomFloat(-32.0f, 26.0f);

            glm::vec2 spawnXZ(spawnCenter.x, spawnCenter.z);
            glm::vec2 playerXZ(cameraPos.x, cameraPos.z);

            validSpawn = true;

            //Spawn away from player
            if (glm::distance(spawnXZ, playerXZ) < 6.0f)
            {
                validSpawn = false;
            }

            //Spawn away from eachother
            for (int j = 0; j < evilFairyOrbitCenters.size(); j++)
            {
                glm::vec2 otherXZ(
                    evilFairyOrbitCenters[j].x,
                    evilFairyOrbitCenters[j].z
                );

                if (glm::distance(spawnXZ, otherXZ) < 10.0f)
                {
                    validSpawn = false;
                    break;
                }
            }
        }

        float angle = randomFloat(0.0f, glm::two_pi<float>());

        evilFairyOrbitCenters.push_back(spawnCenter);
        evilFairyAngleOffsets.push_back(angle);

        glm::mat4 orbit = glm::mat4(1.0f);
        orbit = glm::translate(orbit, spawnCenter);
        orbit = glm::rotate(orbit, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        orbit = glm::translate(orbit, glm::vec3(fairyOrbitRadius, 0.0f, 0.0f));

        glm::vec4 worldPos = orbit * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        evilFairyPositions.push_back(glm::vec3(worldPos));
    }

    fairyCollected.resize(fairyPositions.size(), false);
    totalFairies = static_cast<int>(fairyPositions.size());
}

void SceneBasic_Uniform::compile()
{
	try {
        prog.compileShader("shader/basic_uniform.vert");
        prog.compileShader("shader/basic_uniform.frag");
        prog.link();
        prog.use();

        solidProg.compileShader("shader/solid.vs", GLSLShader::VERTEX);
        solidProg.compileShader("shader/solid.fs", GLSLShader::FRAGMENT);
        solidProg.link();

        uiProg.compileShader("shader/ui.vs", GLSLShader::VERTEX);
        uiProg.compileShader("shader/ui.fs", GLSLShader::FRAGMENT);
        uiProg.link();

        skyboxProg.compileShader("shader/skybox.vs", GLSLShader::VERTEX);
        skyboxProg.compileShader("shader/skybox.fs", GLSLShader::FRAGMENT);
        skyboxProg.link();

        postProg.compileShader("shader/post.vs", GLSLShader::VERTEX);
        postProg.compileShader("shader/post.fs", GLSLShader::FRAGMENT);
        postProg.link();

        brightProg.compileShader("shader/post.vs", GLSLShader::VERTEX);
        brightProg.compileShader("shader/bright.fs", GLSLShader::FRAGMENT);
        brightProg.link();

        blurProg.compileShader("shader/post.vs", GLSLShader::VERTEX);
        blurProg.compileShader("shader/blur.fs", GLSLShader::FRAGMENT);
        blurProg.link();

        particleProg.compileShader("shader/fairy_particle.vs", GLSLShader::VERTEX);
        particleProg.compileShader("shader/fairy_particle.fs", GLSLShader::FRAGMENT);

        GLuint particleHandle = particleProg.getHandle();
        const char* outputNames[] = { "Position", "Velocity", "Age" };
        glTransformFeedbackVaryings(
            particleHandle,
            3,
            outputNames,
            GL_SEPARATE_ATTRIBS
        );

        particleProg.link();

	} catch (GLSLProgramException &e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update(float t)
{
    deltaTime = t - tPrev;
    tPrev = t;
    particleDeltaT = deltaTime;
    particleTime = t;

    //Camera
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.06f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    //clamp
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    //camera movement
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);

    float speed = 6.0f * deltaTime;

    glm::vec3 forward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += forward * speed;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= forward * speed;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= right * speed;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += right * speed;

    float worldLimit = 42.0f;

    cameraPos.x = glm::clamp(cameraPos.x, -worldLimit, worldLimit);
    cameraPos.z = glm::clamp(cameraPos.z, -worldLimit, worldLimit);
    cameraPos.y = 4.0f;

    //Fairy movement
    updateFairyOrbits(t);
    updateEvilFairyOrbits(t);

    //Blue fairy rescue logic
    if (!gameWon)
    {
        for (int i = 0; i < fairyPositions.size(); i++)
        {
            if (!fairyCollected[i])
            {
                glm::vec2 playerXZ(cameraPos.x, cameraPos.z);
                glm::vec2 fairyXZ(fairyPositions[i].x, fairyPositions[i].z);

                float distanceToFairy = glm::distance(playerXZ, fairyXZ);

                if (distanceToFairy < 2.2f)
                {
                    fairyCollected[i] = true;
                    score++;

                    if (score >= totalFairies)
                    {
                        gameWon = true;
                        corruptionTimer = 0.0f;
                        std::cout << "The grove is restored!" << std::endl;
                    }

                    dustPosition = fairyPositions[i];
                    dustTimer = 0.0f;
                    dustActive = true;
                    resetFairyDust();

                    std::cout << "Fairy collected! Score: "
                        << score << " / " << totalFairies << std::endl;

                    std::string title = "Shadow Grove - Fairies: "
                        + std::to_string(score)
                        + " / "
                        + std::to_string(totalFairies);
                }
            }
        }

        checkEvilFairyCollision();
    }

    if (corruptionTimer > 0.0f)
    {
        corruptionTimer -= deltaTime;

        if (corruptionTimer < 0.0f)
            corruptionTimer = 0.0f;
    }

}

void SceneBasic_Uniform::render()
{
    prog.use();
    // Pass 1 (shadow map generation)
    view = lightFrustum.getViewMatrix();
    projection = lightFrustum.getProjectionMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, shadowMapWidth, shadowMapHeight);
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass1Index);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.5f, 10.0f);
    currentPass = 1;
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass1Index);
    drawScene();
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glFlush();
    //spitOutDepthBuffer(); // This is just used to get an image of the depth buffer

    // Pass 2 (render)
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    prog.setUniform("Light.Position", view * vec4(lightFrustum.getOrigin(), 1.0f));
    projection = glm::perspective(glm::radians(50.0f), (float)width / height, 0.1f, 100.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);

    renderSkybox();
    prog.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTex);

    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass2Index);
    currentPass = 2;
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass2Index);

    if (gameWon)
    {
        //win state
        prog.setUniform("Fog.MaxDist", 500.0f);
        prog.setUniform("Fog.MinDist", 100.0f);
        prog.setUniform("Fog.Color", vec3(0.55f, 0.75f, 0.95f));
    }
    else if (corruptionTimer > 0.0f)
    {
        //corrupt state
        prog.setUniform("Fog.MaxDist", 8.0f);
        prog.setUniform("Fog.MinDist", 0.2f);
        prog.setUniform("Fog.Color", vec3(0.005f, 0.0f, 0.012f));
    }
    else
    {
        //normal state
        prog.setUniform("Fog.MaxDist", 22.0f);
        prog.setUniform("Fog.MinDist", 4.0f);
        prog.setUniform("Fog.Color", vec3(0.10f, 0.14f, 0.18f));
    }

    drawScene();

    //Draw the light's frustum
    solidProg.use();
    solidProg.setUniform("UseFairyNoise", 0);
    solidProg.setUniform("Color", vec4(1.0f, 0.0f, 0.0f, 1.0f));
    mat4 mv = view * lightFrustum.getInverseViewMatrix();
    solidProg.setUniform("MVP", projection * mv);
    //lightFrustum.render();

    //Draw good fairies
    solidProg.use();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTex);

    solidProg.setUniform("UseFairyNoise", 1);
    solidProg.setUniform("Time", static_cast<float>(glfwGetTime()));
    solidProg.setUniform("FairyType", 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    for (int i = 0; i < fairyPositions.size(); i++)
    {
        if (!fairyCollected[i])
        {
            float flicker = 0.5f + 0.5f * sin(glfwGetTime() * 6.0f + i * 1.7f);

            mat4 auraModel = mat4(1.0f);
            auraModel = glm::translate(auraModel, fairyPositions[i]);

            float auraScale = 1.1f + 0.4f * flicker;
            auraModel = glm::scale(auraModel, vec3(auraScale));

            solidProg.setUniform(
                "Color",vec4(0.05f, 0.75f,1.0f,0.18f + flicker * 0.12f));
            solidProg.setUniform("NoiseSeed", static_cast<float>(i) * 0.17f);

            mat4 auraMVP = projection * view * auraModel;
            solidProg.setUniform("MVP", auraMVP);
            fairySphere.render();

            mat4 coreModel = mat4(1.0f);
            coreModel = glm::translate(coreModel, fairyPositions[i]);

            float coreScale = 0.7f + 0.25f * flicker;
            coreModel = glm::scale(coreModel, vec3(coreScale));

            solidProg.setUniform("Color", vec4(0.05f, 0.85f, 1.0f, 1.0f));

            mat4 coreMVP = projection * view * coreModel;
            solidProg.setUniform("MVP", coreMVP);
            fairySphere.render();
        }
    }
    
    //Draw bad fairies
    if (!gameWon)
    {
        solidProg.setUniform("UseFairyNoise", 1);
        solidProg.setUniform("FairyType", 2);
        solidProg.setUniform("Time", static_cast<float>(glfwGetTime()));

        for (int i = 0; i < evilFairyPositions.size(); i++)
        {
            float flicker = 0.5f + 0.5f * sin(glfwGetTime() * 7.0f + i * 1.9f);

            solidProg.setUniform("NoiseSeed", static_cast<float>(i) * 0.31f + 8.0f);

            mat4 evilAuraModel = mat4(1.0f);
            evilAuraModel = glm::translate(evilAuraModel, evilFairyPositions[i]);
            evilAuraModel = glm::scale(evilAuraModel, vec3(1.4f + 0.5f * flicker));

            solidProg.setUniform("Color",vec4(0.55f,0.0f,0.03f,0.30f + 0.22f * flicker));

            solidProg.setUniform("MVP", projection * view * evilAuraModel);
            fairySphere.render();

            mat4 evilCoreModel = mat4(1.0f);
            evilCoreModel = glm::translate(evilCoreModel, evilFairyPositions[i]);
            evilCoreModel = glm::scale(evilCoreModel, vec3(0.7f + 0.25f * flicker));

            solidProg.setUniform("Color", vec4(1.0f, 0.0f, 0.0f,1.0f));

            solidProg.setUniform("MVP", projection * view * evilCoreModel);
            fairySphere.render();
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    solidProg.use();
    solidProg.setUniform("UseFairyNoise", 0);
    solidProg.setUniform("FairyType", 0);

    if (dustActive)
    {
        renderFairyDust(dustPosition);


        dustTimer += particleDeltaT;

        if (dustTimer > particleLifetime)
        {
            dustActive = false;
        }
    }
    //Create bloom textures
    renderBrightPass();
    renderBlurPass();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    postProg.use();
    postProg.setUniform("BloomStrength", 0.3f);
    postProg.setUniform("Exposure", 1.0f);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, sceneTex);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, blurTex[1]);

    renderFullScreenQuad();

    glEnable(GL_DEPTH_TEST);

    renderText();
}

void SceneBasic_Uniform::drawScene()
{

    if (currentPass == 2) {
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        vec4 lightPos = vec4(10.0f * cos(angle), 10.0f, 10.0f * sin(angle), 1.0f);
        prog.setUniform("Light.Position", vec4(view * lightPos));

        prog.setUniform("UseFog", true);
    }

    //trees
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, treeTex);

    for (int i = 0; i < treesPosition.size(); i++) {

        float tint = treeGreenTint[i];

        prog.setUniform("Material.Kd", vec3(0.004, 0.0, 0.349));
        prog.setUniform("Material.Ks", vec3(0.03f));
        prog.setUniform("Material.Ka", vec3(0.02f,0.07f,0.28f));
        prog.setUniform("Material.Shininess", 8.0f);
        prog.setUniform("UseTexture", 0);

        model = mat4(1.0f);

        glm::vec3 pos = treesPosition[i];
        pos.y += 3.0f;


        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(treesRotation[i]), vec3(0, 1, 0));
        model = glm::scale(model, vec3(
            treesScale[i] * 0.9f,   // width
            treesScale[i] * 1.2f,   // height
            treesScale[i] * 0.9f    // depth
        ));

        setMatrices();
        treeMeshes[treeMeshIndex[i]]->render();
    }

    //shrubs
    prog.setUniform("UseTexture", 0);
    prog.setUniform("UseFog", false);

    for (int i = 0; i < shrubPositions.size(); i++)
    {
        float tint = shrubGreenTint[i];

        prog.setUniform("Material.Kd", vec3(0.02f * tint, 0.12f * tint, 0.32f * tint));

        prog.setUniform("Material.Ks", vec3(0.06f));
        prog.setUniform("Material.Ka", vec3(0.01f * tint, 0.05f * tint, 0.18f * tint));
        prog.setUniform("Material.Shininess", 18.0f);
        prog.setUniform("UseTexture", 0);

        model = mat4(1.0f);

        glm::vec3 pos = shrubPositions[i];

        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(shrubRotations[i]), vec3(0.0f, 1.0f, 0.0f));

        model = glm::scale(model, vec3(shrubScales[i]));

        setMatrices();
        shrubMeshes[shrubMeshIndex[i]]->render();
    }
    //plane
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, groundTex);

    prog.setUniform("UseFog", true);
    prog.setUniform("Material.Kd", vec3(0.35f, 0.42f, 0.30f));
    prog.setUniform("Material.Ks", vec3(0.02f));
    prog.setUniform("Material.Ka", vec3(0.15f));
    prog.setUniform("Material.Shininess", 10.0f);
    prog.setUniform("UseTexture", 1);
    prog.setUniform("TextureScale", 40.0f);

    model = mat4(1.0f);
    setMatrices();
    plane.render();

    prog.setUniform("UseTexture", 0);
    prog.setUniform("TextureScale", 1.0f);
}

void SceneBasic_Uniform::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    width = w;
    height = h;
    projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);

    if (sceneFBO == 0)
    {
        setupPostProcessing();
    }
}

void SceneBasic_Uniform::setMatrices()
{
    mat4 mv = view*model;
    prog.setUniform("ModelViewMatrix", mv);
    prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));
    prog.setUniform("MVP", projection * mv);
    prog.setUniform("ShadowMatrix", lightPV * model);
}

void SceneBasic_Uniform::setupFBO()
{

    GLfloat border[] = { 1.0f, 0.0f,0.0f,0.0f };
    // The depth buffer texture

    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, shadowMapWidth, shadowMapHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

    // Assign the depth buffer texture to texture channel 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTex);

    // Create and set up the FBO
    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

    GLenum drawBuffers[] = { GL_NONE };
    glDrawBuffers(1, drawBuffers);

    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result == GL_FRAMEBUFFER_COMPLETE) {
        printf("Framebuffer is complete. \n");
    }
    else {
        printf("Framebuffer is not complete. \n");

    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneBasic_Uniform::buildJitterTex()
{
    int size = jitterMapSize;
    int samples = samplesU * samplesV;
    int bufSize = size * size * samples * 2;
    float* data = new float[bufSize];
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < samples; k += 2) {
                int x1, y1, x2, y2;
                x1 = k % (samplesU);
                y1 = (samples - 1 - k) / samplesU;
                x2 = (k + 1) % samplesU;
                y2 = (samples - 1 - k - 1) / samplesU;
                vec4 v;
                // Center on grid and jitter
                v.x = (x1 + 0.5f) + jitter();
                v.y = (y1 + 0.5f) + jitter();
                v.z = (x2 + 0.5f) + jitter();
                v.w = (y2 + 0.5f) + jitter();
                // Scale between 0 and 1
                v.x /= samplesU;
                v.y /= samplesV;
                v.z /= samplesU;
                v.w /= samplesV;
                // Warp to disk
                int cell = ((k / 2) * size * size + j * size + i) * 4;
                data[cell + 0] = sqrtf(v.y) * cosf(glm::two_pi<float>() * v.x);
                data[cell + 1] = sqrtf(v.y) * sinf(glm::two_pi<float>() * v.x);
                data[cell + 2] = sqrtf(v.w) * cosf(glm::two_pi<float>() * v.z);
                data[cell + 3] = sqrtf(v.w) * sinf(glm::two_pi<float>() * v.z);
            }
        }
    }
    glActiveTexture(GL_TEXTURE1);
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_3D, texID);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, size, size, samples / 2);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, size, size, samples / 2, GL_RGBA, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    delete[] data;
}

// Return random float between -0.5 and 0.5
float SceneBasic_Uniform::jitter() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<float> distrib(-0.5f, 0.5f);
    return distrib(generator);
}

void SceneBasic_Uniform::initText()
{
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);

    // stb_easy_font can generate a decent amount of vertex data.
    glBufferData(GL_ARRAY_BUFFER, 99999, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 2,
        nullptr
    );

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}
void SceneBasic_Uniform::drawText(
    const std::string& text,
    float x,
    float y,
    float scale,
    const glm::vec4& color
)
{
    char buffer[99999];

    int numQuads = stb_easy_font_print(
        x,
        y,
        const_cast<char*>(text.c_str()),
        nullptr,
        buffer,
        sizeof(buffer)
    );

    std::vector<glm::vec2> vertices;
    vertices.reserve(numQuads * 6);

    struct StbVertex
    {
        float x, y, z;
        unsigned char color[4];
    };

    StbVertex* stbVerts = reinterpret_cast<StbVertex*>(buffer);

    for (int i = 0; i < numQuads; i++)
    {
        StbVertex& v0 = stbVerts[i * 4 + 0];
        StbVertex& v1 = stbVerts[i * 4 + 1];
        StbVertex& v2 = stbVerts[i * 4 + 2];
        StbVertex& v3 = stbVerts[i * 4 + 3];

        auto convert = [&](StbVertex& v)
            {
                return glm::vec2(
                    x + (v.x - x) * scale,
                    y + (v.y - y) * scale
                );
            };

        glm::vec2 p0 = convert(v0);
        glm::vec2 p1 = convert(v1);
        glm::vec2 p2 = convert(v2);
        glm::vec2 p3 = convert(v3);

        vertices.push_back(p0);
        vertices.push_back(p1);
        vertices.push_back(p2);

        vertices.push_back(p0);
        vertices.push_back(p2);
        vertices.push_back(p3);
    }

    if (vertices.empty())
        return;

    uiProg.use();

    glm::mat4 projectionMatrix = glm::ortho(
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f,
        -1.0f,
        1.0f
    );

    uiProg.setUniform("projection", projectionMatrix);
    uiProg.setUniform("textColor", color);

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

    glBindVertexArray(0);
}
void SceneBasic_Uniform::renderText()
{
    glViewport(0, 0, width, height);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::string scoreText = "Fairies: " + std::to_string(score) + " / " + std::to_string(totalFairies);

    drawText(scoreText, 20.0f, 30.0f, 3.0f, glm::vec4(0.75f, 0.95f, 1.0f, 1.0f));

    drawText("Collect Blue Fairies", 580.0f, 30.0f, 2.0f, glm::vec4(0.0, 0.435, 1.0, 1.0));
    drawText("Avoid Red Fairies", 580.0f, 50.0f, 2.0f, glm::vec4(1.0, 0.0, 0.0, 1.0));

    if (corruptionTimer > 0.0f && !gameWon)
    {
        drawText("Corruption!", 290.0f, 60.0f, 4.0f,glm::vec4(1.0f, 0.2f, 0.45f, 1.0f));
    }

    if (gameWon)
    {
        drawText("The Grove is Restored!", 175.0f, 80.0f, 4.0f, glm::vec4(0.85f, 1.0f, 1.0f, 1.0f));
    }

    glDisable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
}

void SceneBasic_Uniform::initParticleBuffers()
{
    glGenBuffers(2, posBuf);
    glGenBuffers(2, velBuf);
    glGenBuffers(2, ageBuf);

    int vec3Size = nParticles * 3 * sizeof(GLfloat);

    glBindBuffer(GL_ARRAY_BUFFER, posBuf[0]);
    glBufferData(GL_ARRAY_BUFFER, vec3Size, nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_ARRAY_BUFFER, posBuf[1]);
    glBufferData(GL_ARRAY_BUFFER, vec3Size, nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_ARRAY_BUFFER, velBuf[0]);
    glBufferData(GL_ARRAY_BUFFER, vec3Size, nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_ARRAY_BUFFER, velBuf[1]);
    glBufferData(GL_ARRAY_BUFFER, vec3Size, nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_ARRAY_BUFFER, ageBuf[0]);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(GLfloat), nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_ARRAY_BUFFER, ageBuf[1]);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(GLfloat), nullptr, GL_DYNAMIC_COPY);

    std::vector<GLfloat> tempAge(nParticles);
    float rate = particleLifetime / nParticles;

    for (int i = 0; i < nParticles; i++)
    {
        tempAge[i] = rate * (i - nParticles);
    }

    glBindBuffer(GL_ARRAY_BUFFER, ageBuf[0]);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        nParticles * sizeof(GLfloat),
        tempAge.data()
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(2, particleArray);

    for (int i = 0; i < 2; i++)
    {
        glBindVertexArray(particleArray[i]);

        glBindBuffer(GL_ARRAY_BUFFER, posBuf[i]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, velBuf[i]);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, ageBuf[i]);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);
    }

    glBindVertexArray(0);

    glGenTransformFeedbacks(2, feedback);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posBuf[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velBuf[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, ageBuf[0]);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posBuf[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velBuf[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, ageBuf[1]);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
}

void SceneBasic_Uniform::renderFairyDust(const glm::vec3& fairyPos)
{
    particleProg.use();

    particleProg.setUniform("Time", particleTime);
    particleProg.setUniform("DeltaT", particleDeltaT * 0.5f);
    particleProg.setUniform("Emitter", fairyPos);

    mat4 mv = view * mat4(1.0f);
    particleProg.setUniform("MV", mv);
    particleProg.setUniform("Proj", projection);

    // Update particles
    particleProg.setUniform("Pass", 1);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[drawBuf]);
    glBeginTransformFeedback(GL_POINTS);

    glBindVertexArray(particleArray[1 - drawBuf]);

    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);

    glDrawArrays(GL_POINTS, 0, nParticles);

    glBindVertexArray(0);

    glEndTransformFeedback();
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    glDisable(GL_RASTERIZER_DISCARD);

    // Draw particles
    particleProg.setUniform("Pass", 2);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glDepthMask(GL_FALSE);

    glBindVertexArray(particleArray[drawBuf]);

    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, nParticles);

    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    drawBuf = 1 - drawBuf;
}

void SceneBasic_Uniform::resetFairyDust()
{
    std::vector<GLfloat> tempAge(nParticles);

    for (int i = 0; i < nParticles; i++)
    {
        tempAge[i] = particleLifetime + 1.0f;
    }

    for (int i = 0; i < 2; i++)
    {
        glBindBuffer(GL_ARRAY_BUFFER, ageBuf[i]);
        glBufferSubData(
            GL_ARRAY_BUFFER, 0, nParticles * sizeof(GLfloat), tempAge.data());
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void SceneBasic_Uniform::updateFairyOrbits(float t)
{
    for (int i = 0; i < fairyPositions.size(); i++)
    {
        if (fairyCollected[i])
            continue;

        float angle = fairyAngleOffsets[i] + t * fairyOrbitSpeed;

        float bob =
            sin(t * fairyBobSpeed + fairyAngleOffsets[i]) *
            fairyBobAmount;

        glm::mat4 orbit = glm::mat4(1.0f);

        orbit = glm::translate(orbit, fairyOrbitCenters[i]);
        orbit = glm::rotate(orbit, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        orbit = glm::translate(orbit, glm::vec3(fairyOrbitRadius, bob, 0.0f));

        glm::vec4 worldPos = orbit * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        fairyPositions[i] = glm::vec3(worldPos);
    }
}

void SceneBasic_Uniform::updateEvilFairyOrbits(float t)
{
    if (gameWon)
        return;

    for (int i = 0; i < evilFairyPositions.size(); i++)
    {
        float angle = evilFairyAngleOffsets[i] - t * fairyOrbitSpeed * 1.5f;

        float bob =
            sin(t * fairyBobSpeed * 1.4f + evilFairyAngleOffsets[i]) *
            fairyBobAmount;

        glm::mat4 orbit = glm::mat4(1.0f);

        orbit = glm::translate(orbit, evilFairyOrbitCenters[i]);
        orbit = glm::rotate(orbit, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        orbit = glm::translate(orbit, glm::vec3(fairyOrbitRadius * 2.0f, bob, 0.0f));

        glm::vec4 worldPos = orbit * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        evilFairyPositions[i] = glm::vec3(worldPos);
    }
}
void SceneBasic_Uniform::checkEvilFairyCollision()
{
    if (gameWon)
        return;

    for (int i = 0; i < evilFairyPositions.size(); i++)
    {
        glm::vec2 playerXZ(cameraPos.x, cameraPos.z);
        glm::vec2 evilXZ(evilFairyPositions[i].x, evilFairyPositions[i].z);

        float distanceToEvilFairy = glm::distance(playerXZ, evilXZ);

        if (distanceToEvilFairy < 2.2f)
        {
            corruptionTimer = corruptionDuration;

            std::cout << "Corruption touched the player!" << std::endl;

            //Move the evil fairy away
            evilFairyOrbitCenters[i].x = randomFloat(-24.0f, 24.0f);
            evilFairyOrbitCenters[i].z = randomFloat(-24.0f, 16.0f);

            break;
        }
    }
}


void SceneBasic_Uniform::initSkybox()
{
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3 * sizeof(float),(void*)0);

    glBindVertexArray(0);

    std::vector<std::string> faces = {
        "media/texture/stars1.png",
        "media/texture/stars1.png",
        "media/texture/stars1.png",
        "media/texture/stars1.png",
        "media/texture/stars1.png",
        "media/texture/stars1.png"
    };

    skyboxTexture = loadCubemap(faces);

    skyboxProg.use();
    skyboxProg.setUniform("skybox", 0);
}

GLuint SceneBasic_Uniform::loadCubemap(const std::vector<std::string>& faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false);

    int texWidth, texHeight, nrChannels;

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(
            faces[i].c_str(),
            &texWidth,
            &texHeight,
            &nrChannels,
            0
        );

        if (data)
        {
            GLenum format = GL_RGB;

            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,0,format,texWidth,texHeight,0,format,GL_UNSIGNED_BYTE,data);

            stbi_image_free(data);
        }
        else
        {
            std::cerr << "Failed to load skybox texture: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
void SceneBasic_Uniform::renderSkybox()
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glDisable(GL_CULL_FACE);

    skyboxProg.use();

    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

    skyboxProg.setUniform("view", skyboxView);
    skyboxProg.setUniform("projection", projection);

    glBindVertexArray(skyboxVAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}
void SceneBasic_Uniform::setupPostProcessing()
{
    //Full-screen quad
    float quadVertices[] = {
        // positions    // tex coords
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f,

        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,   0.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4 * sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4 * sizeof(float),(void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneTex);
    glBindTexture(GL_TEXTURE_2D, sceneTex);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,width,height,0,GL_RGB,GL_FLOAT,nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D( GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sceneTex,0);

    //Depth buffer for 3D depth testing
    glGenRenderbuffers(1, &sceneDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,sceneDepth);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_NONE);

    GLenum sceneStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (sceneStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Scene framebuffer is not complete. Status: "
            << sceneStatus << std::endl;
    }

    glGenFramebuffers(1, &brightFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);

    glGenTextures(1, &brightTex);
    glBindTexture(GL_TEXTURE_2D, brightTex);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,width / 2,height / 2, 0,GL_RGB,GL_FLOAT,nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightTex, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_NONE);

    GLenum brightStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (brightStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Bright-pass framebuffer is not complete. Status: "
            << brightStatus << std::endl;
    }

    glGenFramebuffers(2, blurFBO);
    glGenTextures(2, blurTex);

    for (int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);

        glBindTexture(GL_TEXTURE_2D, blurTex[i]);

        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,width / 2,height / 2,0,GL_RGB,GL_FLOAT,nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,blurTex[i],0);

        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_NONE);

        GLenum blurStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (blurStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Blur framebuffer " << i << " is not complete. Status: "
                << blurStatus << std::endl;
        }
    }


    //Return to normal screen framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    postProg.use();
    postProg.setUniform("SceneTex", 6);
    postProg.setUniform("BloomTex", 7);
    postProg.setUniform("BloomStrength", 0.3f);
    postProg.setUniform("Exposure", 1.0f);

    brightProg.use();
    brightProg.setUniform("SceneTex", 6);
    brightProg.setUniform("Threshold", bloomThreshold);

    blurProg.use();
    blurProg.setUniform("ImageTex", 6);
}
void SceneBasic_Uniform::renderBlurPass()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    blurProg.use();
    blurProg.setUniform("ImageTex", 6);

    glViewport(0, 0, width / 2, height / 2);


    //Pass 1 horizontal
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[0]);

    GLenum status0 = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status0 != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Blur framebuffer 0 incomplete during render. Status: "
            << status0 << std::endl;
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    blurProg.setUniform("Horizontal", 1);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, brightTex);

    renderFullScreenQuad();


    //Pass 2 vertical
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[1]);

    GLenum status1 = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status1 != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Blur framebuffer 1 incomplete during render. Status: "
            << status1 << std::endl;
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    blurProg.setUniform("Horizontal", 0);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, blurTex[0]);

    renderFullScreenQuad();
}

void SceneBasic_Uniform::renderFullScreenQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
void SceneBasic_Uniform::renderBrightPass()
{
    glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Bright framebuffer incomplete during render. Status: "
            << status << std::endl;
        return;
    }

    glViewport(0, 0, width / 2, height / 2);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    brightProg.use();

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, sceneTex);

    brightProg.setUniform("SceneTex", 6);
    brightProg.setUniform("Threshold", bloomThreshold);

    renderFullScreenQuad();
}