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
   
#ifndef CLUSTERCOMPRESSOR_H
#define CLUSTERCOMPRESSOR_H

#include "DXTBlocks.h"
#include "Cry_Math.h"         // Vec3

class ColorSet;


class ClusterCompressor
{
public:
	ClusterCompressor()
    {
    }

	void SetMetric(float r, float g, float b);

    void Compress(ColorSet const* colourSet, bool dxt1, ColorBlockDXT1& outBlock);

private:
	void PrepareCompressor(ColorSet const* const colorSet);
	void Compress3( ColorBlockDXT1& outBlock );
	void Compress4( ColorBlockDXT1& outBlock );
    	
private:
	Vec3 m_metric;
	Vec3 m_metricSqr;

	ColorSet const* m_colorSet;
	unsigned int m_order[16];

	Vec3 m_weighted[16];
	float m_weights[16];
	Vec3 m_xsum;
	float m_wsum;
    float m_besterror;
};

#endif
