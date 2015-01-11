voxelToy
========

VoxelToy is a simple Qt/OpenGL-based voxel renderer. This is a pet project currently in very early stages of development, and all but the most basic features are still missing. Currently voxelToy implements:

- progressive renderer via path tracing.
- Lambertian BRDF only.
- Orbit camera.
- Camera depth of field.
- Dense voxel representation in 3D texture, DDA traversal. 
- Voxels can be seeded from input meshes. Voxelization carried out in GPU.
- The project is developed in Linux (Ubuntu) using Vim/QtCreator. A CMake-based script for cross-platform building is provided, but no Windows/Mac version has been tested yet.

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot08.png)
