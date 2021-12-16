////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   CompoundSplineTrack.h
//  Created:     16/4/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CompoundSplineTrack_h__
#define __CompoundSplineTrack_h__
#pragma once

#define MAX_SUBTRACKS 4

//////////////////////////////////////////////////////////////////////////
class CCompoundSplineTrack : public IAnimTrack
{
public:
	CCompoundSplineTrack( int nDims,EAnimTrackType inType,EAnimValue inValueType, 
												AnimParamType subTrackParamTypes[MAX_SUBTRACKS] );

	virtual int GetSubTrackCount() const { return m_nDimensions; };
	virtual IAnimTrack* GetSubTrack( int nIndex ) const;
	virtual const char* GetSubTrackName(int nIndex) const;
	virtual void SetSubTrackName(int nIndex, const char *name);

	virtual EAnimTrackType GetType() { return m_type; };
	virtual EAnimValue GetValueType() { return m_valueType; };
	virtual ColorB GetColor() const { return ColorB(90,150,90); }

	virtual AnimParamType  GetParameterType() const { return m_nParamType; };
	virtual void SetParameterType( AnimParamType type ) { m_nParamType = type; }

	//////////////////////////////////////////////////////////////////////////
	virtual void Release() { if (--m_nRefCounter <= 0) delete this; }
	//////////////////////////////////////////////////////////////////////////

	virtual int GetNumKeys() const;
	virtual void SetNumKeys( int numKeys ) { assert(0); };
	virtual void RemoveKey( int num );

	virtual void GetKeyInfo( int key,const char* &description,float &duration );
	virtual int CreateKey( float time ) { assert(0); return 0; };
	virtual int CloneKey( int fromKey ) { assert(0); return 0; };
	virtual int CopyKey( IAnimTrack *pFromTrack, int nFromKey ) { assert(0); return 0; };
	virtual void GetKey( int index,IKey *key ) { assert(0); };
	virtual float GetKeyTime( int index ) const;
	virtual int FindKey( float time ) { assert(0); return 0; };
	virtual int GetKeyFlags( int index ) { assert(0); return 0; };
	virtual void SetKey( int index,IKey *key ) { assert(0); };
	virtual void SetKeyTime( int index,float time );
	virtual void SetKeyFlags( int index,int flags ) { assert(0); };
	virtual void SortKeys() { assert(0); };
	
	virtual bool IsKeySelected(int key) const;
	virtual void SelectKey(int key, bool select);

	virtual int GetFlags() { return m_flags; };
	virtual void SetFlags( int flags ) { m_flags = flags; };

	//////////////////////////////////////////////////////////////////////////
	// Get track value at specified time.
	// Interpolates keys if needed.
	//////////////////////////////////////////////////////////////////////////
	virtual void GetValue( float time,float &value );
	virtual void GetValue( float time,Vec3 &value );
	void GetValue( float time,Vec4 &value );
	virtual void GetValue( float time,Quat &value );
	virtual void GetValue( float time,bool &value ) { assert(0); };

	//////////////////////////////////////////////////////////////////////////
	// Set track value at specified time.
	// Adds new keys if required.
	//////////////////////////////////////////////////////////////////////////
	virtual void SetValue( float time,const float &value,bool bDefault=false );
	virtual void SetValue( float time,const Vec3 &value,bool bDefault=false );
	void SetValue( float time,const Vec4 &value,bool bDefault=false );
	virtual void SetValue( float time,const Quat &value,bool bDefault=false );
	virtual void SetValue( float time,const bool &value,bool bDefault=false ) { assert(0); };

	virtual void OffsetKeyPosition( const Vec3 &value );

	virtual void SetTimeRange( const Range &timeRange );

	virtual bool Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks=true );

	virtual bool SerializeSelection( XmlNodeRef &xmlNode,bool bLoading, bool bCopySelected=false,float fTimeOffset=0);

	virtual void SaveToColladaInFixedFPS( XmlNodeRef libraryAnimationsNode, 
								const char *nodeName, const char *trackName, float fps );

	virtual void SetSelected( bool bSelect );

	virtual int NextKeyByTime(int key) const;

	void SetSubTrackName(const int i, const string& name) { assert (i < MAX_SUBTRACKS); m_subTrackNames[i] = name;}

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		for( int i = 0 ; i < MAX_SUBTRACKS ; ++i)
		{
			pSizer->AddObject(m_subTrackNames[i]);
			pSizer->AddObject(m_subTracks[i]);
		}
	}
protected:
	EAnimTrackType m_type;
	EAnimValue m_valueType;
	int m_nDimensions;
	_smart_ptr<IAnimTrack> m_subTracks[MAX_SUBTRACKS];
	int m_flags;
	AnimParamType m_nParamType;
	Range m_timeRange;
	string m_subTrackNames[MAX_SUBTRACKS];

	void PrepareNodeForSubTrackSerialization(XmlNodeRef &subTrackNode, XmlNodeRef &xmlNode, int i, bool bLoading);
	float PreferShortestRotPath( float degree, float degree0 ) const;
	int GetSubTrackIndex(int& key) const;
};

#endif //__CompoundSplineTrack_h__
