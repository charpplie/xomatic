// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <nvcore/Containers.h> // swap

#include <nvmath/Color.h>

#include <nvimage/ColorBlock.h>
#include <nvimage/BlockDXT.h>

#include "OptimalCompressDXT.h"
#include "SingleColorLookup.h"


using namespace nv;
using namespace OptimalCompress;



namespace
{
	static int computeGreenError(const ColorBlock & rgba, const BlockDXT1 * block)
	{
		nvDebugCheck(block != NULL);

		int palette[4];
		palette[0] = (block->col0.g << 2) | (block->col0.g >> 4);
		palette[1] = (block->col1.g << 2) | (block->col1.g >> 4);
		palette[2] = (2 * palette[0] + palette[1] + 1) / 3;			// Sokov: I added '+1' to have better rounding after division by 3
		palette[3] = (2 * palette[1] + palette[0] + 1) / 3;			// Sokov: I added '+1' to have better rounding after division by 3

		int totalError = 0;

		for (int i = 0; i < 16; i++)
		{
			const int green = rgba.color(i).g;
			
			int error = abs(green - palette[0]);
			error = min(error, abs(green - palette[1]));
			error = min(error, abs(green - palette[2]));
			error = min(error, abs(green - palette[3]));
			
			totalError += error;
		}

		return totalError;
	}

	static uint computeGreenIndices(const ColorBlock & rgba, const Color32 palette[4])
	{
		const int color0 = palette[0].g;
		const int color1 = palette[1].g;
		const int color2 = palette[2].g;
		const int color3 = palette[3].g;
		
		uint indices = 0;
		for (int i = 0; i < 16; i++)
		{
			const int color = rgba.color(i).g;
			
			uint d0 = abs(color0 - color);
			uint d1 = abs(color1 - color);
			uint d2 = abs(color2 - color);
			uint d3 = abs(color3 - color);
			
			uint b0 = d0 > d3;
			uint b1 = d1 > d2;
			uint b2 = d0 > d2;
			uint b3 = d1 > d3;
			uint b4 = d2 > d3;
			
			uint x0 = b1 & b2;
			uint x1 = b0 & b3;
			uint x2 = b0 & b4;
			
			indices |= (x2 | ((x0 | x1) << 1)) << (2 * i);
		}

		return indices;
	}

	// Choose quantized color that produces less error. Used by DXT3 compressor.
	inline static uint quantize4(uint8 a)
	{
		int q0 = (a >> 4) - 1;
		int q1 = (a >> 4);
		int q2 = (a >> 4) + 1;
		
		q0 = (q0 << 4) | q0;
		q1 = (q1 << 4) | q1;
		q2 = (q2 << 4) | q2;
		
		int d0 = abs(q0 - a);
		int d1 = abs(q1 - a);
		int d2 = abs(q2 - a);

		if (d0 < d1 && d0 < d2) return q0 >> 4;
		if (d1 < d2) return q1 >> 4;
		return q2 >> 4;
	}
	
	static uint computeAlphaError(const ColorBlock & rgba, const AlphaBlockDXT5 * block)
	{
		uint8 alphas[8];
		block->evaluatePalette(alphas);

		uint totalError = 0;

		for (uint i = 0; i < 16; i++)
		{
			uint8 alpha = rgba.color(i).a;

			uint besterror = 256*256;
			uint best;
			for (uint p = 0; p < 8; p++)
			{
				int d = alphas[p] - alpha;
				uint error = d * d;

				if (error < besterror)
				{
					besterror = error;
					best = p;
				}
			}

			totalError += besterror;
		}

		return totalError;
	}
	
	static void computeAlphaIndices(const ColorBlock & rgba, AlphaBlockDXT5 * block)
	{
		uint8 alphas[8];
		block->evaluatePalette(alphas);

		for (uint i = 0; i < 16; i++)
		{
			uint8 alpha = rgba.color(i).a;

			uint besterror = 256*256;
			uint best = 8;
			for(uint p = 0; p < 8; p++)
			{
				int d = alphas[p] - alpha;
				uint error = d * d;

				if (error < besterror)
				{
					besterror = error;
					best = p;
				}
			}
			nvDebugCheck(best < 8);

			block->setIndex(i, best);
		}
	}

} // namespace





// Single color compressor, based on:
// https://mollyrocket.com/forums/viewtopic.php?t=392
void OptimalCompress::compressDXT1(Color32 c, BlockDXT1 * dxtBlock)
{
	dxtBlock->col0.r = OMatch5[c.r][0];
	dxtBlock->col0.g = OMatch6[c.g][0];
	dxtBlock->col0.b = OMatch5[c.b][0];
	dxtBlock->col1.r = OMatch5[c.r][1];
	dxtBlock->col1.g = OMatch6[c.g][1];
	dxtBlock->col1.b = OMatch5[c.b][1];
	dxtBlock->indices = 0xaaaaaaaa;

	if (dxtBlock->col0.u < dxtBlock->col1.u)
	{
		swap(dxtBlock->col0.u, dxtBlock->col1.u);
		dxtBlock->indices ^= 0x55555555;
	}
}

void OptimalCompress::compressDXT1a(Color32 rgba, BlockDXT1 * dxtBlock)
{
	if (rgba.a < 128)
	{
		dxtBlock->col0.u = 0;
		dxtBlock->col1.u = 0;
		dxtBlock->indices = 0xFFFFFFFF;
	}
	else
	{
		compressDXT1(rgba, dxtBlock);
	}
}


// Brute force green channel compressor
void OptimalCompress::compressDXT1G_old(const ColorBlock & rgba, BlockDXT1 * block)
{
	nvDebugCheck(block != NULL);
	
	uint8 ming = 63;
	uint8 maxg = 0;
	
	// Get min/max green.
	for (uint i = 0; i < 16; i++)
	{
		uint8 green = rgba.color(i).g >> 2;
		ming = min(ming, green);
		maxg = max(maxg, green);
	}

	block->col0.r = 0; //31;
	block->col1.r = 0; //31;
	block->col0.g = maxg;
	block->col1.g = ming;
	block->col0.b = 0;
	block->col1.b = 0;

	if (maxg - ming > 4)
	{
		int besterror = computeGreenError(rgba, block);
		int bestg0 = maxg;
		int bestg1 = ming;
		
		for (int g0 = ming+5; g0 < maxg; g0++)
		{
			for (int g1 = ming; g1 < g0-4; g1++)
			{
				if ((maxg-g0) + (g1-ming) > besterror)
					continue;
				
				block->col0.g = g0;
				block->col1.g = g1;
				int error = computeGreenError(rgba, block);
				
				if (error < besterror)
				{
					besterror = error;
					bestg0 = g0;
					bestg1 = g1;
				}
			}
		}
		
		block->col0.g = bestg0;
		block->col1.g = bestg1;
	}
	
	Color32 palette[4];
	block->evaluatePalette(palette);
	block->indices = computeGreenIndices(rgba, palette);
}

// Brute force green channel compressor
// Sokov's version
void OptimalCompress::compressDXT1G(const ColorBlock & rgba, BlockDXT1 * block)
{
	nvDebugCheck(block != NULL);

	// Get min/max green.
	int ming = 256;
	int maxg = -1;
	for (uint i = 0; i < 16; i++)
	{
		const int value = rgba.color(i).g;
		if(value < ming)
			ming = value;
		if(value > maxg)
			maxg = value;
	}

	if(ming==maxg)
	{
		OptimalCompress::compressDXT1( Color32(0,ming,0,0), block );
		return;
	}

	// find best green color palette
	{
		block->col0.r = 0;
		block->col0.b = 0;
		block->col1.r = 0;
		block->col1.b = 0;

		// round 8-bit min/max values to 6-bit values (with small expanding, to avoid missing good encoding pair)
		// FIXME: Sokov: we need to try expand ranges even more (it's possible to have lower error sometimes)
		{
			ming = (ming >> 2) - 1;
			if(ming < 0)
			{
				ming = 0;
			}

			maxg = (maxg >> 2) + 1;
			if(maxg > 63)
			{
				maxg = 63;
			}
		}

		int besterror = INT_MAX;			
		int bestmin = 0;
		int bestmax = 0;
		for (int maxvalue = ming; maxvalue <= maxg; ++maxvalue)
		{
			for (int minvalue = ming; minvalue <= maxvalue; ++minvalue)
			{
				block->col0.g = maxvalue;
				block->col1.g = minvalue;
				const int error = computeGreenError(rgba, block);

				if (error < besterror)
				{
					besterror = error;
					bestmin = minvalue;
					bestmax = maxvalue;
				}
			}
		}

		// Some bad decoders use 3-color palette instead of 4-color palette when color0<=color1,
		// so below we enforcing color0>color1
		if(bestmin==bestmax)
		{
			if(bestmin > 0)
			{
				--bestmin;
			}
			else
			{
				++bestmax;
			}
		}

		block->col0.g = bestmax;
		block->col1.g = bestmin;

		Color32 palette[4];

		block->evaluatePalette4(palette);
		block->indices = computeGreenIndices(rgba, palette);
	}
}

void OptimalCompress::compressDXT3A(const ColorBlock & rgba, AlphaBlockDXT3 * dxtBlock)
{
	dxtBlock->alpha0 = quantize4(rgba.color(0).a);
	dxtBlock->alpha1 = quantize4(rgba.color(1).a);
	dxtBlock->alpha2 = quantize4(rgba.color(2).a);
	dxtBlock->alpha3 = quantize4(rgba.color(3).a);
	dxtBlock->alpha4 = quantize4(rgba.color(4).a);
	dxtBlock->alpha5 = quantize4(rgba.color(5).a);
	dxtBlock->alpha6 = quantize4(rgba.color(6).a);
	dxtBlock->alpha7 = quantize4(rgba.color(7).a);
	dxtBlock->alpha8 = quantize4(rgba.color(8).a);
	dxtBlock->alpha9 = quantize4(rgba.color(9).a);
	dxtBlock->alphaA = quantize4(rgba.color(10).a);
	dxtBlock->alphaB = quantize4(rgba.color(11).a);
	dxtBlock->alphaC = quantize4(rgba.color(12).a);
	dxtBlock->alphaD = quantize4(rgba.color(13).a);
	dxtBlock->alphaE = quantize4(rgba.color(14).a);
	dxtBlock->alphaF = quantize4(rgba.color(15).a);
}


void OptimalCompress::compressDXT5A(const ColorBlock & rgba, AlphaBlockDXT5 * dxtBlock)
{
	// get the range for 6-alpha and 8-alpha interpolation
	int min6 = 256;
	int max6 = -1;
	int min8 = 256;
	int max8 = -1;
	
	for( int i = 0; i < 16; ++i )
	{
		int value = rgba.color(i).a;
		if( value != 0 && value < min6 )
			min6 = value;
		if( value != 255 && value > max6 )
			max6 = value;
		if( value < min8 )
			min8 = value;
		if( value > max8 )
			max8 = value;
	}
	
	// try to use 8-alpha errorless compression
	if( max8 - min8 < 8 )
	{
		if(max8 == min8)
		{
			if(max8 < 255)
			{
				dxtBlock->alpha0 = max8+1;
				dxtBlock->alpha1 = max8;
			}
			else
			{
				dxtBlock->alpha0 = max8;
				dxtBlock->alpha1 = max8-1;
			}
		}
		else
		{
			dxtBlock->alpha0 = max8;
			dxtBlock->alpha1 = min8;
		}
		computeAlphaIndices(rgba, dxtBlock);

		return;
	}

	// try to use 6-alpha errorless compression
	if( max6 - min6 < 6 )
	{
		if(max6 < 0)
		{
			dxtBlock->alpha0 = 0;
			dxtBlock->alpha1 = 255;
		}
		else
		{
			dxtBlock->alpha0 = min6;
			dxtBlock->alpha1 = max6;
		}
		computeAlphaIndices(rgba, dxtBlock);

		return;
	}
	
	// FIXME: Sokov: we need to try expand ranges a bit (it's possible to have lower error sometimes)

	// find best 8-alpha palette
	uint besterror8 = UINT_MAX;			
	int bestmin8 = 0;
	int bestmax8 = 0;
	{
		for (int maxa = min8+7; maxa <= max8; ++maxa)
		{
			for (int mina = min8; mina <= maxa-7; ++mina)
			{
				dxtBlock->alpha0 = maxa;
				dxtBlock->alpha1 = mina;
				const uint error = computeAlphaError(rgba, dxtBlock);

				if (error < besterror8)
				{
					besterror8 = error;
					bestmin8 = mina;
					bestmax8 = maxa;
				}
			}
		}
	}

	// find best 6-alpha palette
	uint besterror6 = UINT_MAX;			
	int bestmin6 = 0;
	int bestmax6 = 0;
	{
		for (int maxa = min6+5; maxa <= max6; ++maxa)
		{
			for (int mina = min6; mina <= maxa-5; ++mina)
			{
				dxtBlock->alpha0 = mina;
				dxtBlock->alpha1 = maxa;
				const uint error = computeAlphaError(rgba, dxtBlock);

				if (error < besterror6)
				{
					besterror6 = error;
					bestmin6 = mina;
					bestmax6 = maxa;
				}
			}
		}
	}

	if(besterror6 < besterror8)
	{
		dxtBlock->alpha0 = bestmin6;
		dxtBlock->alpha1 = bestmax6;
	}
	else
	{
		dxtBlock->alpha0 = bestmax8;
		dxtBlock->alpha1 = bestmin8;
	}

	computeAlphaIndices(rgba, dxtBlock);
}
