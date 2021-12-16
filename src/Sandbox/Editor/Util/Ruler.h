/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Ruler helper for Editor to determine distances
  
 -------------------------------------------------------------------------
  History:
  - 02:11:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __RULER_H__
#define __RULER_H__

#include "RulerPoint.h"
#include "RulerPathAgent.h"

//! The Ruler utility helps to determine distances between user-specified points
class CRuler
{
public:
	CRuler();
	~CRuler();

	//! Returns if ruler has queued paths in the path agent
	bool HasQueuedPaths() const;

	//! Activate the ruler
	void SetActive(bool bActive);
	bool IsActive() const { return m_bActive; }

	//! Update
	void Update();

	//! Mouse callback handling from viewport
	bool MouseCallback(CViewport *pView, EMouseEvent event, CPoint &point, int flags);

private:
	//! Mouse callback helpers
	void OnMouseMove(CViewport *pView, CPoint &point, int flags);
	void OnLButtonUp(CViewport *pView, CPoint &point, int flags);
	void OnMButtonUp(CViewport *pView, CPoint &point, int flags);
	void OnRButtonUp(CViewport *pView, CPoint &point, int flags);

	//! Returns world point based on mouse point
	void UpdateRulerPoint(CViewport *pView, const CPoint &point, CRulerPoint &rulerPoint, bool bRequestPath);
	
	//! Request a path using the path agent
	void RequestPath();

	bool IsObjectSelectMode(CViewport *pView) const;

private:
	bool m_bActive;
	GUID m_MouseOverObject;

	// Base point
	CRulerPoint m_startPoint;
	CRulerPoint m_endPoint;

	// Path agent(s)
	CRulerPathAgent* m_pPathAgents;
	int m_numPathAgents;
};

#endif //__RULER_H__
