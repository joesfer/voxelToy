#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <iostream>

#include "renderer/loaders/magicaVoxel.h"

// This namespace contains the functions to load a VOX file according to
// MagicaVoxel's specifications, based on the sample code from		
// https://voxel.codeplex.com/wikipage?title=Sample%20Codes		
namespace MagicaVoxel
{
	// magic number
	int MV_ID( int a, int b, int c, int d ) 
	{
		return ( a )        |
			   ( b << 8 )   |
			   ( c << 16 )  |
			   ( d << 24 );
	}

	struct MV_RGBA
	{
		unsigned char r, g, b, a;
	};

	struct MV_Voxel
	{
		unsigned char x, y, z, colorIndex;
	};	
	
	//================
	// Model
	//================
	class MV_Model 
	{
	public :
		// size
		int sizex, sizey, sizez;
		
		// voxels
		int numVoxels;
		MV_Voxel *voxels;

		// palette
		bool isCustomPalette;
		MV_RGBA palette[ 256 ];
		
		// version
		int version;
		
	public :
		~MV_Model()
		{
			free();
		}
		
		MV_Model() :
			sizex( 0 ),
			sizey( 0 ),
			sizez( 0 ),
			numVoxels( 0 ),
			voxels( NULL ),
			isCustomPalette( false ),
			version( 0 )
		{
		}
		
		void free( void ) 
		{
			if ( voxels )
			{
				delete[] voxels;
				voxels = NULL;
			}
			numVoxels = 0;
			
			sizex = sizey = sizez = 0;
			
			isCustomPalette = false;

			version = 0;
		}
		
		bool LoadModel( const char *path )
		{
			// free old data
			free();
			
			// open file
			FILE *fp = fopen( path, "rb" );
			if ( !fp  ){
				error( "failed to open file" );
				return false;
			}
			
			// read file
			bool success = readModelFile( fp );
			
			// close file
			fclose( fp );
			
			// if failed, free invalid data
			if ( !success ) 
			{
				free();
			}
			
			return success;
		}
		
	private :
		struct chunk_t
		{
			int id;
			int contentSize;
			int childrenSize;
			long end;
		};
		
	private :
		bool readModelFile( FILE *fp )
		{
			const int MV_VERSION = 150;
			
			const int ID_VOX  = MV_ID( 'V', 'O', 'X', ' ' );
			const int ID_MAIN = MV_ID( 'M', 'A', 'I', 'N' );
			const int ID_SIZE = MV_ID( 'S', 'I', 'Z', 'E' );
			const int ID_XYZI = MV_ID( 'X', 'Y', 'Z', 'I' );
			const int ID_RGBA = MV_ID( 'R', 'G', 'B', 'A' );
		   
			// magic number
			int magic = readInt( fp );
			if ( magic != ID_VOX )
			{
				error( "magic number does not match" );
				return false;
			}
			
			// version
			version = readInt( fp );
			if ( version != MV_VERSION )
			{
				error( "version does not match" );
				return false;
			}
			
			// main chunk
			chunk_t mainChunk;
			readChunk( fp, mainChunk );
			if ( mainChunk.id != ID_MAIN )
			{
				error( "main chunk is not found" );
				return false;
			}
			
			// skip content of main chunk
			fseek( fp, mainChunk.contentSize, SEEK_CUR );
			
			// read children chunks
			while ( ftell( fp ) < mainChunk.end )
			{
				// read chunk header
				chunk_t sub;
				readChunk( fp, sub );
				
				if ( sub.id == ID_SIZE )
				{
					// size
					sizex = readInt( fp );
					sizey = readInt( fp );
					sizez = readInt( fp );
				}
				else if ( sub.id == ID_XYZI )
				{
					// numVoxels
					numVoxels = readInt( fp );
					if ( numVoxels < 0 )
					{
						error( "negative number of voxels" );
						return false;
					}
					
					// voxels
					if ( numVoxels > 0 )
					{
						voxels = new MV_Voxel[ numVoxels ];
						fread( voxels, sizeof( MV_Voxel ), numVoxels, fp );
					}
				}
				else if ( sub.id == ID_RGBA )
				{
					// last color is not used, so we only need to read 255 colors
					isCustomPalette = true;
					fread( palette + 1, sizeof( MV_RGBA ), 255, fp );

					// NOTICE : skip the last reserved color
					MV_RGBA reserved;
					fread( &reserved, sizeof( MV_RGBA ), 1, fp );
				}

				// skip unread bytes of current chunk or the whole unused chunk
				fseek( fp, sub.end, SEEK_SET );
			}
			
			// print model info
			printf( "[Log] MV_VoxelModel :: Model : %d %d %d : %d\n",
				   sizex, sizey, sizez, numVoxels
				   );
			
			return true;
		}
		
		void readChunk( FILE *fp, chunk_t &chunk )
		{
			// read chunk
			chunk.id = readInt( fp );
			chunk.contentSize  = readInt( fp );
			chunk.childrenSize = readInt( fp );
			
			// end of chunk : used for skipping the whole chunk
			chunk.end = ftell( fp ) + chunk.contentSize + chunk.childrenSize;
			
			// print chunk info
			const char *c = ( const char * )( &chunk.id );
			printf( "[Log] MV_VoxelModel :: Chunk : %c%c%c%c : %d %d\n",
				   c[0], c[1], c[2], c[3],
				   chunk.contentSize, chunk.childrenSize
				   );
		}
		
		int readInt( FILE *fp ) 
		{
			int v = 0;
			fread( &v, 4, 1, fp );
			return v;
		}
		
		void error( const char *info ) const 
		{
			printf( "[error] MV_VoxelModel :: %s\n", info );
		}
	};

	const unsigned int defaultPalette[ 256 ] = 
	{
		0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
		0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
		0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
		0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
		0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
		0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
		0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
		0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
		0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
		0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
		0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
		0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
		0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
		0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
		0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
		0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
	};
} // namespace MagicaVoxel

bool MagicaVoxelLoader::load(const std::string& filePath,
							 std::vector<GLint>& voxelMaterials, 
							 std::vector<float>& materialData,
							 Imath::V3i& voxelResolution)
{
	using namespace MagicaVoxel;
	MV_Model model;
	if (!model.LoadModel(filePath.c_str())) return false;
	
	// extract model data. Do axis conversion (z<->y)
	voxelResolution.x = model.sizex;
	voxelResolution.y = model.sizez;
	voxelResolution.z = model.sizey;

	size_t numVoxels = voxelResolution.x * voxelResolution.y * voxelResolution.z;
	voxelMaterials.resize(numVoxels, -1);

	const unsigned char* palette = model.isCustomPalette ? 
		reinterpret_cast<const unsigned char*>(model.palette) :
		reinterpret_cast<const unsigned char*>(MagicaVoxel::defaultPalette);

	GLint materialDataOffset[256];
	for(int i = 0; i < 256; ++i ) materialDataOffset[i] = -1;

	for( int i = 0; i < model.numVoxels; ++i )
	{
		const MV_Voxel& v = model.voxels[i];
		size_t voxelOffset = v.x + 
							 v.z * voxelResolution.x + 
							 v.y * voxelResolution.x * voxelResolution.y;

		if (v.colorIndex == 254) continue; // empty voxel

		// index material
		voxelMaterials[voxelOffset] = materialDataOffset[v.colorIndex]; 
		if ( voxelMaterials[voxelOffset] >= 0 )
		{
			// we've inserted this material already, continue
			continue;
		}

		// New material -- append data
		// With the information available we can only generate Lambertian materials
		GLint materialOffset = (GLint)materialData.size();
		materialDataOffset[v.colorIndex] = materialOffset;
		voxelMaterials[voxelOffset] = materialOffset; 

		Imath::V3f emission(0,0,0);
		Imath::V3f albedo((float)palette[4*v.colorIndex+0] / 255,
						  (float)palette[4*v.colorIndex+1] / 255,
						  (float)palette[4*v.colorIndex+2] / 255);

		generateMaterialLambert(emission, albedo, materialData);
	}
	return true;
}

