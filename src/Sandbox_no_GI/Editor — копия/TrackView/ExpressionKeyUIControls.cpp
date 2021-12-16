// ExpressionKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CExpressionKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CExpressionKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariableEnum<CString> mv_name;
	CSmartVariable<float> mv_amp;
	CSmartVariable<float> mv_blendIn;
	CSmartVariable<float> mv_hold;
	CSmartVariable<float> mv_blendOut;

	virtual void OnCreateVars()
	{
		mv_amp.GetVar()->SetLimits(0, 10.0f);
		mv_blendIn.GetVar()->SetLimits(0, 10.0f);
		mv_hold.GetVar()->SetLimits(0, 10.0f);
		mv_blendOut.GetVar()->SetLimits(0, 10.0f);

		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_name,"MorphTarget" );
		AddVariable( mv_table,mv_amp,"Amp" );
		AddVariable( mv_table,mv_blendIn,"BlendIn" );
		AddVariable( mv_table,mv_hold,"Hold" );
		AddVariable( mv_table,mv_blendOut,"BlendOut" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_EXPRESSION)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
private:
	void AddMorphTargets( struct ICharacterInstance *pCharInst );
};
IMPLEMENT_DYNCREATE(CExpressionKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
void CExpressionKeyUIControls::AddMorphTargets( ICharacterInstance *pCharInst )
{

	//	ICryCharModel *pModel=pCharInst->GetModel();
	//	if (pModel)
	//	{
	IAnimationSet *pAnimSet=pCharInst->GetIAnimationSet();
	if (pAnimSet)
	{
		int nMorphTargets=pAnimSet->numMorphTargets();
		for (int i=0;i<nMorphTargets;i++)
		{
			const char *pszName=pAnimSet->GetNameMorphTarget(i);
			mv_name->AddEnumItem(pszName, pszName);
		}
	}
	//}

	// Add morph targets from character attachments.
	IAttachmentManager *pAttachments = pCharInst->GetIAttachmentManager();
	if (pAttachments)
	{
		for (int i = 0; i < pAttachments->GetAttachmentCount(); i++)
		{
			IAttachment *pAttachment = pAttachments->GetInterfaceByIndex(i);
			if (!pAttachment)
				continue;
			IAttachmentObject *pObj = pAttachment->GetIAttachmentObject();
			if (!pObj)
				continue;

			ICharacterInstance *pCharacter = pObj->GetICharacterInstance();
			if (!pCharacter)
				continue;

			AddMorphTargets( pCharacter );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CExpressionKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_EXPRESSION)
		{
			IExprKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_name.SetEnumList(NULL);
			IEntity *pEntity = selectedKeys.keys[0].pNode->GetEntity();
			if (pEntity)
			{
				ICharacterInstance *pCharInst=pEntity->GetCharacter(0);
				if (pCharInst)
				{
					AddMorphTargets( pCharInst );
				}
			}

			mv_name = key.pszName;
			mv_amp = key.fAmp;
			mv_blendIn = key.fBlendIn;
			mv_hold = key.fHold;
			mv_blendOut = key.fBlendOut;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CExpressionKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_EXPRESSION)
		{
			IExprKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if (pVar == mv_name.GetVar())
			{
				CString sName = mv_name;
				strncpy(key.pszName, sName, sizeof(key.pszName));
				key.pszName[sizeof(key.pszName)-1] = 0;
			}
			SyncValue( mv_amp,key.fAmp,false,pVar );
			SyncValue( mv_blendIn,key.fBlendIn,false,pVar );
			SyncValue( mv_hold,key.fHold,false,pVar );
			SyncValue( mv_blendOut,key.fBlendOut,false,pVar );

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CExpressionKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {3B5CD428-E6F2-4c19-9AB4-38A7ECC3AB20}
		static const GUID guid = 
		{ 0x3b5cd428, 0xe6f2, 0x4c19, { 0x9a, 0xb4, 0x38, 0xa7, 0xec, 0xc3, 0xab, 0x20 } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Expression"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CExpressionKeyUIControls); };
};

REGISTER_CLASS_DESC(CExpressionKeyUIControls_Class);