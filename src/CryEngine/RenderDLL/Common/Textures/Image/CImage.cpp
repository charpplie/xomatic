	/*=============================================================================
  CImage.cpp : Common Image class implementation.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Khonich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "CImage.h"
#include "DDSImage.h"
#include "CompressedImage.h"

#if !defined(XENON) && !defined(PS3)
#	include "JpgImage.h"
#	include "TgaImage.h"











#endif

#include "ResourceCompilerHelper.h"				// CResourceCompilerHelper

CImageFile::CImageFile (const string& filename) : m_FileName(filename)
{
  m_pByteImage[0] = NULL;
  m_pByteImage[1] = NULL;
  m_pByteImage[2] = NULL;
  m_pByteImage[3] = NULL;
  m_pByteImage[4] = NULL;
  m_pByteImage[5] = NULL;

  m_eError = eIFE_OK;
  m_eFormat = eTF_Unknown;
  m_eSrcFormat = eTF_Unknown;
  m_NumMips = 0;
  m_Flags = 0;
  m_ImgSize = 0;
  m_Depth = 1;
  m_AvgColor = Col_White;
  m_nStartSeek = 0;
	m_Sides = 1;
}

CImageFile::~CImageFile ()
{
  for (int i=0; i<6; i++)
		mfFree_image(i);
}

void CImageFile::mfSet_dimensions (const int w, const int h)
{
  m_Width = w;
  m_Height = h;
}

void CImageFile::mfSet_error (const EImFileError error, const char* detail)
{
	m_eError = error;
	if (detail)
		TextureWarning(m_FileName.c_str(), detail);
}

CImageFile* CImageFile::mfLoad_file (const string& filename, const bool bReload, const uint32 nFlags)
{
	string sFileToLoad = CResourceCompilerHelper::GetOutputFilename(filename);		// change filename: e.g. instead of TIF, pass DDS

	const char* ext = PathUtil::GetExt(sFileToLoad);

	// is it needed to invoke the resource compiler?
#if defined(WIN32) || defined(WIN64)
	if(CRenderer::CV_r_rc_autoinvoke != 0 && strlen(ext) && CResourceCompilerHelper::IsImageFormat(ext))
	{
		if(/*!gEnv->pRenderer->EF_Query(EFQ_Fullscreen) && */gEnv->pSystem->IsDevMode())
		{
			CResourceCompilerHelper rch;
			sFileToLoad = rch.ProcessIfNeeded(filename, bReload);
			if(rch.IsError())
			{
				gEnv->pLog->LogError("InvokeResourceCompiler() failed (missing rc.exe?)");
				return NULL;
			}
			ext = PathUtil::GetExt(sFileToLoad);	// update extension
		}
		else
			gEnv->pLog->LogWarning("r_rc_autoinvoke of '%s' suppressed (full screen or non DevMode)", filename.c_str());
	}
#endif

	CImageFile* pImageFile = NULL;

	// Try DDS first
	if (!strcmp(ext, "dds"))
		pImageFile = new CImageDDSFile (sFileToLoad, nFlags);
#if !defined(XENON) && !defined(PS3)
	else
	{
		CCryFile file(sFileToLoad.c_str(),"rb");
		if(file.GetHandle())
		{
			const size_t nFileSize = file.GetLength();
			byte *buf = new byte [nFileSize];
			if (!buf)
			{
				assert(0);
				return 0;		// out of memory
			}

			const size_t readSize = file.ReadRaw(buf, nFileSize);
			assert(readSize == nFileSize);

			if (!strcmp(ext, "jpg"))
				pImageFile = new CImageJpgFile (filename, buf, nFileSize);
			else if (!strcmp(ext, "tga"))
				pImageFile = new CImageTgaFile (filename, buf, nFileSize);
			else
				TextureWarning(sFileToLoad.c_str(), "Unsupported texture extension '%s': '%s'", ext, filename.c_str());

			delete [] buf;
		}
	}



#endif

	if(pImageFile && pImageFile->mfGet_error() != eIFE_OK)
	{
		SAFE_DELETE(pImageFile);
	}

	return pImageFile;
}

void CImageFile::mfFree_image( const int nSide )
{
	SAFE_DELETE_ARRAY(m_pByteImage[nSide]);
}

byte* CImageFile::mfGet_image( const int nSide )
{
	if (!m_pByteImage[nSide] && m_ImgSize)
			m_pByteImage[nSide] = new byte [m_ImgSize];
	return m_pByteImage[nSide];
}
