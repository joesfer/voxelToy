voxelToy
========

VoxelToy is a simple Qt/OpenGL-based voxel renderer. The project is in early stages of development, and it currently implements:

- Progressive global illumination renderer (path tracing).
- Importance-sampled image based lighting.
- Lambertian BRDF for matte materials, Torrance-Sparrow for metals.
- Orbit and fly-through camera.
- Camera depth of field.
- Dense voxel representation in 3D texture, DDA traversal. 
- Voxels can be seeded from input meshes. Voxelization carried out in GPU.
- Basic voxel adding/removing tool.
- Requires OpenGL 4.3.
- The project is developed in Linux (Ubuntu) using Vim/QtCreator. A CMake-based script for cross-platform building is provided, but no other platform has been tested yet. 

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot08.png)
GPU voxelization. Head model (c) I-R Entertainment. Downloaded from http://graphics.cs.williams.edu/data/meshes.xml

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot13.png)
Camera DOF and Image-based lighting. Artwork by Mike Judge (@mikelovesrobots)

![alt tag](https://github.com/joesfer/voxelToy/blob/master/resources/screenshot12.png)
Image-based lighting. Biome model by MagicaVoxel (https://voxel.codeplex.com)
