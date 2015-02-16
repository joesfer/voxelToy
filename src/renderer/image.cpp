#include "renderer/image.h"
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <math.h>

// This clamps the resolution of the CDF functions used to sample the
// environment map. It is a tradeoff between quality and performance: larger CDF
// will capture more detail (e.g. small and bright pixels on the image which act
// as point lights) but require more steps on the binary search thus degrading
// performance. The smaller the map, the faster we'll find the region to sample
// from, but we might miss high frequency detail on the original image.
#define MAX_CDF_SIZE 512

/// Forward declaration
float calculateImageIntegral(const OpenImageIO::ImageBuf& image,
							 float* functionU);
bool generateImageFunction( const float* rgbPixels, 
							unsigned int imageWidth, 
							unsigned int imageHeight,
							OpenImageIO::ImageBuf& result );

// =============================================================================
/// Load image and convert it to float RGB format. Return true if successful,
/// false if an error occurred. 
// =============================================================================
bool loadImage(const std::string& path,
			   unsigned int& outWidth, unsigned int& outHeight,
			   std::vector<float>& outPixelData)
{
	OIIO_NAMESPACE_USING

	ImageBuf buf(path.c_str());
	if (!buf.read()) return false;

	ImageSpec spec = buf.spec();
	outWidth = (unsigned int)spec.width;
	outHeight = (unsigned int)spec.height;
	outPixelData.resize(outWidth * outHeight * 3);
	if( spec.nchannels == 3 )
	{
		// the image is in the expected format. We can copy the pixels directly.
		return buf.get_pixels(0, outWidth, 
						      0, outHeight,
						      0, 1,
							  TypeDesc::FLOAT,
						      &outPixelData[0]);
	}

	ImageBuf rgb;
	if (!ImageBufAlgo::channels(rgb, buf, 3, NULL, NULL, NULL, true)) return false;

	return rgb.get_pixels(0, outWidth, 
						  0, outHeight,
						  0, 1,
						  TypeDesc::FLOAT,
						  &outPixelData[0]);
}

// =============================================================================
/// This method computes the distribution functions over a 2D image used to
/// sample such image. We'll do this to perform importance sampling of the
/// environment map. 
/// The implementation here matches that of PBRT 2 (Chapter 14.6.5).
// =============================================================================

bool calculateCDF( const float* rgbPixels, unsigned int imageWidth, unsigned int imageHeight,
				   std::vector<float>& cdfUData, unsigned int& cdfUDataWidth, unsigned int& cdfUDataHeight,
				   std::vector<float>& cdfVData, 
				   float& environmentTextureIntegral )
{
	/*
	 *  pixel intensities
	 *  -----------------
	 *
	 *  We start from an NxM image described as a RGB float array, but we'll 
	 *  simply use the pixels average intensity to calculate the distribution 
	 *  functions we'll sample from.
	 *
	 *   ---------------------------
	 *  |   |   |   |   |   |   |   | Row 0
	 *   ---+---+---+---+---+---+---|
	 *  |   |   |   |   |   |   |   | Row 1
	 *   ---+---+---+---+---+---+---|
	 *  |   |   |   |   |   |   |   | ...
	 *   ---+---+---+---+---+---+---|
	 *  |   |   |   |   |   |   |   | Row M
	 *   ---+---+---+---+---+---+---
	 *   Column 0          |
	 *      Column 1       |
	 *         ...         |
	 *                    Column N
	 *
	 *  CDF U
	 *  -----
	 *
	 *  Has one extra column than the original image, and the same number of
	 *  row. Each row contains the normalized cummulative distribution of the
	 *  matching row in the original image. 
	 *  For a given row r, we calculate the monotonically increasing values of
	 *  the columns by adding the pixel intensity to the previous column value,
	 *  and normalizing the result by the sum of the row's intensities. The last
	 *  column is thus be Sum(Row) / Sum(Row) = 1. It is there to simplify the 
	 *  binary search algorithm we'll use on this data.
	 *
	 *       Example intensities in original image
	 *       ----------------------------
	 *      | 1 | 1 | 0 | 2 | 1 | 0 | 5 |  Row 0
	 *       ---+---+---+---+---+---+---+
	 *       C0                      CM  CM+1
	 *
	 *   first sum intensity values
	 *   -------------------------------
	 *  | 0 | 1 | 2 | 2 | 4 | 5 | 5 |10 | Row 0
	 *   ---+---+---+---+---+---+---+---+
	 *  | ------------------------->|   |
	 *  | monotonically increasing  |   |
	 *
	 *   C0                      CM  CM+1
	 *
	 *   Then normalize (div by 10)
	 *   -------------------------------
	 *  | 0 |0.1|0.2|0.2|0.4|0.5|0.5| 1 | Row 0
	 *   ---+---+---+---+---+---+---+---
	 *  | ------------------------->|   |
	 *  | monotonically increasing  |   |
	 *  |                           |   |
	 *  |                           |   |
	 *   ---+---+---+---+---+---+---+---
	 *  | 0 |   |   |   |   |   |   | 1 | Row M
	 *   ---+---+---+---+---+---+---+---
	 *   C0                      CM  CM+1
	 *
	 * CDF V
	 * -----
	 *
	 * Here we apply a similar process than above, but generating a 1D
	 * distribution by integrating the intensities of each row on the original
	 * image. The CDF V array has one extra row than the original image. Each
	 * row contains the monotonically increasing distribution of the intgral
	 * over each row pixels intensity.
	 *
	 *                                        CDF V      Normalized CDF V 
	 *                                        ----        ----                  
	 *    original image intensities         | 0  |      | 0  | |
	 *   ---------------------------         |----|      |----| |
	 *  | 1 | 1 | 0 | 2 | 1 | 0 | 5 |------->| 10 |----->|0.06| | monotonically
	 *  |---+---+---+---+---+---+---|        |----|      |----| | increasing
	 *  | 2 | 1 | 5 | 0 | 0 | 1 | 3 |        | 22 |      |0.14| |
	 *  |---+---+---+---+---+---+---|        |----|      |----| |
	 *  |                           | ...    | .. |      | .. | V
	 *  |---+---+---+---+---+---+---|        |----|      |----|
	 *  |   |   |   |   |   |   |   | Row M  |150 |      | 1  | Row M+1
	 *   ---+---+---+---+---+---+---          ----        ---- 
	 * 
	 *
	 * Once we have CDF U and V, the sampling process is to simply choose two
	 * uniform random variables, Ux and Uy. We use Uy to pick a row by searching
	 * the corresponding element in CDF V. Then we do the same with Ux and the
	 * corresponding row in CDF U, giving us the pixel column.
	 *
	 */

	// First generate pixel intensities.
	OIIO_NAMESPACE_USING 
	OpenImageIO::ImageBuf filteredIntensities;
	if (!generateImageFunction( rgbPixels, 
								imageWidth, 
								imageHeight,
								filteredIntensities )) return false;

	imageWidth  = filteredIntensities.spec().width;
	imageHeight = filteredIntensities.spec().height;

	cdfUDataWidth                      = imageWidth + 1;
	cdfUDataHeight                     = imageHeight;
	const unsigned int cdfVNumElements = imageHeight + 1;

	cdfUData.resize( cdfUDataWidth * cdfUDataHeight );
	cdfVData.resize( cdfVNumElements );
	float* cdfU = &cdfUData[0];
	float* cdfV = &cdfVData[0];

	// The actual functions we generate the CDF from are:
	// Function U (2D) : pixel intensities
	// Function V (1D) : integral of intensities over each row
	std::vector<float> storageFuncU, storageFuncV;
	storageFuncU.resize( imageWidth * imageHeight );
	storageFuncV.resize( imageHeight + 1 );
	float* functionU = &storageFuncU[0];
	float* functionV = &storageFuncV[0];

	const float iW = (float)imageWidth;
	const float iH = (float)imageHeight;


	// first generate the data for Function U
	environmentTextureIntegral = calculateImageIntegral( filteredIntensities, 
														 functionU );

	// Now generate the CDF U.
	// Normalized 1D distributions in the rows of the 2D buffer, and the
	// marginal CDF in the 1D buffer. 
	// Include the starting 0.0 and the ending 1.0 to avoid special cases during
	// the continuous sampling.

	// note this matches the original image dimensions, only CDFU is 1 element 
	// longer
	const unsigned int functionUWidth = imageWidth; 
	const unsigned int numStepsW = iW; // PBRT2 p.648.
	for (unsigned int y = 0; y < imageHeight; ++y)
	{
		unsigned int row = y * cdfUDataWidth; 
		cdfU[row + 0] = 0.0f; // CDF starts at 0.0f.

		for (unsigned int x = 1; x <= imageWidth; ++x)
		{
			const float f = functionU[y * functionUWidth + x - 1] / numStepsW; 
			unsigned int columnOffset = row + x;
			// this is not yet a CDF, but the step function integral. We'll turn
			// it into an actual CDF when we divide by rowIntegral.
			cdfU[columnOffset] = cdfU[columnOffset - 1] + f; 
		}

		// The last CDF element contains the integral over the row. Note we have
		// not yet normalized the cdf.
		const float rowIntegral = cdfU[row + imageWidth]; 
		// Store this as function values of the marginal CDF.
		functionV[y] = rowIntegral; 

		// If all texels were black in this row, generate an equal distribution.
		if (rowIntegral > 0.0f)
		{
			for (unsigned int x = 1; x <= imageWidth; ++x)
			{
				cdfU[row + x] /= rowIntegral;
			}
		}
		else 
		{
			for (unsigned int x = 1; x <= imageWidth; ++x)
			{
				cdfU[row + x] = (float)x / numStepsW;
			}
		}
	} // for y

	// Now do the same thing with the marginal CDF.

	cdfV[0] = 0.0f; // CDF starts at 0.0f.
	const unsigned int numStepsH = imageHeight;

	for (unsigned int y = 1; y <= imageHeight; ++y)
	{
		const float f = functionV[y - 1] / numStepsH;
		// step function integral.
		cdfV[y] = cdfV[y - 1] + f;
	}
	
	// Convert step function integral into CDF V. 
	// The integral over this marginal CDF is in the last element.
	const float imageIntegral = cdfV[imageHeight]; 
	functionV[imageHeight] = imageIntegral; 

	// If all texels were black in the whole image, generate an equal distribution.
	if (imageIntegral > 0.0f)
	{
		for (unsigned int y = 1; y <= imageHeight; ++y)
		{
			cdfV[y] /= imageIntegral;
		}
	}
	else 
	{
		for (unsigned int y = 1; y <= imageHeight; ++y)
		{
			cdfV[y] = (float)y / iH;
		}
	}

	return true;
}

bool generateImageFunction( const float* rgbPixels, 
							unsigned int imageWidth, 
							unsigned int imageHeight,
							OpenImageIO::ImageBuf& result )
{
	// PBRT2 Page 726.
	// The CDF is generated over a slightly blurred version of the
	// original image. The reason is that we use linear blending of
	// texels during rendering, and that may mean that a black texel has
	// non-zero radiance near its center due to contribution of an
	// adjacent (non-black texel). If we simply copied the texel values
	// for the piecewise _CONSTANT_ CDF we sample from, the whole
	// surface of the texel would be black. This would not happen if we
	// used a piecewise linear CDF, but constant is easier/cheaper. To
	// solve this, we simply blur the source image function slightly
	// which addresses this problem and produces non-zero values for the
	// case described, guaranteed that the "almost" black pixel would be
	// sampled with >0 probability.

	OIIO_NAMESPACE_USING

	ImageSpec spec(imageWidth, imageHeight, 3, TypeDesc::FLOAT);
	ImageBuf source(spec, (void*)rgbPixels);

	if (std::max(imageWidth, imageHeight) > MAX_CDF_SIZE)
	{
		// shrink image
		ImageBuf resized;
		unsigned int w = (unsigned int )((float)imageWidth / std::max(imageWidth, imageHeight) * MAX_CDF_SIZE);
		unsigned int h = (unsigned int )((float)imageHeight / std::max(imageWidth, imageHeight) * MAX_CDF_SIZE);
		ROI roi (0, w, 
				 0, h, 
				 0, 1, 
				 /*chans:*/ 0, source.nchannels());
		ImageBufAlgo::resize(resized, source, "", 0, roi);
		source.swap(resized);
	}

	ImageBuf intensities;
	ImageBuf blurredIntensities;

	// Compute luminance via a weighted sum of R,G,B
	// (assuming Rec709 primaries and a linear scale)
	float lumaWeights[3] = { 0.2126f, 0.7152f, 0.0722f };
	ROI roi = source.roi();
	roi.chbegin = 0; 
	roi.chend = 3;
	if (!ImageBufAlgo::channel_sum (intensities, 
								    source, 
								    lumaWeights, 
								    roi)) return false;
	
	// Blur the image with a 3x3 Gaussian kernel
	ImageBuf kernel;
	ImageBufAlgo::make_kernel (kernel, "gaussian", 3.0f, 3.0f);
	if (!ImageBufAlgo::convolve(blurredIntensities, 
							    intensities, 
								kernel)) return false;

	result.swap(blurredIntensities);
	return true;
}


float calculateImageIntegral(const OpenImageIO::ImageBuf& image,
							 float* functionU)
{
	unsigned int imageWidth  = image.spec().width;
	unsigned int imageHeight = image.spec().height;
	const float iW = (float)imageWidth;
	const float iH = (float)imageHeight;
	const float iA = iW * iH;

	float textureTimesSinSum = 0;

	float value;
	for( unsigned int y = 0; y < imageHeight; ++y )
	{
		// Scale distribution by the sine to get the sampling uniform. (Avoid
		// sampling more value near the poles.)
		// See PBRT2, chapter 14.6.5 on Infinite Area Lights, page 727.
		float sinTheta = (float)sin(M_PI * ((float)y + 0.5f) / iH); 

		for( unsigned int x = 0; x < imageWidth; ++x )
		{
			image.getpixel(x, y, &value, 1);
			value = std::max(0.f, value);
			functionU[y * imageWidth + x] =  value * sinTheta;
			textureTimesSinSum            += value * sinTheta;
		}
	}

	// The integral of the texture times a sin factor is used to calculate the
	// PDF. The idea is to reduce the oversampling that would otherwise occur at
	// the poles of the sampled sphere by 'toning down' the importance at such
	// poles with a sin factor.
	float environmentTextureIntegral = textureTimesSinSum / iA;

	// Roll the 2PI^2 factor required to calculate the jacobian on the area PDF 
	// right into this constant to save the calculation. PBRT2 page 729. This is 
	// taken into account in EnvironmentLight.cu
	environmentTextureIntegral *= 2.0f * M_PI * M_PI;

	return environmentTextureIntegral;
}


