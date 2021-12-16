// TcbKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CTcbKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CTcbKeyUIControls)
public:
	CSmartVariableArray mv_tableTCB;
	CSmartVariableArray mv_tableValue;
	CSmartVariable<float> mv_easeIn;
	CSmartVariable<float> mv_easeOut;
	CSmartVariable<float> mv_tension;
	CSmartVariable<float> mv_cont;
	CSmartVariable<float> mv_bias;

	CSmartVariable<float> mv_x;
	CSmartVariable<float> mv_y;
	CSmartVariable<float> mv_z;
	CSmartVariable<float> mv_w;

	virtual void OnCreateVars()
	{
		mv_easeIn->SetLimits(-1.0f,1.0f);
		mv_easeOut->SetLimits(-1.0f,1.0f);
		mv_tension->SetLimits(-1.0f,1.0f);
		mv_cont->SetLimits(-1.0f,1.0f);
		mv_bias->SetLimits(-1.0f,1.0f);

		AddVariable( mv_tableValue,"Key Value" );
		AddVariable( mv_tableValue,mv_x,"X" );
		AddVariable( mv_tableValue,mv_y,"Y" );
		AddVariable( mv_tableValue,mv_z,"Z" );

		AddVariable( mv_tableTCB,"TCB Parameters" );
		AddVariable( mv_tableTCB,mv_easeIn,"Ease In" );
		AddVariable( mv_tableTCB,mv_easeOut,"Ease Out" );
		AddVariable( mv_tableTCB,mv_tension,"Tension" );
		AddVariable( mv_tableTCB,mv_cont,"Continuity" );
		AddVariable( mv_tableTCB,mv_bias,"Bias" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		switch (type)
		{
		case ATRACK_TCB_FLOAT:
		case ATRACK_TCB_VECTOR:
		case ATRACK_TCB_QUAT:
			return true;
		}
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CTcbKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CTcbKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		//EAnimValue valueType = m_track->GetValueType();
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_TCB_FLOAT || trType == ATRACK_TCB_VECTOR || trType == ATRACK_TCB_QUAT )
		{
			ITcbKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			SyncValue( mv_bias,key.bias,true );
			SyncValue( mv_cont,key.cont,true );
			SyncValue( mv_tension,key.tens,true );
			SyncValue( mv_easeIn,key.easeto,true );
			SyncValue( mv_easeOut,key.easefrom,true );

			if (trType == ATRACK_TCB_QUAT)
			{
				Quat keyQuat = key.GetQuat();
				Ang3 angles = RAD2DEG( Ang3::GetAnglesXYZ(Matrix33(keyQuat)) );
				SyncValue( mv_x,angles.x,true );
				SyncValue( mv_y,angles.y,true );
				SyncValue( mv_z,angles.z,true );
			}
			else
			{
				SyncValue( mv_x,key.fval[0],true );
				SyncValue( mv_y,key.fval[1],true );
				SyncValue( mv_z,key.fval[2],true );
			}

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CTcbKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_TCB_FLOAT || trType == ATRACK_TCB_VECTOR || trType == ATRACK_TCB_QUAT )
		{
			ITcbKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			SyncValue( mv_bias,key.bias,false,pVar );
			SyncValue( mv_cont,key.cont,false,pVar );
			SyncValue( mv_tension,key.tens,false,pVar );
			SyncValue( mv_easeIn,key.easeto,false,pVar );
			SyncValue( mv_easeOut,key.easefrom,false,pVar );

			if (trType == ATRACK_TCB_QUAT)
			{
				Ang3 angles;
				angles.x = mv_x;
				angles.y = mv_y;
				angles.z = mv_z;

				Quat keyQuat = Quat::CreateRotationXYZ( DEG2RAD(angles) );
				key.SetQuat(keyQuat);
			}
			else
			{
				SyncValue( mv_x,key.fval[0],false,pVar );
				SyncValue( mv_y,key.fval[1],false,pVar );
				SyncValue( mv_z,key.fval[2],false,pVar );
			}

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}
	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CTcbKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {B50715F3-9C93-41a9-BDD0-F4C83AF93CE8}
		static const GUID guid = { 0xb50715f3, 0x9c93, 0x41a9, { 0xbd, 0xd0, 0xf4, 0xc8, 0x3a, 0xf9, 0x3c, 0xe8 } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Tcb"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTcbKeyUIControls); };
};

REGISTER_CLASS_DESC(CTcbKeyUIControls_Class);