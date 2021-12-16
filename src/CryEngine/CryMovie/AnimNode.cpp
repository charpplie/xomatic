////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   animnode.cpp
//  Version:     v1.00
//  Created:     23/4/2002 by Timur.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AnimNode.h"
#include "AnimTrack.h"
#include "AnimSequence.h"
#include "CharacterTrack.h"
#include "AnimSplineTrack.h"
#include "BoolTrack.h"
#include "SelectTrack.h"
#include "EventTrack.h"
#include "SoundTrack.h"
#include "ExprTrack.h"
#include "ConsoleTrack.h"
#include "MusicTrack.h"
#include "FaceSeqTrack.h"
#include "LookAtTrack.h"
#include "TrackEventTrack.h"
#include "SequenceTrack.h"
#include "CompoundSplineTrack.h"
#include "GotoTrack.h"
#include "CaptureTrack.h"
#include <I3DEngine.h>
#include <ctime>

//////////////////////////////////////////////////////////////////////////
// Old depricated IDs
//////////////////////////////////////////////////////////////////////////
#define APARAM_CHARACTER4 (APARAM_USER + 0x10)
#define APARAM_CHARACTER5 (APARAM_USER + 0x11)
#define APARAM_CHARACTER6 (APARAM_USER + 0x12)
#define APARAM_CHARACTER7 (APARAM_USER + 0x13)
#define APARAM_CHARACTER8 (APARAM_USER + 0x14)
#define APARAM_CHARACTER9 (APARAM_USER + 0x15)
#define APARAM_CHARACTER10 (APARAM_USER + 0x16)

#define APARAM_EXPRESSION4 (APARAM_USER + 0x20)
#define APARAM_EXPRESSION5 (APARAM_USER + 0x21)
#define APARAM_EXPRESSION6 (APARAM_USER + 0x22)
#define APARAM_EXPRESSION7 (APARAM_USER + 0x23)
#define APARAM_EXPRESSION8 (APARAM_USER + 0x24)
#define APARAM_EXPRESSION9 (APARAM_USER + 0x25)
#define APARAM_EXPRESSION10 (APARAM_USER + 0x26)
//////////////////////////////////////////////////////////////////////////

static const int DEFAULT_TRACK_TYPE = -1;

//////////////////////////////////////////////////////////////////////////
// CAnimNode.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
int CAnimNode::GetTrackCount() const
{
	return m_tracks.size();
}

const char* CAnimNode::GetParamName( AnimParamType param ) const
{
	switch ((int)param)
	{
	case APARAM_FOV:
		return "FOV";
	case APARAM_POS:
		return "Position";
	case APARAM_ROT:
		return "Rotation";
	case APARAM_SCL:
		return "Scale";
	case APARAM_VISIBLE:
		return "Visiblity";
	case APARAM_EVENT:
		return "Events";
	case APARAM_TRACKEVENT:
		return "Track Events";
	case APARAM_CAMERA:
		return "Camera";
	
	// Sound tracks.
	case APARAM_SOUND1:
		return "Sound1";
	case APARAM_SOUND2:
		return "Sound2";
	case APARAM_SOUND3:
		return "Sound3";

	// Character tracks.
	case APARAM_CHARACTER1:
		return "Animation1";
	case APARAM_CHARACTER2:
		return "Animation2";
	case APARAM_CHARACTER3:
		return "Animation3";
	case APARAM_CHARACTER4:
		return "Animation4";
	case APARAM_CHARACTER5:
		return "Animation5";
	case APARAM_CHARACTER6:
		return "Animation6";
	case APARAM_CHARACTER7:
		return "Animation7";
	case APARAM_CHARACTER8:
		return "Animation8";
	case APARAM_CHARACTER9:
		return "Animation9";
	case APARAM_CHARACTER10:
		return "Animation10";


	case APARAM_EXPRESSION1:
		return "Expression1";
	case APARAM_EXPRESSION2:
		return "Expression2";
	case APARAM_EXPRESSION3:
		return "Expression3";
	case APARAM_EXPRESSION4:
		return "Expression4";
	case APARAM_EXPRESSION5:
		return "Expression5";
	case APARAM_EXPRESSION6:
		return "Expression6";
	case APARAM_EXPRESSION7:
		return "Expression7";
	case APARAM_EXPRESSION8:
		return "Expression8";
	case APARAM_EXPRESSION9:
		return "Expression9";
	case APARAM_EXPRESSION10:
		return "Expression10";

	case APARAM_SEQUENCE:
		return "Sequence";
	case APARAM_CONSOLE:
		return "Console";
	case APARAM_MUSIC:
		return "Music";

	case APARAM_FLOAT_1:
		return "Value";
	case APARAM_CAPTURE:
		return "Capture";
	case APARAM_TIMEWARP:
		return "Timewarp";
	}
	return "Unknown";
}

IAnimTrack* CAnimNode::GetTrackForParameter( int param ) const
{
	for (int i = 0,num = (int)m_tracks.size(); i < num; i++)
	{
		if (m_tracks[i].paramId == param)
			return m_tracks[i].track;

		// Search the subtracks also if any.
		for(int k = 0; k < m_tracks[i].track->GetSubTrackCount(); ++k)
		{
			if (m_tracks[i].track->GetSubTrack(k)->GetParameterType() == param)
				return m_tracks[i].track->GetSubTrack(k);
		}
	}
	return 0;
}

IAnimTrack* CAnimNode::GetTrackByIndex( int nIndex ) const
{
	return m_tracks[nIndex].track;
}

void CAnimNode::SetTrack( int param,IAnimTrack *track )
{
	if (track)
	{
		for (unsigned int i = 0; i < m_tracks.size(); i++)
		{
			if (m_tracks[i].paramId == param)
			{
				m_tracks[i].track = track;
				return;
			}
		}
		AddTrack( param,track );
	}
	else
	{
		// Remove track at this id.
		for (unsigned int i = 0; i < m_tracks.size(); i++)
		{
			if (m_tracks[i].paramId == param)
			{
				m_tracks.erase( m_tracks.begin() + i );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::AddTrack( int param,IAnimTrack *track )
{
	TrackDesc td;
	td.paramId = param;
	td.track = track;
	track->SetParameterType( (AnimParamType)param);
	m_tracks.push_back(td);
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::RemoveTrack( IAnimTrack *pTrack )
{
	for (unsigned int i = 0; i < m_tracks.size(); i++)
	{
		if (m_tracks[i].track == pTrack)
		{
			m_tracks.erase( m_tracks.begin() + i );
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimNode::CreateTrackInternal( int paramId,int trackType )
{
	SParamInfo info;
	if (!GetParamInfoFromId(paramId,info))
	{
		return 0;
	}

	IAnimTrack *pTrack = NULL;
	switch (info.valueType)
	{
	case AVALUE_FLOAT:   
		pTrack = CreateTrackInternalFloat(trackType);
		break;
	
	case AVALUE_VECTOR:
		pTrack = CreateTrackInternalVector(trackType, info);
		break;

	case AVALUE_QUAT:
		pTrack = CreateTrackInternalQuat(trackType, info);
		break;
	case AVALUE_EVENT:   pTrack=new CEventTrack; break;
	case AVALUE_BOOL:    pTrack=new CBoolTrack; break;
	case AVALUE_SELECT:  pTrack=new CSelectTrack; break;
	case AVALUE_SOUND:   pTrack=new CSoundTrack; break;
	case AVALUE_CHARACTER: pTrack=new CCharacterTrack; break;
	case AVALUE_EXPRESSION:pTrack=new CExprTrack; break;
	case AVALUE_CONSOLE: pTrack=new CConsoleTrack; break;
	case AVALUE_MUSIC:   pTrack=new CMusicTrack; break;
	case AVALUE_FACESEQ: pTrack=new CFaceSeqTrack; break;
	case AVALUE_LOOKAT:  pTrack=new CLookAtTrack; break;
	case AVALUE_TRACKEVENT:   pTrack=new CTrackEventTrack; break;
	case AVALUE_SEQUENCE: pTrack=new CSequenceTrack; break;
	case AVALUE_DISCRETE_FLOAT:	pTrack = new CGototTrack; break;
	case AVALUE_CAPTURE:	pTrack = new CCaptureTrack; break;

	case AVALUE_VECTOR4:
		pTrack = CreateTrackInternalVector4(info);
		break;
	}
	if (pTrack)
		AddTrack( paramId,pTrack );
	return pTrack;
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimNode::CreateTrack( int paramId )
{
	return CreateTrackInternal( paramId,DEFAULT_TRACK_TYPE );
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::SerializeAnims( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks )
{
	if (bLoading)
	{
		// Delete all tracks.
		m_tracks.clear();

		IAnimNode::SParamInfo info;
		// Loading.
		int paramId = -1;
		int num = xmlNode->getChildCount();
		for (int i = 0; i < num; i++)
		{
			XmlNodeRef trackNode = xmlNode->getChild(i);
			trackNode->getAttr( "ParamId",paramId );

			int nTrackType = DEFAULT_TRACK_TYPE;
			trackNode->getAttr( "Type",nTrackType );
			if (nTrackType == DEFAULT_TRACK_TYPE)
			{
				//////////////////////////////////////////////////////////////////////////
				// Backward compatibility code
				//////////////////////////////////////////////////////////////////////////
				// Legacy animation track.
				// Collapse parameter ID to the single type
				if (paramId >= APARAM_SOUND1 && paramId <= APARAM_SOUND3)
					paramId = APARAM_SOUND1;
				if (paramId >= APARAM_CHARACTER1 && paramId <= APARAM_CHARACTER3)
					paramId = APARAM_CHARACTER1;
				if (paramId >= APARAM_CHARACTER4 && paramId <= APARAM_CHARACTER10)
					paramId = APARAM_CHARACTER1;
				if (paramId >= APARAM_EXPRESSION1 && paramId <= APARAM_EXPRESSION3)
					paramId = APARAM_EXPRESSION1;
				if (paramId >= APARAM_EXPRESSION4 && paramId <= APARAM_EXPRESSION10)
					paramId = APARAM_EXPRESSION1;
				
				// Old tracks always used TCB tracks.
				// Backward compatibility to the CryEngine2 for track type (will make TCB controller)
				nTrackType = -2;
				//////////////////////////////////////////////////////////////////////////
			}
			
			IAnimTrack *track = CreateTrackInternal( paramId,nTrackType );
			if (track)
			{
				if (!track->Serialize( trackNode,bLoading,bLoadEmptyTracks ))
				{
					// Boolean tracks must always be loaded even if empty.
					if (track->GetType() != ATRACK_BOOL)
					{
						RemoveTrack(track);
					}
				}
			}
		}
	}
	else
	{
		// Saving.
		for (unsigned int i = 0; i < m_tracks.size(); i++)
		{
			IAnimTrack *track = m_tracks[i].track;
			if (track)
			{
				int paramid = m_tracks[i].paramId;
				XmlNodeRef trackNode = xmlNode->newChild( "Track" );
				trackNode->setAttr( "ParamId",m_tracks[i].paramId );
				int nTrackType = m_tracks[i].track->GetType();
				trackNode->setAttr( "Type",nTrackType );
				track->Serialize( trackNode,bLoading );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::SetTimeRange( Range timeRange )
{
	for (unsigned int i = 0; i < m_tracks.size(); i++)
	{
		if (m_tracks[i].track)
		{
			m_tracks[i].track->SetTimeRange( timeRange );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CAnimNode::CAnimNode()
{
	m_id = 0;
	m_pOwner = 0;
	m_pAnimator = 0;
	m_pSequence = 0;
	m_flags = 0;
	m_bIgnoreSetParam = false;
	m_pParentNode = 0;
	m_nLoadedParentNodeId = 0;
}

//////////////////////////////////////////////////////////////////////////
CAnimNode::~CAnimNode()
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::SetFlags( int flags )
{
	m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CAnimNode::GetFlags() const
{
	return m_flags;
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::Animate( SAnimContext &ec )
{
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::IsParamValid( int paramId ) const
{
	SParamInfo info;
	if (GetParamInfoFromId(paramId,info))
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::SetParamValue( float time,AnimParamType param,float value )
{
	if (m_bIgnoreSetParam)
		return true;

	IAnimTrack *track = GetTrackForParameter(param);
	if (track && track->GetValueType() == AVALUE_FLOAT)
	{
		// Float track.
		bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags&ANODE_FLAG_SELECTED)); // Only selected nodes can be recorded
		track->SetValue( time,value,bDefault );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::SetParamValue( float time,AnimParamType param,const Vec3 &value )
{
	if (m_bIgnoreSetParam)
		return true;
	
	CCompoundSplineTrack *track = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
	if (track && track->GetValueType() == AVALUE_VECTOR)
	{
		// Vec3 track.
		bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags&ANODE_FLAG_SELECTED)); // Only selected nodes can be recorded
		track->SetValue( time,value,bDefault );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::SetParamValue( float time,AnimParamType param,const Vec4 &value )
{
	if (m_bIgnoreSetParam)
		return true;

	CCompoundSplineTrack *track = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
	if (track && track->GetValueType() == AVALUE_VECTOR4)
	{
		// Vec4 track.
		bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags&ANODE_FLAG_SELECTED)); // Only selected nodes can be recorded
		track->SetValue( time,value,bDefault );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::GetParamValue( float time,AnimParamType param,float &value )
{
	IAnimTrack *track = GetTrackForParameter(param);
	if (track && track->GetValueType() == AVALUE_FLOAT)
	{
		// Float track.
		track->GetValue( time,value );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::GetParamValue( float time,AnimParamType param,Vec3 &value )
{
	CCompoundSplineTrack *track = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
	if (track && track->GetValueType() == AVALUE_VECTOR)
	{
		// Vec3 track.
		track->GetValue( time,value );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::GetParamValue( float time,AnimParamType param,Vec4 &value )
{
	CCompoundSplineTrack *track = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
	if (track && track->GetValueType() == AVALUE_VECTOR4)
	{
		// Vec4 track.
		track->GetValue( time,value );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks )
{
	if (bLoading)
	{
		xmlNode->getAttr( "Id",m_id );
		const char *name = xmlNode->getAttr("Name");
		int flags;
		if (xmlNode->getAttr( "Flags", flags ))
			SetFlags(flags);
		SetName(name);

		m_nLoadedParentNodeId = 0;
		xmlNode->getAttr("ParentNode",m_nLoadedParentNodeId);
	}
	else
	{
		m_nLoadedParentNodeId = 0;
		xmlNode->setAttr( "Id",m_id );
		xmlNode->setAttr("Type", GetType() );
		xmlNode->setAttr("Name", GetName() );
		xmlNode->setAttr( "Flags", GetFlags() );
		if (m_pParentNode)
			xmlNode->setAttr("ParentNode",m_pParentNode->GetId());
	}

	SerializeAnims(xmlNode, bLoading, bLoadEmptyTracks);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CAnimNode::SaveToColladaInFixedFPS(float fps) const
{
	XmlNodeRef nodeCollada	= gEnv->pSystem->CreateXmlNode("COLLADA");
	nodeCollada->setAttr("xmlns", "http://www.collada.org/2005/11/COLLADASchema");
	nodeCollada->setAttr("version", "1.4.1");
	
		// Some meta data
		XmlNodeRef assetNode = nodeCollada->newChild("asset");
			XmlNodeRef contributorNode = assetNode->newChild("contributor");
				contributorNode->newChild("author")->setContent("JJ");
				contributorNode->newChild("authoring_tool")->setContent("CryENGINE TrackView");
				string sourceData;
				sourceData.Format("%s%s", "file://", gEnv->p3DEngine->GetLevelFilePath(""));
				contributorNode->newChild("source_data")->setContent(sourceData.c_str());

			std::time_t time = std::time(0);
			std::tm dateTime = *std::localtime(&time);
			char timeBuffer[1024];
			std::strftime(timeBuffer, sizeof(timeBuffer) / sizeof(timeBuffer[0]), "%Y-%m-%dT%H:%M:%SZ", &dateTime);
			assetNode->newChild("created")->setContent(timeBuffer);
			assetNode->newChild("modified")->setContent(timeBuffer);
			assetNode->newChild("revision")->setContent("1.4.1");
			XmlNodeRef unitNode = assetNode->newChild("unit");
			unitNode->setAttr("meter", 0.01);
			unitNode->setAttr("name", "meter");
			assetNode->newChild("up_axis")->setContent("Z_UP");
			
		// Save each track.
		XmlNodeRef libraryAnimationsNode = nodeCollada->newChild("library_animations");
		for(unsigned int i = 0; i < m_tracks.size(); i++)
		{
			IAnimTrack *track = m_tracks[i].track;
			if(track)
			{
				if(track->GetParameterType() != APARAM_POS 
				&& track->GetParameterType() != APARAM_ROT)
					continue;
				string trackName;
				trackName.Format("%s-%s", m_name.c_str(), GetParamName(track->GetParameterType()));
				track->SaveToColladaInFixedFPS(libraryAnimationsNode, m_name, 
																			trackName.c_str(), fps);
			}
		}

	return nodeCollada;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::LoadFromCollada(XmlNodeRef xmlNode)
{
	// Some sanity checks
	if (strcmp(xmlNode->getTag(), "COLLADA") != 0)
		return false;
	XmlNodeRef assetNode = xmlNode->findChild("asset");
	if (!assetNode)
		return false;
	XmlNodeRef upAxisNode = assetNode->findChild("up_axis");
	if (!upAxisNode)
		return false;
//	if (strcmp(upAxisNode->getContent(), "Z_UP") != 0)
//		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::PostLoad()
{
	if (m_nLoadedParentNodeId)
	{
		IAnimNode *pParentNode = ((CAnimSequence*)m_pSequence)->FindNodeById(m_nLoadedParentNodeId);
		m_pParentNode = pParentNode;
		m_nLoadedParentNodeId = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CAnimNode::GetReferenceMatrix() const
{
	static Matrix34 tm(IDENTITY);
	return tm;
}

IAnimTrack * CAnimNode::CreateTrackInternalFloat( int trackType ) const
{
	IAnimTrack *pTrack;
	switch (trackType)
	{
	case ATRACK_BEZIER_FLOAT:
		pTrack = new C2DSplineTrack;
		break;
	case ATRACK_TCB_FLOAT:
		pTrack = new CTcbFloatTrack;
		break;
	default:
		pTrack = new C2DSplineTrack;
	}	
	return pTrack;
}

 IAnimTrack * CAnimNode::CreateTrackInternalVector( int trackType, const SParamInfo &info ) const
{
	IAnimTrack *pTrack;
	switch (trackType)
	{
	
	case ATRACK_POSITION_XYZ:
	case ATRACK_EULER_XYZ:
	default:
		if(trackType == DEFAULT_TRACK_TYPE )
		{
			if(info.paramId == APARAM_POS || info.paramId == APARAM_SCL)
				trackType = ATRACK_POSITION_XYZ;
			else if(info.paramId == APARAM_ROT)
				trackType = ATRACK_EULER_XYZ;
		}
		{
			AnimParamType subTrackParamTypes[MAX_SUBTRACKS];
			if(info.paramId == APARAM_POS)
			{
				subTrackParamTypes[0] = APARAM_POS_X;
				subTrackParamTypes[1] = APARAM_POS_Y;
				subTrackParamTypes[2] = APARAM_POS_Z;
			}
			else if(info.paramId == APARAM_SCL)
			{
				subTrackParamTypes[0] = APARAM_SCL_X;
				subTrackParamTypes[1] = APARAM_SCL_Y;
				subTrackParamTypes[2] = APARAM_SCL_Z;
			}
			else if(info.paramId == APARAM_ROT)
			{
				subTrackParamTypes[0] = APARAM_ROT_X;
				subTrackParamTypes[1] = APARAM_ROT_Y;
				subTrackParamTypes[2] = APARAM_ROT_Z;
			}
			else if(info.paramId == APARAM_DEPTH_OF_FIELD)
			{
				subTrackParamTypes[0] = APARAM_FOCUS_DISTANCE;
				subTrackParamTypes[1] = APARAM_FOCUS_RANGE;
				subTrackParamTypes[2] = APARAM_BLUR_AMMOUNT;
				pTrack = new CCompoundSplineTrack(3, ATRACK_DEPTH_OF_FIELD, (EAnimValue)info.valueType, subTrackParamTypes);
				pTrack->SetSubTrackName(0, "FocusDist");
				pTrack->SetSubTrackName(1, "FocusRange");
				pTrack->SetSubTrackName(2, "BlurAmount");
				return pTrack;
			}		
			else
				assert(0);
			pTrack = new CCompoundSplineTrack(3,(EAnimTrackType)trackType,(EAnimValue)info.valueType, subTrackParamTypes);
		}
		break;
	case ATRACK_TCB_VECTOR:
		pTrack = new CTcbVectorTrack;
		break;
	case -2:
		// Backward compatibility to the CryEngine2
		pTrack = new CTcbVectorTrack;
		break;
	}
	return pTrack;
 }

 IAnimTrack * CAnimNode::CreateTrackInternalQuat( int trackType, const SParamInfo &info ) const
 {
	IAnimTrack *pTrack;
	 switch (trackType)
	 {
	 case ATRACK_EULER_XYZ:
	 default:
		 if(trackType == DEFAULT_TRACK_TYPE )
			{
				if(info.paramId == APARAM_ROT)
					trackType = ATRACK_EULER_XYZ;
			}
			{
				AnimParamType subTrackParamTypes[MAX_SUBTRACKS];
				if(info.paramId == APARAM_ROT)
				{
					subTrackParamTypes[0] = APARAM_ROT_X;
					subTrackParamTypes[1] = APARAM_ROT_Y;
					subTrackParamTypes[2] = APARAM_ROT_Z;
				}
				else
					assert(0);

				pTrack = new CCompoundSplineTrack(3,(EAnimTrackType)trackType,(EAnimValue)info.valueType, subTrackParamTypes);
			}
			break;
	 case ATRACK_TCB_QUAT:
		 pTrack = new CTcbQuatTrack;
		 break;
	 case -2:
		 // Backward compatibility to the CryEngine2
		 pTrack = new CTcbQuatTrack;
	 }
	return pTrack;
 }

 IAnimTrack * CAnimNode::CreateTrackInternalVector4( const SParamInfo &info ) const
 {
	IAnimTrack *pTrack;

	 AnimParamType subTrackParamTypes[MAX_SUBTRACKS];
	 subTrackParamTypes[0] = APARAM_SHAKEMULT_AMPA;
	 subTrackParamTypes[1] = APARAM_SHAKEMULT_AMPB;
	 subTrackParamTypes[2] = APARAM_SHAKEMULT_FREQA;
	 subTrackParamTypes[3] = APARAM_SHAKEMULT_FREQB;
	 if(info.paramId == APARAM_NOISE)
		 pTrack = new CCompoundSplineTrack(4,ATRACK_NOISE,(EAnimValue)info.valueType, subTrackParamTypes);
	 else // if(info.paramId == APARAM_SHAKEMULT)
		 pTrack = new CCompoundSplineTrack(4,ATRACK_CAMERASHAKE,(EAnimValue)info.valueType, subTrackParamTypes);

	 return pTrack;
 }
