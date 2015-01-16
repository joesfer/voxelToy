#pragma once
#include <vector>
#include <stdint.h>
class Svo
{
public:
	// take an array containing only the morton codes corresponding to the leaf
	// nodes of the octree (with empty values for their voxel depth and child
	// masks, since these are unknown yet), and produce another array containing
	// all the nodes in the corresponding octree -including leaves- with fully
	// formed morton codes (including child masks and depth).
	static bool buildOctreeBottomUp( const std::vector<uint64_t>& leaves,
									 uint8_t leavesVoxelDepth, // all leaves are assumed to be at the same depth
									 std::vector<uint64_t>& fullOctree );
};
