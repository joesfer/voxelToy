voxelToy
========

VoxelToy is a simple Qt/OpenGL-based voxel renderer. This is a pet project currently in very early stages of development, and all but the most basic features are still missing. Currently voxelToy implements:

- progressive renderer via path tracing.
- Lambertian BRDF only.
- Orbit camera.
- Camera depth of field.
- Dense voxel representation in 3D texture, currently hard-coded. 

Most immediate improvements to follow:
- Some authoring tools to create voxel content - I'll probably start porting the voxelization code in http://www.joesfer.com/?p=84
- The project is developed in Linux (Ubuntu) using Vim/QtCreator, but an IDE-agnostic build (CMake or similar) is sorely needed.

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot07.png)
