#pragma once

#include "ReplaySDK/DistributionMap.h"

class CDistributionMapWnd : public CStatic
{
public:
	struct CellInfo
	{
		CellInfo(){}
		CellInfo(u32 bytesUsed, u32 totalAge, u32 count)
			: bytesUsed(bytesUsed)
			, totalAge(totalAge)
			, count(count)
		{
		}

		u32 bytesUsed;
		u32 totalAge;
		u32 count;
	};

	enum
	{
		DMW_ACTIVEALLOCSCHANGED = WM_USER + 1,
	};

public:
	CDistributionMapWnd();

	void SetMemoryMap(const MemoryMapReplay* memMap);

	void SetFrameMarker(int frame);

protected:
	void OnSize(UINT nType, int cx, int cy);
	void OnPaint();
	void OnMouseMove(UINT nFlags, CPoint point);
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	static const double MaxRange;
	static const double MaxAxisRange;

private:
	void UpdateProjection();
	void UpdateScrollBar();
	void RaiseActiveAllocsChanged();

private:
	const MemoryMapReplay* m_memMap;

	int m_panX;
	int m_panY;

	int m_stops;
	int m_invScale;

	int m_frame;

private:
	DECLARE_MESSAGE_MAP()
};
