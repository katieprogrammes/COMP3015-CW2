# COMP3015-Coursework 2 - Shadow Grove
Shadow Grove is an interactive OpenGL forest scene built from the Project Template provided but extended into a small playable graphics prototype using the helper files provided in the module labs.

The player explores a dark magical grove that has been attacked by corrupted red fairies. They collect blue fairies to save them while avoiding the red fairies to restore the forest. The scene uses several GLSL shader techniques from the module, including shadow mapping, bloom post-processing, procedural noise, particle animation, skybox rendering, textured terrain, and fog-style atmospheric blending. The aim of the project was to combine graphics programming techniques into a cohesive game-like scene.

## Dependencies

## How To Compile
You can clone from github into Visual Studio then open the .sln (solution) file and you can then either run through the Debug or Release configuration (x64 version).

The .exe file provided in Release/x64 should run without any additional setup.

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
Bloom is implemented as a multi-pass post-processing effect. The scene is rendered to a texture, bright areas are extracted, blurred, and then layered back over the final image.





