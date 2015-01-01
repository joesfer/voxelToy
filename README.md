voxelToy
========

VoxelToy is a simple Qt/OpenGL-based voxel renderer. This is a pet project currently in very early stages of development, and all but the most basic features are still missing. Currently voxelToy implements:

- voxel rendering via raymarching.
- Orbit camera.
- Simple ambient occlusion based on voxel adjacency.
- Camera depth of field via multi-sampling.
- Dense voxel representation in 3D texture, currently hard-coded. Authoring tools badly needed (I'll probably start porting the voxelization code in http://www.joesfer.com/?p=84).
- Developed in Linux (Ubuntu) using Vim/QtCreator, IDE-agnostic build (CMake or similar) sorely needed.

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot06.png)
