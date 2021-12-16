// 2DBezierKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class C2DBezierKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(C2DBezierKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariable<float> mv_value;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table, "Key Properties" );
		AddVariable( mv_table, mv_value, "Value" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if(type == ATRACK_BEZIER_FLOAT)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(C2DBezierKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool C2DBezierKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_BEZIER_FLOAT)
		{
			I2DBezierKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			SyncValue( mv_value,key.value.y,true );

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void C2DBezierKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_BEZIER_FLOAT )
		{
			I2DBezierKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			SyncValue( mv_value,key.value.y,false,pVar );

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}
	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class C2DBezierKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {DBD76F4B-8EFC-45b6-AFB8-56F171FA150A}
		static const GUID guid = 
		{ 0xdbd76f4b, 0x8efc, 0x45b6, { 0xaf, 0xb8, 0x56, 0xf1, 0x71, 0xfa, 0x15, 0xa } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.2DBezier"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(C2DBezierKeyUIControls); };
};

REGISTER_CLASS_DESC(C2DBezierKeyUIControls_Class);