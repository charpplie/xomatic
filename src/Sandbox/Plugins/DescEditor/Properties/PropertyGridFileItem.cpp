#include "pch.h"
#include "PropertyGridFileItem.h"
#include "ClassProfile.h"

#include "IResourceSelectorHost.h"

using namespace CryGame;


IMPLEMENT_SERIAL(CPropertyGridFileItem, CPropertyGridStringItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridFileItem, FileDialog, eVType_String, "")

IMPLEMENT_SERIAL(CPropertyGridSoundItem, CPropertyGridStringItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridSoundItem, SoundBrowser, eVType_String, "")

CPropertyGridFileItem::CPropertyGridFileItem()
{
	m_nFlags = xtpGridItemHasExpandButton|xtpGridItemHasEdit;
}

CPropertyGridFileItem::CPropertyGridFileItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
	: CPropertyGridStringItem(pGrid, pParent, pObject, pProperty, isElement, name)
{
	m_nFlags = xtpGridItemHasExpandButton|xtpGridItemHasEdit;
}

void CPropertyGridFileItem::UpdateText()
{
	__super::UpdateText();

	Value v;
	GetPropertyValue(v);

	string path;
	path.Format("%s\\%s", PathUtil::GetGameFolder(), (string)v);
	bool exists = (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES);

	// Render red if invalid
	CXTPPropertyGridItemMetrics* pMetrics = GetValueMetrics();
	pMetrics->m_clrFore = (exists) ? RGB(192, 192, 192) : RGB(200, 0, 0);
}

void CPropertyGridFileItem::OnInplaceButtonDown(CXTPPropertyGridInplaceButton* /*pButton*/)
{
	Value v;
	GetPropertyValue(v);

	CString initialPath;
	initialPath.Format("%s/%s", PathUtil::GetGameFolder(), (string)v);
	initialPath = initialPath.Left(initialPath.ReverseFind('/'));

	CString relativePath;
	CString filter;
	if (CFileUtil::SelectSingleFile(EFILE_TYPE_ANY, relativePath, filter, initialPath) == true)
	{
		SetValueFromText((LPCSTR)relativePath);
	}
}

void CPropertyGridSoundItem::OnInplaceButtonDown(CXTPPropertyGridInplaceButton* /*pButton*/)
{
	Value v;
	GetPropertyValue(v);

	CString initialPath = (string)v;
	SResourceSelectorContext x;
	x.typeName = "AudioTrigger";

	dll_string newValue = GetIEditor()->GetResourceSelectorHost()->SelectResource(x, initialPath);
	if (strcmp(initialPath, newValue.c_str()) != 0)
	{
		SetValueFromText((LPCSTR)newValue.c_str());
	}
}