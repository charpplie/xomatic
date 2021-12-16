////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   timedemorecorder.cpp
//  Version:     v1.00
//  Created:     2/8/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TimeDemoRecorder.h"
#include <CryFile.h>
#include <IActorSystem.h>
#include <IAgent.h>
#include <ILevelSystem.h>
#include <ITestSystem.h>
#include <IMovieSystem.h>
#include "IMovementController.h"

#if defined(WIN32)
#include <windows.h>
#endif

#if defined(WIN32) && !defined(WIN64)
//#include "Psapi.h"								// PSAPI is not supported on windows9x
//#pragma comment(lib,"Psapi.lib")	// PSAPI is not supported on windows9x
#endif

//////////////////////////////////////////////////////////////////////////
// Brush Export structures.
//////////////////////////////////////////////////////////////////////////
#define TIMEDEMO_FILE_SIGNATURE "CRY "
#define TIMEDEMO_FILE_TYPE 150
#define TIMEDEMO_FILE_VERSION_1 1
#define TIMEDEMO_FILE_VERSION_2 2
#define TIMEDEMO_FILE_VERSION_3 4 // ?
#define TIMEDEMO_FILE_VERSION_4 6
#define TIMEDEMO_FILE_VERSION TIMEDEMO_FILE_VERSION_4

#define  TIMEDEMO_MAX_INPUT_EVENTS 16
#define  TIMEDEMO_MAX_GAME_EVENTS 1 // For now...
#define  TIMEDEMO_MAX_DESCRIPTION_LENGTH 64

//#define FIXED_TIME_STEP (30) // Assume runing at 30fps.

enum ETimeDemoFileFlags
{
	eTimeDemoCompressed = 0x0001,
};

#pragma pack(push,1)
struct STimeDemoHeader
{
	char signature[4];	// File signature.
	int filetype;				// File type.
	int	version;				// File version.
	int nDataOffset;    // Offset where frame data starts.

	//////////////////////////////////////////////////////////////////////////
	int numFrames;      // Number of frames.
	int nFrameSize;     // Size of the per frame data in bytes.
	float totalTime;
	char levelname[128];
	// @see ETimeDemoFileFlags
	uint32 nDemoFlags;
	uint32 nCompressedDataSize;
	uint32 nUncompressedDataSze;
	char reserved[116];

//////////////////////////////////////////////////////////////////////////
	void SwapEndianThis()
	{
		SwapEndian(filetype);
		SwapEndian(version);
		SwapEndian(nDataOffset);
		SwapEndian(numFrames);
		SwapEndian(nFrameSize);
		SwapEndian(totalTime);
		SwapEndian(nDemoFlags);
		SwapEndian(nCompressedDataSize);
		SwapEndian(nUncompressedDataSze);
	}
};

//////////////////////////////////////////////////////////////////////////
struct STimeDemoHeader_4: public STimeDemoHeader
{
	uint16 fixedTimeStep;

	void SwapEndianThis()
	{
		STimeDemoHeader::SwapEndianThis();
		SwapEndian(fixedTimeStep);
	}
};

//////////////////////////////////////////////////////////////////////////
struct STimeDemoFrame_1
{
	Vec3 pos;
	Ang3 angles;
	float frametime;

	unsigned int nActionFlags[2];
	float fLeaning;
	int nPolygonsPerFrame;
	char reserved[28];
};

//////////////////////////////////////////////////////////////////////////
struct STimeDemoFrameEvent_2
{
#ifndef NEED_ENDIAN_SWAP
	struct {
		uint16 deviceId   : 4;
		uint16 state      : 4;
		uint16 modifiers  : 4;
		uint16 reserved   : 4;
	};
#else
	struct {
		uint16 state      : 4;
		uint16 deviceId   : 4;
		uint16 reserved   : 4;
		uint16 modifiers  : 4;
	};
#endif
	uint16 keyId;
	float value;
	
	void SwapEndianThis()
	{
		SwapEndian(keyId); // this points to the first uint32 in structure
		SwapEndian(value);
	}
};
struct STimeDemoFrame_2
{
	Vec3 pos;
	Ang3 camera_angles;
	Quat player_rotation;
	float frametime;

	unsigned int nActionFlags[2];
	float fLeaning;
	int nPolygonsPerFrame;
	uint8 numInputEvents;

	STimeDemoFrameEvent_2 inputEvents[TIMEDEMO_MAX_INPUT_EVENTS];
	char reserved[32];
	
	//////////////////////////////////////////////////////////////////////////
	void SwapEndianThis()
	{
		SwapEndian(pos);
		SwapEndian(camera_angles);
		SwapEndian(player_rotation);
		SwapEndian(frametime);
		SwapEndian(nActionFlags[0]);
		SwapEndian(nActionFlags[1]);
		SwapEndian(fLeaning);
		SwapEndian(nPolygonsPerFrame);
		SwapEndian(numInputEvents);
		for (int i = 0; i < TIMEDEMO_MAX_INPUT_EVENTS; i++)
			inputEvents[i].SwapEndianThis();
	}
};

struct SRecordedGameEvent
{
	uint32 gameEventType;
	char entityName[TIMEDEMO_MAX_DESCRIPTION_LENGTH];
	char description[TIMEDEMO_MAX_DESCRIPTION_LENGTH];
	char description2[TIMEDEMO_MAX_DESCRIPTION_LENGTH];
	float value;
	int32 extra;

	void operator =(const SGameEvent& event)
	{
		//id = event.id;

		gameEventType = event.gameEventType;
		value = event.value;
		extra = event.extra;
		
		int descLength = event.entityName.length();
		int length = min(descLength, TIMEDEMO_MAX_DESCRIPTION_LENGTH-1);
		strncpy(entityName,event.entityName.c_str(),length+1);
		entityName[length]=0;

		descLength = event.description.length();
		length = min(descLength, TIMEDEMO_MAX_DESCRIPTION_LENGTH-1);
		strncpy(description,event.description.c_str(),length+1);
		description[length]=0;

		descLength = event.description2.length();
		length = min(descLength, TIMEDEMO_MAX_DESCRIPTION_LENGTH-1);
		strncpy(description2,event.description2.c_str(),length+1);
		description2[length]=0;
	}

	void SwapEndianThis()
	{
		SwapEndian(extra); 
		SwapEndian(value); 
//		SwapEndian(id);
	}
};

SGameEvent::SGameEvent(const SRecordedGameEvent& event)
{
//	id = event.id;
	entityName = event.entityName;
	gameEventType = event.gameEventType;
	value = event.value;
	extra = event.extra;
	description = event.description;
	description2 = event.description2;
}

struct STimeDemoFrame_3
{
	int nFrameDataSize; // Size of this frame in bytes.

	Vec3 pos;
	Ang3 camera_angles;
	Quat player_rotation;
	float frametime;

	unsigned int nActionFlags[2];
	float fLeaning;
	int nPolygonsPerFrame;
	int numInputEvents;
	STimeDemoFrameEvent_2 inputEvents[TIMEDEMO_MAX_INPUT_EVENTS];

	char reserved[32];

	//char data[]; // Special frame data.
	//////////////////////////////////////////////////////////////////////////
	void SwapEndianThis()
	{
		SwapEndian(pos);
		SwapEndian(camera_angles);
		SwapEndian(player_rotation);
		SwapEndian(frametime);
		SwapEndian(nActionFlags[0]);
		SwapEndian(nActionFlags[1]);
		SwapEndian(fLeaning);
		SwapEndian(nPolygonsPerFrame);
		SwapEndian(numInputEvents);
		for (int i = 0; i < TIMEDEMO_MAX_INPUT_EVENTS; i++)
			inputEvents[i].SwapEndianThis();
	}
};

struct STimeDemoFrame_4
{
	int nFrameDataSize; // Size of this frame in bytes.

	Vec3 pos;
	Ang3 camera_angles;
	Quat player_rotation;
	float frametime;

	unsigned int nActionFlags[2];
	float fLeaning;
	int nPolygonsPerFrame;
	int numInputEvents;
	STimeDemoFrameEvent_2 inputEvents[TIMEDEMO_MAX_INPUT_EVENTS];
	int numGameEvents;
	SRecordedGameEvent gameEvents[TIMEDEMO_MAX_GAME_EVENTS]; 
	
	uint32 bFollow; // if true, data from the next timedemo frame will be collected in this frame

	char reserved[32];

	//char data[]; // Special frame data.
	//////////////////////////////////////////////////////////////////////////
	void SwapEndianThis()
	{
		SwapEndian(pos);
		SwapEndian(camera_angles);
		SwapEndian(player_rotation);
		SwapEndian(frametime);
		SwapEndian(nActionFlags[0]);
		SwapEndian(nActionFlags[1]);
		SwapEndian(fLeaning);
		SwapEndian(nPolygonsPerFrame);
		SwapEndian(numInputEvents);
		SwapEndian(numGameEvents);
		for (int i = 0; i < TIMEDEMO_MAX_INPUT_EVENTS; i++)
			inputEvents[i].SwapEndianThis();
		for (int i = 0; i < TIMEDEMO_MAX_GAME_EVENTS; i++)
			gameEvents[i].SwapEndianThis();
	}
};

#pragma pack(pop)
//////////////////////////////////////////////////////////////////////////

CTimeDemoRecorder* CTimeDemoRecorder::s_pTimeDemoRecorder = 0;
ICVar* CTimeDemoRecorder::s_timedemo_file = 0;

//////////////////////////////////////////////////////////////////////////
CTimeDemoRecorder::CTimeDemoRecorder( ISystem *pSystem, CTestManager* pTestManager)
{
	s_pTimeDemoRecorder = this;
	m_pSystem = pSystem;
	m_pTestManager = pTestManager;

	gEnv->pGame->GetIGameFramework()->GetIGameplayRecorder()->EnableGameStateRecorder(false, this, false);


	CRY_ASSERT(m_pSystem);

	m_bRecording = false;
	m_bPlaying = false;
	m_bDemoFinished = false;

	m_currFrameIter = m_records.end();

	m_bPaused = false;
	m_fileVersion = TIMEDEMO_FILE_VERSION;

	IConsole *pConsole = gEnv->pConsole;

	// Register demo variables.
	s_timedemo_file = REGISTER_STRING( "demo_file","timedemo",0,"Time Demo Filename" );
	REGISTER_CVAR2( "demo_game_state",&m_demo_gameState,0,0,"enable/disable the game state recording" );
}

//////////////////////////////////////////////////////////////////////////
CTimeDemoRecorder::~CTimeDemoRecorder()
{
	s_pTimeDemoRecorder = 0;

	IConsole* pConsole = gEnv->pConsole;
	if(pConsole)
	{
		pConsole->UnregisterVariable( "demo_file", true );
		pConsole->UnregisterVariable( "demo_game_state", true );
	}
}

//////////////////////////////////////////////////////////////////////////
const char* CTimeDemoRecorder::GetCurrentLevelPath()
{
	static char buf[_MAX_PATH];
	const char *sLevelName = gEnv->pGame->GetIGameFramework()->GetAbsLevelPath(buf,_MAX_PATH);
	
	CryLog("DEBUG Levelpath: %s",sLevelName);

	return sLevelName;
/*
	ILevel *pLevel = gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel();
	if (!pLevel)
		return "";
	ILevelInfo *pLevelInfo = pLevel->GetLevelInfo();
	if (!pLevelInfo)
		return "";
	return pLevelInfo->GetPath();
*/
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::StartRecording( IConsoleCmdArgs *pArgs )
{
	Enable(true);

	if (pArgs->GetArgCount() > 1)
	{
		s_timedemo_file->Set( pArgs->GetArg(1) );
	}
	if (s_pTimeDemoRecorder)
		s_pTimeDemoRecorder->Record(true);
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::StopRecording( IConsoleCmdArgs *pArgs )
{
	s_pTimeDemoRecorder->Record(false);
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::PlayInit( IConsoleCmdArgs *pArgs )
{
	if (pArgs->GetArgCount() > 1)
	{
		s_timedemo_file->Set( pArgs->GetArg(1) );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::Stop( IConsoleCmdArgs *pArgs )
{
	if (s_pTimeDemoRecorder)
		s_pTimeDemoRecorder->Play(false);
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::Record( bool bEnable )
{
	if (bEnable == m_bRecording)
		return;

	if (gEnv->pMovieSystem)
		gEnv->pMovieSystem->StopAllSequences();

	m_bRecording = bEnable;
	m_bPlaying = false;
	if (m_bRecording)
	{
		SaveAllEntitiesState();

		gEnv->pEntitySystem->AddSink(this);

		// Start recording.
		m_records.clear();
		m_records.reserve(1000);

		m_currentFrameInputEvents.clear();
		m_currentFrameEntityEvents.clear();
		// Start listening input events.
		if (gEnv->pInput)
			gEnv->pInput->AddEventListener(this);

		StartSession();

		m_recordStartTime = GetTime();
		//m_lastFrameTime = m_recordStartTime;
	}
	else
	{
		// Stop recording.
		m_recordedDemoTime = m_totalDemoTime;
		//m_lastFrameTime = GetTime();

		gEnv->pEntitySystem->RemoveSink(this);

		m_currentFrameInputEvents.clear();
		m_currentFrameEntityEvents.clear();
		// Stop listening tho the input events.
		if (gEnv->pInput)
			gEnv->pInput->RemoveEventListener(this);

		StopSession();

		// Save after stopping.
		string filename = PathUtil::Make( GetCurrentLevelPath(),s_timedemo_file->GetString(),"tmd" );
		s_pTimeDemoRecorder->Save( filename.c_str() );
	}
	m_currFrameIter = m_records.end();
	m_currentFrame = 0;
	m_totalDemoTime.SetMilliSeconds(0);
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::Play( bool bEnable )
{
	if (bEnable == m_bPlaying)
		return;

	if (bEnable)
	{
		CRY_ASSERT(*GetCurrentLevelPath()!=0);

		// Try to load demo file.
		string filename = PathUtil::Make( GetCurrentLevelPath(),s_timedemo_file->GetString(),"tmd" );
		
		// Put it back later!
			Load( filename );

		if (m_records.empty())
		{
			m_bDemoFinished = true;
			return;
		}
	}

	m_bPlaying = bEnable;

	if (m_bPlaying)
	{
		LogInfo( "==============================================================" );
		LogInfo( "TimeDemo Play Started , (Total Frames: %d, Recorded Time: %.2fs)",(int)m_records.size(),m_recordedDemoTime.GetSeconds() );

		m_bDemoFinished = false;

		RestoreAllEntitiesState();

		// Start demo playback.
		m_currFrameIter = m_records.begin();
		//m_lastPlayedTotalTime = 0;
		StartSession();
	}
	else
	{
		LogInfo( "TimeDemo Play Ended");//, (%d Runs Performed)",m_numLoops );
		//LogInfo( "==============================================================" );

		// End demo playback.
		m_currFrameIter = m_records.end();
		//m_lastPlayedTotalTime = m_totalDemoTime.GetSeconds();
		StopSession();
	}
	m_bRecording = false;
	m_currentFrame = 0;
	m_totalDemoTime.SetMilliSeconds(0);
	m_lastFpsTimeRecorded = GetTime();

}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::Save( const char *filename )
{
	// Not save empty file.
	if (m_records.empty())
		return;

	CCryFile file;
	if (!file.Open( filename,"wb" ))
	{
		m_pSystem->Warning( VALIDATOR_MODULE_GAME,VALIDATOR_WARNING,0,filename,"Cannot open time demo file %s",filename );
		return;
	}

	m_file = filename;

	// Save Time demo file.
// #if TIMEDEMO_FILE_VERSION== TIMEDEMO_FILE_VERSION_4
// 	STimeDemoHeader_4 hdr;
// #else
	STimeDemoHeader hdr;
//#endif

	memset( &hdr,0,sizeof(hdr) );

	strncpy( hdr.levelname,GetCurrentLevelPath(),sizeof(hdr.levelname) );
	hdr.levelname[sizeof(hdr.levelname)-1] = 0;
	hdr.nDataOffset = sizeof(hdr);
	//hdr.nFrameSize = sizeof(STimeDemoFrame_3);
	
	memcpy( hdr.signature,TIMEDEMO_FILE_SIGNATURE,4 );
	hdr.filetype = TIMEDEMO_FILE_TYPE;
	hdr.version = TIMEDEMO_FILE_VERSION;
#if TIMEDEMO_FILE_VERSION== TIMEDEMO_FILE_VERSION_3

		hdr.nFrameSize = sizeof(STimeDemoFrame_3);
	hdr.nDemoFlags = 0;
	hdr.numFrames = m_records.size();
	hdr.totalTime = m_recordedDemoTime.GetSeconds();
		hdr.nUncompressedDataSze = hdr.nFrameSize * hdr.numFrames;

	std::vector<STimeDemoFrame_3> file_records;
	file_records.resize( hdr.numFrames );

	for (int i = 0; i < hdr.numFrames; i++)
	{
		FrameRecord &rec = m_records[i];
		STimeDemoFrame_3 &frame = file_records[i];
		ZeroStruct( frame );
		ZeroStruct( frame.inputEvents );
		frame.nFrameDataSize = sizeof(frame);
		frame.player_rotation = rec.playerRotation;
		frame.camera_angles = rec.cameraAngles;
		frame.pos = rec.playerPos;
		frame.frametime = rec.frameTime;
		*frame.nActionFlags = *rec.nActionFlags;
		frame.fLeaning = rec.fLeaning;
		frame.nPolygonsPerFrame = rec.nPolygons;
		frame.numInputEvents = 0;
		for (InputEventsList::const_iterator it = rec.inputEventsList.begin(); 
			it != rec.inputEventsList.end() && frame.numInputEvents < TIMEDEMO_MAX_INPUT_EVENTS; ++it)
		{
			const SInputEvent &inputEvent = *it;
			frame.inputEvents[frame.numInputEvents].deviceId = inputEvent.deviceId;
			frame.inputEvents[frame.numInputEvents].modifiers = inputEvent.modifiers;
			frame.inputEvents[frame.numInputEvents].state = inputEvent.state;
			frame.inputEvents[frame.numInputEvents].keyId = inputEvent.keyId;
			frame.inputEvents[frame.numInputEvents].value = inputEvent.value;
			frame.numInputEvents++;
		}
	}


#endif
#if TIMEDEMO_FILE_VERSION== TIMEDEMO_FILE_VERSION_4

		hdr.nFrameSize = sizeof(STimeDemoFrame_4);
		hdr.nDemoFlags = 0;
		hdr.numFrames = m_records.size();
		
		//hdr.fixedTimeStep = m_pTestManager->GetDemoFixedTimeStep();
		float* pHdrFixedTimeStep = (float*)hdr.reserved;
		*pHdrFixedTimeStep = m_pTestManager->GetDemoFixedTimeStep();
		// check possible extra frames for carrying extra game events

		int n = hdr.numFrames;
		for (int i = 0; i < n; i++)
		{
			FrameRecord &rec = m_records[i];
			int gesize = rec.gameEvents.size();
			if(gesize> TIMEDEMO_MAX_GAME_EVENTS)
				hdr.numFrames += (gesize-1)/TIMEDEMO_MAX_GAME_EVENTS;
		}

		hdr.totalTime = m_recordedDemoTime.GetSeconds();
		hdr.nUncompressedDataSze = hdr.nFrameSize * hdr.numFrames;

		std::vector<STimeDemoFrame_4> file_records;
		file_records.resize( hdr.numFrames );

		for (int i = 0, fr=0; fr < hdr.numFrames; i++,fr++)
		{
			FrameRecord &rec = m_records[i];

			STimeDemoFrame_4 &frame = file_records[fr];
			
			frame.player_rotation = rec.playerRotation;
			frame.camera_angles = rec.cameraAngles;
			frame.pos = rec.playerPos;
			frame.frametime = rec.frameTime;
			*frame.nActionFlags = *rec.nActionFlags;
			frame.fLeaning = rec.fLeaning;
			frame.nPolygonsPerFrame = rec.nPolygons;
			
			//input events
			frame.numInputEvents = 0;
			frame.bFollow = 0;

			ZeroStruct(frame.inputEvents);
			ZeroStruct(frame.gameEvents);
			for (InputEventsList::const_iterator it = rec.inputEventsList.begin(); 
				it != rec.inputEventsList.end() && frame.numInputEvents < TIMEDEMO_MAX_INPUT_EVENTS; ++it)
			{
				const SInputEvent &inputEvent = *it;
				frame.inputEvents[frame.numInputEvents].deviceId = inputEvent.deviceId;
				frame.inputEvents[frame.numInputEvents].modifiers = inputEvent.modifiers;
				frame.inputEvents[frame.numInputEvents].state = inputEvent.state;
				frame.inputEvents[frame.numInputEvents].keyId = inputEvent.keyId;
				frame.inputEvents[frame.numInputEvents].value = inputEvent.value;
				frame.numInputEvents++;
			}
			
			// game events
			// LEAVE THE GAME EVENTS FOR LAST (extended frames)
			frame.numGameEvents = rec.gameEvents.size();

			int remainingEvents = frame.numGameEvents - TIMEDEMO_MAX_GAME_EVENTS;
			if(frame.numGameEvents > TIMEDEMO_MAX_GAME_EVENTS)
				frame.numGameEvents = TIMEDEMO_MAX_GAME_EVENTS;

			int fc = 0;
			for(int j=0; j < frame.numGameEvents; j++)
				frame.gameEvents[j] = rec.gameEvents[fc++];

			bool bExtended = false;

			STimeDemoFrame_4* pAddedFrame = &frame;
			if(remainingEvents > 0 )
				bExtended = true;
			while(remainingEvents > 0 )
			{
				//	GameWarning("Timedemo: Exceeding number of game events in frame %i. Those game events will not be recorded.",i);
				pAddedFrame->bFollow = 1;
				fr++;
				pAddedFrame = &file_records[fr];
				pAddedFrame->numGameEvents = min(remainingEvents,TIMEDEMO_MAX_GAME_EVENTS);
				remainingEvents -= TIMEDEMO_MAX_GAME_EVENTS;
				CRY_ASSERT( pAddedFrame->numGameEvents < TIMEDEMO_MAX_GAME_EVENTS );
				for(int j=0; j < pAddedFrame->numGameEvents; j++)
				{
					pAddedFrame->gameEvents[j] = rec.gameEvents[fc++];
				}
			}

			if(bExtended)
				pAddedFrame->bFollow = 0;
		
		}
		

#endif
	//////////////////////////////////////////////////////////////////////////
	// Save to file.
	size_t nCompressedSize = hdr.nUncompressedDataSze*2+1024;
	char *pCompressedData = new char[nCompressedSize];
	if (GetISystem()->CompressDataBlock( &file_records[0],hdr.nUncompressedDataSze,pCompressedData,nCompressedSize ))
	{
		// Save compressed.
		hdr.nCompressedDataSize = nCompressedSize;
		hdr.nDemoFlags |= eTimeDemoCompressed;

		file.Write( &hdr,sizeof(hdr) );
		file.Write( pCompressedData,hdr.nCompressedDataSize );
	}
	else
	{
		// Save uncompressed.
		file.Write( &hdr,sizeof(hdr) );
		file.Write( &file_records[0],hdr.nUncompressedDataSze );
	}

	delete []pCompressedData;

/*
	XmlNodeRef root = m_pSystem->CreateXmlNode( "TimeDemo" );
	root->setAttr( "TotalTime",m_recordedDemoTime );
	for (FrameRecords::iterator it = m_records.begin(); it != m_records.end(); ++it)
	{
		FrameRecord &rec = *it;
		XmlNodeRef xmlRecord = root->newChild( "Frame" );
		xmlRecord->setAttr( "Pos",rec.playerPos );
		xmlRecord->setAttr( "Ang",rec.playerRotation );
		xmlRecord->setAttr( "Time",rec.frameTime );
	}
	root->saveToFile( filename );
*/
}

//////////////////////////////////////////////////////////////////////////
bool CTimeDemoRecorder::Load(  const char *filename )
{
	m_records.clear();
	m_recordedDemoTime.SetMilliSeconds(0);
	m_totalDemoTime.SetMilliSeconds(0);

	IInput *pIInput = GetISystem()->GetIInput(); // Cache IInput pointer.

	CCryFile file;
	if (!file.Open( filename,"rb" ))
	{
		char str[256];
		CryGetCurrentDirectory(256, str);

		m_pSystem->Warning( VALIDATOR_MODULE_GAME,VALIDATOR_WARNING,0,filename,"Cannot open time demo file %s (%s)",filename,str);
		return false;
	}

	m_file = filename;

	// Load Time demo file.
// #if TIMEDEMO_FILE_VERSION== TIMEDEMO_FILE_VERSION_4
// 	STimeDemoHeader_4 hdr;
// #else
	STimeDemoHeader hdr;
//#endif

	file.ReadRaw( &hdr,sizeof(hdr) );

	if (hdr.signature[0] == 'C' && hdr.signature[1] == 'R' && hdr.signature[2] == 'Y' && (uint8)hdr.signature[3] == 0x96)
	{
		// This is old header.
		file.Seek(0,SEEK_SET);
		file.ReadRaw( ((char*)&hdr)+1,sizeof(hdr)-1 ); // Old header was 1 byte shorter at the signature.
	}

	// Swap endian if needed.
	hdr.SwapEndianThis();
	
	m_recordedDemoTime = hdr.totalTime;
	m_totalDemoTime = m_recordedDemoTime;

	m_records.reserve(hdr.numFrames);

	m_fileVersion = hdr.version;

	if (m_fileVersion == TIMEDEMO_FILE_VERSION_1)
	{
		for (int i = 0; i < hdr.numFrames && !file.IsEof(); i++)
		{
			STimeDemoFrame_1 frame;
			FrameRecord rec;
			file.ReadRaw( &frame,sizeof(frame) );

			Quat rot;
			rot.SetRotationXYZ( Ang3(DEG2RAD(frame.angles)) );
			rot = rot * Quat::CreateRotationZ(gf_PI); // to fix some game to camera rotation issues.
			rec.playerRotation = rot;
			rec.cameraAngles = frame.angles;
			rec.playerPos = frame.pos;
			rec.frameTime = frame.frametime;
			*rec.nActionFlags = *frame.nActionFlags;
			rec.fLeaning = frame.fLeaning;
			rec.nPolygons = frame.nPolygonsPerFrame;
			m_records.push_back( rec );
		}
	}
	else if (m_fileVersion == TIMEDEMO_FILE_VERSION_2)
	{
		char *pFrameData = new char [hdr.nUncompressedDataSze];
		if (hdr.nDemoFlags & eTimeDemoCompressed)
		{
			char *pCompressedData = new char [hdr.nCompressedDataSize];
			// Read Compressed.
			file.ReadRaw( pCompressedData,hdr.nCompressedDataSize);

			// Uncompress data.
			size_t uncompressedSize = hdr.nUncompressedDataSze;
			GetISystem()->DecompressDataBlock(pCompressedData,hdr.nCompressedDataSize,pFrameData,uncompressedSize );
			CRY_ASSERT( uncompressedSize == hdr.nUncompressedDataSze );
			if (uncompressedSize != hdr.nUncompressedDataSze)
			{
				GameWarning( "Corrupted compressed time demo file %s",filename );
				delete []pCompressedData;
				return false;
			}
			delete []pCompressedData;
		}
		else
		{
			// Read Uncompressed.
			if (file.ReadRaw( pFrameData,hdr.nUncompressedDataSze ) != hdr.nUncompressedDataSze)
			{
				GameWarning( "Corrupted time demo file %s",filename );
				return false;
			}
		}
		CRY_ASSERT( sizeof(STimeDemoFrame_2)*hdr.numFrames == hdr.nUncompressedDataSze );
		if (sizeof(STimeDemoFrame_2)*hdr.numFrames != hdr.nUncompressedDataSze)
		{
			GameWarning( "Corrupted time demo file %s",filename );
			return false;
		}
		STimeDemoFrame_2 *pFileFrame = (STimeDemoFrame_2*)pFrameData;
		for (int i = 0; i < hdr.numFrames; i++)
		{
			STimeDemoFrame_2 &frame = *pFileFrame++;
			frame.SwapEndianThis(); // Swap endian if needed

			FrameRecord rec;

			rec.playerRotation = frame.player_rotation;
			rec.cameraAngles = frame.camera_angles;
			rec.playerPos = frame.pos;
			rec.frameTime = frame.frametime;
			*rec.nActionFlags = *frame.nActionFlags;
			rec.fLeaning = frame.fLeaning;
			rec.nPolygons = frame.nPolygonsPerFrame;
			if (frame.numInputEvents > 0)
			{
				for (int j = 0; j < frame.numInputEvents; j++)
				{
					SInputEvent inputEvent;
					inputEvent.deviceId = (EDeviceId)frame.inputEvents[j].deviceId;
					inputEvent.modifiers = frame.inputEvents[j].modifiers;
					inputEvent.state = (EInputState)frame.inputEvents[j].state;
					inputEvent.keyId = (EKeyId)frame.inputEvents[j].keyId;
					inputEvent.value = frame.inputEvents[j].value;
					SInputSymbol *pInputSymbol = pIInput? pIInput->LookupSymbol( inputEvent.deviceId,inputEvent.keyId ) : 0;
					if (pInputSymbol)
						inputEvent.keyName = pInputSymbol->name;
					rec.inputEventsList.push_back(inputEvent);
				}
			}
			m_records.push_back( rec );
		}

		delete []pFrameData;
	}
	else if (m_fileVersion == TIMEDEMO_FILE_VERSION_3)
	{
		char *pFrameData = new char [hdr.nUncompressedDataSze];
		if (hdr.nDemoFlags & eTimeDemoCompressed)
		{
			char *pCompressedData = new char [hdr.nCompressedDataSize];
			// Read Compressed.
			file.ReadRaw( pCompressedData,hdr.nCompressedDataSize);

			// Uncompress data.
			size_t uncompressedSize = hdr.nUncompressedDataSze;
			GetISystem()->DecompressDataBlock(pCompressedData,hdr.nCompressedDataSize,pFrameData,uncompressedSize );
			CRY_ASSERT( uncompressedSize == hdr.nUncompressedDataSze );
			if (uncompressedSize != hdr.nUncompressedDataSze)
			{
				GameWarning( "Corrupted compressed time demo file %s",filename );
				delete []pCompressedData;
				return false;
			}
			delete []pCompressedData;
		}
		else
		{
			// Read Uncompressed.
			if (file.ReadRaw( pFrameData,hdr.nUncompressedDataSze ) != hdr.nUncompressedDataSze)
			{
				GameWarning( "Corrupted time demo file %s",filename );
				return false;
			}
		}

		CRY_ASSERT( sizeof(STimeDemoFrame_3)*hdr.numFrames == hdr.nUncompressedDataSze );
		if (sizeof(STimeDemoFrame_3)*hdr.numFrames != hdr.nUncompressedDataSze)
		{
			GameWarning( "Corrupted time demo file %s",filename );
			return false;
		}

		STimeDemoFrame_3 *pFileFrame = (STimeDemoFrame_3*)pFrameData;
		for (int i = 0; i < hdr.numFrames; i++)
		{
			STimeDemoFrame_3 &frame = *pFileFrame++;
			frame.SwapEndianThis(); // Swap endian if needed
			FrameRecord rec;

			rec.playerRotation = frame.player_rotation;
			rec.cameraAngles = frame.camera_angles;
			rec.playerPos = frame.pos;
			rec.frameTime = frame.frametime;
			*rec.nActionFlags = *frame.nActionFlags;
			rec.fLeaning = frame.fLeaning;
			rec.nPolygons = frame.nPolygonsPerFrame;

			if (frame.numInputEvents > 0)
			{
				for (int j = 0; j < frame.numInputEvents; j++)
				{
					SInputEvent inputEvent;
					inputEvent.deviceId = (EDeviceId)frame.inputEvents[j].deviceId;
					inputEvent.modifiers = frame.inputEvents[j].modifiers;
					inputEvent.state = (EInputState)frame.inputEvents[j].state;
					inputEvent.keyId = (EKeyId)frame.inputEvents[j].keyId;
					inputEvent.value = frame.inputEvents[j].value;
					SInputSymbol *pInputSymbol = pIInput? pIInput->LookupSymbol( inputEvent.deviceId,inputEvent.keyId ) : 0;
					if (pInputSymbol)
						inputEvent.keyName = pInputSymbol->name;
					rec.inputEventsList.push_back(inputEvent);
				}
			}
			
			m_records.push_back( rec );
		}

		delete []pFrameData;

	}
	else if (m_fileVersion == TIMEDEMO_FILE_VERSION_4)
	{

		float recFixedTimeStep = float(*hdr.reserved);
		if(recFixedTimeStep>0 && recFixedTimeStep<1000)
			m_pTestManager->SetDemoFixedTimeStep((uint16)recFixedTimeStep);

		char *pFrameData = new char [hdr.nUncompressedDataSze];
		if (hdr.nDemoFlags & eTimeDemoCompressed)
		{
			char *pCompressedData = new char [hdr.nCompressedDataSize];
			// Read Compressed.
			file.ReadRaw( pCompressedData,hdr.nCompressedDataSize);

			// Uncompress data.
			size_t uncompressedSize = hdr.nUncompressedDataSze;
			GetISystem()->DecompressDataBlock(pCompressedData,hdr.nCompressedDataSize,pFrameData,uncompressedSize );
			CRY_ASSERT( uncompressedSize == hdr.nUncompressedDataSze );
			if (uncompressedSize != hdr.nUncompressedDataSze)
			{
				GameWarning( "Corrupted compressed time demo file %s",filename );
				delete []pCompressedData;
				return false;
			}
			delete []pCompressedData;
		}
		else
		{
			// Read Uncompressed.
			if (file.ReadRaw( pFrameData,hdr.nUncompressedDataSze ) != hdr.nUncompressedDataSze)
			{
				GameWarning( "Corrupted time demo file %s",filename );
				return false;
			}
		}

		CRY_ASSERT( sizeof(STimeDemoFrame_4)*hdr.numFrames == hdr.nUncompressedDataSze );
		if (sizeof(STimeDemoFrame_4)*hdr.numFrames != hdr.nUncompressedDataSze)
		{
			GameWarning( "Corrupted time demo file %s",filename );
			return false;
		}

		STimeDemoFrame_4 *pFileFrame = (STimeDemoFrame_4*)pFrameData;
		for (int i = 0; i < hdr.numFrames; i++)
		{
			STimeDemoFrame_4 &frame = *pFileFrame++;
			frame.SwapEndianThis(); // Swap endian if needed
			FrameRecord rec;

			rec.playerRotation = frame.player_rotation;
			rec.cameraAngles = frame.camera_angles;
			rec.playerPos = frame.pos;
			rec.frameTime = frame.frametime;
			*rec.nActionFlags = *frame.nActionFlags;
			rec.fLeaning = frame.fLeaning;
			rec.nPolygons = frame.nPolygonsPerFrame;

			if (frame.numInputEvents > 0)
			{
				for (int j = 0; j < frame.numInputEvents; j++)
				{
					SInputEvent inputEvent;
					inputEvent.deviceId = (EDeviceId)frame.inputEvents[j].deviceId;
					inputEvent.modifiers = frame.inputEvents[j].modifiers;
					inputEvent.state = (EInputState)frame.inputEvents[j].state;
					inputEvent.keyId = (EKeyId)frame.inputEvents[j].keyId;
					inputEvent.value = frame.inputEvents[j].value;
					SInputSymbol *pInputSymbol = pIInput? pIInput->LookupSymbol( inputEvent.deviceId,inputEvent.keyId ) : 0;
					if (pInputSymbol)
						inputEvent.keyName = pInputSymbol->name;
					rec.inputEventsList.push_back(inputEvent);
				}
			}
	
			if (frame.numGameEvents > 0)
			{
				for (int j = 0; j < frame.numGameEvents; j++)
					rec.gameEvents.push_back(frame.gameEvents[j]);
			}

			STimeDemoFrame_4* pAddFrame = &frame;
			while(pAddFrame->bFollow && i < hdr.numFrames-1)
			{
				++i;
				pAddFrame = pFileFrame++;
				for (int j = 0; j < pAddFrame->numGameEvents; j++)
					rec.gameEvents.push_back(pAddFrame->gameEvents[j]);
			}

			m_records.push_back( rec );
		}

		delete []pFrameData;

	}

	else
	{
		GameWarning( "Timedemo: Unknown file version" );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::PreUpdate()
{
	m_currentFrame = m_pTestManager->GetCurrentFrame();
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::Update()
{

	//////////////////////////////////////////////////////////////////////////
	// Game state
	if(m_bPlaying && !m_bPaused &&  m_currFrameIter != m_records.end())
	{
		FrameRecord &rec = *m_currFrameIter;

		IGameStateRecorder* pGameStateRecorder = gEnv->pGame->GetIGameFramework()->GetIGameplayRecorder()->GetIGameStateRecorder();
		if(pGameStateRecorder && m_fileVersion >= TIMEDEMO_FILE_VERSION_4 && m_demo_gameState)
		{
			int n = rec.gameEvents.size();
			for(int i = 0; i < n; i++)
	{
				SGameEvent& gameEvent = rec.gameEvents[i];
				//IEntity * pEntity = gEnv->pEntitySystem->GetEntity(gameEvent.id);
				IEntity * pEntity = gEnv->pEntitySystem->FindEntityByName(gameEvent.entityName);
				if(pEntity)
		{
					GameplayEvent event;
					event.event = gameEvent.gameEventType;
					event.value = gameEvent.value;
					event.extra = (void*)(gameEvent.extra);

					if(gameEvent.description.size())
 						event.description = gameEvent.description.c_str();//GetDescriptionFromCache(gameEvent.description);

					if(gameEvent.description2.size())
						event.extra = (void*)gameEvent.description2.c_str();//GetDescriptionFromCache(gameEvent.description2);

					pGameStateRecorder->OnRecordedGameplayEvent(pEntity,event,m_currentFrame);
				}
		}
	}

		m_currFrameIter++;

	}

}


//////////////////////////////////////////////////////////////////////////
const char* CTimeDemoRecorder::GetDescriptionFromCache(const char* desc)
{
	TDescriptionContainer::iterator it=m_Descriptions.begin(),itEnd = m_Descriptions.end();
	for(; it != itEnd; ++it)
{
		string& szval = *it;
		if(szval == desc)
			return szval.c_str();
	}
	m_Descriptions.push_back(desc);
	string& szval = m_Descriptions.back();
	return szval.c_str();
}

//////////////////////////////////////////////////////////////////////////
bool CTimeDemoRecorder::RecordFrame()
{
	CTimeValue time = GetTime();

	if (m_bPaused)
	{
		m_lastFrameTime = time;
		return true;
	}

	FrameRecord rec;
	memset( &rec,0,sizeof(rec) );

	float frameTime = (time - m_lastFrameTime).GetSeconds();
	rec.frameTime = frameTime;

	IEntity *pPlayer = NULL;

	IActor *pClActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClActor)
	{
		pPlayer = pClActor->GetEntity();
			/*
		pClActor->GetEntity()->SetPos(pos);
		SOBJECTSTATE	cont;
		//		cont.m_desiredActions = 0;
		cont.fire = false;
		cont.fMovementUrgency = 1.0f;	// max speed.
		cont.vMoveDir = Vec3( 0,0,0 );
		cont.vLookDir = viewDir;
		pClActor->SetActorMovement( cont );
		*/
	}

	if (pPlayer)
	{
		rec.playerPos = pPlayer->GetPos();

		if (pClActor != NULL)
			rec.playerRotation = pClActor->GetViewRotation();
		else
			rec.playerRotation = pPlayer->GetWorldRotation();
	}
	Ang3 cameraAngles = Ang3( GetISystem()->GetViewCamera().GetMatrix() );
	rec.cameraAngles = RAD2DEG(cameraAngles);

	//CryLog( "[TimeDemoRec] [%04d] Angle: %.2f, %.2f, %.2f",m_currentFrame,rec.playerAngles.x,rec.playerAngles.y,rec.playerAngles.z );
	// Record current processing command.
	/*
	CXEntityProcessingCmd &cmd = ((CXGame*)m_pGame)->m_pClient->m_PlayerProcessingCmd;
	*rec.nActionFlags = *cmd.m_nActionFlags;
	rec.fLeaning = cmd.GetLeaning();
	*/

	//////////////////////////////////////////////////////////////////////////
	// Record input events.
	//////////////////////////////////////////////////////////////////////////
	rec.inputEventsList = m_currentFrameInputEvents;
	rec.entityEvents = m_currentFrameEntityEvents;
	rec.gameEvents = m_CurrentFrameGameEvents;

	m_currentFrameInputEvents.clear();
	m_currentFrameEntityEvents.clear();
	m_CurrentFrameGameEvents.clear();

	//////////////////////////////////////////////////////////////////////////

	m_totalDemoTime += rec.frameTime;

	int nPolygons,nShadowVolPolys;
	gEnv->pRenderer->GetPolyCount(nPolygons,nShadowVolPolys);
	rec.nPolygons = nPolygons;

	m_records.push_back( rec );
	
	FrameRecords::iterator it = m_records.begin();
	FrameRecords::iterator it1 = it;
	++it1;
/*	if(it1!=m_records.end())
	{
		EntityEventRecords* p1 = &(it->entityEvents);

		EntityEventRecords* p2 = &(it1->entityEvents);
		EntityEventRecords* p21 = &(it1->entityEvents);
	}
*/
	m_currentFrame = m_pTestManager->GetCurrentFrame();
	m_lastFrameTime = GetTime();

	if (m_currentFrame >= m_pTestManager->GetNumberOfFrames())//m_demo_max_frames)
	{
		Record(false);
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CTimeDemoRecorder::PlayFrame()
{
	if (m_records.empty() || m_currFrameIter == m_records.end()) // can't playback empty records.
		return false;

	CTimeValue time = GetTime();
	//CTimeValue deltaFrameTime = (time - m_lastFrameTime);
	//float frameTime = deltaFrameTime.GetSeconds();

	if (m_bPaused)
	{
		m_lastFrameTime = time;
		return true;
	}

	FrameRecord &rec = *m_currFrameIter;
	m_nTotalPolysRecorded += rec.nPolygons;

	//////////////////////////////////////////////////////////////////////////
	// Play input events.
	//////////////////////////////////////////////////////////////////////////
	if (!rec.inputEventsList.empty())
	{
		//string str;
		for (InputEventsList::const_iterator it = rec.inputEventsList.begin(), end = rec.inputEventsList.end(); it != end; ++it)
		{
			const SInputEvent &inputEvent = *it;
			if (gEnv->pInput)
				gEnv->pInput->PostInputEvent( inputEvent );
		}
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Play back entity events.
	//////////////////////////////////////////////////////////////////////////
	{
		for (int i = 0; i < rec.entityEvents.size(); i++)
		{
			PlayBackEntityEvent( rec.entityEvents[i] );
		}
	}
	//////////////////////////////////////////////////////////////////////////

	IEntity *pPlayer = NULL;
	IActor *pClActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClActor)
	{
		pPlayer = pClActor->GetEntity();
	}

	Quat qPlayRot;
	if (pPlayer)
	{
		if (pPlayer->GetParent() == 0)
		{
			// Only if player is not linked to anything.
			pPlayer->SetPos( rec.playerPos,ENTITY_XFORM_TIMEDEMO );
			CRY_ASSERT( rec.playerRotation.IsValid() );
			pPlayer->SetRotation(rec.playerRotation, ENTITY_XFORM_TIMEDEMO );
			//pClActor->SetViewRotation( rec.playerRotation );
		}
	}



	// Play looped.
	if (m_currFrameIter == m_records.end() || m_currentFrame >= m_pTestManager->GetNumberOfFrames())//m_demo_max_frames)
	{
		m_lastFrameTime = GetTime();
		return false;
	}

	m_lastFrameTime = GetTime();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::Restart()
{
	m_currFrameIter = m_records.begin();
	m_currentFrame = 0;
	//m_nTotalPolysPlayed = 0;
	m_nTotalPolysRecorded = 0;

}


//////////////////////////////////////////////////////////////////////////
CTimeValue CTimeDemoRecorder::GetTime()
{
	// Must be asynchronius time, used for profiling.
	return gEnv->pTimer->GetAsyncTime();
}

//////////////////////////////////////////////////////////////////////////
int CTimeDemoRecorder::GetNumFrames() const
{
	return m_records.size();
}

//////////////////////////////////////////////////////////////////////////
float CTimeDemoRecorder::GetAverageFrameRate() const
{
	if(m_currentFrame)
	{
		float aveFrameTime = m_pTestManager->GetTotalTime().GetSeconds() / m_currentFrame;
		float aveFrameRate = 1.0f / aveFrameTime;
		return aveFrameRate;
	}
	return 0.0f;
}

//////////////////////////////////////////////////////////////////////////
float CTimeDemoRecorder::RenderInfo(float y)
{
	float retY = 0;

//	if (m_demo_noinfo != 0)
//		return retY ;

	const char *sInfo = m_bPaused ? " (Paused)" : "";
	
	IRenderer *pRenderer = gEnv->pRenderer;
	
	if (m_bRecording)
	{
		float fColor[4] = {1,0,0,1};
		pRenderer->Draw2dLabel( 1,y+retY, 1.3f, fColor,false,"TimeDemo%s, Frames: %d",sInfo,m_currentFrame );
		retY +=15;
	}
	else if (m_bPlaying)
	{
		int numFrames = GetNumFrames();
		float fColor[4] = {0,1,0,1};
		pRenderer->Draw2dLabel( 1,y+retY, 1.3f, fColor,false,"TimeDemo%s, Frame %d of %d",sInfo,m_currentFrame,numFrames);
		retY +=15;
	}

	if(!m_bPaused && m_demo_gameState && m_fileVersion >= TIMEDEMO_FILE_VERSION_4)
	{
		IGameStateRecorder* pGameStateRecorder = gEnv->pGame->GetIGameFramework()->GetIGameplayRecorder()->GetIGameStateRecorder();
		if(pGameStateRecorder)
			retY += pGameStateRecorder->RenderInfo(y+retY,m_bRecording);
	}

	return retY;
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::SetConsoleVar( const char *sVarName,float value )
{
	ICVar *pVar = gEnv->pConsole->GetCVar( sVarName );
	if (pVar)
		pVar->Set( value );
}

//////////////////////////////////////////////////////////////////////////
float CTimeDemoRecorder::GetConsoleVar( const char *sVarName )
{
	ICVar *pVar = gEnv->pConsole->GetCVar( sVarName );
	if (pVar)
		return pVar->GetFVal();
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::StartSession()
{
\
#ifdef SP_DEMO
	//allow devmode in SP_DEMO only during demo playback
	CCryAction::GetCryAction()->CreateDevMode();
#endif

		m_nTotalPolysRecorded = 0;

	//////////////////////////////////////////////////////////////////////////
	if (m_bRecording && m_pTestManager->IsAIEnabled())
	{
		SaveAllEntitiesState();
	}
	//////////////////////////////////////////////////////////////////////////

	// Register to frame profiler.

	// Not cut-scenes during timedemo.
	SetConsoleVar( "mov_NoCutscenes",1 );

	// No wait for key-press on level load.
	SetConsoleVar( "hud_startPaused",0 );

	//ResetSessionLoop();

	if(m_demo_gameState && m_fileVersion >= TIMEDEMO_FILE_VERSION_4)
		gEnv->pGame->GetIGameFramework()->GetIGameplayRecorder()->EnableGameStateRecorder(true,this, m_bRecording);

	if (m_bPlaying)
	{
		IActor *pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
		if (pClientActor)
		{
			pClientActor->EnableTimeDemo(true);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Force player out of the vehicle on start.
	//////////////////////////////////////////////////////////////////////////
	// Luc TO DO: remove this
	gEnv->pConsole->ExecuteString( "v_exit_player" );

	m_lastFrameTime = GetTime();

	m_Descriptions.clear();
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::StopSession()

{
#ifdef SP_DEMO
	CCryAction::GetCryAction()->RemoveDevMode();
#endif

	IActor *pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor)
		pClientActor->EnableTimeDemo(false);

	gEnv->pGame->GetIGameFramework()->GetIGameplayRecorder()->EnableGameStateRecorder(false, this, false);

	}
	

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::LogInfo( const char *format,... )
{
	if (!m_pTestManager->IsInfoEnabled())
		return;

	va_list	ArgList;
	char szBuffer[1024];

	va_start(ArgList, format);
	vsprintf_s(szBuffer, format, ArgList);
	va_end(ArgList);

	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	gEnv->pLog->Log( szBuffer  );

	_splitpath( m_file.c_str(), drive, dir, fname, ext );
	_makepath( path_buffer, drive, dir,fname,"log" );
	FILE *hFile = fopen( path_buffer,"at" );
	if (hFile)
	{
		// Write the string to the file and close it
		fprintf(hFile, "%s\n",szBuffer );
		fclose(hFile);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CTimeDemoRecorder::OnInputEvent( const SInputEvent &event )
{
	if ((event.deviceId == eDI_Keyboard) && event.keyId == eKI_Tilde)
	{
		return false;
	}
	if (event.deviceId != eDI_Unknown)
	{
		m_currentFrameInputEvents.push_back(event);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::EndLog()
{
	int numFrames = m_records.size();
	LogInfo( "!TimeDemo Run Finished.");

#if defined(WIN32) && !defined(WIN64)

	// PSAPI is not supported on window9x
	// so, don't use it

	//PROCESS_MEMORY_COUNTERS pc;
	//HANDLE hProcess = GetCurrentProcess();
	//pc.cb = sizeof(pc);
	//GetProcessMemoryInfo( hProcess,&pc,sizeof(pc) );
	//int MB = 1024*1024;
	//LogInfo( "    Memory Usage: WorkingSet=%dMb, PageFile=%dMb, PageFaults=%d",pc.WorkingSetSize/MB,pc.PagefileUsage/MB,pc.PageFaultCount );

#endif
}

void CTimeDemoRecorder::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "TimeDemoRecorder");
	s->Add(*this);
	s->AddContainer(m_records);
	s->AddContainer(m_currentFrameInputEvents);
	s->AddContainer(m_currentFrameEntityEvents);
	s->AddContainer(m_CurrentFrameGameEvents);
}


bool CTimeDemoRecorder::OnBeforeSpawn( SEntitySpawnParams &params )
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::OnSpawn( IEntity *pEntity,SEntitySpawnParams &params )
{
}

//////////////////////////////////////////////////////////////////////////
bool CTimeDemoRecorder::OnRemove( IEntity *pEntity )
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::OnEvent( IEntity *pEntity, SEntityEvent &event )
{
	if (m_bRecording)
	{
		// Record entity event for this frame.
		EntityGUID guid = pEntity->GetGuid();
		if (!guid)
			return;

		// Record entity event for this frame.
		switch (event.event)
		{
			// Events to save.
		case ENTITY_EVENT_XFORM:
		case ENTITY_EVENT_HIDE:
		case ENTITY_EVENT_UNHIDE:
		case ENTITY_EVENT_ATTACH:
		case ENTITY_EVENT_DETACH:
		case ENTITY_EVENT_DETACH_THIS:
		case ENTITY_EVENT_ENABLE_PHYSICS:
		case ENTITY_EVENT_ENTER_SCRIPT_STATE:
			{
				EntityEventRecord rec;
				memset( &rec,0,sizeof(rec) );
				rec.entityId = pEntity->GetId();
				rec.guid = guid;
				rec.eventType = event.event;
				rec.nParam[0] = event.nParam[0];
				rec.nParam[1] = event.nParam[1];
				rec.nParam[2] = event.nParam[2];
				rec.nParam[3] = event.nParam[3];
				rec.pos = pEntity->GetPos();
				rec.q = pEntity->GetRotation();
				m_currentFrameEntityEvents.push_back(rec);
			}
			break;

			// Skip all other events.
		default:
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::PlayBackEntityEvent( const EntityEventRecord &rec )
{
	EntityId entityId = gEnv->pEntitySystem->FindEntityByGuid(rec.guid);
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	switch (rec.eventType)
	{
		// Events to save.
	case ENTITY_EVENT_XFORM:
		pEntity->SetPosRotScale( rec.pos,rec.q,pEntity->GetScale(),ENTITY_XFORM_TIMEDEMO );
		break;
	case ENTITY_EVENT_HIDE:
		pEntity->Hide( true );
		break;
	case ENTITY_EVENT_UNHIDE:
		pEntity->Hide( false );
		break;
	case ENTITY_EVENT_ATTACH:
		{
			IEntity *pChild = (IEntity*)gEnv->pEntitySystem->GetEntity((EntityId)rec.nParam[0]); // Get Child entity.
			if (pChild)
				pEntity->AttachChild( pChild );
		}
		break;
	case ENTITY_EVENT_DETACH:
		break;
	case ENTITY_EVENT_DETACH_THIS:
		pEntity->DetachThis(0,ENTITY_XFORM_TIMEDEMO);
		break;
	case ENTITY_EVENT_ENABLE_PHYSICS:
		if (rec.nParam[0] == 0)
			pEntity->EnablePhysics(false);
		else
			pEntity->EnablePhysics(true);
		break;
	case ENTITY_EVENT_ENTER_SCRIPT_STATE:
		{
			IEntityScriptProxy *pScriptProxy = (IEntityScriptProxy*)pEntity->GetProxy(ENTITY_PROXY_SCRIPT);
			if (pScriptProxy)
			{
				pScriptProxy->GotoStateId( (int)rec.nParam[0] );
			}
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::SaveAllEntitiesState()
{
	m_firstFrameEntityState.clear();
	// Record all objects positions.
	IEntity *pEntity;
	IEntityItPtr pEntityIter = gEnv->pEntitySystem->GetEntityIterator();
	while (pEntity = pEntityIter->Next())
	{
		EntityGUID guid = pEntity->GetGuid();
		if (guid)
		{
			EntityEventRecord rec;
			memset( &rec,0,sizeof(rec) );
			rec.entityId = pEntity->GetId();
			rec.guid = guid;
			rec.eventType = ENTITY_EVENT_RESET;
			rec.pos = pEntity->GetPos();
			rec.q = pEntity->GetRotation();
			rec.flags |= (pEntity->IsHidden()) ? EntityEventRecord::HIDDEN : 0;
			m_firstFrameEntityState.push_back(rec);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::RestoreAllEntitiesState()
{
	for (int i = 0; i < m_firstFrameEntityState.size(); i++)
	{
		EntityEventRecord &rec = m_firstFrameEntityState[i];
		if (rec.eventType == ENTITY_EVENT_RESET)
		{
			EntityId entityId = gEnv->pEntitySystem->FindEntityByGuid(rec.guid);
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityId);
			if (!pEntity)
				continue;
			
			pEntity->Hide( (rec.flags & EntityEventRecord::HIDDEN) != 0 );
			pEntity->SetPosRotScale( rec.pos,rec.q,pEntity->GetScale(),ENTITY_XFORM_TIMEDEMO );
		}
	}

}

///////////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::ParseParams(XmlNodeRef baseNode) 
{
	
}

///////////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::SetVariable(const char* name,const char* szValue)
{

}

///////////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::SetVariable(const char* name,float value)
{

}

///////////////////////////////////////////////////////////////////////////////
int CTimeDemoRecorder::GetNumberOfFrames()
{
	return m_records.size();
}

///////////////////////////////////////////////////////////////////////////////
void CTimeDemoRecorder::OnGameplayEvent(IEntity *pEntity, const GameplayEvent &event)
{
	if(IsRecording())
	{
		SGameEvent ge(pEntity,event);
		m_CurrentFrameGameEvents.push_back(ge);
	}
}
