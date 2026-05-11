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

