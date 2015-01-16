#pragma once

#include <stdint.h>

class Morton
{
public:
	// enconde a set of 3D coordinates and a corresponding depth on the octree
	// into a 64-bit morton code. 
	static uint64_t mortonEncode(uint32_t x,       // 10 bits
								 uint32_t y,       // 10 bits
								 uint32_t z,       // 10 bits
								 uint8_t depth,    // 4 bits
								 uint8_t childMask // 8 bits
								 );
	// Decode a 64-bit morton code into its corresponding integer coordinates
	// and octree depth.
	static void mortonDecode(uint64_t mortonCode, 
							 uint32_t& x, 
							 uint32_t& y, 
							 uint32_t& z, 
							 uint8_t& depth,
							 uint8_t& childMask );
};
