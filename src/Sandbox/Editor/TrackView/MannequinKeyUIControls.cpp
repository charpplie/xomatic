// CharacterKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "IAnimatedCharacter.h"
#include "ICryMannequin.h"

//////////////////////////////////////////////////////////////////////////
class CMannequinKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CMannequinKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariable<CString> mv_fragment;
	CSmartVariable<CString> mv_tags;
	CSmartVariable<int> mv_priority;
	CSmartVariable<bool> mv_trump;
	
	virtual void OnCreateVars()
	{
		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_fragment,"mannequin fragment");
		AddVariable( mv_table,mv_tags,"fragment tags");
		AddVariable( mv_table,mv_priority,"priority");		
	}
	bool SupportTrackType( const CAnimParamType &paramType, EAnimCurveType trackType, EAnimValue valueType ) const
	{
		return paramType == eAnimParamType_Mannequin;
	}
	virtual bool OnKeySelectionChange( CTrackViewKeyBundle &selectedKeys );
	virtual void OnUIChange( IVariable *pVar, CTrackViewKeyBundle &selectedKeys );

	virtual unsigned int GetPriority() const { return 1; }
};
IMPLEMENT_DYNCREATE(CMannequinKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CMannequinKeyUIControls::OnKeySelectionChange( CTrackViewKeyBundle &selectedKeys )
{
	if (!selectedKeys.AreAllKeysOfSameType())
		return false;

	bool bAssigned = false;
	if (selectedKeys.GetKeyCount() == 1)	
	{
		CTrackViewKeyHandle &keyHandle = selectedKeys.GetKey(0);

		CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
		if (paramType == eAnimParamType_Mannequin)
		{
			IMannequinKey key;
			keyHandle.GetKey(&key);

			mv_fragment = (CString)key.m_fragmentName;
			mv_tags = (CString)key.m_tags;
			mv_priority = key.m_priority;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CMannequinKeyUIControls::OnUIChange( IVariable *pVar, CTrackViewKeyBundle &selectedKeys )
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();

	if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
	{
		return;
	}

	for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
	{
		CTrackViewKeyHandle &keyHandle = selectedKeys.GetKey(keyIndex);

		CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
		if (paramType == eAnimParamType_Mannequin)
		{
			IMannequinKey key;
			keyHandle.GetKey(&key);

			if (mv_fragment.GetVar() == pVar)
			{
				strncpy( key.m_fragmentName,(CString)mv_fragment,sizeof(key.m_fragmentName) );
				key.m_fragmentName[sizeof(key.m_fragmentName)-1] = '\0';
			}

			if (strlen(key.m_fragmentName) > 0)
			{
				IEntity *entity = keyHandle.GetTrack()->GetAnimNode()->GetEntity();
				if (entity)
				{
					ICharacterInstance* pCharacter = entity->GetCharacter(0);
					if (pCharacter)
					{
						//find fragment duration

						IGameObject* pGameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(entity->GetId());
						if(pGameObject)
						{
							IAnimatedCharacter * pAnimChar = (IAnimatedCharacter*) pGameObject->QueryExtension("AnimatedCharacter");
							if(pAnimChar)
							{
								float fFragDuration = 0.0f;
								float fTransDuration = 0.0f;
								CString animation;
								mv_fragment->Get(animation);
								const uint32  valueName = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase( animation );

								const FragmentID fragID = pAnimChar->GetActionController()->GetContext().controllerDef.m_fragmentIDs.Find(valueName);

								bool isValid = !(FRAGMENT_ID_INVALID == fragID);

								IActionPtr pAction = new TAction<SAnimationContext>(3, fragID, TAG_STATE_EMPTY, 0u, 0xffffffff);
								pAnimChar->GetActionController()->QueryDuration(*pAction, fFragDuration, fTransDuration);
								key.m_duration = fFragDuration + fTransDuration;
								key.m_priority = mv_priority;
								strncpy( key.m_tags,(CString)mv_tags,sizeof(key.m_tags) );
							}
						}
					}
				}
			}
			
			keyHandle.SetKey(&key);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
class CMannequinKeyUIControls_Class : public IClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		static const GUID guid = { 0x1a7867f0, 0xf20a, 0x4f88, { 0xa5, 0xff, 0x80, 0xf0, 0x25, 0xd1, 0x6, 0x8f } };

		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Mannequin"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CMannequinKeyUIControls); };
};

REGISTER_CLASS_DESC(CMannequinKeyUIControls_Class);
