/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureDatabaseCreator.cpp  ,v 1.1 2008/08/28 11:27:08 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file implements the database creator for the texture viewer
which implements the ITextureDatabaseUpdater interface.
Objects of this class are responsible for updating the texture viewer with
textures in a thread based way.
-------------------------------------------------------------------------
History:
- 28:08:2008   11:27 : Created by Paulo Zaffari

*************************************************************************/

#include "stdafx.h"
#include "TextureDatabaseCreator.h"
#include "TextureDatabaseItem.h"
#include "..\Controls\TextureViewer.h"

#include "ImageExtensionHelper.h"

#include "../Util/ImageUtil.h"

CTextureDatabaseCreator::CTextureDatabaseCreator()
{
	m_poAssociatedViewer=NULL;
	m_boMustEndThread=false;
}

CTextureDatabaseCreator::~CTextureDatabaseCreator()
{

}

bool CTextureDatabaseCreator::SetAssociatedViewer(CTextureViewer* poAssociatedViewer)
{
	// Hot changing of associated viewer is not allowed, for now.
	if (m_poAssociatedViewer)
	{
		return false;
	}

	m_poAssociatedViewer=poAssociatedViewer;

	return true;
}

void CTextureDatabaseCreator::AddPreLoadedTextures(TDFilenameToItemMap& rcFilenameSet)
{
	// If no associated viewer, no need to add textures at all...
	if (!m_poAssociatedViewer)
	{
		// TOD: Must add a log entry about this.
		return;
	}

	std::vector<ITexture*>	cpiTextures;

	int											nCurrentTexture(0);
	int											nNumberOfTextures(0);
	int											nTextureWidth(0);
	int											nTextureHeight(0);
	const char*							szTextureName(NULL);
	string									strIntermediateTextureName;
	CString									strOutputTextureName;
	CTextureDatabaseItem*		poTextureDatabaseItem(NULL);
	CCryFile								oFile;
	size_t									nFileSize(0);

	nNumberOfTextures=*(int *)GetIEditor()->GetRenderer()->EF_Query(EFQ_GetAllTextures,NULL);

	cpiTextures.resize(nNumberOfTextures,NULL);
	GetIEditor()->GetRenderer()->EF_Query(EFQ_GetAllTextures,(INT_PTR)&cpiTextures.front());

	for (nCurrentTexture=0;nCurrentTexture<nNumberOfTextures;++nCurrentTexture,m_oAddItemMutex.Unlock())
	{
		m_oAddItemMutex.Lock();

		// Checks if we must do a premature thread killing.
		if (m_boMustEndThread)
		{
			m_oAddItemMutex.Unlock();
			return;
		}

		ITexture*& piCurrentTexture=cpiTextures[nCurrentTexture];
		if (piCurrentTexture->IsTextureLoaded())
		{
			if (piCurrentTexture->GetTextureType()==eTT_3D)
			{
				// Currently the texture browser does not support 3D textures, as they make
				// everything crash when calling ITexture::GetData32().
				continue;
			}
			nTextureWidth=piCurrentTexture->GetWidth();
			nTextureHeight=piCurrentTexture->GetHeight();

			// Any texture with a side equals or less than 0 is impossible to display...
			// .. so we won't wate our time on them...
			if ((nTextureWidth<=0)||(nTextureHeight<=0))
			{
				continue;	
			}

			// Textures without names are not important for us.
			szTextureName=piCurrentTexture->GetName();
			if (!szTextureName)
			{
				continue;
			}

			// We don't want empty names as well.
			if (strlen(szTextureName)==0)
			{
				continue;
			}

			// Textures which name starts with '$' are dynamic textures or in any way textures that
			// don't interest us.
			if (szTextureName[0]=='$')
			{
				continue;
			}
		}
		else
		{
			// For now we are not loading textures, just using the ones already loaded.
		}

		strIntermediateTextureName=szTextureName;
		strIntermediateTextureName.MakeLower();
		strOutputTextureName=strIntermediateTextureName;
		Path::ConvertSlashToBackSlash(strOutputTextureName);

		if (rcFilenameSet.find(strOutputTextureName.GetBuffer())!=rcFilenameSet.end())
		{
			continue;
		}

		poTextureDatabaseItem=new CTextureDatabaseItem();

		//OBS: To get the file size we will need to check with the OS, as this information
		// is not stored anywhere else.
		// This may be QUITE slow.
		oFile.Open(szTextureName,"rb");
		if (oFile.GetHandle())
		{
			nFileSize=oFile.GetLength();
		}
		else
		{
			nFileSize=0;
		}
		poTextureDatabaseItem->SetFileSize(nFileSize); // A safe invalid value for now.
		oFile.Close();

		poTextureDatabaseItem->SetSurfaceType(piCurrentTexture->GetFormatName());
		poTextureDatabaseItem->SetMips(piCurrentTexture->GetNumMips());
		poTextureDatabaseItem->SetTextureWidth(piCurrentTexture->GetWidth());
		poTextureDatabaseItem->SetTextureHeight(piCurrentTexture->GetHeight());

		// We are storing the original filename here for textures which come from the
		// the engine (without using backslashes) as it is done with all other  textures.
		// For some bizarre reason, if we did, the engine would have problems cashing the
		// textures. On the other hand, if we adopted as a standard the normal slashes
		// it seems that some textures loaded from the disk would have problems... so...
		// there doesn't seem to be a perfect cenario here.
		poTextureDatabaseItem->SetFilename(strIntermediateTextureName.c_str());
		//poTextureDatabaseItem->SetFilename(strOutputTextureName.c_str());

		poTextureDatabaseItem->SetUsedInLevel(true);
		poTextureDatabaseItem->SetHasAlphaChannelFlag(CImageExtensionHelper::HasAlphaForName(piCurrentTexture->GetFormatName()));
		
		bool bDeleteAndExit = false;

		if( !m_boMustEndThread )
		{
			bDeleteAndExit = !m_poAssociatedViewer->AddTextureDatabaseItem(poTextureDatabaseItem);
		}

		if( m_boMustEndThread || bDeleteAndExit )
		{
			delete poTextureDatabaseItem;
			m_oAddItemMutex.Unlock();
			return;
		}

		rcFilenameSet.insert(TDInsertionElement(strOutputTextureName.GetBuffer(),poTextureDatabaseItem));
	}
}

void CTextureDatabaseCreator::GatherPreloadedTextures()
{
	std::vector<ITexture*>	cpiPreloadedTextures;
	int											nNumberOfPreloadedTextures(0);
	CString									strFilename;

	//
	// get all textures that are already loaded
	//
	nNumberOfPreloadedTextures = *(int*)GetIEditor()->GetRenderer()->EF_Query( EFQ_GetAllTextures, NULL );
	cpiPreloadedTextures.resize( nNumberOfPreloadedTextures, NULL );
	m_cPreloadedTexturesFilenames.clear();

	if( nNumberOfPreloadedTextures )
	{
		GetIEditor()->GetRenderer()->EF_Query( EFQ_GetAllTextures,(INT_PTR)&cpiPreloadedTextures.front() );
	}

	for( size_t i = 0, iCount = cpiPreloadedTextures.size(); i < iCount; ++i )
	{
		strFilename = CString( cpiPreloadedTextures[i]->GetName() ).MakeLower();
		Path::ConvertSlashToBackSlash( strFilename );
		m_cPreloadedTexturesFilenames.insert( strFilename.GetBuffer() );
	}
}

void CTextureDatabaseCreator::AddDiskTexture( TDFilenameToItemMap &rcFilenameSet )
{
	// If no associated viewer, no need to add textures at all...
	if (!m_poAssociatedViewer)
	{
		// TOD: Must add a log entry about this.
		return;
	}

	int																			nTextureWidth(0);
	int																			nTextureHeight(0);
	const char															*szTextureName(NULL);
	CString																	strExtension;
	CString																	strFilename;
	string																	strSurfaceType;

	CFileUtil::FileArray										cFiles;
	int																			nTotalFiles(0);
	int																			nCurrentFile(0);
	string																	strIntermediateFilename;
	CString																	strOutputTextureName;
	bool																		bIsUsedInLevel = false;

	CCryFile																file;
	CImageExtensionHelper::DDS_FILE_DESC		stFileDesc;

	CTextureDatabaseItem*										poTextureDatabaseItem(NULL);

	// Will first look for all files and just add those which are actually textures...
	// Responsiveness optimization: instead of traversing directories just once and check
	// all files, in order to have faster response times we do it twice: once for the .dds
	// files and once for .tif. As this is threaded, both searches will be MUCH faster alone
	// even though the combined time will be bigger.
	// Still, the method used will have some results MUCH faster and thus the user will
	// have time to have fun with them before the final results will be there.
	//CFileUtil::ScanDirectory(PathUtil::GetGameFolder().c_str(),"*.*"/*"*.*"*/,cFiles,true);

	CFileUtil::ScanDirectory(PathUtil::GetGameFolder().c_str(),"*.dds"/*"*.*"*/,cFiles,true);
	// For every file, check if is a texture...
	nTotalFiles=cFiles.size();
	for (nCurrentFile=0;nCurrentFile<nTotalFiles;++nCurrentFile,m_oAddItemMutex.Unlock())
	{
		m_oAddItemMutex.Lock();
		// Checks if we must do a premature thread killing.
		if (m_boMustEndThread)
		{
			m_oAddItemMutex.Unlock();
			return;
		}

		CFileUtil::FileDesc& rstFileDescriptor=cFiles[nCurrentFile];

		strIntermediateFilename=rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		strOutputTextureName=strIntermediateFilename;
		Path::ConvertSlashToBackSlash(strOutputTextureName);

		// No need to load files already loaded by the engine...
		if (rcFilenameSet.find(strOutputTextureName.GetBuffer())!=rcFilenameSet.end())
		{
			continue;
		}

		// No need for temp files.
		if (strstr(rstFileDescriptor.filename,"~~~TempFile~~~")!=NULL)
		{
			continue;
		}
		
		strExtension=Path::GetExt(strOutputTextureName.GetBuffer());

		if (!file.Open( rstFileDescriptor.filename,"rb" ))
		{
			continue;
		}

		// Read magic number
		file.ReadTypeRaw( &stFileDesc );
		if (!stFileDesc.IsValid())
		{
			continue;
		}

		ETEX_Format format = DDSFormats::GetFormatByDesc(stFileDesc.header.ddspf);
		int  nHorizontalFacesMultiplier(1);
		int  nVerticalFacesMultiplier(1);
		bool boIsCubemap(false);
		if ((stFileDesc.header.dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) && (stFileDesc.header.dwCubemapFlags & DDS_CUBEMAP_ALLFACES))
		{
			boIsCubemap=true;

			if (
				(format == eTF_A8R8G8B8)
				||
				(format == eTF_X8R8G8B8)
				||
				(format == eTF_R8G8B8)
				||
				(format == eTF_L8)
				||
				(format == eTF_A8)
				)
			{
				nHorizontalFacesMultiplier=3;
				nVerticalFacesMultiplier=2;
			}
		}

		// Textures without names are not important for us.
		szTextureName=strOutputTextureName.GetBuffer();
		if (!szTextureName)
		{
			continue;
		}

		nTextureWidth=stFileDesc.header.dwWidth;
		nTextureHeight=stFileDesc.header.dwHeight;

		// Any texture with a side equals or less than 0 is impossible to display...
		// .. so we won't wate our time on them...
		if ((nTextureWidth<=0)||(nTextureHeight<=0))
		{
			continue;	
		}

		// Textures which name starts with '$' are dynamic textures or in any way textures that
		// don't interest us.
		if (szTextureName[0]=='$')
		{
			continue;
		}

		poTextureDatabaseItem=new CTextureDatabaseItem();

		strSurfaceType=CImageExtensionHelper::NameForDesc(stFileDesc.header.ddspf);
		poTextureDatabaseItem->SetFileSize(rstFileDescriptor.size);
		poTextureDatabaseItem->SetSurfaceType(strSurfaceType);
		poTextureDatabaseItem->SetMips(stFileDesc.header.GetMipCount());
		poTextureDatabaseItem->SetTextureWidth(nTextureWidth*nHorizontalFacesMultiplier);
		poTextureDatabaseItem->SetTextureHeight(nTextureHeight*nVerticalFacesMultiplier);
		poTextureDatabaseItem->SetFilename(strOutputTextureName.GetBuffer());
		bIsUsedInLevel = m_cPreloadedTexturesFilenames.end() != m_cPreloadedTexturesFilenames.find( strOutputTextureName.GetBuffer() );
		poTextureDatabaseItem->SetUsedInLevel( bIsUsedInLevel );
		poTextureDatabaseItem->SetIsCubeMap(boIsCubemap);
		poTextureDatabaseItem->SetHasAlphaChannelFlag(CImageExtensionHelper::HasAlphaForName(strSurfaceType.c_str()));

		if (!m_boMustEndThread)
		{
			m_poAssociatedViewer->AddTextureDatabaseItem(poTextureDatabaseItem);
		}
		else
		{
			m_oAddItemMutex.Unlock();
			delete poTextureDatabaseItem;
			return;
		}

		rcFilenameSet.insert(TDInsertionElement(strOutputTextureName.GetBuffer(),poTextureDatabaseItem));
	}
	m_oAddItemMutex.Unlock();
	

	CFileUtil::ScanDirectory(PathUtil::GetGameFolder().c_str(),"*.tif",cFiles,true);
	// For every file, check if is a texture...
	nTotalFiles=cFiles.size();
	for (nCurrentFile=0;nCurrentFile<nTotalFiles;++nCurrentFile,m_oAddItemMutex.Unlock())
	{
		m_oAddItemMutex.Lock();
		// Checks if we must do a premature thread killing.
		if (m_boMustEndThread)
		{
			m_oAddItemMutex.Lock();
			return;
		}

		CFileUtil::FileDesc& rstFileDescriptor=cFiles[nCurrentFile];

		strIntermediateFilename=rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		strOutputTextureName=strIntermediateFilename;
		Path::ConvertSlashToBackSlash(strOutputTextureName);

		// No need to load files already loaded...
		if (rcFilenameSet.find(strOutputTextureName.GetBuffer())!=rcFilenameSet.end())
		{
			continue;
		}

		// No need for temp files.
		if (strstr(strOutputTextureName.GetBuffer(),"~~~TempFile~~~")!=NULL)
		{
			continue;
		}

		strFilename=strOutputTextureName.GetBuffer();
		strExtension=Path::GetExt(strOutputTextureName.GetBuffer());
		strFilename=Path::ReplaceExtension(strFilename,"dds");

		// No need to load files already loaded as it's equivalent dds.
		if (rcFilenameSet.find(strFilename.GetBuffer())!=rcFilenameSet.end())
		{
			continue;
		}

		CImage		oImage;
		CImageUtil::LoadImage(rstFileDescriptor.filename,oImage);
	
		// Textures without names are not important for us.
		szTextureName=(char*)rstFileDescriptor.filename.GetBuffer();
		if (!szTextureName)
		{
			continue;
		}

		nTextureWidth=oImage.GetWidth();
		nTextureHeight=oImage.GetHeight();

		// Any texture with a side equals or less than 0 is impossible to display...
		// .. so we won't wate our time on them...
		if ((nTextureWidth<=0)||(nTextureHeight<=0))
		{
			continue;	
		}

		// Textures which name starts with '$' are dynamic textures or in any way textures that
		// don't interest us.
		if (szTextureName[0]=='$')
		{
			continue;
		}

		strSurfaceType=oImage.GetFormatDescription();
		poTextureDatabaseItem=new CTextureDatabaseItem();
		poTextureDatabaseItem->SetFileSize(rstFileDescriptor.size);
		poTextureDatabaseItem->SetSurfaceType(strSurfaceType);
		poTextureDatabaseItem->SetMips(oImage.GetNumberOfMipMaps()); // Currently setting to one... but we could have more accurate info on that.
		poTextureDatabaseItem->SetTextureWidth(oImage.GetWidth());
		poTextureDatabaseItem->SetTextureHeight(oImage.GetHeight());
		poTextureDatabaseItem->SetFilename(strOutputTextureName.GetBuffer());
		poTextureDatabaseItem->SetUsedInLevel(false);
		poTextureDatabaseItem->SetIsCubeMap(oImage.IsCubemap());
		poTextureDatabaseItem->SetHasAlphaChannelFlag(CImageExtensionHelper::HasAlphaForName(strSurfaceType.c_str()));

		// This is a good place to check if we should stop processing files.
		if (m_boMustEndThread)
		{
			m_oAddItemMutex.Unlock();
			delete poTextureDatabaseItem;
			return;	
		}

		m_poAssociatedViewer->AddTextureDatabaseItem(poTextureDatabaseItem);

		rcFilenameSet.insert(TDInsertionElement(strOutputTextureName.GetBuffer(),poTextureDatabaseItem));
	}
	m_oAddItemMutex.Unlock();
}

void CTextureDatabaseCreator::NotifyShutDown()
{
	Lock();
	m_boMustEndThread=true;
	Unlock();
}

void CTextureDatabaseCreator::Lock()
{
	CryThread<CTextureDatabaseCreator>::Lock();
}

void CTextureDatabaseCreator::Unlock()
{
	CryThread<CTextureDatabaseCreator>::Unlock();
}

void CTextureDatabaseCreator::WaitForThread()
{
	CryThread<CTextureDatabaseCreator>::WaitForThread();
}

CTextureDatabaseItem*	CTextureDatabaseCreator::GetItem(const char* szAddItem)
{
	if (!m_poAssociatedViewer)
	{
		return NULL;
	}

	if (strlen(szAddItem)==0)
	{
		return NULL;
	}

	m_oAddItemMutex.Lock();

	CTextureDatabaseItem*										poReturn(NULL);
	CString                                 strOriginalAddItem(szAddItem);
	CString                                 strAddItem(szAddItem);
	CString																  strExtension;
	CString																	strOutputFilename;

	// Replace the tif extension for the dds extension, as in other modules.
	strExtension=Path::GetExt(strAddItem);
	if (strExtension.GetLength()>0)
	{
		if (strExtension.CompareNoCase("tif")==0)
		{
			strAddItem=Path::ReplaceExtension(strAddItem,"dds");
		}			
	}		


	TDFilenameToItemMap::iterator				itIterator;
	itIterator=m_cKnwonTextures.find(strAddItem.GetBuffer());
	if (itIterator!=m_cKnwonTextures.end())
	{
		m_oAddItemMutex.Unlock();
		return itIterator->second;
	}

	itIterator=m_cKnwonTextures.find(strOriginalAddItem.GetBuffer());
	if (itIterator!=m_cKnwonTextures.end())
	{
		m_oAddItemMutex.Unlock();
		return itIterator->second;
	}


	CFileUtil::FileArray										cFiles;
	CString																	strPath(PathUtil::GetGameFolder().c_str());
	string																	strSurfaceType;

	strOutputFilename=strAddItem.GetBuffer();
	CFileUtil::ScanDirectory(strPath,strAddItem,cFiles,false);
	if (cFiles.empty())
	{	
		strOutputFilename=strOriginalAddItem.GetBuffer();
		CFileUtil::ScanDirectory(strPath,strOriginalAddItem,cFiles,false);
		if (cFiles.empty())
		{			
			m_oAddItemMutex.Unlock();
			return NULL;
		}				
	}

	int										nTextureWidth(0),nTextureHeight(0);
	CFileUtil::FileDesc&	rstFileDescriptor=cFiles[0];
	CImage								oImage;
	CImageUtil::LoadImage(strOutputFilename.GetBuffer(),oImage);
	
	Path::ConvertSlashToBackSlash(strOutputFilename);
	strlwr((char*)strOutputFilename.GetBuffer());

	nTextureWidth=oImage.GetWidth();
	nTextureHeight=oImage.GetHeight();

	strSurfaceType=oImage.GetFormatDescription();
	poReturn=new CTextureDatabaseItem();
	poReturn->SetFileSize(rstFileDescriptor.size);
	poReturn->SetSurfaceType(strSurfaceType);
	poReturn->SetMips(1); // Currently setting to one... but we could have more accurate info on that.
	poReturn->SetTextureWidth(oImage.GetWidth());
	poReturn->SetTextureHeight(oImage.GetHeight());
	poReturn->SetFilename(strOutputFilename.GetBuffer());
	poReturn->SetUsedInLevel(true);

	if (m_boMustEndThread)
	{				
		delete poReturn;
		m_oAddItemMutex.Unlock();
		return NULL;	
	}

	m_poAssociatedViewer->AddTextureDatabaseItem(poReturn);
	m_cKnwonTextures.insert(TDInsertionElement(strOutputFilename.GetBuffer(),poReturn));
	m_oAddItemMutex.Unlock();

	return poReturn;
}

void CTextureDatabaseCreator::Run()
{
	if (!m_poAssociatedViewer)
	{
		return;
	}

	//TODO: delete, took out this function because it crashes the engine, due to usage of renderer inside this thread, and not main
	//AddPreLoadedTextures(m_cKnwonTextures);
	GatherPreloadedTextures();
	AddDiskTexture(m_cKnwonTextures);

	m_poAssociatedViewer->FinishedDatabaseUpdate();
}
