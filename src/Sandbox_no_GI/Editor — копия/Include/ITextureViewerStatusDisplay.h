/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: ITextureViewerStausDisplay.h  ,v 1.1 2008/09/1 12:47:51 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file declares the interface used by the texture viewer
and (implemented first implemented by the Texture Browser Dialog) update
display information in other related windows.
-------------------------------------------------------------------------
History:
- 1:09:2008   12:47 : Created by Paulo Zaffari

*************************************************************************/
#ifndef ITextureViewerStatusDisplay_h__
#define ITextureViewerStatusDisplay_h__

struct ITextureViewerStatusDisplay
{
	virtual void SetTotalFileSize(unsigned long long nTotalFilzeSize)=0;
	virtual void SetCurrentFilename(const char* cszDirectory,const char* cszCurrentFilename)=0;

	virtual void SetTextureType(const char* cszTextureType)=0;
	virtual void SetNumberofMips(const char* szNumberOfMips)=0;
	virtual void SetResolution(const char*  szResolution)=0;
	virtual void SetFileSize(const char* szTotalFilzeSize)=0;
};


#endif // ITextureViewerStatusDisplay_h__