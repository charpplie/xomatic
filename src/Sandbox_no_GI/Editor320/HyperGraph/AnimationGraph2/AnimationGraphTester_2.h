#ifndef __ANIMATIONGRAPHTESTER2_H__
#define __ANIMATIONGRAPHTESTER2_H__

#pragma once

class CAnimationGraphDialog2;

class CAnimationGraphTester2 : public CXTPTaskPanel
{
public:
	void Init( CAnimationGraphDialog2 * pParent );
	void Reload();

	void OnCommand(int cmd);

private:
	CAnimationGraphDialog2 * m_pParent;
	CXTPTaskPanelGroup * m_pGroup;

	void AddVerb( CString name, int id );
};

#endif
