// forward declarations
int binarySearchCDFV( int indexHigh, float randomSample );
int binarySearchCDFU( int row, int indexHigh, float randomSample );

// To sample the environment texture we start with a pair of uniform random
// variables and find the row/column indices corresponding to the low end of the
// "bracket" containing the value of the random sample. PBRT does this by using 
// std::lower_bound, but on the GPU we'll turn it into a binary search as we 
// know the CDF arrays are sorted. 
// For instance:
//
// Uniform random value: 0.7
//
// CDF [0, 0.1, 0.5, 0.6, 0.75, 0.9, 1]
//                    |     |
//                  low     high
//                    |
//                    returned: 3
//
// The CDF "bucket size" are proportional to the pixel intensity, thus this 
// method will sample higher intensities more often.

vec2 sampleEnvironmentTexture( vec2 uniformRandomSample )
{
	const float sampleU = uniformRandomSample.x;
	const float sampleV = uniformRandomSample.y;
	const int sizeU = textureSize(backgroundCDFUTexture, 0).x;
	const int sizeV = textureSize(backgroundCDFVTexture, 0);

	// Index on the last entry containing 1.0f. Can never be reached with the 
	// sample in the range [0.0f, 1.0f).	
	int indexHigh = sizeV - 1 ; 

	// Binary search the ROW index to look up.
	int i = 0;
	int row = binarySearchCDFV(indexHigh, sampleV);

	ivec2 index; // Resulting 2D index we'll use to fetch texels
	index.y = row; // This is the row we found.
		
	// Index on the last entry containing 1.0f. Can never be reached with the 
	// sample in the range [0.0f, 1.0f).
	indexHigh = sizeU - 1; 

	// Binary search the COLUMN index to look up.
	int column = binarySearchCDFU(row, indexHigh, sampleU);
	index.x = column; // The column result.

	// The previous searches have given us the lower bound of the 'brackets' on
	// both CDF U and V, we now get the other end of the bracket and perform the 
	// continuous sampling of the CDF to get the final u and v coordinates.
	const float cdfLowerU = texelFetch(backgroundCDFUTexture, index, 0).r;
	const float cdfUpperU = texelFetch(backgroundCDFUTexture, ivec2(index.x + 1, index.y), 0).r;
	const float du = float(sampleU - cdfLowerU) / (cdfUpperU - cdfLowerU);

	const float cdfLowerV = texelFetch(backgroundCDFVTexture, index.y, 0).r;
	const float cdfUpperV = texelFetch(backgroundCDFVTexture, index.y + 1, 0).r;
	const float dv = float(sampleV - cdfLowerV) / (cdfUpperV - cdfLowerV);

	// Texture lookup coordinate.
	const float u = (float(index.x) + du) / float(sizeU - 1);
	const float v = (float(index.y) + dv) / float(sizeV - 1);

	return vec2(u,v);
}

////////////////////////////////////////////////////////////////////////////////

// Binary search the ROW corresponding to the given random sample
int binarySearchCDFV( int indexHigh, float randomSample )
{
	int indexLow = 0; 
	
	// When a pair of limits have been found the lower index indicates the cell 
	// to use.
	while (indexLow != indexHigh - 1) 
	{
		int row = (indexLow + indexHigh) / 2;
		const float cdf = texelFetch(backgroundCDFVTexture, row, 0).r;
		if (randomSample < cdf) 
		{
			// If the cdf is greater than the sample, use that as new higher limit.
			indexHigh = row;
		}
		else 
		{
			// If the sample is greater than or equal to the CDF value, use 
			// that as new lower limit.
			indexLow = row; 
		}
	}

	return indexLow;
}

// Binary search the COLUMN corresponding to the given random sample, and a
// pre-selected row (which we chose from CDF V)
int binarySearchCDFU( int row, int indexHigh, float randomSample )
{
	int indexLow = 0; 

	// When a pair of limits have been found the lower index indicates the cell 
	// to use.
	while (indexLow != indexHigh - 1) 
	{
		int column = (indexLow + indexHigh) / 2;
		ivec2 uv = ivec2(column, row);
		const float cdf = texelFetch(backgroundCDFUTexture, uv, 0).r;
		if (randomSample < cdf) 
		{
			// If the cdf is greater than the sample, use that as new higher limit.
			indexHigh = column;
		}
		else 
		{
			// If the sample is greater than or equal to the CDF value, use 
			// that as new lower limit.
			indexLow = column; 
		}
	}

	return indexLow;
}


