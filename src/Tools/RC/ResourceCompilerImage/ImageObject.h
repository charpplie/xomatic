#include "SimpleBitmap.h"							// CSimpleBitmap<>
#include "Cry_Math.h"									// Vec3

#include <ddraw.h>										// LPDIRECT3DSURFACE9

#include <assert.h>										// assert()
#include <Dbghelp.h>									// GetTimestampForLoadedLibrary
#include <BitFiddling.h>							// IsPowOfTwo()

#include "IRCLog.h"										// IRCLog
#include "IConfig.h"									// IConfig

#include "ImageCompiler.h"						// CImageCompiler
#include "ImageUserDialog.h"					// CImageUserDialog
#include "ImageExtensionHelper.h"			// CImageExtensionHelper

// so that you can write swizzle32('CEnd');     to get a unit32 that is readable in ASCII form in the file
inline uint32 swizzle32( const uint32 dwIn )
{
	return (dwIn>>24) | ((dwIn&0xff0000)>>8) | ((dwIn&0xff00)<<8) | ((dwIn&0xff)<<24);
}






// ImageObject allows the abstraction of different kinds of
// images generated during conversion
struct ImageObject
{
	// Returns:
	//   0 if internal representation is not CRAWImage
	virtual class CRAWImage *GetCRAWImage() { return 0; }

	virtual void FreeData()=0;

	//! write image to disk, overwrites any existing file
	virtual bool SaveImage(const char *filename) = 0;
	
	//! converts the psurfSrc to the native format for this object and stores it as the right mip
	virtual HRESULT UpdateSurface(LPDIRECT3DSURFACE9 psurfSrc, int mip, int filter, D3DCUBEMAP_FACES facetype, LPDIRECT3DDEVICE9 pd3ddev) = 0;	

	//! creates a new XRGB texture of this image for images that can't be directly displayed (caller deallocates)
	virtual IDirect3DTexture9 *CopyToXRGB(LPDIRECT3DDEVICE9 pd3ddev) { return NULL; }
	
	//! gets the internal DX texture, if the object has one
  virtual LPDIRECT3DBASETEXTURE9 GetDXTex() { return 0; }

	// use when you convert on image to another
	virtual void CopyPropertiesFrom( const ImageObject &rInput )=0;

	//
	virtual uint32 GetWidth( const uint32 dwMip ) const
	{
		uint32 w,h,depth,sides,mips;
		GetExtend( w,h,depth,sides,mips );
		
		bool bCompressed = CPixelFormats::GetPixelFormatInfo(GetPixelFormat())->bCompressed;

		uint32 dwRet = max((uint32)1,w>>dwMip);

		if(bCompressed)
			dwRet = (dwRet+3)&(~3);		// expand to div4

		return dwRet;
	}

	//
	virtual uint32 GetHeight( const uint32 dwMip ) const
	{
		uint32 w,h,depth,sides,mips;
		GetExtend( w,h,depth,sides,mips );

		bool bCompressed = CPixelFormats::GetPixelFormatInfo(GetPixelFormat())->bCompressed;

		uint32 dwRet = max((uint32)1,h>>dwMip);

		if(bCompressed)
			dwRet = (dwRet+3)&(~3);		// expand to div4

		return dwRet;
	}

	virtual CSimpleBitmap<Vec4> *GetSimpleBitmap(){ return 0; }

  virtual void GetDXTexI(IDirect3DTexture9 *&pTexture) { pTexture = NULL; }

	virtual void GetExtend( uint32 &dwWidth, uint32 &dwHeight, uint32 &dwDepth, uint32 &dwSides, uint32 &dwMips ) const=0;
	virtual bool Lock( const uint32 dwSide, const uint32 dwLevel, char * &pMem, uint32 &dwPitch )=0;
	virtual void Unlock( const uint32 dwLevel )=0;

	// ARGB
	virtual void SetAverageColor( const uint32 colAverageARGB )=0;
	
	// ARGB
	virtual uint32 GetAverageColor() const=0;

	// Arguments:
	//   dwImageFlags - combined from CImageExtensionHelper::EIF_Cubemap,...
	virtual void SetImageFlags( const uint32 dwImageFlags )=0;

	// Return:
	//   combined from CImageExtensionHelper::EIF_Cubemap,...
	virtual uint32 GetImageFlags() const=0;

	// Return: pixel format index of image
	virtual TPixelFormatID GetPixelFormat() const=0;

	// Set pixel format index of image
	virtual void SetPixelFormat( TPixelFormatID fmt )=0;

	virtual bool HasAlpha()
	{
		D3DFORMAT fmt = CPixelFormats::GetPixelFormatInfo(GetPixelFormat())->DxtNo;

		if (fmt==D3DFMT_R8G8B8 || fmt==D3DFMT_X8R8G8B8 || fmt==D3DFMT_X8B8G8R8)
			return false;

		if (fmt != D3DFMT_A8R8G8B8 && fmt != D3DFMT_A8B8G8R8 && fmt!=D3DFMT_A32B32G32R32F)
			return false;

		bool bHasAlpha = false;

		uint32 dwWidth,dwHeight,depth,sides,mips;
		GetExtend( dwWidth,dwHeight,depth,sides,mips );
		char *buf = 0;
		uint32 dwPitch;
		Lock( 0,0,buf,dwPitch );
		if (!buf)
		{
			if (fmt == D3DFMT_A8R8G8B8 || fmt == D3DFMT_A8B8G8R8)
				return true;

			return false;
		}

		if(fmt==D3DFMT_A32B32G32R32F)
		{
			for(uint32 dwY=0;dwY<dwHeight;dwY++)
			{
				float *colbuf = (float *)&buf[dwPitch*dwY]+3;

				for(uint32 dwX=0;dwX<dwWidth;dwX++)
				{
					if(*colbuf != 1.0f)
					{
						bHasAlpha = true;
						goto LabelAlphaOut;
					}
					colbuf+=4;
				}
			}
		}
		else
		{
			for(uint32 dwY=0;dwY<dwHeight;dwY++)
			{
				uint32 *colbuf = (uint32 *)&buf[dwPitch*dwY];

				for(uint32 dwX=0;dwX<dwWidth;dwX++)
				{
					if (((*colbuf) >> 24) != 255)
					{
						bHasAlpha = true;
						goto LabelAlphaOut;
					}
					colbuf++;
				}
			}
		}

LabelAlphaOut:

		Unlock( 0 );

		return bHasAlpha;
	}

	// Check if image is power of two.
	bool IsPowerOfTwo()
	{
		uint32 w,h,depth,sides,mips;
		GetExtend( w,h,depth,sides,mips );
		bool bIsPowerByTwo = false;
		if(IsCubemap())
			w/=6;
		if (::IsPowerOfTwo(w) && ::IsPowerOfTwo(h))
			return true;
		return false;
	}

	bool IsCubemap() const
	{
		uint32 w,h,depth,sides,mips;
		GetExtend( w,h,depth,sides,mips );
		return w==h*6;
	}

	// copy over the first mip level
	virtual bool CopyToSimpleBitmap( CSimpleBitmap<Vec4> &rOut ) const=0;

	virtual ImageObject *ExtractAlphaAsA8Image()=0;

	// Arguments:
	//   pImage can be 0 - then no processing is done
	virtual void SetAlphaFromA8Image( ImageObject *pImage )=0;

	virtual void SetAttachedImage( ImageObject *pValue )=0;

	virtual ImageObject *GetAttachedImage() const=0;

	// deallocates all internally held texture objects
	virtual ~ImageObject() {}
};


class CImageObjectBase :public ImageObject
{
public:
	// constructor
	CImageObjectBase() :m_colAverageARGB(0xffffffff), m_pAttachedImage(0), m_dwImageFlags(0)
	{
	}

	~CImageObjectBase()
	{
		FreeData();
	}

	// extend the DDS format by some user data (magic word to detect this DDS file was extended)
	void AppendExtendedData( FILE *out );

	// interface ImageObject ---------------------------------------------------

	virtual void FreeData()
	{
		delete m_pAttachedImage;m_pAttachedImage=0;
	}

	virtual void SetAverageColor( const uint32 colAverageARGB )
	{
		m_colAverageARGB=colAverageARGB;
	}

	virtual uint32 GetAverageColor() const
	{
		return m_colAverageARGB;
	}

	virtual void SetImageFlags( const uint32 dwImageFlags )
	{
		m_dwImageFlags=dwImageFlags;
	}

	virtual uint32 GetImageFlags() const 
	{ 
		return m_dwImageFlags;	
	}

	virtual void CopyPropertiesFrom( const ImageObject &rInput )
	{
		CImageObjectBase &rThisInput = (CImageObjectBase &)rInput;			// all CImageObjectBase must derive from ImageObject

		m_colAverageARGB = rThisInput.m_colAverageARGB;
		m_dwImageFlags = rThisInput.m_dwImageFlags;
	}

	virtual ImageObject *ExtractAlphaAsA8Image();

	virtual void SetAlphaFromA8Image( ImageObject *pImage );

	virtual void SetAttachedImage( ImageObject *pValue ) { delete m_pAttachedImage; m_pAttachedImage=pValue; }

	virtual ImageObject *GetAttachedImage() const { return m_pAttachedImage; }

private: // ------------------------------------------------------------------

	uint32											m_colAverageARGB;				// ARGB will be added the end of the DDS file
	ImageObject *								m_pAttachedImage;				// alpha channel in the case it was lost through format conversion
	uint32											m_dwImageFlags;					// combined from CImageExtensionHelper::EIF_Cubemap,...
};



// base DX Image, used for normal textures with mipmaps
struct DXImage : public CImageObjectBase
{
	LPDIRECT3DBASETEXTURE9					m_pTex;					//
	
	DXImage(LPDIRECT3DBASETEXTURE9 tex) : m_pTex(tex) { assert(tex); }

	virtual ~DXImage() { FreeData(); }

	// interface ImageObject --------------------------------------------------------

	virtual bool SaveImage(const char *filename)
	{
		assert(0);		// better to get rid of this functionality - CRAWImage should be used for saving

		// watch out: directX saving code does not add the average color - use NVidia or ATI compressor instead
		if(D3DXSaveTextureToFile(filename, D3DXIFF_DDS, m_pTex, NULL)!=S_OK)
			return false;

		FILE *out=fopen(filename,"ab");

		if(out)
		{	
			AppendExtendedData(out);

			fclose(out);
			return true;
		}
		
		return false;
	}

	
	virtual HRESULT UpdateSurface(LPDIRECT3DSURFACE9 psurfSrc, int mip, int filter, D3DCUBEMAP_FACES facetype, LPDIRECT3DDEVICE9 pd3ddev)
	{
		LPDIRECT3DSURFACE9 psurfDest = NULL;

		HRESULT hr = ((LPDIRECT3DTEXTURE9)m_pTex)->GetSurfaceLevel(mip, &psurfDest);

		if(hr==S_OK) hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, filter, 0);

		ReleasePpo(&psurfDest);
		return hr;
	}
	virtual void GetExtend( uint32 &dwWidth, uint32 &dwHeight, uint32 &dwDepth, uint32 &dwSides, uint32 &dwMips ) const
	{
		dwDepth=1;dwSides=1;

		D3DSURFACE_DESC sd;
		((LPDIRECT3DTEXTURE9)m_pTex)->GetLevelDesc(0, &sd);
		dwWidth=sd.Width;dwHeight=sd.Height;
		dwMips=m_pTex->GetLevelCount();
	}
	virtual bool Lock( const uint32 dwSide, const uint32 dwLevel, char * &pMem, uint32 &dwPitch )
	{
		D3DLOCKED_RECT rect;

		pMem=0;dwPitch=0;

		HRESULT hRes=((LPDIRECT3DTEXTURE9)m_pTex)->LockRect(dwLevel,&rect,0,D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_NOSYSLOCK);

		if(FAILED(hRes))
			return 0;

		pMem=(char *)rect.pBits;
		dwPitch=rect.Pitch;

		return true; 
	}
	virtual void Unlock( const uint32 dwLevel )
	{
		((LPDIRECT3DTEXTURE9)m_pTex)->UnlockRect(dwLevel);
	}
	virtual LPDIRECT3DBASETEXTURE9 GetDXTex() { return m_pTex; }
  virtual void GetDXTexI(IDirect3DTexture9 *&pTexture) { m_pTex->QueryInterface(IID_IDirect3DTexture9,(void **)&pTexture); }

	virtual TPixelFormatID GetPixelFormat() const
	{
		LPDIRECT3DTEXTURE9 ptexCur = (LPDIRECT3DTEXTURE9)m_pTex;
		D3DSURFACE_DESC sd;

		ptexCur->GetLevelDesc(0, &sd);

		return CPixelFormats::GetNoFromD3DFormat(sd.Format);
	}

	virtual void SetPixelFormat( TPixelFormatID fmt )
	{
		assert(0);		// not supported
	}

	virtual bool CopyToSimpleBitmap( CSimpleBitmap<Vec4> &rOut ) const
	{
		return false;
	}

	virtual void FreeData()
	{
		if(m_pTex)
		{
			m_pTex->Release();m_pTex=0; 
		}

		CImageObjectBase::FreeData();
	}
};






/*
// palettized 8 bit image, doesn't use DX internally at all
struct P8Image : ImageObject
{
	unsigned char *m_pPalette;
	unsigned char **m_ppIndices;
	int m_nWidth, m_nHeight, m_nMips;
	
	P8Image(int w, int h, int m) : m_pPalette(NULL), m_nWidth(w), m_nHeight(h), m_nMips(m)
	{
		m_ppIndices = new unsigned char *[m];
		for(int i = 0; i<m_nMips; i++) m_ppIndices[i] = NULL;
	};
	
	~P8Image()
	{
		for(int i = 0; i<m_nMips; i++) if(m_ppIndices[i]) free(m_ppIndices[i]);
		delete[] m_ppIndices;
	};
	
	bool Save(const char *filename);	
	void SavePaletizedTextureHeader(FILE *fh);
	void SavePaletizedTextureMip(FILE *fh, unsigned char *buf, int mip);
	
	HRESULT UpdateSurface(LPDIRECT3DSURFACE9 psurfSrc, int mip, int filter, D3DCUBEMAP_FACES facetype, LPDIRECT3DDEVICE9 pd3ddev);	

	IDirect3DTexture9 *CopyToXRGB(LPDIRECT3DDEVICE9 pd3ddev);
};
*/


// float image, doesn't use DX internally at all
// Vec4(red,green,blue,alpha)
class CFloat4Image :public CImageObjectBase
{
public:
	// constructor
	CFloat4Image( const uint32 dwWidth, const uint32 dwHeight )
	{
		const Vec4 null(0,0,0,0);

		m_ImageData.Alloc(dwWidth,dwHeight,&null);
	}

	virtual ~CFloat4Image() { FreeData(); }


	// interface ImageObject --------------------------------------------------------

	virtual bool SaveImage( const char *filename ) 
	{ 
		assert(0); return false;			// not a destination format
	}

	virtual HRESULT UpdateSurface(LPDIRECT3DSURFACE9 psurfSrc, int mip, int filter, D3DCUBEMAP_FACES facetype, LPDIRECT3DDEVICE9 pd3ddev){ return S_FALSE; }

	virtual void GetExtend( uint32 &dwWidth, uint32 &dwHeight, uint32 &dwDepth, uint32 &dwSides, uint32 &dwMips ) const
	{
		dwDepth=1;dwSides=1;dwMips=1;
		dwWidth=m_ImageData.GetWidth();
		dwHeight=m_ImageData.GetHeight();
	}
	virtual bool Lock( const uint32 dwSide, const uint32 dwLevel, char * &pMem, uint32 &dwPitch )
	{ 
		pMem=(char *)m_ImageData.GetPointer();
		dwPitch=m_ImageData.GetWidth()*sizeof(Vec4); 
		return true; 
	}

	virtual void Unlock( const uint32 dwLevel ){}

	virtual TPixelFormatID GetPixelFormat() const
	{
		return CPixelFormats::GetNoFromD3DFormat(D3DFMT_A32B32G32R32F);
	}

	virtual void SetPixelFormat( TPixelFormatID fmt )
	{
		assert(0);		// not supported
	}

	virtual CSimpleBitmap<Vec4> *GetSimpleBitmap()
	{ 
		return &m_ImageData; 
	}

	virtual bool CopyToSimpleBitmap( CSimpleBitmap<Vec4> &rOut ) const
	{
		rOut = m_ImageData;

		return true;
	}

	virtual void FreeData()
	{
		m_ImageData.FreeData();

		CImageObjectBase::FreeData();
	}

	// --------------------------------------------------------------------------------

	IDirect3DTexture9 *CopyToXRGB(LPDIRECT3DDEVICE9 pd3ddev) { return 0; }

	virtual bool HasAlpha() 
	{
		uint32 dwWidth = m_ImageData.GetWidth();
		uint32 dwHeight = m_ImageData.GetHeight();

		for(uint32 dwY=0;dwY<dwHeight;++dwY)
		for(uint32 dwX=0;dwX<dwWidth;++dwX)
		{
			Vec4 &ref = m_ImageData.GetRef(dwX,dwY);

			if(ref.w!=1.0f)
				return true;
		}

		return false;
	}


private: // ------------------------------------------------------------------------.

	CSimpleBitmap<Vec4>				m_ImageData;				// Vec4(alpha,red,green,blue)
};




// raw image, doesn't use DX internally at all
class CRAWImage :public CImageObjectBase
{
public:
	// constructor
	CRAWImage( const uint32 dwWidth, const uint32 dwHeight, const uint32 dwMips, const TPixelFormatID PixelFormatID, const bool bFinalPixelFormatIsCompressed ) 
		:CImageObjectBase(), m_PixelFormatID(PixelFormatID)
	{
		m_Levels.reserve(dwMips);

		SPixelFormats *pFmt = CPixelFormats::GetPixelFormatInfo(m_PixelFormatID);
		uint32 dwBitsPerPixel = pFmt->iBitsPerPixel;
//		uint32 dwBytesPerPixel = (dwBitsPerPixel+7)/8;

		uint32 dwMinSize = bFinalPixelFormatIsCompressed ? 4 : 1;			// compressed textures need special treatment for textures smaller than 4x4

		for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
		{
			SMipLevel *pEntry = new SMipLevel;

			uint32 dwLocalWidth = dwWidth>>dwMip;
			uint32 dwLocalHeight = dwHeight>>dwMip;

			if(dwLocalWidth<dwMinSize)
				dwLocalWidth=dwMinSize;
			if(dwLocalHeight<dwMinSize)
				dwLocalHeight=dwMinSize;

			pEntry->m_dwWidth = dwLocalWidth;
			pEntry->m_dwHeight = dwLocalHeight;

			// from DX documentation:
			//   The pitch for DXTn formats is different from what was returned in DirectX 7.0. Pitch is now measured in bytes (not blocks). For example, if you have a width of 16, then you will have a pitch of four blocks (4*8 for DXT1, 4*16 for DXT2-5).
			if(pFmt->bCompressed)
			{
				pEntry->m_dwPitch = (((pEntry->m_dwWidth+3)/4)*4*4*dwBitsPerPixel)/8;
				pEntry->m_dwLines = (dwLocalHeight+3)/4;
			}
			else
			{
				pEntry->m_dwPitch = (pEntry->m_dwWidth*dwBitsPerPixel)/8;
				pEntry->m_dwLines = dwLocalHeight;
			}

			pEntry->Alloc();

			m_Levels.push_back(pEntry);
		}
	}

	virtual ~CRAWImage() { FreeData(); }

	void DropTopMip()
	{
		// recursive for all attached images
		if(ImageObject *pAttached = GetAttachedImage())
		{
			CRAWImage *pAttachedRAW = pAttached->GetCRAWImage();		assert(pAttachedRAW);

			pAttachedRAW->DropTopMip();
		}

		if(m_Levels.size()>2)
		{
			m_Levels.front()->Free();
			m_Levels.erase(m_Levels.begin());
		}
	}

	bool SaveImageObject( const char *filename ) 
	{ 
		uint32 dwMips = (uint32)m_Levels.size();

		if(!dwMips)
			return false;		// nothing to save

		assert(dwMips);
		assert(m_Levels[0]);

		DDSURFACEDESC2 surf;

		CPixelFormats::BuildSurfaceDesc(this,surf);

		uint32 dwMagic=' SDD';		// 'DDS '

		FILE *out=fopen(filename,"wb");

		if(!out)
		{
			assert(0);
			return false;
		}

		fwrite(&dwMagic,sizeof(uint32),1,out);
		fwrite(&surf,sizeof(DDSURFACEDESC2),1,out);

		int nSides = 1;
		if(GetImageFlags() & CImageExtensionHelper::EIF_Cubemap)
			nSides = 6;

		for(int side=0;side<nSides;++side)
		{
			for(uint32 dwI=0;dwI<dwMips;++dwI)
			{
				const SMipLevel& level = *m_Levels[dwI];

				//char str[256];
				//sprintf(str,"DDS MipsSize: %d %d\n", level.m_dwPitch, level.GetSize());
				//OutputDebugString(str);

				size_t nPitch = level.m_dwPitch;
				size_t nSidePitch = nPitch / nSides;
				assert(nSidePitch * nSides == nPitch);
				size_t nSideSize = level.GetSize() / nSides;
				assert(nSideSize * nSides == level.GetSize());
				for(int row=0;row<level.m_dwLines;++row)
					fwrite(level.m_pData + row*nPitch + side*nSidePitch, nSidePitch, 1, out);
			}
		}

		AppendExtendedData(out);

		fclose(out);

		return true;
	}

	virtual void FreeData()
	{
		std::vector<SMipLevel *>::iterator it,end=m_Levels.end();

		for(it=m_Levels.begin();it!=end;++it)
		{
			SMipLevel *pPtr=*it;

			delete pPtr;
		}

		m_Levels.clear();

		CImageObjectBase::FreeData();
	}

	// interface ImageObject --------------------------------------------------------

	virtual CRAWImage *GetCRAWImage() { return this; }

	//! write image to disk, overwrites any existing file
	virtual bool SaveImage( const char *filename )
	{
		return SaveImageObject(filename);
	}

	virtual HRESULT UpdateSurface(LPDIRECT3DSURFACE9 psurfSrc, int mip, int filter, D3DCUBEMAP_FACES facetype, LPDIRECT3DDEVICE9 pd3ddev){ return S_FALSE; }

	virtual void GetExtend( uint32 &dwWidth, uint32 &dwHeight, uint32 &dwDepth, uint32 &dwSides, uint32 &dwMips ) const
	{
		dwDepth=1;dwSides=1;dwMips=(uint32)m_Levels.size();

		assert(dwMips);
		assert(m_Levels[0]);

		dwWidth=m_Levels[0]->m_dwWidth;
		dwHeight=m_Levels[0]->m_dwHeight;
	}

	//
	virtual uint32 GetWidth( const uint32 dwLevel ) const
	{
		assert(dwLevel<(uint32)m_Levels.size());
		assert(m_Levels[dwLevel]);

		return m_Levels[dwLevel]->m_dwWidth;
	}

	//
	virtual uint32 GetHeight( const uint32 dwLevel ) const
	{
		assert(dwLevel<(uint32)m_Levels.size());
		assert(m_Levels[dwLevel]);

		return m_Levels[dwLevel]->m_dwHeight;
	}

	// memory blocksize
	// for compressed: 
	//     dwPitch*((dwHeight+3)/4)
	// for not compressed: 
	//     dwPitch*dwHeight
	virtual bool Lock( const uint32 dwSide, const uint32 dwLevel, char * &pMem, uint32 &dwPitch )
	{	
		assert(CPixelFormats::GetPixelFormatInfo(m_PixelFormatID));

		assert(dwLevel<(uint32)m_Levels.size());
		assert(m_Levels[dwLevel]);

		pMem=(char *)(m_Levels[dwLevel]->m_pData);
		dwPitch=m_Levels[dwLevel]->m_dwPitch; 
		return true; 
	}

	virtual void Unlock( const uint32 dwLevel ){}

	virtual TPixelFormatID GetPixelFormat() const
	{
		return m_PixelFormatID;
	}

	virtual void SetPixelFormat( TPixelFormatID fmt )
	{
		m_PixelFormatID=fmt;
	}

	virtual bool CopyToSimpleBitmap( CSimpleBitmap<Vec4> &rOut ) const
	{
		uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

		GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

		if(dwMips==0)
			return false;		// no mips

		if(m_PixelFormatID==CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8) || m_PixelFormatID==CPixelFormats::GetNoFromD3DFormat(D3DFMT_X8R8G8B8))
		{
			if(!rOut.Alloc(dwWidth,dwHeight))
				return false;

			char *pStartMem;
			uint32 dwPitch;

			// cast to remove const - as we don't change the data
			if(((CRAWImage *)this)->Lock(0,0,pStartMem,dwPitch))
			{
				const float fScale = 1.0f/255.0f;
				const Vec4 vOffset(0.5f*fScale,0.5f*fScale,0.5f*fScale,0.5f*fScale);

				for(uint32 dwY=0;dwY<dwHeight;++dwY)
				{
					uint8 *pMemLine = (uint8 *)&pStartMem[dwPitch*dwY];

					for(uint32 dwX=0;dwX<dwWidth;++dwX)
					{
						Vec4 vValue = Vec4(pMemLine[2],pMemLine[1],pMemLine[0],pMemLine[3]) * fScale + vOffset;

						rOut.Set(dwX,dwY,vValue);

						pMemLine+=4;
					}
				}

				// cast to remove const - as we don't change the data
				((CRAWImage *)this)->Unlock(0);
				return true;
			}
			else assert(0);
		}

		return false;
	}

	// --------------------------------------------------------------------------------

	IDirect3DTexture9 *CopyToXRGB(LPDIRECT3DDEVICE9 pd3ddev) { return 0; }

private: // -------------------------------------------------------------------------

	struct SMipLevel
	{
		// constructor
		SMipLevel() :m_dwWidth(0), m_dwLines(0), m_dwHeight(0), m_dwPitch(0), m_pData(0)
		{
		}

		// destructor
		~SMipLevel()
		{
			Free();
		}
	
		uint32										m_dwWidth;					//
		uint32										m_dwHeight;				//
		uint32										m_dwLines;					// for compressed textures: height = m_dwLines*4
		uint32										m_dwPitch;					// in Bytes
		uint8 *										m_pData;						//

		void Alloc()
		{
			m_pData = new uint8[m_dwPitch*m_dwLines];
		}

		void Free()
		{
			delete m_pData;m_pData=0;
		}

		uint32 GetSize() const
		{
			assert(m_dwPitch);

			return m_dwPitch*m_dwLines;
		}
	};

	// --------------------------------------------------------------------------------

	TPixelFormatID							m_PixelFormatID;		//
	std::vector<SMipLevel *>		m_Levels;						// stores pointers to avoid reallocations weh elements are copied
};

