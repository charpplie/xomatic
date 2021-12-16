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

#ifndef __SCHEMATYC_PLUGINUTILS_H__
#define __SCHEMATYC_PLUGINUTILS_H__

// TODO : Merge with DragAndDrop, EditorGUID and String util headers?

#define SET_LOCAL_RESOURCE_SCOPE Schematyc::PluginUtils::CLocalResourceScope localResourceScope;

namespace Schematyc
{
	typedef std::vector<string> TStringVector;

	namespace PluginUtils
	{
		class CLocalResourceScope
		{
		public:

			CLocalResourceScope();

			~CLocalResourceScope();

		private:

			HINSTANCE	m_hPrevInstance;
		};

		struct SDragAndDropData
		{
			SDragAndDropData();

			SDragAndDropData(size_t _icon, const SGUID& _guid = SGUID());

			static inline const UINT GetClipboardFormat();

			size_t	icon;
			SGUID		guid;
		};

		bool BeginDragAndDrop(const SDragAndDropData &data);

		bool GetDragAndDropData(COleDataObject* pDataObject, SDragAndDropData &data);

		bool LoadTrueColorImageList(UINT nIDResource, int iconWidth, COLORREF maskColor, CImageList &imageList);

		void GetSubFoldersAndFileNames(const char* folderName, const char* extension, bool ignorePakFiles, TStringVector& subFolderNames, TStringVector& fileNames);

		bool IsNumericChar(char x);
		
		bool IsAlphanumericChar(char x);

		bool StringContainsNonAlphaNumericChars(const char* input, const char* exceptions);

		bool IsValidName(const char* name, stack_string& errorMessage);

		bool IsValidFilePath(const char* fileName, stack_string& errorMessage);

		bool IsValidFileName(const char* fileName, stack_string& errorMessage);
	}
}

#endif __SCHEMATYC_PLUGINUTILS_H__