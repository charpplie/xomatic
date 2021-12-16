////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   CompoundSplineTrack.cpp
//  Created:     16/4/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CompoundSplineTrack.h"
#include "AnimSplineTrack.h"

CCompoundSplineTrack::CCompoundSplineTrack( int nDims,EAnimTrackType inType,EAnimValue inValueType, 
																					 AnimParamType subTrackParamTypes[MAX_SUBTRACKS] )
{
	assert( nDims > 0 && nDims <= MAX_SUBTRACKS );
	m_nDimensions = nDims;
	m_type = inType;
	m_valueType = inValueType;

	m_nParamType = (AnimParamType)0;
	m_flags = 0;

	ZeroStruct(m_subTracks);

	for (int i = 0; i < m_nDimensions; i++)
	{
		m_subTracks[i] = new C2DSplineTrack;
		m_subTracks[i]->SetParameterType(subTrackParamTypes[i]);
	}

	m_subTrackNames[0] = "X";
	m_subTrackNames[1] = "Y";
	m_subTrackNames[2] = "Z";
	m_subTrackNames[3] = "W";
}


//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetTimeRange( const Range &timeRange )
{
	m_timeRange = timeRange;
	for (int i = 0; i < m_nDimensions; i++)
	{
		m_subTracks[i]->SetTimeRange(timeRange);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::PrepareNodeForSubTrackSerialization(XmlNodeRef &subTrackNode, 
																															 XmlNodeRef &xmlNode, int i, bool bLoading)
{
	assert(!bLoading || xmlNode->getChildCount() == m_nDimensions);

	if(bLoading)
	{
		subTrackNode = xmlNode->getChild(i);
		// First, check its version.
		if(strcmp(subTrackNode->getTag(), "SubTrack") == 0)
			// So, it's an old format.
		{
			AnimParamType paramType = m_subTracks[i]->GetParameterType();
			// Recreate subtracks as the old format.
			m_subTracks[i] = new CTcbFloatTrack;
			m_subTracks[i]->SetParameterType(paramType);
		}
	}
	else
	{
		if(m_subTracks[i]->GetType() == ATRACK_BEZIER_FLOAT)
			// It's a new 2D Bezier curve.
			subTrackNode = xmlNode->newChild("NewSubTrack");
		else
		// Old TCB spline
		{
			assert(m_subTracks[i]->GetType() == ATRACK_TCB_FLOAT);
			subTrackNode = xmlNode->newChild("SubTrack");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CCompoundSplineTrack::Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks/*=true */ )
{
	for (int i = 0; i < m_nDimensions; i++)
	{
		XmlNodeRef subTrackNode;
		PrepareNodeForSubTrackSerialization(subTrackNode, xmlNode, i, bLoading);
		m_subTracks[i]->Serialize(subTrackNode,bLoading,bLoadEmptyTracks);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CCompoundSplineTrack::SerializeSelection( XmlNodeRef &xmlNode,bool bLoading, 
																							bool bCopySelected/*=false*/,float fTimeOffset/*=0*/ )
{
	for (int i = 0; i < m_nDimensions; i++)
	{
		XmlNodeRef subTrackNode;
		PrepareNodeForSubTrackSerialization(subTrackNode, xmlNode, i, bLoading);
		m_subTracks[i]->SerializeSelection(subTrackNode,bLoading,bCopySelected,fTimeOffset);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SaveToColladaInFixedFPS( XmlNodeRef libraryAnimationsNode, 
																									 const char *nodeName, const char *trackName, float fps )
{
	string subTrackName;
	for (int i = 0; i < m_nDimensions; i++)
	{
		subTrackName.Format("%s-%s", trackName, GetSubTrackName(i));
		m_subTracks[i]->SaveToColladaInFixedFPS(libraryAnimationsNode, nodeName, subTrackName.c_str(), fps);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::GetValue( float time,float &value )
{
	for (int i = 0; i < 1 && i < m_nDimensions; i++)
	{
		m_subTracks[i]->GetValue(time,value);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::GetValue( float time,Vec3 &value )
{
	for (int i = 0; i < m_nDimensions; i++)
	{
		float v = value[i];
		m_subTracks[i]->GetValue(time,v);
		value[i] = v;
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::GetValue( float time,Vec4 &value )
{
	for (int i = 0; i < m_nDimensions; i++)
	{
		float v = value[i];
		m_subTracks[i]->GetValue(time,v);
		value[i] = v;
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::GetValue( float time,Quat &value )
{
	if (m_nDimensions == 3)
	{
		// Assume Euler Angles XYZ
		float angles[3] = {0,0,0};
		for (int i = 0; i < m_nDimensions; i++)
		{
			m_subTracks[i]->GetValue(time,angles[i]);
		}
		value = Quat::CreateRotationXYZ(Ang3(DEG2RAD(angles[0]),DEG2RAD(angles[1]),DEG2RAD(angles[2])));
	}
	else
	{
		assert(0);
		value.SetIdentity();
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetValue( float time,const float &value,bool bDefault )
{
	for (int i = 0; i < m_nDimensions; i++)
	{
		m_subTracks[i]->SetValue(time,value,bDefault);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetValue( float time,const Vec3 &value,bool bDefault )
{
	for (int i = 0; i < m_nDimensions; i++)
	{
		m_subTracks[i]->SetValue(time,value[i],bDefault);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetValue( float time,const Vec4 &value,bool bDefault )
{
	for (int i = 0; i < m_nDimensions; i++)
	{
		m_subTracks[i]->SetValue(time,value[i],bDefault);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetValue( float time,const Quat &value,bool bDefault )
{
	if (m_nDimensions == 3)
	{
		// Assume Euler Angles XYZ
		Ang3 angles = Ang3::GetAnglesXYZ(value);
		for (int i = 0; i < 3; i++)
		{
			float degree = RAD2DEG(angles[i]);
			if(false == bDefault)
			{
				// Try to prefer the shortest path of rotation.
				float degree0 = 0.0f;
				m_subTracks[i]->GetValue(time, degree0);
				degree = PreferShortestRotPath(degree, degree0);

			}
			m_subTracks[i]->SetValue(time,degree,bDefault);
		}
	}
	else
	{
		assert(0);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::OffsetKeyPosition( const Vec3 &offset )
{
	if (m_nDimensions == 3)
	{
		for (int i = 0; i < 3; i++)
		{
			IAnimTrack *pSubTrack = m_subTracks[i];
			// Iterate over all keys.
			for (int k = 0,num = pSubTrack->GetNumKeys(); k < num; k++)
			{
				// Offset each key.
				float time = pSubTrack->GetKeyTime(k);
				float value = 0;
				pSubTrack->GetValue(time,value);
				value = value + offset[i];
				pSubTrack->SetValue(time,value);
			}
		}
	}
	else
	{
		assert(0);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetSelected( bool bSelect )
{
	for (int i = 0; i < m_nDimensions; i++)
	{
		m_subTracks[i]->SetSelected(bSelect);
	}
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CCompoundSplineTrack::GetSubTrack( int nIndex ) const
{
	assert( nIndex >= 0 && nIndex < m_nDimensions );
	return m_subTracks[nIndex];
}

//////////////////////////////////////////////////////////////////////////
const char* CCompoundSplineTrack::GetSubTrackName(int nIndex) const
{
	assert( nIndex >= 0 && nIndex < m_nDimensions );
	return m_subTrackNames[nIndex];
}


//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetSubTrackName(int nIndex, const char *name)
{
	assert( nIndex >= 0 && nIndex < m_nDimensions );
	assert(name);
	m_subTrackNames[nIndex] = name;
}

//////////////////////////////////////////////////////////////////////////
int CCompoundSplineTrack::GetNumKeys() const
{
	int nKeys = 0;
	for (int i = 0; i < m_nDimensions; i++)
	{
		nKeys += m_subTracks[i]->GetNumKeys();
	}
	return nKeys;
}

float CCompoundSplineTrack::PreferShortestRotPath( float degree, float degree0 ) const
{
	// Assumes the degree is in (-PI, PI).
	assert(-181.0f < degree && degree < 181.0f);
	float degree00 = degree0;
	degree0 = cry_fmod(degree0, 360.0f);
	float n = (degree00 - degree0)/360.0f;
	float degreeAlt;
	if(degree >= 0)
		degreeAlt = degree - 360.0f;
	else
		degreeAlt = degree + 360.0f;
	if(fabs(degreeAlt-degree0) < fabs(degree-degree0))
		return degreeAlt + n*360.0f;
	else
		return degree + n*360.0f;
}

int CCompoundSplineTrack::GetSubTrackIndex( int& key ) const
{
	assert( key >= 0 && key < GetNumKeys() );
	int count = 0;
	for (int i = 0; i < m_nDimensions; i++)
	{
		if(key < count + m_subTracks[i]->GetNumKeys())
		{
			key = key - count;
			return i;
		}
		count += m_subTracks[i]->GetNumKeys(); 
	}
	return -1;
}

void CCompoundSplineTrack::RemoveKey( int num )
{
	assert( num >= 0 && num < GetNumKeys() );
	int i = GetSubTrackIndex(num);
	assert(i >= 0);
	if(i < 0)
		return;
	m_subTracks[i]->RemoveKey(num);
}

void CCompoundSplineTrack::GetKeyInfo( int key,const char* &description,float &duration )
{
	static char str[64];
	duration = 0;
	description = str;
	const char *subDesc = NULL;
	float time = GetKeyTime(key);
	int m = 0;
	/// Using the time obtained, combine descriptions from keys of the same time 
	/// in sub-tracks if any into one compound description.
	strcpy_s(str, "");
	// A head case
	for(m=0; m<m_subTracks[0]->GetNumKeys(); ++m)
	{
		if(m_subTracks[0]->GetKeyTime(m) == time)
		{
			float dummy;
			m_subTracks[0]->GetKeyInfo(m, subDesc, dummy);
			strcat_s(str, subDesc);
			break;
		}
	}
	if(m == m_subTracks[0]->GetNumKeys())
		strcat_s(str, m_subTrackNames[0]);
	// Tail cases
	for(int i=1; i<GetSubTrackCount(); ++i)
	{
		strcat_s(str, ",");
		for(m=0; m<m_subTracks[i]->GetNumKeys(); ++m)
		{
			if(m_subTracks[i]->GetKeyTime(m) == time)
			{
				float dummy;
				m_subTracks[i]->GetKeyInfo(m, subDesc, dummy);
				strcat_s(str, subDesc);
				break;
			}
		}
		if(m == m_subTracks[i]->GetNumKeys())
			strcat_s(str, m_subTrackNames[i]);
	}
}

float CCompoundSplineTrack::GetKeyTime( int index ) const
{
	assert( index >= 0 && index < GetNumKeys() );
	int i = GetSubTrackIndex(index);
	assert(i >= 0);
	if(i < 0)
		return 0;
	return m_subTracks[i]->GetKeyTime(index);
}

void CCompoundSplineTrack::SetKeyTime( int index,float time )
{
	assert( index >= 0 && index < GetNumKeys() );
	int i = GetSubTrackIndex(index);
	assert(i >= 0);
	if(i < 0)
		return;
	m_subTracks[i]->SetKeyTime(index, time);
}

bool CCompoundSplineTrack::IsKeySelected(int key) const
{
	assert( key >= 0 && key < GetNumKeys() );
	int i = GetSubTrackIndex(key);
	assert(i >= 0);
	if(i < 0)
		return false;
	return m_subTracks[i]->IsKeySelected(key);
}

void CCompoundSplineTrack::SelectKey(int key, bool select)
{
	assert( key >= 0 && key < GetNumKeys() );
	int i = GetSubTrackIndex(key);
	assert(i >= 0);
	if(i < 0)
		return;
	float keyTime = m_subTracks[i]->GetKeyTime(key);
	// In the case of compound tracks, animators want to 
	// select all keys of the same time in the sub-tracks together.
	const float timeEpsilon = 0.001f;
	for(int k=0; k<m_nDimensions; ++k)
	{
		for(int m=0; m<m_subTracks[k]->GetNumKeys(); ++m)
		{
			if(fabs(m_subTracks[k]->GetKeyTime(m) - keyTime) < timeEpsilon)
			{
				m_subTracks[k]->SelectKey(m, select);
				break;
			}
		}
	}
}

int CCompoundSplineTrack::NextKeyByTime(int key) const
{
	assert( key >= 0 && key < GetNumKeys() );
	float time = GetKeyTime(key);
	int count = 0, result = -1;
	float timeNext = FLT_MAX;
	for(int i=0; i<GetSubTrackCount(); ++i)
	{
		for(int k=0; k<m_subTracks[i]->GetNumKeys(); ++k)
		{
			float t = m_subTracks[i]->GetKeyTime(k);
			if(t > time)
			{
				if(t < timeNext)
				{
					timeNext = t;
					result = count + k;
				}
				break;
			}
		}
		count += m_subTracks[i]->GetNumKeys();
	}
	return result;
}