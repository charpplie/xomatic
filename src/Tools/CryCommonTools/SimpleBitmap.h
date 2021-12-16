#pragma once

#include <assert.h>
#include <vector>														// STL vector
#include "platform.h"												// uint32
#include "Cry_Math.h"												// uint32

enum EImageFilteringMode
{
	eifm2DBorder = 0,
	eifmCubemapFilter = 1,
};

namespace
{
	enum ECubeFace
	{
		ecfPosX = 0,
		ecfNegX = 1,
		ecfPosY = 2,
		ecfNegY = 3,
		ecfPosZ = 4,
		ecfNegZ = 5,
		ecfUnknown = -1,
	};

	struct JumpEntry
	{
		ECubeFace face;
		int rot;
	};

	static const JumpEntry XJmpTable[] = {	{ecfNegZ, 0}, {ecfPosZ, 2},		// ecfPosXa
																					{ecfPosZ, 0}, {ecfNegZ, 2},		// ecfNegXa
																					{ecfPosX, 1}, {ecfNegX, 3},		// ecfPosYa
																					{ecfPosX, 3}, {ecfNegX, 1},		// ecfNegYa
																					{ecfPosX, 0}, {ecfNegX, 0},		// ecfPosZa
																					{ecfPosX, 2}, {ecfNegX, 2} };	// ecfNegZa

	static const JumpEntry YJmpTable[] = {  {ecfPosY, 3}, {ecfNegY, 1},		// ecfPosXa
																					{ecfPosY, 1}, {ecfNegY, 3},		// ecfNegXa
																					{ecfNegZ, 2}, {ecfPosZ, 0},		// ecfPosYa
																					{ecfPosZ, 0}, {ecfNegZ, 2},		// ecfNegYa
																					{ecfNegY, 0}, {ecfPosY, 2},		// ecfPosZa
																					{ecfNegY, 2}, {ecfPosY, 0} };	// ecfNegZa
}

//! memory block used as bitmap
//! if you might need mipmaps use CMippedBitmap from beginning on as saves you time dealing with on type only
template <class RasterElement>
class CSimpleBitmap
{
protected:

public:

	//! default constructor
	CSimpleBitmap() :m_dwWidth(0), m_dwHeight(0), m_pData(0)
	{
	}

	//! destructor
	~CSimpleBitmap()
	{
		FreeData();
	}

	// copy constructor
	CSimpleBitmap( const CSimpleBitmap<RasterElement> &rhs ) :m_dwWidth(0), m_dwHeight(0), m_pData(0)
	{
		*this = rhs;			// call assignment operator
	}

	// assignment operator
	CSimpleBitmap<RasterElement> &operator=( const CSimpleBitmap<RasterElement> &rhs )
	{
		FreeData();

		if(rhs.m_pData)
		{
			Alloc(rhs.GetWidth(),rhs.GetHeight());
			memcpy(m_pData,rhs.m_pData,m_dwWidth*m_dwHeight*sizeof(RasterElement));
		}

		return *this;
	}

	//! free all the memory resources
	void FreeData()
	{
		delete [] m_pData;
		m_pData=0;
		m_dwWidth=0;
		m_dwHeight=0;
	}

	// used to save the whole memory
	// \return in bytes
	uint32 CalcBitmapSize() const
	{
		return m_dwWidth*m_dwHeight*sizeof(RasterElement);
	}

	//!
	//! /param inWidth 1..
	//! /param inHeight 1..
	//! /return true=operator was successful, false otherwise (low memory)
	bool Alloc( const uint32 indwWidth, const uint32 indwHeight, const RasterElement *inpInitial=0 )
	{
		assert(indwWidth>0);
		assert(indwHeight>0);

		FreeData();

		m_pData=new RasterElement[indwWidth*indwHeight];
		assert(m_pData);		// should be tested before to avoid wastefull processing

		if(!m_pData)
			return false;

		m_dwWidth=indwWidth;
		m_dwHeight=indwHeight;

		if(inpInitial)
			Clear(*inpInitial);		// could be done better (copy constructor calls)

		return true;
	}

	ECubeFace JumpX(const ECubeFace srcFace, const bool isdXPos, int* rotCoords) const
	{
		int index = (int)srcFace*2+(isdXPos?0:1);
		assert(index<sizeof(XJmpTable));
		const JumpEntry& jmp = XJmpTable[index];
		(*rotCoords)+=jmp.rot;
		return jmp.face;
	}

	ECubeFace JumpY(const ECubeFace srcFace, const bool isdYPos, int* rotCoords) const
	{
		int index = (int)srcFace*2+(isdYPos?0:1);
		assert(index<sizeof(YJmpTable));
		const JumpEntry& jmp = YJmpTable[index];
		(*rotCoords)+=jmp.rot;
		return jmp.face;
	}

	// table that shows to which face we jump
	ECubeFace JumpTable(const ECubeFace srcFace, int isdXPos, int isdYPos, int* rotCoords) const
	{
		if(isdXPos!=0)	// recursive jump until dx==0
		{
			int newSwap = 0;
			ECubeFace newFace = JumpX(srcFace, isdXPos>0, &newSwap);
			(*rotCoords) += newSwap;
			isdXPos-=((isdXPos>0)?1:-1);
			RotateCoord(&isdXPos, &isdYPos, newSwap);
			return JumpTable(newFace, isdXPos, isdYPos, rotCoords);
		}
		if(isdYPos!=0)	// recursive jump until dy==0
		{
			int newSwap = 0;
			ECubeFace newFace = JumpY(srcFace, isdYPos>0, &newSwap);
			(*rotCoords) += newSwap;
			isdYPos -= ((isdYPos>0)?1:-1);
			RotateCoord(&isdXPos, &isdYPos, newSwap);
			return JumpTable(newFace, isdXPos, isdYPos, rotCoords);
		}
		assert(isdXPos==0&&isdYPos==0);
		return srcFace;
	}

	void RotateCoord(int* x, int* y, int mode) const
	{
		if(mode!=0)
		{
			if(mode == 2)	// 180 degrees
			{
				(*x) = -(*x);
				(*y) = -(*y);
			}
			else
			{
				if(mode==1)	// 90 dergees
				{
					int tmp = (*y);
					(*y) = (*x);
					(*x) = -tmp;
				}
				else		// 270 degrees
				{
					assert(mode==3);
					int tmp = (*y);
					(*y) = -(*x);
					(*x) = tmp;
				}
			}
		}
	}

	//! works only within the Bitmap for filter kernels
	//! /param inX 0..m_dwWidth-1 or the method returns false
	//! /param inY 0..m_dwHeight-1 or the method returns false
	//! /param outValue
	//! /return true=position was in the bitmap, false otherwise
	bool GetForFiltering( const int inX, const int inY, const int srcX, const int srcY, RasterElement &outValue, EImageFilteringMode mode ) const
	{
		if(mode == eifm2DBorder)
			return Get(inX, inY, outValue);

		assert(mode == eifmCubemapFilter);
		assert(m_dwWidth == m_dwHeight*6);

		if(!m_pData)
			return false;

		const int sideSize = m_dwHeight;

		assert(srcX>=0&&srcX<m_dwWidth);
		assert(srcY>=0&&srcY<m_dwHeight);

		ECubeFace srcFace = (ECubeFace)(srcX/sideSize);

		if(inX>=0&&inX<m_dwWidth && inY>=0&&inY<m_dwHeight)	// if we're inside the cubemap
		{
			ECubeFace destFace = (ECubeFace)(inX/sideSize);
			if(destFace==srcFace)	// we have the same face as src texel
			{
				outValue=m_pData[inY*m_dwWidth+inX];
				return true;
			}
		}
		const int halfSideSize = max(1, sideSize/2);

		// ternary logic
		const int isdXPositive = int(floorf((float)inX/sideSize)-floorf((float)srcX/sideSize));
		const int isdYPositive = int(floorf((float)inY/sideSize)-floorf((float)srcY/sideSize));
		//if(isdXPositive==0&&isdYPositive<0&&srcFace==ecfPosY)
		//{
		//	int tmp = 0;
		//}
		assert(isdXPositive!=0 || isdYPositive!=0);
		int rotCoords = 0;	// quadrants to rotate coords
		ECubeFace destFace = JumpTable(srcFace, isdXPositive, isdYPositive,&rotCoords);
		rotCoords = ((rotCoords%4)+4)%4;
		int destX = inX-srcFace*sideSize;
		int destY = inY;

		// rotate coords
		destX -= halfSideSize;	// center coords
		destY -= halfSideSize;
		RotateCoord(&destX, &destY, rotCoords);
		destX += halfSideSize;	// shift back
		destY += halfSideSize;

		destX = ((destX+sideSize)%sideSize+sideSize)%sideSize;	// tile in the face
		destY = ((destY+sideSize)%sideSize+sideSize)%sideSize;

		destX = min(destX, sideSize-1);
		destY = min(destY, sideSize-1);
		destX += sideSize*destFace;

		assert(destX<m_dwWidth);
		assert((ECubeFace)(destX/sideSize)==destFace);

		outValue=m_pData[destY*m_dwWidth+destX];
		return true;
	}

	void GetForFiltering( const Vec3& inDir, RasterElement &outValue ) const
	{
		Vec3 vcAbsDir(fabsf(inDir.x), fabsf(inDir.y), fabsf(inDir.z));
		ECubeFace face;
		int rotQuadrant = 0;
		Vec2 texCoord;
		if(vcAbsDir.x > vcAbsDir.y && vcAbsDir.x > vcAbsDir.z)
		{
			if(inDir.x > 0)
			{
				rotQuadrant = 3;
				face = ecfPosX;
			}
			else
			{
				rotQuadrant = 1;
				face = ecfNegX;
			}
			texCoord = Vec2(inDir.y, inDir.z) / vcAbsDir.x;
		}
		else if(vcAbsDir.y > vcAbsDir.x && vcAbsDir.y > vcAbsDir.z)
		{
			if(inDir.y > 0)
			{
				rotQuadrant = 2;
				face = ecfPosY;
			}
			else
			{
				rotQuadrant = 0;
				face = ecfNegY;
			}
			texCoord = Vec2(inDir.x, inDir.z) / vcAbsDir.y;
		}
		else
		{
			assert(vcAbsDir.z >= vcAbsDir.x && vcAbsDir.z >= vcAbsDir.y);
			if(inDir.z > 0)
			{
				rotQuadrant = 1;
				face = ecfPosZ;
			}
			else
			{
				rotQuadrant = 3;
				face = ecfNegZ;
			}
			texCoord = Vec2(inDir.x, inDir.y) / vcAbsDir.z;
		}

		texCoord = texCoord * .5f + Vec2(.5f, .5f);
		assert(texCoord.x <= 1.f && texCoord.x >= 0);
		assert(texCoord.y <= 1.f && texCoord.y >= 0);

		Vec2i texelPos(texCoord.x*(m_dwHeight-1), texCoord.y*(m_dwHeight-1));

		texelPos.x += face * m_dwHeight;	// plus face
		outValue=m_pData[texelPos.y*m_dwWidth+texelPos.x];
	}

	//! works only within the Bitmap
	//! /param inX 0..m_dwWidth-1 or the method returns false
	//! /param inY 0..m_dwHeight-1 or the method returns false
	//! /param outValue
	//! /return true=position was in the bitmap, false otherwise
	bool Get( const uint32 inX, const uint32 inY, RasterElement &outValue ) const
	{
		if(!m_pData)
			return false;

		if(inX>=m_dwWidth || inY>=m_dwHeight)
			return false;

		outValue=m_pData[inY*m_dwWidth+inX];

		return true;
	}

		
	//! bilinear, workes only well within 0..1
	bool GetFiltered( const float infX, const float infY, RasterElement &outValue ) const
	{
		float fIX=floorf(infX),	fIY=floorf(infY);
		float fFX=infX-fIX,			fFY=infY-fIY;
		int		iXa=(int)fIX,			iYa=(int)fIY;
		int		iXb=iXa+1,			iYb=iYa+1;

		if(iXb==m_dwWidth)
			iXb=0;

		if(iYb==m_dwHeight)
			iYb=0;

		RasterElement p[4];

		if(Get(iXa,iYa,p[0]) && Get(iXb,iYa,p[1]) && Get(iXa,iYb,p[2]) && Get(iXb,iYb,p[3]))
		{
			outValue = p[0]	* ((1.0f-fFX)*(1.0f-fFY))		// left top
								+p[1] * ((     fFX)*(1.0f-fFY))		// right top
								+p[2] * ((1.0f-fFX)*(     fFY))		// left bottom
								+p[3] * ((     fFX)*(     fFY));	// right bottom

			return true;
		}

		return false;
	}


	//! works only within the Bitmap
	//! /param inX 0..m_dwWidth-1 or the method returns false
	//! /param inY 0..m_dwHeight-1 or the method returns false
	RasterElement &GetRef( const uint32 inX, const uint32 inY )
	{
		assert(m_pData);
		assert(inX<m_dwWidth && inY<m_dwHeight);

		return m_pData[inY*m_dwWidth+inX];
	}

	//! works even outside of the Bitmap (tiled)
	//! /param inX 0..m_dwWidth-1 or the method returns false
	//! /param inY 0..m_dwHeight-1 or the method returns false
	RasterElement &GetTiledRef( const uint32 inX, const uint32 inY )
	{ 
		assert(m_pData);
		
		uint32 X=inX%m_dwWidth;
		uint32 Y=inY%m_dwHeight;

		return m_pData[Y*m_dwWidth+X];
	}

	//! works only within the Bitmap
	//! /param inX 0..m_dwWidth-1 or the method returns false
	//! /param inY 0..m_dwHeight-1 or the method returns false
	const RasterElement &GetRef( const uint32 inX, const uint32 inY ) const
	{
		assert(m_pData);
		assert(inX<m_dwWidth && inY<m_dwHeight);

		return m_pData[inY*m_dwWidth+inX];
	}

	//! works only within the Bitmap
	//! /param inX 0..m_dwWidth-1 or the method returns false
	//! /param inY 0..m_dwHeight-1 or the method returns false
	//! /param inValue
	bool Set( const uint32 inX, const uint32 inY, const RasterElement &inValue )
	{
		if(!m_pData)
		{
			assert(m_pData);		// should be tested before to avoid wastefull processing
			return false;
		}

		if(inX>=m_dwWidth || inY>=m_dwHeight)
			return false;

		m_pData[inY*m_dwWidth+inX]=inValue;

		return true;
	}

	uint32 GetWidth() const
	{
		return m_dwWidth;
	}

	// Return
	//   size of one line in bytes
	size_t GetPitch() const
	{
		return m_dwWidth*sizeof(RasterElement);
	}

	uint32 GetHeight() const
	{
		return m_dwHeight;
	}

	//! /return could be 0 if point is outside the bitmap
	RasterElement *GetPointer( const uint32 inX=0, const uint32 inY=0 ) const
	{
		if(inX>=m_dwWidth || inY>=m_dwHeight)
			return 0;

		return &m_pData[inY*m_dwWidth+inX];
	}

	void Clear( const RasterElement &inValue )
	{
		RasterElement *pPtr=m_pData;

		for(uint32 y=0;y<m_dwHeight;y++)
			for(uint32 x=0;x<m_dwWidth;x++)
				*pPtr++=inValue;
	}

/*
	//! all elements that are equal to the given mask and have a neighbor become the blended value of the neighbors
	//! not too fast but convenient - call several times to expand more than one pixel
	//! RasterElement need to support: operator+=(const RasterElement )    operator*( const float )
	//! RasterElement2 need to support: operator==
	template <class RasterElement2>
		void ExpandBorder1Pixel( CSimpleBitmap<RasterElement2> &rMaskBitmap, const RasterElement2 &inMaskElement, const uint32 dwElementWidth=1 )
	{
		if(!m_pData)
			return;

		assert(dwElementWidth);

		CSimpleBitmap<RasterElement> tempBimap;

		tempBimap.Alloc(m_dwWidth,m_dwHeight);

		RasterElement *pSrc=m_pData;
		RasterElement *pDest=tempBimap.GetPointer();

		uint32 dwWidthDiv = m_dwWidth/dwElementWidth;

		for(uint32 y=0;y<m_dwHeight;y++)
			for(uint32 x=0;x<dwWidthDiv;x++)
			{
				if(rMaskBitmap.GetRef(x,y)==inMaskElement)
				{
					uint32 dwI=0;

					if(x!=0							&& !(rMaskBitmap.GetRef(x-1,y)==inMaskElement))	dwI++;
					if(x!=dwWidthDiv-1	&& !(rMaskBitmap.GetRef(x+1,y)==inMaskElement))	dwI++;
					if(y!=0							&& !(rMaskBitmap.GetRef(x,y-1)==inMaskElement))	dwI++;
					if(y!=m_dwHeight-1	&& !(rMaskBitmap.GetRef(x,y+1)==inMaskElement))	dwI++;

					if(dwI)
					{
						float fInvI = 1.0f/dwI;

						for(uint32 dwE=0;dwE<dwElementWidth;++dwE)
						{
							RasterElement Sum = pSrc[0] * 0;		// set to 0

							if(x!=0							&& !(rMaskBitmap.GetRef(x-1,y)==inMaskElement))	Sum += static_cast<RasterElement>(pSrc[-(int)dwElementWidth]*fInvI);
							if(x!=dwWidthDiv-1	&& !(rMaskBitmap.GetRef(x+1,y)==inMaskElement))	Sum += static_cast<RasterElement>(pSrc[dwElementWidth]*fInvI);
							if(y!=0							&& !(rMaskBitmap.GetRef(x,y-1)==inMaskElement))	Sum += static_cast<RasterElement>(pSrc[-(int)m_dwWidth]*fInvI);
							if(y!=m_dwHeight-1	&& !(rMaskBitmap.GetRef(x,y+1)==inMaskElement))	Sum += static_cast<RasterElement>(pSrc[m_dwWidth]*fInvI);

							pSrc++;
							*pDest++ = Sum;		// set pixel based on neighbors
						}
						continue;
					}
				}

				// copy pixels
				for(uint32 dwE=0;dwE<dwElementWidth;++dwE)
					*pDest++=*pSrc++;
			}

		memcpy(m_pData,tempBimap.GetPointer(),sizeof(RasterElement)*m_dwWidth*m_dwHeight);
	}
*/

	bool IsValid() const
	{
		return m_pData!=0;
	}

protected: // ------------------------------------------------------

	RasterElement *						m_pData;			//!< [m_dwWidth*m_dwHeight]
	uint32										m_dwWidth;		//!< width of m_pData
	uint32										m_dwHeight;		//!< height of m_pData
};




