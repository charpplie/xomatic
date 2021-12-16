#include "stdafx.h"
#include "CryTextureCompressor.h"
#include "ColorBlock4x4.h"
#include "ColorSet.h"
#include "ClusterCompressor.h"
#include "OptimalCompressor.h"


void CryTextureCompressor::CompressDXT1(const CryTextureCompressor::CompressorParameters& compress)
{
	const bool dxt1 = true;
	const bool mergeMatchingColors = true;

	ClusterCompressor compressor;
	compressor.SetMetric( compress.rWeight, compress.gWeight, compress.bWeight );

	ColorBlock4x4 srcBlock;
	BlockDXT1 dstBlock;

	const unsigned int w = compress.width;
	const unsigned int h = compress.height;

	for (unsigned int y = 0; y < h; y += 4)
	{
		for (unsigned int x = 0; x < w; x += 4)
		{
			srcBlock.set(compress.srcBGRA8, w,h, compress.pitch, x,y);

			ColorSet colorSet(srcBlock, dxt1, mergeMatchingColors);
			compressor.Compress(&colorSet, dxt1, dstBlock.color);

			if (compress.userOutputFunction)
			{
				compress.userOutputFunction(compress, &dstBlock, sizeof(dstBlock));
			}
		}
	}
}


void CryTextureCompressor::CompressDXT1ForceOpaque(const CryTextureCompressor::CompressorParameters& compress)
{
	const bool dxt1 = true;
	const bool mergeMatchingColors = true;

	ClusterCompressor compressor;
	compressor.SetMetric( compress.rWeight, compress.gWeight, compress.bWeight );

	ColorBlock4x4 srcBlock;
	BlockDXT1 dstBlock;

	const unsigned int w = compress.width;
	const unsigned int h = compress.height;

	for (unsigned int y = 0; y < h; y += 4)
	{
		for (unsigned int x = 0; x < w; x += 4)
		{
			srcBlock.set(compress.srcBGRA8, w,h, compress.pitch, x,y);

			// Force pixels to be opaque
			{
				ColorBGRA8* const c = srcBlock.colors();
				for(unsigned int i=0; i<16; ++i)
				{
					c[i].a = 0xFF;
				}
			}

			if(srcBlock.isSingleColorIgnoringAlpha())
			{
				OptimalCompressor::CompressDXT1SingleColor3or4(srcBlock.color(0), dstBlock.color);
			}
			else
			{
				ColorSet colorSet(srcBlock, dxt1, mergeMatchingColors);
				compressor.Compress(&colorSet, dxt1, dstBlock.color);
			}

			if (compress.userOutputFunction)
			{
				compress.userOutputFunction(compress, &dstBlock, sizeof(dstBlock));
			}
		}
	}
}


void CryTextureCompressor::CompressDXT3(const CryTextureCompressor::CompressorParameters& compress)
{
	const bool dxt1 = false;
	const bool mergeMatchingColors = true;

	ClusterCompressor compressor;
	compressor.SetMetric( compress.rWeight, compress.gWeight, compress.bWeight );

	ColorBlock4x4 srcBlock;
	BlockDXT3 dstBlock;

	const unsigned int w = compress.width;
	const unsigned int h = compress.height;

	for (unsigned int y = 0; y < h; y += 4)
	{
		for (unsigned int x = 0; x < w; x += 4)
		{
			srcBlock.set(compress.srcBGRA8, w,h, compress.pitch, x,y);

			OptimalCompressor::CompressDXT3Alpha(srcBlock, dstBlock.alpha);

			if(srcBlock.isSingleColorIgnoringAlpha())
			{
				OptimalCompressor::CompressDXT1SingleColor4(srcBlock.color(0), dstBlock.color);
			}
			else
			{
				ColorSet colorSet(srcBlock, dxt1, mergeMatchingColors);
				compressor.Compress(&colorSet, dxt1, dstBlock.color);
			}
			
			if (compress.userOutputFunction)
			{
				compress.userOutputFunction(compress, &dstBlock, sizeof(dstBlock));
			}
		}
	}
}


void CryTextureCompressor::CompressDXT5(const CryTextureCompressor::CompressorParameters& compress)
{
	const bool dxt1 = false;
	const bool mergeMatchingColors = true;

	ClusterCompressor compressor;
	compressor.SetMetric( compress.rWeight, compress.gWeight, compress.bWeight );

	ColorBlock4x4 srcBlock;
	BlockDXT5 dstBlock;

	const unsigned int w = compress.width;
	const unsigned int h = compress.height;

	for (unsigned int y = 0; y < h; y += 4)
	{
		for (unsigned int x = 0; x < w; x += 4)
		{
			srcBlock.set(compress.srcBGRA8, w,h, compress.pitch, x,y);

			OptimalCompressor::CompressDXT5Alpha(srcBlock, dstBlock.alpha);

			if(srcBlock.isSingleColorIgnoringAlpha())
			{
				OptimalCompressor::CompressDXT1SingleColor4(srcBlock.color(0), dstBlock.color);
			}
			else
			{
				ColorSet colorSet(srcBlock, dxt1, mergeMatchingColors);
				compressor.Compress(&colorSet, dxt1, dstBlock.color);
			}
			
			if (compress.userOutputFunction)
			{
				compress.userOutputFunction(compress, &dstBlock, sizeof(dstBlock));
			}
		}
	}
}


void CryTextureCompressor::CompressDXT5n(const CryTextureCompressor::CompressorParameters& compress)
{
	ColorBlock4x4 srcBlock;
	BlockDXT5 dstBlock;

	const unsigned int w = compress.width;
	const unsigned int h = compress.height;

	for (unsigned int y = 0; y < h; y += 4)
	{
		for (unsigned int x = 0; x < w; x += 4)
		{
			srcBlock.set(compress.srcBGRA8, w,h, compress.pitch, x,y);

			// Swizzle pixels (r,g,b,a) -> (0,r,0,g)
			{
				ColorBGRA8* const colors = srcBlock.colors();
				for(unsigned int i=0; i<16; ++i)
				{
					const ColorBGRA8 c = colors[i];
					colors[i].setBGRA(0, c.r, 0, c.g);
				}
			}

			// Compress X.
			OptimalCompressor::CompressDXT5Alpha(srcBlock, dstBlock.alpha);

			// Compress Y.
			OptimalCompressor::CompressDXT1Green(srcBlock, dstBlock.color);

			if (compress.userOutputFunction)
			{
				compress.userOutputFunction(compress, &dstBlock, sizeof(dstBlock));
			}
		}
	}
}


