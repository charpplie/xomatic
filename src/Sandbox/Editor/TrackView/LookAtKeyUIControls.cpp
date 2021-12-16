// LookAtKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "..\objects/entity.h"

//////////////////////////////////////////////////////////////////////////
class CLookAtKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CLookAtKeyUIControls)
public:
	CLookAtKeyUIControls()
	{
		m_boneSetNamesMap[eLookAtKeyBoneSet_Eyes] = "Eyes";
		m_boneSetNamesMapReverse["Eyes"] = eLookAtKeyBoneSet_Eyes;
		m_boneSetNamesMap[eLookAtKeyBoneSet_HeadEyes] = "Head and eyes";
		m_boneSetNamesMapReverse["Head and eyes"] = eLookAtKeyBoneSet_HeadEyes;
		m_boneSetNamesMap[eLookAtKeyBoneSet_SpineHeadEyes] = "Spine, head and eyes";
		m_boneSetNamesMapReverse["Spine, head and eyes"] = eLookAtKeyBoneSet_SpineHeadEyes;
	}

	CSmartVariableArray mv_table;

	CSmartVariableEnum<CString> mv_name;
	CSmartVariableEnum<CString> mv_boneSet;
	CSmartVariable<bool> mv_allowAdditionalTransforms;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_name,"Entity" );
		AddVariable( mv_table,mv_allowAdditionalTransforms,"Allow Additional Eye Animation" );
		AddVariable( mv_table,mv_boneSet,"Manipulated Bones" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_LOOKAT)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
private:
	std::map<ELookAtKeyBoneSet, CString> m_boneSetNamesMap;
	std::map<CString, ELookAtKeyBoneSet> m_boneSetNamesMapReverse;
};
IMPLEMENT_DYNCREATE(CLookAtKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CLookAtKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_LOOKAT)
		{
			// Add the options for the boneSets to the combo box.
			mv_boneSet.SetEnumList(NULL);
			for (int i = 0; i < eLookAtKeyBoneSet_COUNT; ++i)
				mv_boneSet->AddEnumItem(m_boneSetNamesMap[ELookAtKeyBoneSet(i)],
				m_boneSetNamesMap[ELookAtKeyBoneSet(i)]);

			mv_name.SetEnumList(NULL);
			std::vector<CBaseObject*> objects;
			// Get All entity nodes
			GetIEditor()->GetObjectManager()->GetObjects( objects );
			for (int i = 0; i < objects.size(); ++i)
			{
				if (objects[i]->IsKindOf(RUNTIME_CLASS(CEntity)))
					mv_name->AddEnumItem( objects[i]->GetName(), objects[i]->GetName() );
			}

			ILookAtKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_name = key.szSelection;
			mv_allowAdditionalTransforms = key.bAllowAdditionalTransforms;
			mv_boneSet = m_boneSetNamesMap[key.boneSet];

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CLookAtKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_LOOKAT)
		{
			ILookAtKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if (pVar == mv_name.GetVar())
			{
				CString sName;
				sName = mv_name;
				strncpy( key.szSelection,sName,sizeof(key.szSelection) );
				key.szSelection[sizeof(key.szSelection)-1] = '\0';
			}

			if (pVar == mv_boneSet.GetVar())
			{
				CString sBoneSet;
				sBoneSet = mv_boneSet;
				key.boneSet = m_boneSetNamesMapReverse[sBoneSet];
			}

			SyncValue( mv_allowAdditionalTransforms,key.bAllowAdditionalTransforms,false,pVar );

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CLookAtKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {9BE53EAD-D36C-4d8d-A6E4-8B31D173B826}
		static const GUID guid = 
		{ 0x9be53ead, 0xd36c, 0x4d8d, { 0xa6, 0xe4, 0x8b, 0x31, 0xd1, 0x73, 0xb8, 0x26 } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.LookAt"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CLookAtKeyUIControls); };
};

REGISTER_CLASS_DESC(CLookAtKeyUIControls_Class);