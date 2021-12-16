// FacialSeqKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include <IFacialAnimation.h>
#include "IFacialEditor.h"

//////////////////////////////////////////////////////////////////////////
class CFacialSeqKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CFacialSeqKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariableEnum<CString> mv_name;
	CSmartVariable<bool> mv_openInFacialEditor;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_name,"Facial Sequence" );
		AddVariable( mv_table,mv_openInFacialEditor,"Open in Facial Editor..." );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_FACESEQ)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
private:
	void OnOpenInFE(const SelectedKey& selectedKey);
};
IMPLEMENT_DYNCREATE(CFacialSeqKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CFacialSeqKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_FACESEQ)
		{
			IEntity *entity = selectedKeys.keys[0].pNode->GetEntity();
			if (entity)
			{
				// Add available animations.
				ICharacterInstance *pCharacter = entity->GetCharacter(0);
				if (pCharacter)
				{
					mv_name.SetEnumList(NULL);
					IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
					assert (pAnimations);

					uint32 numAnims		= pAnimations->GetNumFacialAnimations();
					for (uint32 i=0; i < numAnims; ++i)
						mv_name->AddEnumItem(pAnimations->GetFacialAnimationName(i),
						pAnimations->GetFacialAnimationName(i));
				}
			}

			IFaceSeqKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_name = key.szSelection;
			mv_openInFacialEditor = false;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CFacialSeqKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	if(mv_openInFacialEditor == true)
	{
		mv_openInFacialEditor = false;
		OnOpenInFE(selectedKeys.keys[0]);
		return;
	}

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_FACESEQ)
		{
			IFaceSeqKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			CString sName;
			sName = mv_name;

			if (pVar == mv_name.GetVar())
			{
				strncpy( key.szSelection,sName,sizeof(key.szSelection) );
				key.szSelection[sizeof(key.szSelection)-1] = '\0';
			}

			IEntity *pEntity = selectedKey.pNode->GetEntity();
			ICharacterInstance* pCharacter = (pEntity ? pEntity->GetCharacter(0) : 0);
			IFacialInstance* pFacialInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
			IFacialAnimSequence* pSequence = (pFacialInstance && strlen(key.szSelection) > 0 ? 
																					pFacialInstance->LoadSequence(key.szSelection) : 0);
			if (pSequence)
				key.fDuration = pSequence->GetTimeRange().Length();

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

void CFacialSeqKeyUIControls::OnOpenInFE(const SelectedKey& selectedKey)
{
	// Open the facial editor if it is not already open.
	IEditor* pEditor = GetIEditor();
	IFacialEditor* pFacialEditor = (pEditor ? pEditor->GetFacialEditor(true) : 0);
	if (!pFacialEditor)
		CryMessageBox("Unable to open facial editor.", "Facial Editor Error", MB_ICONERROR | MB_OK);

	// Find the character from the node.
	string characterPath;
	string sequencePath;
	{
		IEntity* pEntity = (selectedKey.pNode ? selectedKey.pNode->GetEntity() : 0);
		ICharacterInstance* pCharacter = (pEntity ? pEntity->GetCharacter(0) : 0);
		{
			const char* characterPathBuffer = (pCharacter ? pCharacter->GetFilePath() : 0);
			characterPath = (characterPathBuffer ? characterPathBuffer : "");
		}

		// Find the sequence from the key.
		{
			IFaceSeqKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );
			const char* sequenceName = key.szSelection;
			IAnimationSet* pAnimationSet = (pCharacter ? pCharacter->GetIAnimationSet() : 0);
			const char* sequencePathBuffer = (pAnimationSet && sequenceName ? 
																						pAnimationSet->GetFacialAnimationPathByName(sequenceName) : 0);
			sequencePath = (sequencePathBuffer ? sequencePathBuffer : "");
		}
	}

	bool foundCharacter = !characterPath.empty();
	bool foundSequence = !sequencePath.empty();

	if (!foundSequence)
	{
		string message = "Unable to find sequence path";
		if (!foundCharacter)
			message += " - character path could not be found";
		message += ".";

		CryMessageBox(message.c_str(), "Facial Editor Error", MB_ICONERROR | MB_OK);
	}

	if (pFacialEditor && !characterPath.empty())
		pFacialEditor->LoadCharacter(characterPath.c_str());
	if (pFacialEditor && !sequencePath.empty())
		pFacialEditor->LoadSequence(sequencePath.c_str());
}

//////////////////////////////////////////////////////////////////////////
class CFacialSeqKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {C4CA0898-E4C6-430d-89D7-54FBE1AA1489}
		static const GUID guid = 
		{ 0xc4ca0898, 0xe4c6, 0x430d, { 0x89, 0xd7, 0x54, 0xfb, 0xe1, 0xaa, 0x14, 0x89 } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.FacialSequence"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CFacialSeqKeyUIControls); };
};

REGISTER_CLASS_DESC(CFacialSeqKeyUIControls_Class);