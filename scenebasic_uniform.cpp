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
#include "helper/glutils.h"
#include "helper/texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include "helper/particleutils.h"
#include "helper/noisetex.h"

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

SceneBasic_Uniform::SceneBasic_Uniform() : 
    tPrev(0), 
    plane(250.0f, 250.0f, 1, 1), 
    fairySphere(0.2f, 16, 16), 
    angle(90.0f), 
    rotSpeed(glm::pi<float>()/16.0f), 
    drawBuf(1), 
    nParticles(55),
    particleLifetime(3.5f),
    particleTime(0.0f),
    particleDeltaT(0.0f)
{
    shadowMapWidth = 512;
    shadowMapHeight = 512;

    samplesU = 4;
    samplesV = 8;
    jitterMapSize = 8;
    radius = 7.0f;
    tree = ObjMesh::load("media/Linden.obj", true);
}

void SceneBasic_Uniform::initScene()
{
    compile();
    
    glClearColor(0.0, 0.0, 0.1, 1.0); //setup background colour
    glEnable(GL_DEPTH_TEST);

    initText();

    view = glm::lookAt(vec3(0.0f, 4.0f, 6.0f), vec3(0.0f, 2.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = mat4(1.0f);

    angle = 0.0f;

    setupFBO();
    buildJitterTex();
    noiseTex = NoiseTex::generatePeriodic2DTex(4.0f, 0.5f, 256, 256);

    glEnable(GL_BLEND);

    glActiveTexture(GL_TEXTURE3);
    Texture::loadTexture("media/texture/bluewater.png");

    glActiveTexture(GL_TEXTURE4);
    ParticleUtils::createRandomTex1D(nParticles * 3);

    initParticleBuffers();

    particleProg.use();
    particleProg.setUniform("RandomTex", 4);
    particleProg.setUniform("ParticleLifetime", particleLifetime);
    particleProg.setUniform("ParticleSize", 0.035f);
    particleProg.setUniform("Accel", vec3(0.0f, 0.0f, 0.0f));

    GLuint programHandle = prog.getHandle();
    pass1Index = glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "recordDepth");
    pass2Index = glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "shadeWithShadow");

    shadowBias = mat4(vec4(0.5f, 0.0f, 0.0f, 0.0f),
        vec4(0.0f, 0.5f, 0.0f, 0.0f),
        vec4(0.0f, 0.0f, 0.5f, 0.0f),
        vec4(0.5f, 0.5f, 0.5f, 1.0f)

    );

    vec3 lightPos = vec3(0.0f, 30.0f, 30.0f);
    lightFrustum.orient(lightPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    lightFrustum.setPerspective(60.0f, 1.0f, 1.0f, 300.0f);

    lightPV = shadowBias * lightFrustum.getProjectionMatrix() * lightFrustum.getViewMatrix();


    prog.use(); 
    prog.setUniform("Light.Intensity", vec3(0.85f));
    prog.setUniform("Light.L", vec3(0.9f));
    prog.setUniform("Light.La", vec3(0.5f));
    prog.setUniform("ShadowMap", 0);
    prog.setUniform("OffsetTex", 1);
    prog.setUniform("Radius", radius / 512.0f);
    prog.setUniform("OffsetTexSize", vec3(jitterMapSize, jitterMapSize, samplesU * samplesV / 2.0f));

    window = glfwGetCurrentContext();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //Randomising tree placement

    for (int i = 0; i < treeCount; i++) {
        float x = (rand() % 100 - 50); //randomise placement
        float z = (rand() % 100 - 50);

        treesPosition.push_back(glm::vec3(x, 2.0f, z));
        treesScale.push_back(0.5f + (rand() % 20) * 0.01f);
        treesRotation.push_back((rand() % 360));

        float tint = 0.6f + (rand() % 40) / 100.0f;
        treeGreenTint.push_back(tint);
    }
    //fairy placement
    fairyPositions = {
    glm::vec3(0.0f, 4.0f, 3.0f),
    glm::vec3(-6.0f, 4.0f, -6.0f),
    glm::vec3(4.0f, 4.0f, -8.0f),
    glm::vec3(8.0f, 4.0f, 2.0f),
    glm::vec3(-8.0f, 4.0f, 4.0f),
    glm::vec3(0.0f, 4.0f, -10.0f),
    glm::vec3(10.0f, 4.0f, -4.0f),
    glm::vec3(-10.0f, 4.0f, -2.0f),
    glm::vec3(2.0f, 4.0f, 8.0f)
    };

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

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    //clamp
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    //calculate direction
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);

    float speed = 8.0f * deltaTime;

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

    //game mechanic logic

    if (!gameWon)
    {
        for (int i = 0; i < fairyPositions.size(); i++)
        {
            if (!fairyCollected[i])
            {
                float distanceToFairy = glm::distance(cameraPos, fairyPositions[i]);

                if (distanceToFairy < 1.5f)
                {
                    fairyCollected[i] = true;
                    score++;

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

                    glfwSetWindowTitle(window, title.c_str());

                    if (score >= totalFairies)
                    {
                        gameWon = true;
                        std::cout << "You collected all fairies!" << std::endl;
                        glfwSetWindowTitle(window, "Shadow Grove - All Fairies Collected!");
                    }
                }
            }
        }
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTex);

    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass2Index);
    currentPass = 2;
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass2Index);

    if (gameWon)
    {
        prog.setUniform("Fog.MaxDist", 80.0f);
        prog.setUniform("Fog.MinDist", 25.0f);
        prog.setUniform("Fog.Color", vec3(0.55f, 0.75f, 0.95f));
    }
    else
    {
        prog.setUniform("Fog.MaxDist", 50.0f);
        prog.setUniform("Fog.MinDist", 5.0f);
        prog.setUniform("Fog.Color", vec3(0.3f));
    }

    drawScene();

    // Draw the light's frustum
    solidProg.use();
    solidProg.setUniform("UseFairyNoise", 0);
    solidProg.setUniform("Color", vec4(1.0f, 0.0f, 0.0f, 1.0f));
    mat4 mv = view * lightFrustum.getInverseViewMatrix();
    solidProg.setUniform("MVP", projection * mv);
    //lightFrustum.render();

    // Draw visible fairies
    solidProg.use();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTex);

    solidProg.setUniform("UseFairyNoise", 1);
    solidProg.setUniform("Time", static_cast<float>(glfwGetTime()));

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
                "Color",
                vec4(
                    0.25f,
                    0.75f,
                    1.0f,
                    0.18f + flicker * 0.12f
                )
            );
            solidProg.setUniform("NoiseSeed", static_cast<float>(i) * 0.17f);

            mat4 auraMVP = projection * view * auraModel;
            solidProg.setUniform("MVP", auraMVP);
            fairySphere.render();

            mat4 coreModel = mat4(1.0f);
            coreModel = glm::translate(coreModel, fairyPositions[i]);

            float coreScale = 0.7f + 0.25f * flicker;
            coreModel = glm::scale(coreModel, vec3(coreScale));

            solidProg.setUniform( "Color", vec4(0.35f, 0.75f, 1.0f, 1.0f));

            mat4 coreMVP = projection * view * coreModel;
            solidProg.setUniform("MVP", coreMVP);
            fairySphere.render();
        }
    }
    if (dustActive)
    {
        renderFairyDust(dustPosition);

        dustTimer += particleDeltaT;

        if (dustTimer > particleLifetime)
        {
            dustActive = false;
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    solidProg.use();
    solidProg.setUniform("UseFairyNoise", 0);

    renderText();
}

void SceneBasic_Uniform::drawScene()
{

    if (currentPass == 2) {
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        vec4 lightPos = vec4(10.0f * cos(angle), 10.0f, 10.0f * sin(angle), 1.0f);
        prog.setUniform("Light.Position", vec4(view * lightPos));
    }

    //tree
    for (int i = 0; i < treeCount; i++) {

        prog.setUniform("Material.Kd", vec3(0.05f, 0.35f, 0.05f));
        prog.setUniform("Material.Ks", vec3(0.2f));
        prog.setUniform("Material.Ka", vec3(0.05f));
        prog.setUniform("Material.Shininess", 50.0f);
        prog.setUniform("UseTexture", 0);

        model = mat4(1.0f);

        glm::vec3 pos = treesPosition[i];
        pos.y += 1.0f;


        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(treesRotation[i]), vec3(0, 1, 0));
        model = glm::scale(model, vec3(treesScale[i]));

        setMatrices();
        tree->render();
    }
    //plane
    //prog.setUniform("Material.Kd", vec3(0.122, 0.078, 0.078));
    prog.setUniform("Material.Kd", vec3(0.35f, 0.42f, 0.30f));
    prog.setUniform("Material.Ks", vec3(0.02f));
    prog.setUniform("Material.Ka", vec3(0.20f));
    prog.setUniform("Material.Shininess", 10.0f);
    prog.setUniform("UseTexture", 0);
    model = mat4(1.0f);
    setMatrices();
    plane.render();
}

void SceneBasic_Uniform::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    width = w;
    height = h;
    projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);

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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthTex, 0);

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

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(glm::vec2),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

    glBindVertexArray(0);
}
void SceneBasic_Uniform::renderText()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::string scoreText =
        "Fairies: "
        + std::to_string(score)
        + " / "
        + std::to_string(totalFairies);

    drawText(scoreText, 20.0f, 30.0f, 2.0f, glm::vec4(0.75f, 0.95f, 1.0f, 1.0f));

    if (gameWon)
    {
        drawText("The grove is restored", 20.0f, 65.0f, 2.0f, glm::vec4(0.85f, 1.0f, 1.0f, 1.0f));
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
    particleProg.setUniform("DeltaT", particleDeltaT);
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
            GL_ARRAY_BUFFER,
            0,
            nParticles * sizeof(GLfloat),
            tempAge.data()
        );
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}



