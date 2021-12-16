#include "StdAfx.h"

#include "UIHitIndicators.h"

#include "Graphics/2DRenderUtils.h"

#include "HUD/HUD.h"
#include "Game.h"
#include "PerkDbgDisplay.h"
#include "IWorldQuery.h"

//-----------------------------------------

const float UIHitIndicators::kTimeForIndicatorsToRemainOnScreen = 1.75f;

//-----------------------------------------

UIHitIndicators::UIHitIndicators()
 : m_numUsedHitIndicators(0)
 , THitIndicatorsParent()
{
	CHUD::AddHUDEventListener(this,"OnShowHitIndicator");
}

UIHitIndicators::~UIHitIndicators()
{
	CHUD::RemoveHUDEventListener(this);
}

void UIHitIndicators::AddHitIndicator(float angle, float damage)
{
	int numHitIndicators = m_numUsedHitIndicators;
	int foundHitIndicator = -1;
	float diff = gf_PI * 0.02f;

	for(int i = 0; i < numHitIndicators; i++)
	{
		float newDiff = (m_hitIndicators[i].angle - angle);
		float wrappedDiff = (float)__fsel(newDiff - gf_PI, newDiff - gf_PI2, newDiff);
		wrappedDiff = (float)__fsel(newDiff + gf_PI, newDiff, newDiff - gf_PI2);

		float absNewDiff = fabsf(wrappedDiff);

		if(absNewDiff < diff )
		{
			foundHitIndicator = i;
			diff = absNewDiff;
		}
	}

	//CryLogAlways ("Adding hit indicator (angle=%f damage=%f) got %d active already, reusing number %d", angle, damage, numHitIndicators, foundHitIndicator);

	if(foundHitIndicator >= 0)
	{
		m_hitIndicators[foundHitIndicator].timeAdded = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		m_hitIndicators[foundHitIndicator].depth = (m_hitIndicators[foundHitIndicator].depth * 0.75f) + damage;
		m_hitIndicators[foundHitIndicator].angle = angle;
	}
	else if(m_numUsedHitIndicators < kMaxNumHitIndicators)
	{
		m_hitIndicators[m_numUsedHitIndicators].angle = angle;
		m_hitIndicators[m_numUsedHitIndicators].depth = damage;
		m_hitIndicators[m_numUsedHitIndicators].timeAdded = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		m_numUsedHitIndicators++;
	}
}

void UIHitIndicators::ClearHitIndicators()
{
	float currentTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();

	int numHitIndicators = m_numUsedHitIndicators;

	for(int i = 0; i < numHitIndicators;)
	{
		if(currentTime - m_hitIndicators[i].timeAdded > kTimeForIndicatorsToRemainOnScreen)
		{
			m_hitIndicators[i] = m_hitIndicators[numHitIndicators - 1];
			--numHitIndicators;
		}
		else
		{
			++i;
		}
	}

	m_numUsedHitIndicators = numHitIndicators;
	HudDbgDisplay("numHitIndicators: %d", numHitIndicators);
}

void UIHitIndicators::Update( float ftime ) // Draw_WarsHitIndicators
{
	if (m_numUsedHitIndicators > 0)
	{
		//Remove old hit indicators
		ClearHitIndicators();
	}

}

void UIHitIndicators::Draw() const // Draw_WarsHitIndicators
{
	if (m_numUsedHitIndicators > 0)
	{
		const float width  = g_pGame->GetHUD()->GetLayoutManager()->GetVirtualWidth();
		const float height = g_pGame->GetHUD()->GetLayoutManager()->GetVirtualHeight();

		const float kDamageIndicatorRadius = 0.07f;
		const float kDamageIndicatorWidth = 0.015f;
		const float kScaleIndicator = 0.025f;

		Vec2 lowerLeft(-kDamageIndicatorWidth * width, 0.0f);
		Vec2 lowerRight(kDamageIndicatorWidth * width, 0.0f);	
		Vec2 up(0.0f, 0.0f);

		int numHitIndicators = m_numUsedHitIndicators;

		Vec2 o(width * 0.5f, height * 0.5f);

		float currentTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();

		float playerRotationAngle = 0.0f;

		assert (g_pGame->GetIGameFramework()->GetClientActor());
		assert (g_pGame->GetIGameFramework()->GetClientActor()->GetGameObject());
		assert (g_pGame->GetIGameFramework()->GetClientActor()->GetGameObject()->GetWorldQuery());
		const Vec3 & eyeDir = g_pGame->GetIGameFramework()->GetClientActor()->GetGameObject()->GetWorldQuery()->GetDir();
		playerRotationAngle = cry_atan2f(eyeDir.x, eyeDir.y);

		C2DRenderUtils* renderUtils = g_pGame->GetHUD()->Get2DRenderUtils();

		for(int i = 0; i < numHitIndicators; ++i)
		{
			Matrix33 rotateToDamageDir;

			float angleDiff = m_hitIndicators[i].angle - playerRotationAngle;

			lowerLeft.y = kDamageIndicatorRadius * height;
			lowerRight.y = kDamageIndicatorRadius * height;	
			up.y = (kDamageIndicatorRadius + m_hitIndicators[i].depth) * height;

			rotateToDamageDir.SetRotationZ(angleDiff);

			Vec2 finalLowerLeft = rotateToDamageDir * lowerLeft;
			Vec2 finalLowerRight = rotateToDamageDir * lowerRight;
			Vec2 finalUp = rotateToDamageDir * up;

			float fractionOfIndicatorLifeSpan = ((currentTime - m_hitIndicators[i].timeAdded)/ kTimeForIndicatorsToRemainOnScreen);

			ColorF shapeColor(1.0, 0, 0, min((3.0f - (3.0f * fractionOfIndicatorLifeSpan)), 1.0f));
			renderUtils->DrawTriangle(o.x + finalLowerRight.x, o.y + finalLowerRight.y, o.x + finalLowerLeft.x, o.y + finalLowerLeft.y, o.x + finalUp.x, o.y + finalUp.y, shapeColor );
		}
	}
}

void UIHitIndicators::OnHUDEvent( const SHUDEvent& event )
{
	//if( event.type == eHUDEvent_OnShowHitIndicator )
	{
		Vec3 dir( event.GetData(0).GetFloat(), event.GetData(1).GetFloat(), event.GetData(2).GetFloat() );

		const float kDamageIndicatorDepth = 0.0125f;

		if(dir.IsZero())
		{
			AddHitIndicator(0.0f, kDamageIndicatorDepth * 0.75f);
			AddHitIndicator(gf_PI, kDamageIndicatorDepth * 0.75f);
			AddHitIndicator(gf_PI * 0.5f, kDamageIndicatorDepth * 0.75f);
			AddHitIndicator(-gf_PI * 0.5f, kDamageIndicatorDepth * 0.75f);
		}
		else
		{
			float rotZ = cry_atan2f(dir.x, dir.y);

			AddHitIndicator(rotZ, kDamageIndicatorDepth);
		}
		return;
	}
}
