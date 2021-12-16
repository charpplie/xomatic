//

#include "stdafx.h"
#include <assert.h>							// assert()
#include <Dbghelp.h>						// GetTimestampForLoadedLibrary
#include "IRCLog.h"							// IRCLog
#include "IConfig.h"						// IConfig
#include "ImageCompiler.h"			// CImageCompiler
#include "ImageUserDialog.h"		// CImageUserDialog
#include "ICfgFile.h"						// ICfgFile
#include <ddraw.h>
#include "BitFiddling.h"				// IntegerLog2()	
//#include "neuquant.h"
#include "ImageObject.h"

/*
HRESULT P8Image::Convert(LPDIRECT3DSURFACE9 psurfSrc, int mip, int filter, D3DCUBEMAP_FACES facetype, LPDIRECT3DDEVICE9 pd3ddev)
{
	assert(psurfSrc);

	D3DSURFACE_DESC sd;
	psurfSrc->GetDesc(&sd);

	LPDIRECT3DSURFACE9 psurfTarget;
	HRESULT hr = pd3ddev->CreateOffscreenPlainSurface(sd.Width, sd.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &psurfTarget, NULL);
	if(FAILED(hr)) return hr;
	hr = D3DXLoadSurfaceFromSurface(psurfTarget, NULL, NULL, psurfSrc, NULL, NULL, D3DX_FILTER_TRIANGLE, 0);
	if(FAILED(hr)) return hr;

	unsigned char *pic = (unsigned char*)malloc(3*sd.Width*sd.Height+1);
	assert(pic);

	{
		D3DLOCKED_RECT lrTarg;
		hr = psurfTarget->LockRect(&lrTarg, NULL, 0);
		if(FAILED(hr)) return hr;
		
		DWORD* pdwRowTarg = (DWORD*)lrTarg.pBits;
		unsigned char *p = pic;

		for (DWORD yp = 0; yp < sd.Height; yp++)
		{
			DWORD* pdwPtrTarg = pdwRowTarg;
			for (DWORD xp = 0; xp < sd.Width; xp++)
			{
				
				*((DWORD *)p) = *pdwPtrTarg++;	// both use BGR order
				p += 3;							// quick and dirty: only works correctly on x86 cpus
			}
			pdwRowTarg += lrTarg.Pitch / 4;
		};
		
		psurfTarget->UnlockRect();
	};

	ReleasePpo(&psurfTarget);

	if(!m_pPalette)	// FIXME: for now, generate palette based solely on main image, may give better result if mipmaps are taken into account
	{
		initnet(pic, 3*sd.Width*sd.Height, 1);		// can use 1 instead of 10 for gold master :)

		learn();
		unbiasnet();
		
		m_pPalette = (unsigned char *)malloc(256*3);
		writecolourmap(m_pPalette);

		inxbuild();
	};
	
	{		
		unsigned char *d = (unsigned char*)malloc(sd.Width*sd.Height);
		assert(d);
		m_ppIndices[mip] = d;

		unsigned char *p = pic;

		for (unsigned int yp = 0; yp < sd.Height; yp++)
		{
			for (unsigned int xp = 0; xp < sd.Width; xp++)
			{
				*d++ = inxsearch(p[0], p[1], p[2]);
				p += 3;	
			}
		};
	};

	free(pic);

	return S_OK;
};

bool P8Image::Save(const char *name)
{
	// FIXME: do this extension workarounding somewhere else, it is not appropriate here
	string filename = name;
	filename.Truncate(filename.GetLength()-4);
	filename += ".tga";

	FILE *fh = fopen(filename.GetString(), "wb");
	if(!fh) return false;
	
	SavePaletizedTextureHeader(fh);

*/
/*
	for(unsigned int mip = 0; mip<tex->GetLevelCount(); mip++)
	{
		LPDIRECT3DSURFACE9 psurf = NULL;
		DXERRORCHECK(tex->GetSurfaceLevel(mip, &psurf));
		D3DSURFACE_DESC sd;
		psurf->GetDesc(&sd);

		D3DLOCKED_RECT lr;
		DXERRORCHECK(psurf->LockRect(&lr, NULL, 0));
		
		SavePaletizedTextureMip(fh, (unsigned char *)lr.pBits, sd.Width, sd.Height, lr.Pitch, mip);

		psurf->UnlockRect();

		ReleasePpo(&psurf);
	};
*/
/*
	fclose(fh);
	
	return true;
};

void P8Image::SavePaletizedTextureHeader(FILE *fh)
{
	unsigned char tgaheader[] =
	{
		0, 1, 1,
		0, 0, 0, 1,
		24,
		0, 0, 0, 0,
		(m_nWidth  & 0x00FF), (m_nWidth  & 0xFF00) / 256,
		(m_nHeight & 0x00FF), (m_nHeight & 0xFF00) / 256,
		8, 0,
	};
	
	fwrite(&tgaheader, 18, 1, fh);
	fwrite(m_pPalette, 3*256, 1, fh);
};

void P8Image::SavePaletizedTextureMip(FILE *fh, unsigned char *buf, int mip)
{
	// if this line is commented, it will save an "extended TGA" with mipmaps following the main picture
	//if(mip) return;

	for(int y = m_nHeight-1; y>=0; y--)
	{
		fwrite(buf+y*m_nWidth, 1, m_nWidth, fh);
		//if(xs&3) fwrite("    ", 4-(xs&3), 1, fh);	// dword align for dds
	};
};

IDirect3DTexture9 *P8Image::CopyToXRGB(LPDIRECT3DDEVICE9 pd3ddev)
{
	assert(m_pPalette);

	IDirect3DTexture9 *ptexNew = NULL;

	if(FAILED(pd3ddev->CreateTexture(m_nWidth, m_nHeight, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &ptexNew, NULL))) return NULL;

	LPDIRECT3DSURFACE9 psurfd = NULL;
	if(FAILED(ptexNew->GetSurfaceLevel(0, &psurfd))) return NULL;

	D3DLOCKED_RECT lrd;
	if(FAILED(psurfd->LockRect(&lrd, NULL, 0))) return NULL;
	
	for(int y = m_nHeight-1; y>=0; y--)
	{
		for(int x = 0; x<m_nHeight; x++)
		{
			int index = *(m_ppIndices[0]+y*m_nWidth+x);
			unsigned char *rgb = m_pPalette+index*3;
			unsigned char *dest = ((unsigned char *)lrd.pBits)+y*lrd.Pitch+x*4;
			dest[0] = rgb[0];						
			dest[1] = rgb[1];						
			dest[2] = rgb[2];						
			dest[3] = 0xFF;						
		};
	};

	psurfd->UnlockRect();
	ReleasePpo(&psurfd);	
	return ptexNew;
};
*/


/*
HRESULT CFloat4Image::UpdateSurface(LPDIRECT3DSURFACE9 psurfSrc, int mip, int filter, D3DCUBEMAP_FACES facetype, LPDIRECT3DDEVICE9 pd3ddev)
{
	assert(psurfSrc);
	assert(mip==0);

	D3DSURFACE_DESC sd;
	psurfSrc->GetDesc(&sd);

	LPDIRECT3DSURFACE9 psurfTarget;
	HRESULT hr = pd3ddev->CreateOffscreenPlainSurface(sd.Width, sd.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &psurfTarget, NULL);
	if(FAILED(hr)) return hr;
	hr = D3DXLoadSurfaceFromSurface(psurfTarget, NULL, NULL, psurfSrc, NULL, NULL, D3DX_FILTER_TRIANGLE, 0);
	if(FAILED(hr)) return hr;

	m_ImageData.Alloc(sd.Width,sd.Height);

	{
		D3DLOCKED_RECT lrTarg;
		hr = psurfTarget->LockRect(&lrTarg, NULL, 0);
		if(FAILED(hr)) return hr;

		DWORD* pdwRowTarg = (DWORD*)lrTarg.pBits;

		for (DWORD yp = 0; yp < sd.Height; yp++)
		{
			DWORD* pdwPtrTarg = pdwRowTarg;
			for (DWORD xp = 0; xp < sd.Width; xp++)
			{
				uint8 ucA = (uint8)( ((*pdwPtrTarg)>>24)&0xff );
				uint8 ucR = (uint8)( ((*pdwPtrTarg)>>16)&0xff );
				uint8 ucG = (uint8)( ((*pdwPtrTarg)>>8)&0xff );
				uint8 ucB = (uint8)( (*pdwPtrTarg)&0xff );

				Vec4 val(ucA/255.0f,ucR/255.0f,ucG/255.0f,ucB/255.0f);

				m_ImageData.Set(xp,yp,val);

				pdwPtrTarg++;
			}
			pdwRowTarg += lrTarg.Pitch / 4;
		}

		psurfTarget->UnlockRect();
	}

	ReleasePpo(&psurfTarget);

	return S_OK;
}
*/

void CImageObjectBase::AppendExtendedData( FILE *out )
{
	uint32 dwStartMagicWord = swizzle32('CExt');		// Crytek Extended -start
	fwrite(&dwStartMagicWord,4,1,out);

	/* // no longer needed - average color per texture was used for off-line global illumination computation (Photon bounce)
	{
		uint32 dwChunkName = swizzle32('AvgC');		// Chunk AverageColor
		fwrite(&dwChunkName,4,1,out);
		uint32 dwChunkSize = 4;
		fwrite(&dwChunkSize,4,1,out);
		fwrite(&m_colAverageARGB,4,1,out);
	}
	*/


/*
	char str[256];
	sprintf(str,"   DEBUG: imageflags=%d\n",m_dwImageFlags);
	OutputDebugString(str);
*/

	if(GetAttachedImage())
	{
		char szTempAttachedData[L_tmpnam_s];
		errno_t err = tmpnam_s(szTempAttachedData);
		assert (!err);

		if(GetAttachedImage()->SaveImage(szTempAttachedData))
		{
			FILE *in=fopen(szTempAttachedData,"rb");		assert(in);

			if(in)
			{	
				fseek(in,0,SEEK_END);
				size_t nFileSize = ftell(in);
				fseek(in,0,SEEK_SET);

				std::vector<uint8> temp;

				temp.resize(nFileSize);

				fread(&temp[0],nFileSize,1,in);

				fclose(in);

				::remove(szTempAttachedData);

				uint32 dwChunkName = swizzle32('AttC');		// Chunk Attached Channel
				fwrite(&dwChunkName,4,1,out);
				uint32 dwChunkSize = nFileSize;
				fwrite(&dwChunkSize,4,1,out);

				fwrite(&temp[0],nFileSize,1,out);
			}

		}
	}

	uint32 dwEndMagicWord = swizzle32('CEnd');		// Crytek Extended - end
	fwrite(&dwEndMagicWord,4,1,out);
}





ImageObject *CImageObjectBase::ExtractAlphaAsA8Image()
{
	ImageObject *ret=0;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	D3DFORMAT fmt = CPixelFormats::GetPixelFormatInfo(GetPixelFormat())->DxtNo;

	if (fmt == D3DFMT_R8G8B8)
		return 0;
	if (fmt != D3DFMT_A8R8G8B8 && fmt != D3DFMT_X8R8G8B8 && fmt != D3DFMT_A8B8G8R8 && fmt != D3DFMT_X8B8G8R8 && fmt!=D3DFMT_A32B32G32R32F)
		return 0;

	ret = new CRAWImage(dwWidth,dwHeight,dwMips,CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8),false);

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		char *pSrc=0,*pDst=0;
		uint32 dwSrcPitch,dwDstPitch;

		Lock(0,dwMip,pSrc,dwSrcPitch);												assert(pSrc);
		ret->Lock(0,dwMip,pDst,dwDstPitch);		assert(pDst);

		if(pSrc && pDst)
		{
			uint32 dwLocalWidth=max((uint32)1,dwWidth>>dwMip), dwLocalHeight=max((uint32)1,dwHeight>>dwMip);

			if(fmt==D3DFMT_A32B32G32R32F)
			{
				// 4*32bit ARGB
				for(uint32 dwY=0;dwY<dwLocalHeight;dwY++)
				{
					float *pSrcLine = (float *)&pSrc[dwSrcPitch*dwY]+3;
					char *pDstLine = &pDst[dwDstPitch*dwY];

					for(uint32 dwX=0;dwX<dwLocalWidth;dwX++)
					{
						uint8 Alpha = (uint8)min(255.0f,max(0.0f,(*pSrcLine)*255.0f+0.5f));

						*pDstLine++ = Alpha;

						pSrcLine+=4;
					}
				}
			}
			else
			{
				// 32bit ARGB
				for(uint32 dwY=0;dwY<dwLocalHeight;dwY++)
				{
					uint32 *pSrcLine = (uint32 *)&pSrc[dwSrcPitch*dwY];
					char *pDstLine = &pDst[dwDstPitch*dwY];

					for(uint32 dwX=0;dwX<dwLocalWidth;dwX++)
					{
						uint8 Alpha = (uint8)((*pSrcLine)>>24);

						*pDstLine++ = Alpha;

						pSrcLine++;
					}
				}	
			}
		}

		if(pDst)
			ret->Unlock(dwMip);

		if(pSrc)
			Unlock(dwMip);
	}

	return ret;
}



void CImageObjectBase::SetAlphaFromA8Image( ImageObject *pImage )
{
	if(!pImage)
		return;

	uint32 dwSrcWidth,dwSrcHeight,dwSrcDepth,dwSrcSides,dwSrcMips;

	pImage->GetExtend(dwSrcWidth,dwSrcHeight,dwSrcDepth,dwSrcSides,dwSrcMips);

	uint32 dwDstWidth,dwDstHeight,dwDstDepth,dwDstSides,dwDstMips;

	GetExtend(dwDstWidth,dwDstHeight,dwDstDepth,dwDstSides,dwDstMips);

	D3DFORMAT fmtSrc = CPixelFormats::GetPixelFormatInfo(pImage->GetPixelFormat())->DxtNo;
	D3DFORMAT fmtDst = CPixelFormats::GetPixelFormatInfo(GetPixelFormat())->DxtNo;

	// assuming attached image can have lower res
	uint32 dwGlobalReduce = dwDstMips-dwSrcMips;
	
	if (fmtSrc != D3DFMT_A8 || fmtDst != D3DFMT_A8R8G8B8 || dwDstWidth!=(dwSrcWidth<<dwGlobalReduce) || dwDstHeight!=(dwSrcHeight<<dwGlobalReduce))
	{
		assert(0);		// should not be
		return;
	}

	for(uint32 dwMip=0;dwMip<dwDstMips;++dwMip)
	{
		char *pSrc=0,*pDst=0;
		uint32 dwSrcPitch,dwDstPitch;

		uint32 dwSrcMip = (uint32)max((int)0,(int)dwMip-(int)dwGlobalReduce);
		uint32 localReduce = dwGlobalReduce-(dwMip-dwSrcMip);

		pImage->Lock(0,dwSrcMip,pSrc,dwSrcPitch);				assert(pSrc);
		Lock(0,dwMip,pDst,dwDstPitch);									assert(pDst);

		if(pSrc && pDst)
		{
			uint32 dwLocalWidth=max((uint32)1,dwDstWidth>>dwMip), dwLocalHeight=max((uint32)1,dwDstHeight>>dwMip);

			for(uint32 dwY=0;dwY<dwLocalHeight;dwY++)
			{
				uint32 *pDstLine = (uint32 *)&pDst[dwDstPitch*dwY];

				for(uint32 dwX=0;dwX<dwLocalWidth;dwX++)
				{
					uint8 cAlpha = pSrc[dwSrcPitch*(dwY>>localReduce)+ (dwX>>localReduce)];

					*pDstLine = ((*pDstLine) & 0xffffff) | ((uint32)(cAlpha)<<24);
					++pDstLine;
				}
			}	
		}

		if(pDst)
			Unlock(dwMip);

		if(pSrc)
			pImage->Unlock(dwMip);
	}
}
