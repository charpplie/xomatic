/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc plugin utilities.
-------------------------------------------------------------------------
History:
- 10:03:2014: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_PluginUtils.h"

#include <IActionMapManager.h>
#include <IEntityClass.h>
#include <IEntitySystem.h>
#include <IResourceSelectorHost.h>

#include "Schematyc_QuickSearchDlg.h"

namespace
{
	typedef std::vector<string> TStringVector;

	class CQuickSearchOptions : public Schematyc::IQuickSearchOptions
	{
	public:

		// IQuickSearchOptions

		virtual size_t GetCount() const
		{
			return m_options.size();
		}

		virtual const char* GetName(size_t iOption) const
		{
			return iOption < m_options.size() ? m_options[iOption].c_str() : "";
		}

		// ~IQuickSearchOptions

		void AddOption(const char* option)
		{
			CRY_ASSERT(option != NULL);
			if(option != NULL)
			{
				m_options.push_back(option);
			}
		}

	private:

		TStringVector	m_options;
	};

	dll_string EntityClassNameSelector(const SResourceSelectorContext& context, const char* previousValue, Serialization::StringListValue* pStringListValue)
	{
		CPoint	cursorPos;
		GetCursorPos(&cursorPos);

		CQuickSearchOptions		quickSearchOptions;
		IEntityClassRegistry&	entityClassRegistry = *gEnv->pEntitySystem->GetClassRegistry();
		entityClassRegistry.IteratorMoveFirst();
		while(IEntityClass* pClass = entityClassRegistry.IteratorNext())
		{
			quickSearchOptions.AddOption(pClass->GetName());
		}

		SET_LOCAL_RESOURCE_SCOPE
		Schematyc::CQuickSearchDlg	quickSearchDlg(CWnd::FromHandle(context.parentWindow), CPoint(cursorPos.x - 10, cursorPos.y - 10), quickSearchOptions);
		if(quickSearchDlg.DoModal() == IDOK)
		{
			return quickSearchOptions.GetName(quickSearchDlg.GetSelectedOption());
		}
		else
		{
			return "";
		}
	}

	dll_string ActionMapNameSelector(const SResourceSelectorContext& context, const char* previousValue, Serialization::StringListValue* pStringListValue)
	{
		CPoint	cursorPos;
		GetCursorPos(&cursorPos);

		CQuickSearchOptions		quickSearchOptions;
		IActionMapIteratorPtr iActionMap = gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->CreateActionMapIterator();
		while(IActionMap* pActionMap = iActionMap->Next())
		{
			quickSearchOptions.AddOption(pActionMap->GetName());
		}

		SET_LOCAL_RESOURCE_SCOPE
		Schematyc::CQuickSearchDlg	quickSearchDlg(CWnd::FromHandle(context.parentWindow), CPoint(cursorPos.x - 10, cursorPos.y - 10), quickSearchOptions);
		if(quickSearchDlg.DoModal() == IDOK)
		{
			return quickSearchOptions.GetName(quickSearchDlg.GetSelectedOption());
		}
		else
		{
			return "";
		}
	}

	REGISTER_RESOURCE_SELECTOR("EntityClassName", EntityClassNameSelector, "")
	REGISTER_RESOURCE_SELECTOR("ActionMapName", ActionMapNameSelector, "")
}

namespace Schematyc
{
	namespace PluginUtils
	{
		//////////////////////////////////////////////////////////////////////////
		CLocalResourceScope::CLocalResourceScope()
			: m_hPrevInstance(AfxGetResourceHandle())
		{
			AfxSetResourceHandle(GetHInstance());
		}

		//////////////////////////////////////////////////////////////////////////
		CLocalResourceScope::~CLocalResourceScope()
		{
			AfxSetResourceHandle(m_hPrevInstance);
		}

		//////////////////////////////////////////////////////////////////////////
		SDragAndDropData::SDragAndDropData()
			: icon(INVALID_INDEX)
		{}

		//////////////////////////////////////////////////////////////////////////
		SDragAndDropData::SDragAndDropData(size_t _icon, const SGUID& _guid)
			: icon(_icon)
			, guid(_guid)
		{}

		//////////////////////////////////////////////////////////////////////////
		const UINT SDragAndDropData::GetClipboardFormat()
		{
			static const UINT clipboardFormat = RegisterClipboardFormat("Schematyc::DragAndDropItem");
			return clipboardFormat;
		}

		//////////////////////////////////////////////////////////////////////////
		bool BeginDragAndDrop(const SDragAndDropData &data)
		{
			CSharedFile	sharedFile(GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT);
			sharedFile.Write(&data, sizeof(data));
			if(HGLOBAL hData = sharedFile.Detach())
			{
				COleDataSource*	pDataSource = new COleDataSource();
				pDataSource->CacheGlobalData(SDragAndDropData::GetClipboardFormat(), hData);
				pDataSource->DoDragDrop(DROPEFFECT_MOVE | DROPEFFECT_LINK);
				return true;
			}
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		bool GetDragAndDropData(COleDataObject* pDataObject, SDragAndDropData &data)
		{
			if(HGLOBAL hData = pDataObject->GetGlobalData(SDragAndDropData::GetClipboardFormat()))
			{
				data = *static_cast<const SDragAndDropData*>(GlobalLock(hData));
				GlobalUnlock(hData);
				return true;
			}
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		bool LoadTrueColorImageList(UINT nIDResource, int iconWidth, COLORREF maskColor, CImageList &imageList)
		{
			if(HANDLE hImage = LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResource), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION))
			{
				CBitmap bitmap;
				if(bitmap.Attach(hImage))
				{
					BITMAP	bitmapData;
					if(bitmap.GetBitmap(&bitmapData))
					{
						const CSize	size(bitmapData.bmWidth, bitmapData.bmHeight); 
						const int		imageCount = size.cx / iconWidth;
						if(imageList || (imageList.Create(iconWidth, size.cy, ILC_COLOR32 | ILC_MASK, imageCount, 0) == TRUE))
						{
							return imageList.Add(&bitmap, maskColor) != -1;
						}
					}
				}
			}
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		void GetSubFoldersAndFileNames(const char* folderName, const char* extension, bool ignorePakFiles, TStringVector& subFolderNames, TStringVector& fileNames)
		{
			CRY_ASSERT(folderName != NULL);
			if(folderName != NULL)
			{
				string	searchPath = folderName;
				searchPath.append("/");
				searchPath.append(extension ? extension : "*.*");
				_finddata_t	findData;
				intptr_t		handle = gEnv->pCryPak->FindFirst(searchPath.c_str(), &findData);
				if(handle >= 0)
				{
					do
					{
						if(findData.name[0] != '.')
						{
							if(findData.attrib & _A_SUBDIR)
							{
								bool	ignoreSubDir = false;
								if(ignorePakFiles)
								{
									stack_string	fileName = gEnv->pCryPak->GetGameFolder();
									fileName.append("/");
									fileName.append(folderName);
									fileName.append("/");
									fileName.append(findData.name);
									if(GetFileAttributes(fileName.c_str()) == INVALID_FILE_ATTRIBUTES)
									{
										ignoreSubDir = true;
									}
								}
								if(ignoreSubDir == false)
								{
									subFolderNames.push_back(findData.name);
								}
							}
							else if((ignorePakFiles == false) || ((findData.attrib & _A_IN_CRYPAK) == 0))
							{
								fileNames.push_back(findData.name);
							}
						}
					} while(gEnv->pCryPak->FindNext(handle, &findData) >= 0);
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		bool IsNumericChar(char x)
		{
			return ((x >= '0') && (x <= '9'));
		}

		//////////////////////////////////////////////////////////////////////////
		bool IsAlphanumericChar(char x)
		{
			return ((x >= 'a') && (x <= 'z')) || ((x >= 'A') && (x <= 'Z')) || ((x >= '0') && (x <= '9'));
		}

		//////////////////////////////////////////////////////////////////////////
		bool StringContainsNonAlphaNumericChars(const char* input, const char* exceptions)
		{
			for(const char* pos = input; *pos != '\0'; ++ pos)
			{
				if(!IsAlphanumericChar(*pos) && (strchr(exceptions, *pos) == NULL))
				{
					return true;
				}
			}
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		bool IsValidName(const char* name, stack_string& errorMessage)
		{
			if((name == NULL) || (name[0] == '\0'))
			{
				errorMessage = "Name cannot be empty.";
				return false;
			}
			else if(IsNumericChar(name[0]))
			{
				errorMessage = "Name cannot start with a number.";
				return false;
			}
			else if(StringContainsNonAlphaNumericChars(name, "_"))
			{
				errorMessage = "Name can only contain alphanumeric characters and underscores.";
				return false;
			}
			return true;
		}

		//////////////////////////////////////////////////////////////////////////
		bool IsValidFilePath(const char* filePath, stack_string& errorMessage)
		{
			if((filePath == NULL) || (filePath[0] == '\0'))
			{
				errorMessage = "File path cannot be empty.";
				return false;
			}
			else if(IsNumericChar(filePath[0]))
			{
				errorMessage = "File path cannot start with a number.";
				return false;
			}
			else if(StringContainsNonAlphaNumericChars(filePath, "_/\\"))
			{
				errorMessage = "File path can only contain alphanumeric characters, underscores and slashes.";
				return false;
			}
			return true;
		}

		//////////////////////////////////////////////////////////////////////////
		bool IsValidFileName(const char* fileName, stack_string& errorMessage)
		{
			if((fileName == NULL) || (fileName[0] == '\0'))
			{
				errorMessage = "File name cannot be empty.";
				return false;
			}
			else if(IsNumericChar(fileName[0]))
			{
				errorMessage = "File name cannot start with a number.";
				return false;
			}
			else if(StringContainsNonAlphaNumericChars(fileName, "_"))
			{
				errorMessage = "File name can only contain alphanumeric characters and underscores.";
				return false;
			}
			return true;
		}
	}
}