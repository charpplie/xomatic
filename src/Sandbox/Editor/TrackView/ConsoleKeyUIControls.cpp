// ConsoleKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CConsoleKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CConsoleKeyUIControls)
public:
	CSmartVariableArray mv_table;
	CSmartVariable<CString> mv_command;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table, "Key Properties" );
		AddVariable( mv_table, mv_command, "Command" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if(type == ATRACK_CONSOLE)
			return true;
		else
			return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CConsoleKeyUIControls, CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CConsoleKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_CONSOLE )
		{
			IConsoleKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_command = (CString)key.command;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CConsoleKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_CONSOLE)
		{
			IConsoleKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if (pVar == mv_command.GetVar())
			{
				strncpy(key.command, (const char *)(CString)mv_command, sizeof(key.command)/sizeof(key.command[0]));
				key.command[sizeof(key.command)/sizeof(key.command[0]) - 1] = '\0';
			}

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}
	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CConsoleKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {3E9D2C57-BFB1-42f9-82AC-A393C1062634}
		static const GUID guid = 
		{ 0x3e9d2c57, 0xbfb1, 0x42f9, { 0x82, 0xac, 0xa3, 0x93, 0xc1, 0x6, 0x26, 0x34 } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Console"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CConsoleKeyUIControls); };
};

REGISTER_CLASS_DESC(CConsoleKeyUIControls_Class);