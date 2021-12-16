// GotoKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CGotoKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CGotoKeyUIControls)
public:
	CSmartVariableArray mv_table;
	CSmartVariable<float> mv_command;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table, "Key Properties" );
		AddVariable( mv_table, mv_command, "Goto Time" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if(type == ATRACK_GOTO)
			return true;
		else
			return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CGotoKeyUIControls, CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CGotoKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_GOTO )
		{
			IDiscreteFloatKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_command = key.m_fValue;

			bAssigned = true;
		}
	}
	return bAssigned;
}
//////////////////////////////////////////////////////////////////////////
// Called when UI variable changes.
void CGotoKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_GOTO)
		{
			IDiscreteFloatKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			SyncValue( mv_command,key.m_fValue,false,pVar );
			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}
	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}
//////////////////////////////////////////////////////////////////////////
class CGotoKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {3E9D2C57-BFB1-42f9-82AC-A393C1062634}
		static const GUID guid = 
		{ 0x9b79c8b6, 0xe332, 0x4b9b, { 0xb2, 0x63, 0xef, 0x7e, 0x82, 0x7, 0xa4, 0x47 } };
		return guid;
	}

	virtual const char* ClassName() { return "TrackView.KeyUI.Goto"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CGotoKeyUIControls); };
};
//////////////////////////////////////////////////////////////////////////

REGISTER_CLASS_DESC(CGotoKeyUIControls_Class);