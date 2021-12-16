#include "StdAfx.h"
#include "FlashHelper.h"

void SFlashHelper::updateViewPort(IFlashPlayer* pFlashPlayer, bool bScale, EFlashPositionType px, EFlashPositionType py)
{
	assert(pFlashPlayer);
	if (!pFlashPlayer) return;

	int x, y, dx, dy;
	gEnv->pRenderer->GetViewport(&x, &y, &dx, &dy);


	int shiftX, shiftY, width, height;
	if (bScale)
	{
		float screenAspectRatio = (float) dx / dy;
		float hudAspectRatio = (float) pFlashPlayer->GetWidth() / pFlashPlayer->GetHeight();
		width = screenAspectRatio > hudAspectRatio ? (int) (dy * hudAspectRatio) :  dx;
		height = screenAspectRatio < hudAspectRatio ? (int) (dx / hudAspectRatio) :  dy;;

		switch (px)
		{
		default:
		case eFPT_Lower: shiftX = 0; break;
		case eFPT_Mid: shiftX = screenAspectRatio > hudAspectRatio ? (int) ((dx - dy * hudAspectRatio) / 2.f) : 0; break;
		case eFPT_Upper: shiftX = screenAspectRatio > hudAspectRatio ? (int) (dx - dy * hudAspectRatio) : 0; break;
		}
		switch (py)
		{
		default:
		case eFPT_Upper: shiftY = 0; break;
		case eFPT_Mid: shiftY = screenAspectRatio < hudAspectRatio ? (int) ((dy - dx / hudAspectRatio) / 2.f) : 0; break;
		case eFPT_Lower: shiftY = screenAspectRatio < hudAspectRatio ? (int) (dy - dx / hudAspectRatio) : 0; break;
		}

	}
	else
	{
		width = pFlashPlayer->GetWidth();
		height = pFlashPlayer->GetHeight();

		switch (px)
		{
		default:
		case eFPT_Lower: shiftX = 0; break;
		case eFPT_Mid: shiftX = (dx - width) / 2; break;
		case eFPT_Upper: shiftX =  dx - width; break;
		}
		switch (py)
		{
		default:
		case eFPT_Upper: shiftY = 0; break;
		case eFPT_Mid: shiftY = (dy - height) / 2; break;
		case eFPT_Lower: shiftY = dy - height; break;
		}

	}
	pFlashPlayer->SetViewport(shiftX, shiftY, width, height, 1);
}

std::vector<string> SFlashHelper::SliceArguments(const char* args)
{
	std::vector<string> result;
	string str = args;
	string::size_type loc = str.find( ",", 0);
	while(loc != string::npos)
	{
		result.push_back(str.substr(0, loc));
		str = str.substr(loc + 1);
		loc = str.find( ",", 0);
	}
	result.push_back(str);
	return result;
}