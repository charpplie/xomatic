#pragma once

#include "ReplaySDK/SizeInfo.h"

class IMemoryGraphStreamSource
{
public:
	struct StreamInfo
	{
		StreamInfo()
			: colour(0)
			, name("")
		{
		}

		StreamInfo(u32 colour, const std::string& name)
			: colour(colour)
			, name(name)
		{
		}
		
		u32 colour;
		std::string name;
	};

public:
	virtual ~IMemoryGraphStreamSource() {}

	virtual int GetStreamCount() const = 0;
	virtual StreamInfo GetStreamInfo(int index) const = 0;

	virtual u64 GetSamplePointCount() const = 0;
	virtual void SampleAt(int* out, s64 t, int mip) const = 0;
	virtual int SampleAt(int stream, s64 t, int mip) const = 0;
};

class IMemoryGraphMarkerSource
{
public:
	struct MarkerInfo
	{
		double offset;
		u32 colour;
		const char* name;
	};

public:
	virtual ~IMemoryGraphMarkerSource() {}

	virtual size_t GetMarkerCount() const = 0;
	virtual MarkerInfo GetMarkerInfo(size_t idx) const = 0;
};

class CMemoryGraphWnd : public CStatic
{
public:
	enum
	{
		MGW_RANGECHANGED = WM_USER + 1,
		MGW_RANGECHANGING,
		MGW_HOVERCHANGED,
	};

	struct HoverChangedHeader
	{
		NMHDR hdr;
		double cursorX;
		double cursorY;
	};

public:
	CMemoryGraphWnd();

	void SetStreamSource(const IMemoryGraphStreamSource* source);
	const IMemoryGraphStreamSource* GetStreamSource() const { return m_source; }

	void SetMarkerSource(const IMemoryGraphMarkerSource* source);
	const IMemoryGraphMarkerSource* GetMarkerSource() const { return m_markerSource; }

	double GetHoverCursorPosition() const { return m_cursor; }
	double GetStartCursorPosition() const { return m_startCursor; }
	double GetEndCursorPosition() const { return m_endCursor; }

	void SetHoverCursorPosition(double c);

	void ClearSelection();
	bool HasSelection() const { return abs(m_endCursor - m_startCursor) > 0.001; }

protected:
	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnSize(UINT nType, int cx, int cy);
	BOOL OnEraseBkgnd(CDC* pDC);
	void OnPaint();
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnLButtonUp(UINT nFlags, CPoint point);
	void OnRButtonDown(UINT nFlags, CPoint point);
	void OnRButtonUp(UINT nFlags, CPoint point);
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	enum
	{
		TickFontHeight = 10,
		LabelFontHeight = 12,
	};

private:
	int ClientXFromSampleT(double t);
	double SampleTFromClientX(int t);

	int ClientYFromSampleV(double v);
	double SampleVFromClientY(int y);

	void ResetCursor(double& cursorPos, double val, bool invalidateRange = true);

	void PaintCursor(CDC& dc, CBrush& brush, float cursorPos);

	void UpdateScrollRange();

	void RaiseRangeChangedMessage();
	void RaiseRangeChangingMessage();
	void RaiseHoverChangedMessage(double cursorX, double cursorY);

private:
	const IMemoryGraphStreamSource* m_source;
	const IMemoryGraphMarkerSource* m_markerSource;

	double m_cursor;
	double m_startCursor;
	double m_endCursor;
	double* m_draggedCursor;

	float m_scaleX;
	float m_scaleY;

	CPoint m_rmbDragStart;
	CPoint m_rmbDragOrigin;

	CRect m_rcGraphClient;

	CBitmap m_bbBitmap;
	CFont m_tickFont;
	CFont m_labelFont;

private:
	DECLARE_MESSAGE_MAP()
};
