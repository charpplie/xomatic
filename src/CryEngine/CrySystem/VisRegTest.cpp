////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   VisRegTest.cpp
//  Version:     v1.00
//  Created:     07/07/2009 by Nicolas Schulz.
//  Description: Visual Regression Test
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "VisRegTest.h"

#include "ISystem.h"
#include "I3DEngine.h"
#include "IRenderer.h"
#include "IConsole.h"
#include "ITimer.h"
#include "IInput.h"
#include "IEntitySystem.h"


CVisRegTest::CVisRegTest() : m_nextCmd( 0 ), m_waitFrames( 0 )
{
	CryLog( "Enabled visual regression tests" );
}


void CVisRegTest::Init()
{
	// Fill cmd buffer
	if( !LoadConfig( "VisRegTest.xml" ) )
	{
		CryWarning( VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "VisRegTest: Failed to load VisRegTest.xml from game folder" );
		return;
	}

	//gEnv->pConsole->ExecuteString( "hud_startPaused 0" );

	gEnv->pTimer->SetTimeScale( 0 );
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_RANDOM_SEED, 0, 0 );
	srand( 0 );

	// Disable features that are known to cause problems (that are non-deterministic)
	gEnv->pConsole->ExecuteString( "e_ParticlesThread 0" );
	gEnv->pConsole->ExecuteString( "r_multithreaded 0" );

	// Disable user input
	gEnv->pInput->EnableDevice( eDI_Keyboard, false );
	gEnv->pInput->EnableDevice( eDI_Mouse, false );
}


void CVisRegTest::AfterRender()
{
	ExecCommands();
}


bool CVisRegTest::LoadConfig( const string &fileName )
{
	XmlNodeRef rootNode = GetISystem()->LoadXmlFile( PathUtil::GetGameFolder() + "/" + fileName.c_str() );
	if( !rootNode || !rootNode->isTag( "VisRegTest" ) ) return false;

	string mapCmd( "map " );
	mapCmd.append( rootNode->getAttr( "map" ) );
	gEnv->pConsole->ExecuteString( mapCmd.c_str(), false, true );
	
	m_cmdBuf.clear();
	m_cmdBuf.push_back( SCmd( eCMDInit, "" ) );
	
	for( int i = 0; i < rootNode->getChildCount(); ++i )
	{
		XmlNodeRef node = rootNode->getChild( i );
		SCmd cmd;

		if( node->isTag( "WaitFrames" ) )
		{
			cmd.cmd = eCMDWaitFrames;
			cmd.args = node->getAttr( "frames" );
		}
		else if( node->isTag( "ConsoleCmd" ) )
		{
			cmd.cmd = eCMDConsoleCmd;
			cmd.args = node->getAttr( "cmd" );
		}
		else if( node->isTag( "Goto" ) )
		{
			cmd.cmd = eCMDGoto;
			cmd.args = "";
			cmd.args.append( node->getAttr( "px" ) ); cmd.args.append( " " );
			cmd.args.append( node->getAttr( "py" ) ); cmd.args.append( " " );
			cmd.args.append( node->getAttr( "pz" ) ); cmd.args.append( " " );
			cmd.args.append( node->getAttr( "rx" ) ); cmd.args.append( " " );
			cmd.args.append( node->getAttr( "ry" ) ); cmd.args.append( " " );
			cmd.args.append( node->getAttr( "rz" ) );
		}
		else if( node->isTag( "GotoEntity" ) )
		{
			cmd.cmd = eCMDGotoEntity;
			cmd.args = node->getAttr( "name" );
		}
		else if( node->isTag( "Screenshot" ) )
		{
			cmd.cmd = eCMDScreenshot;
			cmd.args = node->getAttr( "name" );
		}
		else
		{
			continue;
		}

		m_cmdBuf.push_back( cmd );
	}

	m_cmdBuf.push_back( SCmd( eCMDFinish, ""  ) );

	return true;
}


void CVisRegTest::ExecCommands()
{	
	if( m_nextCmd >= m_cmdBuf.size() ) return;
	
	float col[] = {0, 1, 0, 1};
	gEnv->pRenderer->Draw2dLabel( 10, 10, 2, col, false, "Visual Regression Test" );
	
	if( m_waitFrames > 0 )
	{
		--m_waitFrames;
		return;
	}
	
	while( m_nextCmd < m_cmdBuf.size() )
	{		
		string tmp;
		SCmd &cmd = m_cmdBuf[m_nextCmd++];
		
		switch( cmd.cmd )
		{
		case eCMDInit:
			AfterMapLoaded();
			break;
		case eCMDFinish:
			Finish();
			break;
		case eCMDWaitFrames:
			m_waitFrames = (uint32)atoi( cmd.args.c_str() );
			break;
		case eCMDConsoleCmd:
			gEnv->pConsole->ExecuteString( cmd.args.c_str() );
			break;
		case eCMDGoto:
			tmp = "goto ";
			tmp.append( cmd.args );
			gEnv->pConsole->ExecuteString( tmp.c_str() );
			m_waitFrames = 1;
			break;
		case eCMDGotoEntity:
			ExecGotoEntity( cmd.args.c_str() );
			m_waitFrames = 1;
			break;
		case eCMDScreenshot:
			stack_string filename( "Autotest/VisReg/results/" );
			filename += cmd.args.c_str();
			gEnv->pRenderer->ScreenShot( filename );
			CryLog( "VisRegTest: Took screenshot '%s', current random number value is %i", cmd.args.c_str(), cry_rand() );
			break;
		}

		if( m_waitFrames > 0 ) break;
	}
}


void CVisRegTest::AfterMapLoaded()
{
	gEnv->pTimer->SetTimer( ITimer::ETIMER_GAME, 0 );
	gEnv->pTimer->SetTimer( ITimer::ETIMER_UI, 0 );
	gEnv->pConsole->ExecuteString( "t_FixedStep 0.033333" );
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_RANDOM_SEED, 0, 0 );
	srand( 0 );

	CryLog( "VisRegTest: Starting tests..." );

	// Validate initial value of random number generator
	if( cry_rand() != 15196 )
		CryWarning( VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "VisRegTest: Random number generator has unexpected value" );
}


void CVisRegTest::ExecGotoEntity( const char *name )
{
	IEntity *entity = gEnv->pEntitySystem->FindEntityByName( name );

	if( entity )
	{
		Vec3 pos = entity->GetPos();
		Ang3 rot = Ang3::GetAnglesXYZ( entity->GetRotation() );
		
		char strbuf[256];
		sprintf( strbuf, "goto %f %f %f %f %f %f", pos.x, pos.y, pos.z, RAD2DEG( rot.x ), RAD2DEG( rot.y ), RAD2DEG( rot.z ) );
		gEnv->pConsole->ExecuteString( strbuf );
	}
	else
		CryWarning( VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "VisRegTest: GotoEntity '%s' failed", name );
}


void CVisRegTest::Finish()
{
	CryLog( "VisRegTest: Finished tests" );
	
	gEnv->pInput->EnableDevice( eDI_Keyboard, true );
	gEnv->pInput->EnableDevice( eDI_Mouse, true );
	
	gEnv->pConsole->ExecuteString( "t_FixedStep 0" );
	gEnv->pTimer->SetTimeScale( 1 );
}
