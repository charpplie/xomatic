#ifndef __ANIMATIONGRAPHUNITTESTER2_H__
#define __ANIMATIONGRAPHUNITTESTER2_H__

#pragma once

#include "AnimationGraph_2.h"

class CAnimationGraphUnitTester2
{
public:
	
	bool Load( CAnimationGraph2Ptr pGraph, const CString &sGraphFileName, const CString &sGraphWorkingName, CString *sError=NULL );
	bool RunAllTests( std::vector<CString> *sErrors=NULL, int *pNumTestsRun=NULL, int *pNumTestsPassed=NULL );
	bool RunTest( const CString &sTestName, std::vector<CString> *sErrors=NULL );
	bool RunTest( XmlNodeRef testXmlNode, std::vector<CString> *sErrors=NULL );

private:
	
	CAnimationGraph2Ptr	m_pGraph;
	CString							m_sGraphFileName;
	CString							m_sGraphWorkingName;
	XmlNodeRef					m_unitTests;
};

#endif // __ANIMATIONGRAPHUNITTESTER_H__
