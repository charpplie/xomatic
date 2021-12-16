#include "StdAfx.h"
#include "EventPresetButtonSet.h"
#include "EventPresetDlg.h"
#include "ModelViewportCE.h"
#include "CharacterEditor/CharacterEditor.h"

enum
{
	ID_EVENTPRESET_NEW = 1,
	ID_EVENTPRESET_EDIT,
	ID_EVENTPRESET_CLONE,
	ID_EVENTPRESET_DELETE,
	ID_EVENTPRESET_SORT
};

BEGIN_MESSAGE_MAP(CEventPresetButtonSet, CButtonSetCtrl)	
	ON_WM_RBUTTONDOWN()
	ON_WM_CREATE()
	ON_COMMAND( ID_EVENTPRESET_NEW, OnEventPresetNew )
	ON_COMMAND( ID_EVENTPRESET_EDIT, OnEventPresetEdit )
	ON_COMMAND( ID_EVENTPRESET_CLONE, OnEventPresetClone )
	ON_COMMAND( ID_EVENTPRESET_DELETE, OnEventPresetDelete )
	ON_COMMAND( ID_EVENTPRESET_SORT, OnEventPresetSort )

	ON_WM_SIZE()

END_MESSAGE_MAP()

void CEventPresetButtonSet::OnLButtonUp( UINT nFlags, CPoint point )
{
	if( m_pButtonListener )	
	{
		CPoint cursorPos;
		GetCursorPos(&cursorPos);
		ButtonItemPtr pButtonItem = HitTest(cursorPos);
		if( pButtonItem )
			m_pButtonListener->OnPresetButtonClicked( pButtonItem, GetAnimEventData(pButtonItem) );
	}
}

void CEventPresetButtonSet::OnRButtonDown( UINT nFlags, CPoint point )
{
	CPoint cursorPos;
	GetCursorPos(&cursorPos);
	ButtonItemPtr pButtonItem = HitTest(cursorPos);
	DisplayPopMenus(pButtonItem ? true : false);
}

int CEventPresetButtonSet::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	ReadEventPresetsToRegistry();
	return __super::OnCreate(lpCreateStruct);
}

void CEventPresetButtonSet::DisplayPopMenus( bool bHitButton )
{
	CMenu menu;
	menu.CreatePopupMenu();

	UINT disabledNew = bHitButton ? MF_DISABLED : 0;
	UINT disabledOthers = !bHitButton ? MF_DISABLED : 0;

	menu.AppendMenu( MF_STRING|disabledNew, ID_EVENTPRESET_NEW, "New Preset" );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING|disabledOthers, ID_EVENTPRESET_EDIT, "Edit Preset" );
	menu.AppendMenu( MF_STRING|disabledOthers, ID_EVENTPRESET_CLONE, "Clone Preset" );
	menu.AppendMenu( MF_STRING|disabledOthers, ID_EVENTPRESET_DELETE, "Delete Preset" );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING, ID_EVENTPRESET_SORT, "Sort" );

	GetCursorPos( &m_PopupMenuPos );
	menu.TrackPopupMenu( TPM_LEFTALIGN|TPM_LEFTBUTTON,m_PopupMenuPos.x,m_PopupMenuPos.y,this );
}

void CEventPresetButtonSet::OnEventPresetNew()
{
	ButtonItemPtr pButtonItem = AddButton(SButtonItem(NewButtonName("Preset"),Vec3(0.95f,0.95f,0.95f)));	
	CAnimEventData animEventData;
	ShowPresetEditUp(pButtonItem, &animEventData);
	m_AnimEventDataMap[pButtonItem] = animEventData;
}

bool CEventPresetButtonSet::DoEventPresetModal( SButtonItem* pButtonItem, CAnimEventData* pAnimEventData )
{	
	std::vector<string> bones;
	bones.push_back("<none>");
	if( m_pModelViewportCE )
		m_pModelViewportCE->GetJointNameList(bones);

	CEventPresetDlg dlg(pAnimEventData,pButtonItem,bones);
	return dlg.DoModal() == IDOK;
}

void CEventPresetButtonSet::OnEventPresetEdit()
{
	ButtonItemPtr pButtonItem = HitTest(m_PopupMenuPos);
	if( pButtonItem == NULL )
		return;

	CAnimEventData* pAnimEventData = GetAnimEventData(pButtonItem);	
	ShowPresetEditUp(pButtonItem, pAnimEventData);
}

void CEventPresetButtonSet::ShowPresetEditUp( ButtonItemPtr pButtonItem, CAnimEventData* pAnimEventData )
{
	assert( pButtonItem && pAnimEventData );

	if( !pButtonItem || !pAnimEventData )
		return;

	if( DoEventPresetModal(pButtonItem, pAnimEventData) )
		UpdateButtons();
}

CAnimEventData* CEventPresetButtonSet::GetAnimEventData( ButtonItemPtr pButtonItem )
{
	assert( m_AnimEventDataMap.find(pButtonItem) != m_AnimEventDataMap.end() );
	if( m_AnimEventDataMap.find(pButtonItem) == m_AnimEventDataMap.end() )
		return NULL;
	return &m_AnimEventDataMap[pButtonItem];
}

void CEventPresetButtonSet::OnEventPresetClone()
{
	ButtonItemPtr pButtonItem = HitTest(m_PopupMenuPos);
	if( pButtonItem == NULL )
		return;

	CAnimEventData* pAnimEventData = GetAnimEventData(pButtonItem);

	CAnimEventData clonedEventData(*pAnimEventData);
	SButtonItem clonedButtonItem(*pButtonItem);
	clonedButtonItem.m_Name += "-Clone";

	if( DoEventPresetModal(&clonedButtonItem,&clonedEventData) )
	{
		ButtonItemPtr pNewButton = AddButton(clonedButtonItem);
		m_AnimEventDataMap[pNewButton] = clonedEventData;
	}
}

void CEventPresetButtonSet::OnEventPresetDelete()
{
	ButtonItemPtr pButtonItem = HitTest(m_PopupMenuPos);
	if( pButtonItem == NULL )
		return;

	if( CryMessageBox( "Do you want to delete it?", "Delete", MB_YESNO ) == IDYES )
	{
		DeleteButton(pButtonItem);
		UpdateButtons();
	}
}

void CEventPresetButtonSet::OnEventPresetSort()
{
	SortAlphabetically();
	UpdateButtons();
}

void CEventPresetButtonSet::OnMouseEventFromButtons( EMouseEvent event, CButton* pButton, UINT nFlags, CPoint point )
{	
	if( event == eMouseRDown )
	{
		CPoint cursorPos;
		GetCursorPos(&cursorPos);
		ButtonItemPtr pButtonItem = HitTest(cursorPos);
		DisplayPopMenus(pButtonItem ? true : false);
	}

	__super::OnMouseEventFromButtons( event, pButton, nFlags, point );
}

void CEventPresetButtonSet::DeleteButton( const ButtonItemPtr& pButtonItem )
{
	if( m_AnimEventDataMap.find(pButtonItem) != m_AnimEventDataMap.end() )
		m_AnimEventDataMap.erase(pButtonItem);
	__super::DeleteButton(pButtonItem);
}

void CEventPresetButtonSet::RemoveAll()
{
	m_AnimEventDataMap.clear();
	__super::RemoveAll();	
}

static const CString kPrefaxAnimEvent = "@AnimEvent";

void CEventPresetButtonSet::SaveEventPresetsToRegistry( CXTRegistryManager& regMgr, const CString& strSection )
{
	CString entryStr;
	int nRealCount = 0;

	for( int i = 0, iListCount(GetButtonListCount()); i < iListCount; ++i )
	{
		ButtonItemPtr pButtonItem = GetButtonItem(i);
		if( !pButtonItem )
			continue;

		CAnimEventData* pAnimEventData = GetAnimEventData(GetButtonItem(i));
		if( !pAnimEventData )
			continue;

		++nRealCount;

		entryStr.Format("%s%d.PresetName", kPrefaxAnimEvent, i);
		regMgr.WriteProfileString(strSection, entryStr, pButtonItem->m_Name);

		entryStr.Format("%s%d.Color", kPrefaxAnimEvent, i);
		regMgr.WriteProfileBinary(strSection, entryStr, (LPBYTE)&(pButtonItem->m_Color), sizeof(Vec3));

		entryStr.Format("%s%d.Name", kPrefaxAnimEvent, i);
		regMgr.WriteProfileString(strSection, entryStr, pAnimEventData->GetName());

		entryStr.Format("%s%d.Parameter", kPrefaxAnimEvent, i);
		regMgr.WriteProfileString(strSection, entryStr, pAnimEventData->GetCustomParameter());

		entryStr.Format("%s%d.Bone", kPrefaxAnimEvent, i);
		regMgr.WriteProfileString(strSection, entryStr, pAnimEventData->GetBoneName());

		entryStr.Format("%s%d.Offset", kPrefaxAnimEvent, i);
		regMgr.WriteProfileBinary(strSection, entryStr, (LPBYTE)&(pAnimEventData->GetOffset()), sizeof(Vec3));

		entryStr.Format("%s%d.Dir", kPrefaxAnimEvent, i);
		regMgr.WriteProfileBinary(strSection, entryStr, (LPBYTE)&(pAnimEventData->GetDirection()), sizeof(Vec3));

		entryStr.Format("%s%d.Model", kPrefaxAnimEvent, i);
		regMgr.WriteProfileString(strSection, entryStr, pAnimEventData->GetModelName());
	}

	entryStr.Format("%s.Size",kPrefaxAnimEvent);
	regMgr.WriteProfileInt(strSection, entryStr, nRealCount);
}

void CEventPresetButtonSet::ReadEventPresetsToRegistry()
{
	CXTRegistryManager regMgr;	
	const CString& strSection = CCharacterEditor::strRegistrySection;

	RemoveAll();

	CString entryStr;
	int nCount = 0;

	entryStr.Format("%s.Size",kPrefaxAnimEvent);
	nCount = regMgr.GetProfileInt(strSection, entryStr, -1);
	if( nCount == -1 )
		return;

	for( int i = 0; i < nCount; ++i )
	{
		entryStr.Format("%s%d.PresetName", kPrefaxAnimEvent, i);
		CString sPresetName = regMgr.GetProfileString(strSection, entryStr, "");
		
		LPBYTE pBuffer = NULL;
		UINT nSize = 0;

		Vec3 color(0,0,0);
		entryStr.Format("%s%d.Color", kPrefaxAnimEvent, i);
		if( regMgr.GetProfileBinary(strSection, entryStr, &pBuffer, &nSize) && nSize == sizeof(Vec3) )
			color = *reinterpret_cast<Vec3*>(pBuffer);

		entryStr.Format("%s%d.Name", kPrefaxAnimEvent, i);
		CString sName = regMgr.GetProfileString(strSection, entryStr, "");

		entryStr.Format("%s%d.Parameter", kPrefaxAnimEvent, i);
		CString sParameter = regMgr.GetProfileString(strSection, entryStr, "");

		entryStr.Format("%s%d.Bone", kPrefaxAnimEvent, i);
		CString sBone = regMgr.GetProfileString(strSection, entryStr, "");

		Vec3 offset(0,0,0);
		entryStr.Format("%s%d.Offset", kPrefaxAnimEvent, i);
		if( regMgr.GetProfileBinary(strSection, entryStr, &pBuffer, &nSize) && nSize == sizeof(Vec3) )
			offset = *reinterpret_cast<Vec3*>(pBuffer);

		Vec3 dir(0,0,0);
		entryStr.Format("%s%d.Dir", kPrefaxAnimEvent, i);
		if( regMgr.GetProfileBinary(strSection, entryStr, &pBuffer, &nSize) && nSize == sizeof(Vec3) )
			dir = *reinterpret_cast<Vec3*>(pBuffer);

		entryStr.Format("%s%d.Model", kPrefaxAnimEvent, i);
		CString sModel = regMgr.GetProfileString(strSection, entryStr, "");

		ButtonItemPtr pButtonItem = AddButton(SButtonItem(sPresetName,color));
		CAnimEventData animEventData;
		animEventData.SetName(sName);
		animEventData.SetCustomParameter(sParameter);
		animEventData.SetBoneName(sBone);
		animEventData.SetOffset(offset);
		animEventData.SetDirection(dir);
		animEventData.SetModelName(sModel);
		m_AnimEventDataMap[pButtonItem] = animEventData;
	}

	UpdateButtons();
}

void CEventPresetButtonSet::SaveEventPresetsToFile( const CString& pathName )
{
	XmlNodeRef rootNode = GetISystem()->CreateXmlNode("AniEventPreset");

	for( int i = 0, iListCount(GetButtonListCount()); i < iListCount; ++i )
	{
		ButtonItemPtr pButtonItem = GetButtonItem(i);
		if( !pButtonItem )
			continue;

		CAnimEventData* pAnimEventData = GetAnimEventData(GetButtonItem(i));
		if( !pAnimEventData )
			continue;

		XmlNodeRef presetNode = rootNode->newChild("Preset");

		presetNode->setAttr("PresetName", pButtonItem->m_Name);
		presetNode->setAttr("Color", pButtonItem->m_Color);
		presetNode->setAttr("Name", pAnimEventData->GetName());
		presetNode->setAttr("Parameter", pAnimEventData->GetCustomParameter());
		presetNode->setAttr("Bone", pAnimEventData->GetBoneName());
		presetNode->setAttr("Offset", pAnimEventData->GetOffset());
		presetNode->setAttr("Dir", pAnimEventData->GetDirection());
		presetNode->setAttr("Model", pAnimEventData->GetModelName());
	}

	rootNode->saveToFile(pathName);
}

void CEventPresetButtonSet::LoadEventPresetsFromFile( const CString& pathName )
{
	XmlNodeRef rootNode = GetISystem()->LoadXmlFromFile(pathName);

	if( rootNode == NULL )
		return;

	RemoveAll();

	for( int i = 0, iChildCount(rootNode->getChildCount()); i < iChildCount; ++i )
	{
		XmlNodeRef presetNode = rootNode->getChild(i);

		const char* sPresetName = NULL;
		if( !presetNode->getAttr("PresetName", &sPresetName) )
			continue;

		Vec3 color;
		if( !presetNode->getAttr("Color", color) )
			continue;

		ButtonItemPtr pButtonItem = AddButton(SButtonItem(sPresetName,color));
		CAnimEventData animEventData;

		const char* name = NULL;
		if( presetNode->getAttr("Name", &name) )
			animEventData.SetName(name);

		const char* parameter = NULL;
		if( presetNode->getAttr("Parameter", &parameter) )
			animEventData.SetCustomParameter(parameter);

		const char* bonename = NULL;
		if( presetNode->getAttr("Bone", &bonename) )
			animEventData.SetBoneName(bonename);

		Vec3 offset;
		if( presetNode->getAttr("Offset", offset) )
			animEventData.SetOffset(offset);

		Vec3 dir;
		if( presetNode->getAttr("Dir", dir) )
			animEventData.SetDirection(dir);

		const char* modelname = NULL;
		if( presetNode->getAttr("Model", &modelname) )
			animEventData.SetModelName(modelname);

		m_AnimEventDataMap[pButtonItem] = animEventData;
	}

	UpdateButtons();
}



void CEventPresetButtonSet::OnSize(UINT nType, int cx, int cy)
{
	CButtonSetCtrl::OnSize(nType, cx, cy);

	UpdateButtons();
}

