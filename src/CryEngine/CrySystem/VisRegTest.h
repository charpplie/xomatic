////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   VisRegTest.h
//  Version:     v1.00
//  Created:     07/07/2009 by Nicolas Schulz.
//  Description: Visual Regression Test
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __VisRegTest_h__
#define __VisRegTest_h__
#pragma once

class CVisRegTest
{
protected:

	enum ECmd
	{
		eCMDInit,
		eCMDFinish,
		eCMDWaitFrames,
		eCMDConsoleCmd,
		eCMDGoto,
		eCMDGotoEntity,
		eCMDScreenshot
	};
	
	struct SCmd
	{
		ECmd    cmd;
		string  args;

		SCmd() {}
		SCmd( ECmd cmd, const string &args ) { this->cmd = cmd; this->args = args; }
	};
	
	std::vector< SCmd >  m_cmdBuf;
	uint32               m_nextCmd;
	uint32               m_waitFrames;

public:

	CVisRegTest();
	
	void Init();
	void AfterRender();

protected:

	bool LoadConfig( const string &fileName );
	void ExecCommands();
	void AfterMapLoaded();
	void ExecGotoEntity( const char *name );
	void Finish();
};

#endif // __VisRegTest_h__
