/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2006 Ignacio Castano                      icastano@nvidia.com

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */

#include "stdafx.h"
#include "ClusterCompressor.h"
#include "OptimalCompressor.h"
#include "ColorSet.h"
#include <cmath>


class Sym3x3
{
public:
	Sym3x3()
	{
	}

	Sym3x3( float a )
	{
		for( int i = 0; i < 6; ++i )
		{
			m_x[i] = a;
		}
	}

	float operator[]( unsigned int index ) const
	{
		return m_x[index];
	}

	float& operator[]( unsigned int index )
	{
		return m_x[index];
	}

private:
	float m_x[6];
};


static Sym3x3 ComputeWeightedCovariance( unsigned int n, Vec3 const* points, float const* weights, const Vec3& metric )
{
	// compute the centroid
	float total = 0.0f;
	Vec3 centroid( 0.0f );
	for( unsigned int i = 0; i < n; ++i )
	{
		total += weights[i];
		centroid += weights[i]*points[i];
	}
	centroid /= total;

	// accumulate the covariance matrix
	Sym3x3 covariance( 0.0f );
	for( unsigned int i = 0; i < n; ++i )
	{
		const Vec3 a = (points[i] - centroid).CompMul( metric );
		const Vec3 b = weights[i]*a;
		
		covariance[0] += a.x*b.x;
		covariance[1] += a.x*b.y;
		covariance[2] += a.x*b.z;
		covariance[3] += a.y*b.y;
		covariance[4] += a.y*b.z;
		covariance[5] += a.z*b.z;
	}
	
	return covariance;
}


static Vec3 ComputePrincipleComponent( Sym3x3 const& matrix )
{
	const unsigned int NUM = 8;

	Vec3 v(1, 1, 1);

	for(unsigned int guess = 0; guess < 3; ++guess)
	{
		for(unsigned int i = 0; i < NUM; ++i) 
		{
			const float x = v.x * matrix[0] + v.y * matrix[1] + v.z * matrix[2];
			const float y = v.x * matrix[1] + v.y * matrix[3] + v.z * matrix[4];
			const float z = v.x * matrix[2] + v.y * matrix[4] + v.z * matrix[5];
		
			const float norm = MAX(MAX(x, y), z);

			if (norm == 0.0f) 
			{
				// Looks like our initial guess for 'v' was wrong (probably 'v' is orthogonal 
				// to the sought-for principle component vector). Let's try other 
				// value for 'v'.
				goto nextGuess;
			}

			const float iv = 1.0f / norm;		
			v = Vec3(x*iv, y*iv, z*iv);
		}
		
		// return found approximation of principle component
		return v;

nextGuess:
		v = ((guess==0) ? Vec3(1,0,0) : Vec3(0,1,0));
	}

	// Every initial guess for 'v' failed. I think this
	// is almost impossible (except the case when the input is single point).
	// We have nothing to do, let's just return some fixed vector as result.
	return Vec3(1,1,1);
}


static int FloatToInt( float const a, int const maxlimit )
{
	// use ANSI round-to-zero behaviour to get round-to-nearest
	const int i = (int)( a + 0.5f );
	return (i < 0) ? 0 : ((i>maxlimit) ? maxlimit : i);
}

static ColorB5G6R5 FloatToColorB5G6R5( const Vec3& color )
{
	ColorB5G6R5 c;

	c.r = FloatToInt( 31.0f*color.x, 31 );
	c.g = FloatToInt( 63.0f*color.y, 63 );
	c.b = FloatToInt( 31.0f*color.z, 31 );

	return c;
}

void FillColorBlock( ColorB5G6R5 const color0, ColorB5G6R5 const color1, const uint8* const indices, ColorBlockDXT1& block )
{
	block.col0 = color0;
	block.col1 = color1;

	block.row[0] = indices[ 0] | ( indices[ 1] << 2 ) | ( indices[ 2] << 4 ) | ( indices[ 3] << 6 );
	block.row[1] = indices[ 4] | ( indices[ 5] << 2 ) | ( indices[ 6] << 4 ) | ( indices[ 7] << 6 );
	block.row[2] = indices[ 8] | ( indices[ 9] << 2 ) | ( indices[10] << 4 ) | ( indices[11] << 6 );
	block.row[3] = indices[12] | ( indices[13] << 2 ) | ( indices[14] << 4 ) | ( indices[15] << 6 );
}

void FillColorBlock3( const Vec3& start, const Vec3& end, uint8 const* indices, ColorBlockDXT1& block )
{
	ColorB5G6R5 color0 = FloatToColorB5G6R5( start );
	ColorB5G6R5 color1 = FloatToColorB5G6R5( end );

	if( color0.u <= color1.u )
	{
		FillColorBlock(color0, color1, indices, block);
	}
	else
	{
		// remap the indices
		static const uint8 remapping[4] = { 1, 0, 2, 3 };
		uint8 remapped[16];
		for( unsigned int i = 0; i < 16; ++i )
		{
			remapped[i] = remapping[ indices[i] ];
		}

		FillColorBlock(color1, color0, remapped, block);
	}
}

void FillColorBlock4( const Vec3& start, const Vec3& end, uint8 const* indices, ColorBlockDXT1& block )
{
	ColorB5G6R5 color0 = FloatToColorB5G6R5( start );
	ColorB5G6R5 color1 = FloatToColorB5G6R5( end );

	if( color0.u > color1.u )
	{
		FillColorBlock(color0, color1, indices, block);
	}
	else if( color0.u == color1.u )
	{
		static const uint8 zero[16] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };

		FillColorBlock(color0, color0, zero, block);
	}
	else
	{
		// remap the indices
		uint8 remapped[16];
		for( unsigned int i = 0; i < 16; ++i )
		{
			remapped[i] = indices[i] ^ 0x01;
		}

		FillColorBlock(color1, color0, remapped, block);
	}
}



void ClusterCompressor::SetMetric( float r, float g, float b )
{
	m_metric = Vec3(r, g, b);
	m_metricSqr = m_metric.CompMul(m_metric);
}


void ClusterCompressor::Compress(ColorSet const* const colorSet, bool const dxt1, ColorBlockDXT1& outBlock)
{
	const unsigned int count = colorSet->GetCount();

	if( dxt1 )
    {
		if( colorSet->HasTransparent() )		
		{
			if( count == 0 )
			{
				OptimalCompressor::CompressDXT1FullyTransparent(outBlock);
			}
			else
			{
				if(count == 1)
				{
					OptimalCompressor::CompressDXT1SingleColor3( colorSet->GetSomeNonTransparentColor(), outBlock );
					uint8 indices[16];
					colorSet->RemapIndices( outBlock.bits0, indices );
					FillColorBlock( outBlock.col0, outBlock.col1, indices, outBlock );
				}
				else
				{
					PrepareCompressor(colorSet);
            		Compress3(outBlock);
				}
			}
		}
		else
		{
			if(count == 1)
			{
				OptimalCompressor::CompressDXT1SingleColor3or4( colorSet->GetSomeNonTransparentColor(), outBlock );
			}
			else
			{
				PrepareCompressor(colorSet);
           		Compress3(outBlock);
           		Compress4(outBlock);
			}
		}
	}
    else
    {
		if(count == 1)
		{
			OptimalCompressor::CompressDXT1SingleColor4( colorSet->GetSomeNonTransparentColor(), outBlock );
		}
		else
		{
			PrepareCompressor(colorSet);
       		Compress4(outBlock);
		}
    }
}


void ClusterCompressor::PrepareCompressor(ColorSet const* const colorSet)
{
	m_colorSet = colorSet;

	m_besterror = FLT_MAX;

	unsigned int const count = m_colorSet->GetCount();
	Vec3 const* const unweighted = m_colorSet->GetPoints();
	float const* const weights = m_colorSet->GetWeights();
	
	// get the covariance matrix
	Sym3x3 covariance = ComputeWeightedCovariance( count, unweighted, weights, m_metric );
	
	// compute the principle component
	Vec3 principle = ComputePrincipleComponent( covariance );

	// build the list of values
	float dps[16];
	for( unsigned int i = 0; i < count; ++i )
	{
		dps[i] = principle.Dot( unweighted[i] );
		m_order[i] = i;
	}
	
	// stable sort
	for( unsigned int i = 0; i < count; ++i )
	{
		for( unsigned int j = i; j > 0 && dps[j] < dps[j - 1]; --j )
		{
			std::swap( dps[j], dps[j - 1] );
			std::swap( m_order[j], m_order[j - 1] );
		}
	}
	
	// precompute frequently used data
	m_xsum = Vec3( 0.0f );
	m_wsum = 0.0f;	
	for( unsigned int i = 0; i < count; ++i )
	{
		const unsigned int p = m_order[i];
		m_weights[i] = weights[p];
		m_wsum += m_weights[i];
		m_weighted[i] = weights[p]*unweighted[p];
		m_xsum += m_weighted[i];
	}
}


inline Vec3 Floor( CONST Vec3& v )
{
	return Vec3(
		std::floor( v.x ),
		std::floor( v.y ),
		std::floor( v.z ));
}


void ClusterCompressor::Compress3( ColorBlockDXT1& outBlock )
{
	unsigned int const count = m_colorSet->GetCount();
	assert(count>1);

	// declare variables
	Vec3 beststart( 0.0f );
	Vec3 bestend( 0.0f );
	float besterror = m_besterror;

	Vec3 x0(0.0f);
	float w0 = 0.0f;
	
	int b0 = 0, b1 = 0;

	// check all possible clusters for this total order
	for( int c0 = 0; c0 <= count; c0++)
	{	
		Vec3 x1(0.0f);
		float w1 = 0.0f;
		
		for( int c1 = 0; c1 <= count-c0; c1++)
		{	
			// "all point in same cluster" will lead to division by zero in computing factor, so 
			// we avoiding it by using following 'if'.
			// FIXME: write code to find best single point match for "all point in same cluster"
			if(! ((c0==count)||(c1==count)||(c0+c1==0)) )
			{
				float w2 = m_wsum - w0 - w1;
				
				// These factors could be entirely precomputed.
				float const alpha2_sum = w0 + w1 * 0.25f;
				float const beta2_sum = w2 + w1 * 0.25f;
				float const alphabeta_sum = w1 * 0.25f;
				float const factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);
				
				Vec3 const alphax_sum = x0 + x1 * 0.5f;
				Vec3 const betax_sum = m_xsum - alphax_sum;
				
				Vec3 a = (alphax_sum*beta2_sum - betax_sum*alphabeta_sum) * factor;
				Vec3 b = (betax_sum*alpha2_sum - alphax_sum*alphabeta_sum) * factor;
				
				// clamp the output to [0, 1]
				Vec3 const one( 1.0f );
				Vec3 const zero( 0.0f );
				a.CheckMax(zero);
				a.CheckMin(one);
				b.CheckMax(zero);
				b.CheckMin(one);
				
				// clamp to the grid
				Vec3 const grid( 31.0f, 63.0f, 31.0f );
				Vec3 const gridrcp( 1/31.0f, 1/63.0f, 1/31.0f ); // Vec3 const gridrcp( 0.03227752766457f, 0.01583151765563f, 0.03227752766457f );
				Vec3 const half( 0.5f );
				a = Floor(a.CompMul(grid) + half).CompMul(gridrcp);
				b = Floor(b.CompMul(grid) + half).CompMul(gridrcp);
//				a = Floor( grid*a + half )*gridrcp;
//				b = Floor( grid*b + half )*gridrcp;
			
				// compute the error
				Vec3 e1 = a.CompMul(a)*alpha2_sum + b.CompMul(b)*beta2_sum + 2.0f*( a.CompMul(b)*alphabeta_sum - a.CompMul(alphax_sum) - b.CompMul(betax_sum) );
				
				// apply the metric to the error term
				float error = e1.Dot( m_metricSqr );
				
				// keep the solution if it wins
				if( error < besterror )
				{
					besterror = error;
					beststart = a;
					bestend = b;
					b0 = c0;
					b1 = c1;
				}
			}
			
			x1 += m_weighted[c0+c1];
			w1 += m_weights[c0+c1];
		}
		
		x0 += m_weighted[c0];
		w0 += m_weights[c0];
	}

	// save the block if necessary
	if( besterror < m_besterror )
	{
		// compute indices from cluster sizes.
		uint8 bestindices[16];
		{
			int i = 0;
			for(; i < b0; i++) {
				bestindices[i] = 0;
			}
			for(; i < b0+b1; i++) {
				bestindices[i] = 2;
			}
			for(; i < count; i++) {
				bestindices[i] = 1;
			}
		}
		
		// remap the indices
		uint8 ordered[16];
		for( int i = 0; i < count; ++i )
			ordered[m_order[i]] = bestindices[i];

		m_colorSet->RemapIndices(ordered, bestindices);
		
		// save the block
		FillColorBlock3( beststart, bestend, bestindices, outBlock );
		
		// save the error
		m_besterror = besterror;
	}
}


void ClusterCompressor::Compress4( ColorBlockDXT1& outBlock )
{
	unsigned int const count = m_colorSet->GetCount();
	assert(count>1);

	// declare variables
	Vec3 beststart( 0.0f );
	Vec3 bestend( 0.0f );
	float besterror = m_besterror;

	Vec3 x0(0.0f);
	float w0 = 0.0f;
	int b0 = 0, b1 = 0, b2 = 0;

	// check all possible clusters for this total order
	for( int c0 = 0; c0 <= count; c0++)
	{	
		Vec3 x1(0.0f);
		float w1 = 0.0f;
		
		for( int c1 = 0; c1 <= count-c0; c1++)
		{	
			Vec3 x2(0.0f);
			float w2 = 0.0f;
			
			for( int c2 = 0; c2 <= count-c0-c1; c2++)
			{
				// "all point in same cluster" will lead to division by zero in computing factor, so 
				// we avoiding it by using following 'if'.
				// FIXME: write code to find best single point match for "all point in same cluster"
				if(! ((c0==count)||(c1==count)||(c2==count)||(c0+c1+c2==0)) )
				{
					float w3 = m_wsum - w0 - w1 - w2;
					
					float const alpha2_sum = w0 + w1 * (4.0f/9.0f) + w2 * (1.0f/9.0f);
					float const beta2_sum = w3 + w2 * (4.0f/9.0f) + w1 * (1.0f/9.0f);
					float const alphabeta_sum = (w1 + w2) * (2.0f/9.0f);
					float const factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);
					
					Vec3 const alphax_sum = x0 + x1 * (2.0f / 3.0f) + x2 * (1.0f / 3.0f);
					Vec3 const betax_sum = m_xsum - alphax_sum;
					
					Vec3 a = ( alphax_sum*beta2_sum - betax_sum*alphabeta_sum )*factor;
					Vec3 b = ( betax_sum*alpha2_sum - alphax_sum*alphabeta_sum )*factor;
					
					// clamp the output to [0, 1]
					Vec3 const one( 1.0f );
					Vec3 const zero( 0.0f );
					a.CheckMax(zero);
					a.CheckMin(one);
					b.CheckMax(zero);
					b.CheckMin(one);
					
					// clamp to the grid
					Vec3 const grid( 31.0f, 63.0f, 31.0f );
					Vec3 const gridrcp( 1/31.0f, 1/63.0f, 1/31.0f ); // Vec3 const gridrcp( 0.03227752766457f, 0.01583151765563f, 0.03227752766457f );
					Vec3 const half( 0.5f );
					a = Floor(a.CompMul(grid) + half).CompMul(gridrcp);
					b = Floor(b.CompMul(grid) + half).CompMul(gridrcp);
//					a = Floor( grid*a + half )*gridrcp;
//					b = Floor( grid*b + half )*gridrcp;
					
					// compute the error
					Vec3 e1 = a.CompMul(a)*alpha2_sum + b.CompMul(b)*beta2_sum + 2.0f*( a.CompMul(b)*alphabeta_sum - a.CompMul(alphax_sum) - b.CompMul(betax_sum) );
//					Vec3 e1 = a*a*alpha2_sum + b*b*beta2_sum + 2.0f*( a*b*alphabeta_sum - a*alphax_sum - b*betax_sum );
					
					// apply the metric to the error term
					const float error = e1.Dot( m_metricSqr );
					
					// keep the solution if it wins
					if( error < besterror )
					{
						besterror = error;
						beststart = a;
						bestend = b;
						b0 = c0;
						b1 = c1;
						b2 = c2;
					}
				}
				
				x2 += m_weighted[c0+c1+c2];
				w2 += m_weights[c0+c1+c2];
			}
			
			x1 += m_weighted[c0+c1];
			w1 += m_weights[c0+c1];
		}
		
		x0 += m_weighted[c0];
		w0 += m_weights[c0];
	}

	// save the block if necessary
	if( besterror < m_besterror )
	{
		// compute indices from cluster sizes.
		uint8 bestindices[16];
		{
			int i = 0;
			for(; i < b0; i++) {
				bestindices[i] = 0;
			}
			for(; i < b0+b1; i++) {
				bestindices[i] = 2;
			}
			for(; i < b0+b1+b2; i++) {
				bestindices[i] = 3;
			}
			for(; i < count; i++) {
				bestindices[i] = 1;
			}
		}
		
		// remap the indices
		uint8 ordered[16];
		for( int i = 0; i < count; ++i )
			ordered[m_order[i]] = bestindices[i];
		
		m_colorSet->RemapIndices(ordered, bestindices);

		// save the block
		FillColorBlock4( beststart, bestend, bestindices, outBlock );

		// save the error
		m_besterror = besterror;
	}
}
