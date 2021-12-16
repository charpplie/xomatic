// CaptureKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CCaptureKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CCaptureKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariable<float> mv_duration;
	CSmartVariable<float> mv_timeStep;
	CSmartVariableEnum<CString> mv_format;
	CSmartVariable<CString> mv_folder;
	CSmartVariable<bool> mv_once;

	virtual void OnCreateVars()
	{
		mv_duration.GetVar()->SetLimits(0, 100000.0f);
		mv_timeStep.GetVar()->SetLimits(0.001f, 1.0f);
		mv_format.SetEnumList(NULL);
		mv_format->AddEnumItem("jpg", "jpg");
		mv_format->AddEnumItem("bmp", "bmp");
		mv_format->AddEnumItem("tga", "tga");
		mv_format->AddEnumItem("hdr", "hdr");

		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_duration,"Duration" );
		AddVariable( mv_table,mv_timeStep,"Time Step" );
		AddVariable( mv_table,mv_format,"Output Format" );
		AddVariable( mv_table,mv_folder,"Output Folder" );
		AddVariable( mv_table,mv_once,"Just one frame?" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_CAPTURE)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CCaptureKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CCaptureKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_CAPTURE)
		{
			ICaptureKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_duration = key.duration;
			mv_timeStep = key.timeStep;
			mv_format = key.GetFormat();
			mv_folder = key.folder;
			mv_once = key.once;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CCaptureKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_CAPTURE)
		{
			ICaptureKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			SyncValue( mv_duration,key.duration,false,pVar );
			SyncValue( mv_timeStep,key.timeStep,false,pVar );
			if (pVar == mv_format.GetVar())
			{
				if(strcmp((CString)mv_format, "jpg") == 0)
					key.FormatJPG();
				else if(strcmp((CString)mv_format, "bmp") == 0)
					key.FormatBMP();
				else if(strcmp((CString)mv_format, "hdr") == 0)
					key.FormatHDR();
				else
					key.FormatTGA();
			}
			if (pVar == mv_folder.GetVar())
			{
				CString sFolder = mv_folder;
				strcpy_s(key.folder, sFolder);
			}
			SyncValue( mv_once,key.once,false,pVar );

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CCaptureKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {543197BF-5E43-4abc-8F07-B84078846E4C}
		static const GUID guid = 
		{ 0x543197bf, 0x5e43, 0x4abc, { 0x8f, 0x7, 0xb8, 0x40, 0x78, 0x84, 0x6e, 0x4c } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Capture"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CCaptureKeyUIControls); };
};

REGISTER_CLASS_DESC(CCaptureKeyUIControls_Class);