#ifndef AFX_MEASUREMENTSYSTEM_H
#define AFX_MEASUREMENTSYSTEM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MEASUREMENT_SYSTEM_WINDOW_NAME "Measurement System Tool"

#include "Objects/RoadObject.h"
#include "Dialogs/BaseFrameWnd.h"

class CRoadObjectEnhanced;
class CMeasurementSystem;

//////////////////////////////////////////////////////////////////////////
// Measurement System Dialog
class CMeasurementSystemDialog : public CDialog
{
public:
	CMeasurementSystemDialog(CWnd* pParent = NULL);
	~CMeasurementSystemDialog();
	enum { IDD = IDD_MEASUREMENTSYSTEM_DIALOG };
	
	afx_msg void UpdateObjectLength();
	afx_msg void Optimize();
	afx_msg void UpdateSelectedLength(const float &fNewValue);
	afx_msg void UpdateTotalLength(const float &fNewValue);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////
class CMeasurementSystemFrame : public CBaseFrameWnd
{
	DECLARE_DYNCREATE(CMeasurementSystemFrame)

public:
	CMeasurementSystemFrame();

	enum { IDD = IDD_MEASUREMENTSYSTEM_VIEW };

	afx_msg CMeasurementSystemDialog* GetDialog();

protected:
	virtual BOOL OnInitDialog();
	CMeasurementSystemDialog dlg;
};

//////////////////////////////////////////////////////////////////////////
class CMeasurementSystem
{
public:
	CMeasurementSystem();

	void SetStartPos(const int &iStartPosIndex){ m_startPosIndex = iStartPosIndex; SwitchMeasuringPoint(); };
	void SetEndPos(const int &iEndPosIndex) { m_endPosIndex = iEndPosIndex; SwitchMeasuringPoint(); };
	int GetStartPointIndex(){ return m_startPosIndex; };
	int GetEndPointIndex(){ return m_endPosIndex; };
	// Should "Start" point be set in this click?
	bool ShouldStartPointBecomeSelected(){ return m_firstMeasurePointClicked; };
	bool ProcessedLButtonClick(int endPoint);
	bool ProcessedDblButtonClick(int clickedSegmentPointIndex);
	void ResetSelectedDistance();

	float GetSelectionLength(){ return m_selectionLength; };
	void SetTotalDistance(const float &newSelectionLength ){ m_selectionLength=newSelectionLength; };


	void ShutdownMeasurementSystem();
	bool IsMeasurementToolActivated(){ return m_bMeasurementToolActivated; };
	void SetMeasurementToolActivation(bool bActivated) { m_bMeasurementToolActivated = bActivated; };

	// Changes points of distance selection
	void SwitchMeasuringPoint(){ m_firstMeasurePointClicked=!m_firstMeasurePointClicked; };

	CMeasurementSystemDialog* GetMeasurementDialog();

	static CMeasurementSystem& GetMeasurementSystem(){ static CMeasurementSystem oMeasurementSystem; return oMeasurementSystem; }
protected:
	int m_startPosIndex;
	int m_endPosIndex;
	bool m_firstMeasurePointClicked;
	float m_selectionLength;
	bool m_bMeasurementToolActivated;
};
//////////////////////////////////////////////////////////////////////////
class CRoadObjectEnhanced : public CRoadObject
{
public:
	void DrawMeasurementSystemInfo( DisplayContext &dc, COLORREF col );
	void DrawCenterLine(const int& startPosIndex, const int& endPosIndex, float& segmentDist, float& totalDist, DisplayContext& dc, bool bDrawMeasurementInfo);
	void DrawSegmentBox(DisplayContext &dc, const int &startPosIndex, const int &endPosIndex, COLORREF col);
	void GetSectorLength(const int &sectorIndex,const int &endIndex, float &sectorLength);
	void GetObjectLength(float &objectLength);
};
//////////////////////////////////////////////////////////////////////////

#endif // #if !defined(AFX_MEASUREMENTSYSTEM_H)