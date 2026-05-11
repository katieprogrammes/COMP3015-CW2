# COMP3015-Coursework 2 - Shadow Grove
Shadow Grove is an interactive OpenGL forest scene built from the Project Template provided but extended into a small playable graphics prototype using the helper files provided in the module labs.

The player explores a dark magical grove that has been attacked by corrupted red fairies. They collect blue fairies to save them while avoiding the red fairies to restore the forest. The scene uses several GLSL shader techniques from the module, including shadow mapping, bloom post-processing, procedural noise, particle animation, skybox rendering, textured terrain, and fog-style atmospheric blending. The aim of the project was to combine graphics programming techniques into a cohesive game-like scene.

## Dependencies
This project uses the COMP3015 OpenGL template and helper files provided during the module labs aside from shader_m.h and camera.h which were taken from learnopenGL to support the skybox and camera movement. It also includes:
- OpenGL 4.6 / GLSL
- GLFW / GLAD / GLM as provided by the COMP3015 template

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

The player is kept inside the forest area using a camera position clamp. A tree border surrounds the playable area to visually represent the boundary - this was inspired by the Pokemon games.

## Gameplay Objectives

The objective is to collect all the blue fairies in the grove. Each collected fairy increases the score. When all blue fairies are collected, the grove enters the restored state and displays: "The Grove is Restored". Red fairies act as hazards. When the player gets too close to one, a corruption effect is triggered. This temporarily changes the atmosphere using darker fog and displays a corruption warning.

## Main Implemented Features

### 1. Shadow Mapping

<img width="399" height="297" alt="image" src="https://github.com/user-attachments/assets/9605e13c-45f1-410c-9a4a-c0720d1694e8" />

The project uses a shadow map pass before the main render pass. The shadow map is rendered from the light’s point of view into a depth texture. This is then sampled in the main fragment shader to determine whether each fragment is lit or shadowed.
Framebuffer Object setup function:
<img width="380" height="363" alt="image" src="https://github.com/user-attachments/assets/ca4fdae5-85f1-440f-8123-e4861c05fc98" />

Light frustrum setup:
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





