# COMP3015 - Coursework 1 - OpenGL Shader Prototype

## Development Environment
- Visual Studio Version 2022
- Operating System: Windows 64-bit
- OpenGL Version 4.6

The project was developed and tested entirely on this configuration.

## How the Prototype Works

This prototype demonstrates a custom Blinn-Phong shading model implemented in GLSL, applied to two textured 3D models inside a fog‑based environment.
The scene includes a cube‑mapped skybox, distance fog, rim lighting, and a rotating camera.
Fog can be toggled on/off using the F key, and camera rotation can be toggled on/off with the space bar.

When the program starts, the camera loads into a fog‑covered plain. Ahead, five zombies stand partially obscured by distance fog. As the camera rotates, the player sees that their escape route is blocked by another figure, reinforcing the eerie tone of the scene.

The scene is rendered using two shader programs:
- basic_uniform - used for all 3D models
- skybox - used for the skybox

The Key Features are:

- Blinn-Phong lighting with ambient, diffuse, and specular components. Blinn–Phong was chosen because it provides clear specular highlights and works well with textured models in a foggy environment
- Rim lighting to emphasise silhouettes and enhance the moody atmosphere
- Distance fog, blending object colour with a fog colour based on camera distance
- Cube‑mapped skybox surrounding the scene
- Camera rotation, which can be toggled on/off
- Fog toggle, allowing the user to compare fogged vs. unfogged rendering

The Controls are:

- Space bar: Toggles camera rotation on and off
- F key: Toggles fog on and off
- ESC: Exits the program

## Code Structure

scenebasic_uniform.cpp  
This is the core of the project. It handles:
- Loading and drawing the models
- Applying textures
- Updating the camera
- Fog toggling
- Model transformations
- Lighting setup
- Passing uniforms to shaders

basic_uniform.vert / basic_uniform.frag
These are the vertex and fragment shaders for all 3D models and contain:
- Blinn-Phong shading
- Rim lighting
- Fog for the models

skybox.vert / skybox.frag are the vertex and fragment shaders for the cube mapped skybox and also contain the general scene fog.

All other .cpp and .h files are helper files, with the only changes being made in scene.h and scenerunner.h for the logic on key presses.


## AI Usage

This was the Assisted Work Sections that my AI usage fell under:

A8 - I was having problems understanding an error with a variable not being recognised. It helped me understand why I was getting the error and where I needed to move the variable to. 
The prompt and response can be seen here: https://copilot.microsoft.com/shares/WLerBNcmcbx7syVhCBB8f

## How to Run the Protoype
The Release folder contains a standalone .exe that runs without Visual Studio.
All required assets (models, textures, shaders) are included in the project folder.
Simply run the executable to start the prototype.

## Youtube Link

https://youtu.be/S_VnKrvuzCI
