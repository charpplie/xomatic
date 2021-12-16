#include "stdafx.h"
#include "AnimationGraphPreviewManager_2.h"

CAnimationGraphPreviewManager2::CAnimationGraphPreviewManager2()
:	m_pViewport(0),
	m_enablePreview(false)
{
}

void CAnimationGraphPreviewManager2::SetViewport(CModelViewportCE* pViewport)
{
	m_pViewport = pViewport;
}

ICharacterInstance* CAnimationGraphPreviewManager2::GetCharacter() const
{
	return m_pViewport ? m_pViewport->GetCharacterBase() : NULL;
}

void CAnimationGraphPreviewManager2::SetState(CAGState2Ptr pState)
{
	// Store the state.
	m_pState = pState;
	if (m_enablePreview)
		StartPreview();
}

void CAnimationGraphPreviewManager2::EnablePreview(bool enablePreview)
{
	m_enablePreview = enablePreview;
	if (m_enablePreview)
		StartPreview();
	else
		StopPreview();
}

void CAnimationGraphPreviewManager2::SetParameter(const char* name, const char* value)
{
	CString lowercaseName(name);
	lowercaseName.MakeLower();
	m_mapLastParamValues[lowercaseName] = value;
	if (m_enablePreview)
		StartPreview();
}

void CAnimationGraphPreviewManager2::StartPreview()
{
	ICharacterInstance* pCharacter = GetCharacter();

	// Try to find an animation to play for the state.
	string animationName;
	if (m_pState)
	{
		string name = "";
		name = m_pState->GetAnimationName();
		ReplaceParameters(name);
		if (pCharacter && pCharacter->GetIAnimationSet()->GetAnimIDByName(name.c_str()) >= 0)
		{
			animationName = name;
		}
	}

	// Play the animation.
	if (pCharacter)
	{
		if (!animationName.empty())
		{
			m_pViewport->PlayAnimation(animationName.c_str());
		}
		else
		{
			pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();
			pCharacter->GetISkeletonPose()->SetDefaultPose();
		}
	}
}

void CAnimationGraphPreviewManager2::StopPreview()
{
	if (!GetCharacter())
		return;

	ISkeletonAnim* pSkeletonAnim = GetCharacter()->GetISkeletonAnim();
	ISkeletonPose* pSkeletonPose = GetCharacter()->GetISkeletonPose();
	pSkeletonAnim->StopAnimationsAllLayers();
	pSkeletonPose->SetDefaultPose();
}

void CAnimationGraphPreviewManager2::ReplaceParameters( string& name ) const
{
	CParamsDeclaration2* pDecl = m_pState->GetParamsDeclaration();
	if ( !pDecl )
		return;

	CParamsDeclaration2::iterator it, itEnd = pDecl->end();
	for ( it = pDecl->begin(); it != itEnd; ++it )
	{
		CString paramName(it->first);
		paramName.MakeLower();

		TParameterizationId2::const_iterator itFind = m_mapLastParamValues.find(paramName);
		if ( itFind == m_mapLastParamValues.end() )
			continue;
		CString lastValue( itFind->second );

		// make sure the last selected value is defined for this state too
		if ( it->second.find(lastValue) == it->second.end() )
			continue;

		paramName.Insert(0,'[');
		paramName += ']';
		int len = paramName.GetLength();

		int pos = name.find(paramName);
		if ( pos != string::npos )
		{
			string temp(lastValue);
			temp.MakeLower();
			name.replace( pos, len, temp );
		}

		paramName.SetAt( 1, toupper(paramName.GetAt(1)) );
		pos = name.find(paramName);
		if ( pos != string::npos )
		{
			name.replace( pos, len, lastValue );
		}

		paramName.MakeUpper();
		pos = name.find(paramName);
		if ( pos != string::npos )
		{
			lastValue.MakeUpper();
			name.replace( pos, len, lastValue );
		}
	}
}
