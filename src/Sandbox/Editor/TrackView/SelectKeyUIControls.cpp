// SelectKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "..\objects/CameraObject.h"

//////////////////////////////////////////////////////////////////////////
class CSelectyKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CSelectyKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariableEnum<CString> mv_camera;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_camera,"Camera" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_SELECT)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CSelectyKeyUIControls, CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CSelectyKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_SELECT)
		{
			mv_camera.SetEnumList(NULL);

			// Get All cameras.
			std::vector<CBaseObject*> objects;
			GetIEditor()->GetObjectManager()->GetObjects( objects );
			for (int i = 0; i < objects.size(); ++i)
			{
				if (objects[i]->IsKindOf(RUNTIME_CLASS(CCameraObject)))
					mv_camera->AddEnumItem( objects[i]->GetName(),objects[i]->GetName() );
			}

			ISelectKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_camera = (CString)key.szSelection;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CSelectyKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_SELECT)
		{
			ISelectKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if (pVar == mv_camera.GetVar())
			{
				strncpy( key.szSelection,(CString)mv_camera,sizeof(key.szSelection) );
				key.szSelection[sizeof(key.szSelection)-1] = '\0';
			}

			if (strlen(key.szSelection) > 0)
			{
				IAnimSequence *pSequence = GetIEditor()->GetSystem()->GetIMovieSystem()->FindSequence(key.szSelection);
				if (pSequence)
				{
					key.fDuration = pSequence->GetTimeRange().Length();
				}
			}

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CSelectyKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {9018D0D1-24CC-45e5-9D3D-16D3F9E591B2}
		static const GUID guid = 
		{ 0x9018d0d1, 0x24cc, 0x45e5, { 0x9d, 0x3d, 0x16, 0xd3, 0xf9, 0xe5, 0x91, 0xb2 } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Select"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CSelectyKeyUIControls); };
};

REGISTER_CLASS_DESC(CSelectyKeyUIControls_Class);