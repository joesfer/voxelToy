voxelToy
========

VoxelToy is a simple Qt/OpenGL-based voxel renderer. This is a pet project currently in early stages of development, currently voxelToy implements:

- Progressive global illumination renderer (path tracing).
- Importance-sampled image based lighting.
- Lambertian BRDF.
- Orbit and fly-through camera.
- Camera depth of field.
- Dense voxel representation in 3D texture, DDA traversal. 
- Voxels can be seeded from input meshes. Voxelization carried out in GPU.
- Basic voxel adding/removing tool.
- Requires OpenGL 4.3.
- The project is developed in Linux (Ubuntu) using Vim/QtCreator. A CMake-based script for cross-platform building is provided, but no other platform has been tested yet. 

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot08.png)
Head model (c) I-R Entertainment. Downloaded from http://graphics.cs.williams.edu/data/meshes.xml

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot11.png)
Artwork by Mike Judge (@mikelovesrobots)

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot12.png)
Biome model by MagicaVoxel (https://voxel.codeplex.com)
