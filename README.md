# COMP3015-Coursework 2 - Shadow Grove
Shadow Grove is an interactive OpenGL forest scene built from the Project Template provided and extended into a small playable graphics prototype using helper files provided during the module labs.

The player explores a dark magical grove that has been attacked by corrupted red fairies. The goal is to collect the blue fairies while avoiding the corrupted red fairies in order to restore the forest. The scene combines several GLSL shader techniques from the module, including shadow mapping, bloom, procedural noise, particle animation, skybox rendering, textured terrain, and fog. The aim of the project was to combine graphics programming techniques into a cohesive game-like scene.

## Dependencies
This project uses the COMP3015 OpenGL template and helper files provided during the module labs aside from shader_m.h and camera.h which were taken from learnopenGL to support the skybox and camera movement. It also includes:
- OpenGL 4.6 / GLSL
- GLFW / GLAD / GLM as provided by the COMP3015 template
- -Visual Studio 2022
- Windows 11 Operating System
- Both x64 Debug and x64 Release for build configuration

## How To Compile
You can clone from github into Visual Studio then open the .sln (solution) file and you can then either run through the Debug or Release configuration (x64 version).

Alternatively, the .exe file provided in Release/x64 should run without any additional setup.

## Controls
Mouse - Look Around

W key - Move Forward

A - Move Left

S - Move backward

D - Move right

Move into a blue fairy - collect/rescue them

Move into a red fairy - trigger the corruption effect 

## Gameplay Objectives

The objective is to collect all the blue fairies in the grove. Each collected fairy increases the score and triggers a dust particle effect. When all blue fairies have been collected, the grove enters the restored state and displays: "The Grove is Restored". Red fairies act as hazards. 
When the player gets too close to one, a corruption effect is triggered. This temporarily darkens the atmosphere using fog and displays a corruption warning.
The player is kept inside the forest area using a camera position clamp. A tree border surrounds the player area to visually represent the boundary. This was inspired by the way Pokemon games often use enviromental objects to frame the playable area.

## Main Implemented Features

### 1. Shadow Mapping

<img width="399" height="297" alt="image" src="https://github.com/user-attachments/assets/9605e13c-45f1-410c-9a4a-c0720d1694e8" />

The project uses a shadow map pass before the main render pass. The shadow map is rendered from the light’s point of view into a depth texture. This is then sampled in the main fragment shader to determine whether each fragment is lit or shadowed.
Framebuffer Object setup function:
<img width="380" height="363" alt="image" src="https://github.com/user-attachments/assets/ca4fdae5-85f1-440f-8123-e4861c05fc98" />

Light frustum setup:
<img width="363" height="62" alt="image" src="https://github.com/user-attachments/assets/1dd70a15-0363-40f4-bab9-f1d779ae4c27" />

Shadow map generation:
<img width="361" height="188" alt="image" src="https://github.com/user-attachments/assets/b9068b47-44ce-443e-88de-7b39c514df15" />

Shadow logic in the fragment shader:
<img width="383" height="443" alt="image" src="https://github.com/user-attachments/assets/b9c7f37f-5486-4568-a6e3-3326161fbcc6" />

### 2. Bloom Post-Processing

<img width="398" height="245" alt="image" src="https://github.com/user-attachments/assets/055ed5aa-d3c3-4b7f-862a-67f732901938" />


Bloom is implemented as a multi-pass post-processing effect. The scene is rendered to a texture, bright areas are extracted, blurred, and then layered back over the final image.

Scene Framebuffer:
<img width="371" height="283" alt="image" src="https://github.com/user-attachments/assets/dec275cf-66cc-4eda-af6f-ede8e1914de8" />

Brightness Framebuffer:
<img width="374" height="225" alt="image" src="https://github.com/user-attachments/assets/96eba648-9ed6-4e41-b688-066c01fe2982" />

Blur Framebuffer:
<img width="370" height="257" alt="image" src="https://github.com/user-attachments/assets/59430faa-0e19-44df-8a77-bad991d29f54" />

Brightness Extraction:
<img width="302" height="244" alt="image" src="https://github.com/user-attachments/assets/8a1cf564-1799-4143-b57d-da34094b29b5" />

Blurring the brightness:
<img width="332" height="455" alt="image" src="https://github.com/user-attachments/assets/8a8729d2-3989-41ca-939b-abc880d5f546" />

Combining the scene texture with the bloom texture:
<img width="274" height="172" alt="image" src="https://github.com/user-attachments/assets/d0ab74f1-3350-4a19-8a09-f3b6b4bcb1a6" />

### 3. Particle System

<img width="800" height="632" alt="Recording 2026-05-12 000652" src="https://github.com/user-attachments/assets/d9df78e7-9bc2-42ac-aafb-f8d593793680" />

The fairy dust effect uses GPU-side particle updates. Each particle stores position, velocity and age, then resets around the collected fairy position to create a falling magical dust effect.

Initialising particle buffers:
<img width="852" height="456" alt="image" src="https://github.com/user-attachments/assets/4592976f-e6c7-4350-a61f-e9e6e41d3a9d" />


Particle vertex shader update function that updates particle position, velocity and age:
<img width="263" height="334" alt="image" src="https://github.com/user-attachments/assets/0fc36e37-56d0-4d20-9252-d8991d03d8b5" />

Rendering the particles:
<img width="284" height="505" alt="image" src="https://github.com/user-attachments/assets/830f0be4-cc8e-446d-a795-3e8a4ac0a577" />

### 4. Procedural Fairy Noise

<img width="798" height="600" alt="Recording 2026-05-12 002135" src="https://github.com/user-attachments/assets/3a32389d-9c30-4e23-9704-3d92c5abf0c3" />

The fairies use animated noise to create a shimmering surface. The same shader supports both good and corrupted fairies by switching colour palettes using `FairyType`.

The fragment shader for noise sampling and to differentiate between the two types of fairies:
<img width="443" height="503" alt="image" src="https://github.com/user-attachments/assets/cf829bd9-7e1b-48de-8154-5906e63d5e04" />

Rendering both fairies with their noise settings implemented:
<img width="1125" height="531" alt="image" src="https://github.com/user-attachments/assets/0f4c49aa-8af9-4bf5-b746-74b3b427a3b6" />

### 5. Gameplay and Scene Interaction

<img width="399" height="98" alt="image" src="https://github.com/user-attachments/assets/dbf75d33-d338-4092-8dd2-c146be9a0cfe" />

The shader effects are connected to the game mechanics. Blue fairies act as collectibles, red fairies act as hazards, and the world state changes depending on the player’s progress.

Collecting a blue fairy increases the score and triggers the particle effect:

<img width="796" height="598" alt="Recording 2026-05-12 005124" src="https://github.com/user-attachments/assets/0f34847c-7700-49c3-b95d-64ea59980738" />

<img width="308" height="341" alt="image" src="https://github.com/user-attachments/assets/96c647cd-1a3b-4260-af9e-ec4ed95989c5" />

Touching a red fairy triggers the corruption state.

<img width="796" height="592" alt="Recording 2026-05-12 005646" src="https://github.com/user-attachments/assets/d2226d05-98f8-41ff-b0d5-373d88610354" />

<img width="304" height="226" alt="image" src="https://github.com/user-attachments/assets/b6342940-907b-4f26-acfc-ff99eb99503f" />

Collecting all blue fairies triggers the restored grove state.

<img width="796" height="600" alt="Recording 2026-05-12 010229" src="https://github.com/user-attachments/assets/cfa5901c-145e-46fb-8a20-f85229f1026f" />

<img width="263" height="64" alt="image" src="https://github.com/user-attachments/assets/caa3c4f3-17ff-4572-b83a-aca887303870" />

## Code Structure

Most of the project logic is handled in "scenebasic_uniform.cpp" and "scenebasic_uniform.h".

The main "update(float t)" function handles the gameplay loop, including delta time, camera movement, player boundary clamping, fairy orbit animation, collection checks, corruption collision checks and timer updates.

The "render()" function has several stages:
1) Render the shadow map from the light's point of view
2) Render the main scene into an off screen framebuffer
3) Render the skybox
4) Render good and corrupted fairies
5) Render fairy dust particles when activated
6) Run the bright and blur passes for bloom effect
7) Integrate the final scene and bloom texture
8) Draw UI text

## What Makes the Shader Program Special
The project combines multiple shader techniques into one interactive scene. While each feature are in different shaders, they are all connected to the theme and mechanics of the grove.
