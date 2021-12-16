#ifndef __HUDUTILS_H__
#define __HUDUTILS_H__

#include "IViewSystem.h"


namespace CHUDUtils
{

	static void ClampToScreen(Vec3& inoutPos, const bool isInFront)
	{
		Vec3 center(50.0f,50.0f, 0.0f);
		Vec3 dir = center - inoutPos;

		if(!isInFront)
		{
			dir.Normalize();
			dir *= -72.0f;
		}

		Vec3 dirModify(dir);
		if(dirModify.x < 0.0f)
		{
			dirModify.x *= -1.0f;
		}
		if(dirModify.y < 0.0f)
		{
			dirModify.y *= -1.0f;
		}
		float modifier = 1.0f;
		if(dirModify.x > dirModify.y)
		{
			modifier = (50.0f * __fres(dirModify.x));
		}
		else
		{
			modifier = (50.0f * __fres(dirModify.y));
		}

		modifier = min(modifier, 1.0f);
		dir *= modifier;

		inoutPos = center - dir;
	}


	static Vec3 GetClientPos()
	{
		IView *pView = gEnv->pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
		if(!pView)
			return Vec3(0,0,0);

		return pView->GetCurrentParams()->position;
	}



	static Vec3 GetClientDir()
	{
		IView *pView = gEnv->pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
		if(!pView)
			return FORWARD_DIRECTION;

		return pView->GetCurrentParams()->rotation.GetColumn1();
	}



	static bool IsInFront(const Vec3& worldPos)
	{
		Vec3 dir = GetClientDir().GetNormalized();
		Vec3 toTarget = (worldPos - GetClientPos()).GetNormalized();

		return (dir.Dot(toTarget) > 0.0f);
	}

};


#endif // __HUDDEFINES_H__