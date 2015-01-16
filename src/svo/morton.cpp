#include "svo/morton.h"

/*
 * Morton key
 *
 * |00[z9y9x9z8y8x8....z0y0x0]00..000[cccccccc][ddddd] 
 * |2 |       3 * 10 = 30    |   20  |    8    |  4  |
 * ------------------------ 64 ----------------------
 */


uint32_t part1By2(uint32_t x)
{
	x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
	x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
	x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
	x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
	x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
	return x;
}

uint32_t encodeMortonPosition(uint32_t x, uint32_t y, uint32_t z)
{
	return (part1By2(z)<<2) | (part1By2(y)<<1) | part1By2(x);
}

uint32_t compact1By2(uint32_t x)
{
	x &= 0x09249249;                  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
	x = (x ^ (x >>  2)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
	x = (x ^ (x >>  4)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
	x = (x ^ (x >>  8)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
	x = (x ^ (x >> 16)) & 0x000003ff; // x = ---- ---- ---- ---- ---- --98 7654 3210
	return x;
}

uint32_t decodeMorton3X(uint32_t code) { return compact1By2(code >> 0); }
uint32_t decodeMorton3Y(uint32_t code) { return compact1By2(code >> 1); }
uint32_t decodeMorton3Z(uint32_t code) { return compact1By2(code >> 2); }

/*static*/ uint64_t Morton::mortonEncode(uint32_t x, uint32_t y, uint32_t z, 
										 uint8_t depth,
										 uint8_t childMask)
{
	// clamp inputs to their respective maximum number of bits
	x &= 0x3FF;
	y &= 0x3FF;
	z &= 0x3FF;
	depth &= 0xF;

	uint32_t mortonHi = encodeMortonPosition(x, y, z);
	uint32_t mortonLow = uint32_t(depth) | (uint32_t(childMask) << 8);
    uint64_t mortonCode = uint64_t(mortonLow) | (uint64_t(mortonHi) << 32);
	return mortonCode;
}

// Decode a 64-bit morton code into its corresponding integer coordinates
// and octree depth.
/*static*/ void Morton::mortonDecode(uint64_t mortonCode, 
									 uint32_t& x, 
									 uint32_t& y, 
									 uint32_t& z, 
									 uint8_t& depth,
									 uint8_t& childMask)
{
    uint32_t mortonLow = uint32_t(mortonCode & 0x00000000FFFFFFFF);
    uint32_t mortonHi = uint32_t((mortonCode & 0xFFFFFFFF00000000) >> 32);
	
	x = decodeMorton3X(mortonHi);
	y = decodeMorton3Y(mortonHi);
	z = decodeMorton3Z(mortonHi);

	depth = uint8_t(mortonLow & 0xF);
	childMask = uint8_t((mortonLow >> 8) & 0xFF);
}

