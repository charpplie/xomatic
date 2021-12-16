#ifndef OPTIMALCOMPRESSOR_H
#define OPTIMALCOMPRESSOR_H

#include "DXTBlocks.h"
#include "ColorBlock4x4.h"

class OptimalCompressor
{
public:
	static void CompressDXT1FullyTransparent(ColorBlockDXT1& outBlock);
	static void CompressDXT1SingleColor3(ColorBGRA8 color, ColorBlockDXT1& outBlock);
	static void CompressDXT1SingleColor4(ColorBGRA8 color, ColorBlockDXT1& outBlock);
	static void CompressDXT1SingleColor3or4(ColorBGRA8 color, ColorBlockDXT1& outBlock);
	static void CompressDXT1Green(const ColorBlock4x4& colors, ColorBlockDXT1& outBlock);

	static void CompressDXT3Alpha(const ColorBlock4x4& colors, AlphaBlockDXT3& outBlock);

	static void CompressDXT5Alpha(const ColorBlock4x4& colors, AlphaBlockDXT5& outBlock);
};

#endif
