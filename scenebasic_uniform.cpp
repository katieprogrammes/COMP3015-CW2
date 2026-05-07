#include "scenebasic_uniform.h"

#include <cstdio>
#include <cstdlib>

#include <sstream>
#include <string>
using std::string;

#include <iostream>
using std::cerr;
using std::endl;

#include "helper/glutils.h"
#include "helper/texture.h"
#include <glm/gtc/matrix_transform.hpp>

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

SceneBasic_Uniform::SceneBasic_Uniform() : tPrev(0), shadowMapWidth(512), shadowMapHeight(512), plane(250.0f, 250.0f, 1, 1), angle(90.0f), rotSpeed(glm::pi<float>()/16.0f)
{
    tree = ObjMesh::load("media/Linden.obj", true);
}

void SceneBasic_Uniform::initScene()
{
    compile();
    
    glClearColor(0.0, 0.0, 0.1, 1.0); //setup background colour
    glEnable(GL_DEPTH_TEST);

    view = glm::lookAt(vec3(0.0f, 4.0f, 6.0f), vec3(0.0f, 2.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = mat4(1.0f);

    angle = 0.0f;

    setupFBO();

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




    prog.use(); 
    prog.setUniform("Light.Intensity", vec3(0.85f));
    prog.setUniform("Light.L", vec3(0.9f));
    prog.setUniform("Light.La", vec3(0.5f));
    prog.setUniform("Fog.MaxDist", 50.0f);
    prog.setUniform("Fog.MinDist", 5.0f);
    prog.setUniform("Fog.Color", vec3(0.3f));
    prog.setUniform("ShadowMap", 0);

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

	} catch (GLSLProgramException &e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update(float t)
{
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
}

void SceneBasic_Uniform::render()
{
    prog.use();
    lightPV = shadowBias * lightFrustum.getProjectionMatrix() * lightFrustum.getViewMatrix();
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
    glFlush();
    //spitOutDepthBuffer(); // This is just used to get an image of the depth buffer

    // Pass 2 (render)
    float c = 2.0f;
    vec3 cameraPos(c * 11.5f * cos(angle), c * 7.0f, c * 11.5f * sin(angle));
    view = glm::lookAt(cameraPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
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
    drawScene();

    // Draw the light's frustum
    solidProg.use();
    solidProg.setUniform("Color", vec4(1.0f, 0.0f, 0.0f, 1.0f));
    mat4 mv = view * lightFrustum.getInverseViewMatrix();
    solidProg.setUniform("MVP", projection * mv);
    lightFrustum.render();
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
    prog.setUniform("Material.Kd", vec3(1.0f, 1.0f, 1.0f));
    prog.setUniform("Material.Ks", vec3(0.02f, 0.04f, 0.02f));
    prog.setUniform("Material.Ka", vec3(0.05f));
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
