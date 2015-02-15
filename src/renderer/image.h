#pragma once

#include <string>
#include <vector>

/// Load image and convert it to float RGB format. Return true if successful,
/// false if an error occurred. 
bool loadImage(const std::string& path,
			   unsigned int& outWidth, unsigned int& outHeight,
			   std::vector<float>& outPixelData);

/// This method computes the piece-wise constant distribution functions over a 
/// 2D image used to sample such image. We'll do this to perform importance 
/// sampling of the environment map. 
/// The implementation here matches that of PBRT 2 (Chapter 14.6.5).
///
/// Parameters:
///
///		rgbPixels, 
///     imagewidth, 
///     imageHeight : original RGB float image
///
///     cdfUData, 
///     cdfUDataWidth, 
///     cdfUDataHeight: 2D buffer describing the resulting 2D CDF function.
///
///     cdfVData : 1D buffer describing the marginal 1D CDF function. The size
///                is given by cdfVData.size().
///
///     environmentTextureIntegral : intgral of image pixel intensities (used to
///                                  calculate each sample's PDF).
///
bool calculateCDF( const float* rgbPixels, unsigned int imageWidth, unsigned int imageHeight,
				   std::vector<float>& cdfUData, unsigned int& cdfUDataWidth, unsigned int& cdfUDataHeight, // (imageWidth + 1 ) x imageHeight
				   std::vector<float>& cdfVData, 
				   float& environmentTextureIntegral );
