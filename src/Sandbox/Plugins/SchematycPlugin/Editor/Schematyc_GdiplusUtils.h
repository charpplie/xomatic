/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc Gdiplus utilities.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_GDIPLUSUTILS_H__
#define __SCHEMATYC_GDIPLUSUTILS_H__

// TODO : Move functions to .cpp file?

namespace Schematyc
{
	namespace GdiplusUtils
	{
		inline void CreateBeveledRectPath(const Gdiplus::RectF& rect, const float (&bevels)[4], Gdiplus::GraphicsPath& path)
		{
			if(bevels[0] > 0.0f)
			{
				path.AddArc(Gdiplus::RectF(rect.X, rect.Y, bevels[0], bevels[0]), 180.0f, 90.0f);
			}
			else
			{
				path.AddLine(Gdiplus::PointF(rect.X, rect.Y), Gdiplus::PointF(rect.X + rect.Width - bevels[1], rect.Y));
			}

			if(bevels[1] > 0.0f)
			{
				path.AddArc(Gdiplus::RectF(rect.X + rect.Width - bevels[1], rect.Y, bevels[1], bevels[1]), 270.0f, 90.0f);
			}
			else
			{
				path.AddLine(Gdiplus::PointF(rect.X + rect.Width, rect.Y), Gdiplus::PointF(rect.X + rect.Width, rect.Y + rect.Height - bevels[2]));
			}

			if(bevels[2] > 0.0f)
			{
				path.AddArc(Gdiplus::RectF(rect.X + rect.Width - bevels[2], rect.Y + rect.Height - bevels[2], bevels[2], bevels[2]), 0.0f, 90.0f);
			}
			else
			{
				path.AddLine(Gdiplus::PointF(rect.X + rect.Width, rect.Y + rect.Height), Gdiplus::PointF(rect.X + bevels[3], rect.Y + rect.Height));
			}

			if(bevels[3] > 0.0f)
			{
				path.AddArc(Gdiplus::RectF(rect.X, rect.Y + rect.Height - bevels[3], bevels[3], bevels[3]), 90.0f, 90.0f);
			}
			else
			{
				path.AddLine(Gdiplus::PointF(rect.X, rect.Y + rect.Height), Gdiplus::PointF(rect.X, rect.Y + bevels[0]));
			}
			path.CloseFigure();
		}

		inline void PaintBeveledRect(Gdiplus::Graphics& graphics, const Gdiplus::RectF& rect, const float (&bevels)[4], const Gdiplus::Pen& pen)
		{
			Gdiplus::GraphicsPath	path;
			CreateBeveledRectPath(rect, bevels, path);
			graphics.DrawPath(&pen, &path);
		}

		inline void FillBeveledRect(Gdiplus::Graphics& graphics, const Gdiplus::RectF& rect, const float (&bevels)[4], const Gdiplus::Brush& brush)
		{
			Gdiplus::GraphicsPath	path;
			CreateBeveledRectPath(rect, bevels, path);
			graphics.FillPath(&brush, &path);
		}

		inline Gdiplus::Color LerpColor(Gdiplus::Color from, Gdiplus::Color to, float lerp)
		{
			const float	fromAlpha = from.GetAlpha();
			const float	fromRed = from.GetRed();
			const float	fromGreed = from.GetGreen();
			const float	fromBlue = from.GetBlue();

			const float	toAlpha = to.GetAlpha();
			const float	toRed = to.GetRed();
			const float	toGreed = to.GetGreen();
			const float	toBlue = to.GetBlue();

			lerp = std::min(lerp, 1.0f);
			const float	invLerp = 1.0f - lerp;

			const float	lerpAlpha = (fromAlpha * invLerp) + (toAlpha * lerp);
			const float	lerpRed = (fromRed * invLerp) + (toRed * lerp);
			const float	lerpGreed = (fromGreed * invLerp) + (toGreed * lerp);
			const float	lerpBlue = (fromBlue * invLerp) + (toBlue * lerp);

			return Gdiplus::Color(static_cast<BYTE>(lerpAlpha), static_cast<BYTE>(lerpRed), static_cast<BYTE>(lerpGreed), static_cast<BYTE>(lerpBlue));
		}
	}
}

#endif //__SCHEMATYC_GDIPLUSUTILS_H__