////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: ScreenFaderKeyUIControls.cpp
//  Version:   v1.00
//  Created:   28-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "Stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//-----------------------------------------------------------------------------
//!
class CScreenFaderKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CScreenFaderKeyUIControls)

	CSmartVariableArray		mv_table;
	CSmartVariable<float>	mv_fadeTime;
	CSmartVariable<Vec3>	mv_fadeColor;
	CSmartVariable<CString> mv_strTexture;
	CSmartVariable<bool>	mv_bUseCurColor;
	CSmartVariableEnum<int> mv_fadeType;
	CSmartVariableEnum<int> mv_fadechangeType;

public:

	//-----------------------------------------------------------------------------
	//!
	virtual bool SupportTrackType( EAnimTrackType type ) const
	{
		if(type == ATRACK_SCREENFADER)
			return true;
		else
			return false;
	}

	//-----------------------------------------------------------------------------
	//!
	virtual void OnCreateVars()
	{
		AddVariable( mv_table, "Key Properties" );

		mv_fadeType->SetEnumList(NULL);
		mv_fadeType->AddEnumItem("FadeIn", IScreenFaderKey::eFT_FadeIn);
		mv_fadeType->AddEnumItem("FadeOut", IScreenFaderKey::eFT_FadeOut);
		AddVariable( mv_table, mv_fadeType, "Type");

		mv_fadechangeType->SetEnumList(NULL);
		mv_fadechangeType->AddEnumItem("Linear", IScreenFaderKey::eFCT_Linear);
		mv_fadechangeType->AddEnumItem("Square", IScreenFaderKey::eFCT_Square);
		mv_fadechangeType->AddEnumItem("Cubic Square", IScreenFaderKey::eFCT_CubicSquare);
		mv_fadechangeType->AddEnumItem("Square Root", IScreenFaderKey::eFCT_SquareRoot);
		mv_fadechangeType->AddEnumItem("Sin", IScreenFaderKey::eFCT_Sin);
		AddVariable( mv_table, mv_fadechangeType, "ChangeType" );

		AddVariable( mv_table, mv_fadeColor, "Color", IVariable::DT_COLOR);

		mv_fadeTime->SetLimits(0.f, 100.f);
		AddVariable( mv_table, mv_fadeTime, "Duration");
		AddVariable( mv_table, mv_strTexture, "Texture",IVariable::DT_TEXTURE);
		AddVariable( mv_table, mv_bUseCurColor,"Use Current Color");
	}

	//-----------------------------------------------------------------------------
	//!
	virtual bool OnKeySelectionChange( SelectedKeys &keys );

	//-----------------------------------------------------------------------------
	//!
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &keys );
};

IMPLEMENT_DYNCREATE(CScreenFaderKeyUIControls, CTrackViewKeyUIControls)

//-----------------------------------------------------------------------------
bool CScreenFaderKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_SCREENFADER )
		{
			IScreenFaderKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_fadeTime = key.m_fadeTime;
			mv_fadeColor = Vec3(key.m_fadeColor.x,key.m_fadeColor.y,key.m_fadeColor.z);
			mv_strTexture = key.m_strTexture;
			mv_bUseCurColor = key.m_bUseCurColor;
			mv_fadeType = (int)key.m_fadeType;
			mv_fadechangeType = (int)key.m_fadeChangeType;

			bAssigned = true;
		}
	}
	return bAssigned;
}

//-----------------------------------------------------------------------------
void CScreenFaderKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (size_t keyIndex = 0,num = selectedKeys.keys.size(); keyIndex < num; ++keyIndex)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_SCREENFADER)
		{
			IScreenFaderKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			SyncValue( mv_fadeTime, key.m_fadeTime, false, pVar);

			SyncValue( mv_bUseCurColor, key.m_bUseCurColor, false, pVar);
			
			if(pVar == mv_fadeTime.GetVar())
			{
				key.m_fadeTime = MAX((float)mv_fadeTime, 0.f);

			}
			else if (pVar == mv_strTexture.GetVar())
			{
				CString sTexture = mv_strTexture;
				strcpy_s(key.m_strTexture, sTexture);
			}
			else if (pVar == mv_fadeType.GetVar())
			{
				key.m_fadeType = IScreenFaderKey::EFadeType((int)mv_fadeType);
			}
			else if( pVar == mv_fadechangeType.GetVar())
			{
				key.m_fadeChangeType = IScreenFaderKey::EFadeChangeType((int)mv_fadechangeType);
			}
			else if (pVar == mv_fadeColor.GetVar())
			{
				Vec3 color = mv_fadeColor;
				key.m_fadeColor = Vec4(color, key.m_fadeType == IScreenFaderKey::eFT_FadeIn ? 1.f : 0.f);
			}
			
			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}
	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//-----------------------------------------------------------------------------
//!
class CScreenFaderKeyUIControls_Class : public IClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {FBBC2407-C36B-45b2-9A54-0CF9CD3908FD}
		static const GUID guid = 
		{ 0xfbbc2407, 0xc36b, 0x45b2, { 0x9a, 0x54, 0xc, 0xf9, 0xcd, 0x39, 0x8, 0xfd } };
		return guid;
	}

	virtual const char* ClassName() { return "TrackView.KeyUI.ScreenFader"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CScreenFaderKeyUIControls); };
};
REGISTER_CLASS_DESC(CScreenFaderKeyUIControls_Class);