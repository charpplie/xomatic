////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   animsplinetrack.h
//  Version:     v1.00
//  Created:     22/4/2002 by Timur.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __animsplinetrack_h__
#define __animsplinetrack_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IMovieSystem.h"
#include "TCBSpline.h"
#include "2DSpline.h"

#define MIN_TIME_PRECISION 0.01f

/*!
		Templated class that used as a base for all Tcb spline tracks.
 */
template <class ValueType>
class TAnimSplineTrack : public IAnimTrack
{
public:
	TAnimSplineTrack()
	{
		AllocSpline();
		m_flags = 0;
	}
	~TAnimSplineTrack()
	{
		delete m_spline;
	}
	virtual void Release() { if (--m_nRefCounter <= 0) delete this; }

	virtual int GetSubTrackCount() const { return 0; };
	virtual IAnimTrack* GetSubTrack( int nIndex ) const { return 0; };
	virtual const char* GetSubTrackName(int nIndex) const { return NULL; };
	virtual void SetSubTrackName(int nIndex, const char *name) { assert(0); }
	
	virtual AnimParamType  GetParameterType() const { return m_nParamType; };
	virtual void SetParameterType( AnimParamType type ) { m_nParamType = type; };

	ISplineInterpolator* GetSpline() const { return m_spline; };

	virtual bool IsKeySelected(int key) const
	{
		if(GetSpline() && GetSpline()->IsKeySelectedAtAnyDimension(key))
			return true;
		return false;
	}

	virtual void SelectKey(int key, bool select)
	{
		if(GetSpline())
			GetSpline()->SelectKeyAllDimensions(key, select);
	}

	int GetNumKeys() const
	{
		return m_spline->num_keys();
	}

	void SetNumKeys( int numKeys )
	{
		m_spline->resize(numKeys);
	}

	void RemoveKey( int num )
	{
		if(m_spline && m_spline->num_keys() > num)
			m_spline->erase(num);
		else
			assert(0);
	}

	void GetKey( int index,IKey *key )
	{
		assert( index >= 0 && index < GetNumKeys() );
		assert( key != 0 );
		typename Spline::key_type &k = m_spline->key(index);
		ITcbKey *tcbkey = (ITcbKey*)key;
		tcbkey->time = k.time;
		tcbkey->flags = k.flags;
		
		tcbkey->tens = k.tens;
		tcbkey->cont = k.cont;
		tcbkey->bias = k.bias;
		tcbkey->easeto = k.easeto;
		tcbkey->easefrom = k.easefrom;

		tcbkey->SetValue( k.value );
	}

	void SetKey( int index,IKey *key )
	{
		assert( index >= 0 && index < GetNumKeys() );
		assert( key != 0 );
		typename Spline::key_type &k = m_spline->key(index);
		ITcbKey *tcbkey = (ITcbKey*)key;
		k.time = tcbkey->time;
		k.flags = tcbkey->flags;
		k.tens = tcbkey->tens;
		k.cont = tcbkey->cont;
		k.bias = tcbkey->bias;
		k.easeto = tcbkey->easeto;
		k.easefrom = tcbkey->easefrom;
		tcbkey->GetValue( k.value );
		Invalidate();
	}

	float GetKeyTime( int index ) const
	{
		assert( index >= 0 && index < GetNumKeys() );
		return m_spline->time(index);
	}
	void SetKeyTime( int index,float time )
	{
		assert( index >= 0 && index < GetNumKeys() );
		m_spline->SetKeyTime(index, time);
		Invalidate();
	}
	int GetKeyFlags( int index )
	{
		assert( index >= 0 && index < GetNumKeys() );
		return m_spline->key(index).flags;
	}
	void SetKeyFlags( int index,int flags )
	{
		assert( index >= 0 && index < GetNumKeys() );
		m_spline->key(index).flags = flags;
	}

	virtual ColorB GetColor() const { return ColorB(180,200,180); }

	virtual EAnimTrackType GetType() { assert(0); return 0; }
	virtual EAnimValue GetValueType() { assert(0); return 0; }

	virtual void GetValue( float time,float &value ) { assert(0); }
	virtual void GetValue( float time,Vec3 &value ) { assert(0); }
	virtual void GetValue( float time,Quat &value ) { assert(0); }
	virtual void GetValue( float time,bool &value ) { assert(0); }

	virtual void SetValue( float time,const float &value,bool bDefault=false ) { assert(0); }
	virtual void SetValue( float time,const Vec3 &value,bool bDefault=false ) { assert(0); }
	virtual void SetValue( float time,const Quat &value,bool bDefault=false ) { assert(0); }
	virtual void SetValue( float time,const bool &value,bool bDefault=false ) { assert(0); }

	virtual void OffsetKeyPosition( const Vec3 &value ) { assert(0); };

	bool Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks );
	bool SerializeSelection( XmlNodeRef &xmlNode,bool bLoading, bool bCopySelected,float fTimeOffset);
	virtual void SaveToColladaInFixedFPS( XmlNodeRef libraryAnimationsNode, const char *nodeName, const char *trackName, float fps ) {}
	virtual bool LoadFromCollada( XmlNodeRef animationNode ) { return true; }

	virtual void SetSelected( bool bSelect ) { if (bSelect) m_flags |= ATRACK_SELECTED; else m_flags &= ~ATRACK_SELECTED; }

	void GetKeyInfo( int key,const char* &description,float &duration )
	{
		description = 0;
		duration = 0;
	}

	//! Sort keys in track (after time of keys was modified).
	void SortKeys() {
		m_spline->sort_keys();
	};

	//! Get track flags.
	int GetFlags() { return m_flags; };
	
	//! Set track flags.
	void SetFlags( int flags )
	{
		m_flags = flags;
		if (m_flags & ATRACK_LOOP)
		{
			m_spline->ORT( Spline::ORT_LOOP );
		}
		else if (m_flags & ATRACK_CYCLE)
		{
			m_spline->ORT( Spline::ORT_CYCLE );
		}
		else
		{
			m_spline->ORT( Spline::ORT_CONSTANT );
		}
		if (m_flags & ATRACK_LINEAR)
		{
			//m_spline->flag_set( ORT_LINEAR );
		}
	}

	void Invalidate() {
		m_spline->flag_set( Spline::MODIFIED );
	};

	void SetTimeRange( const Range &timeRange )
	{
		m_spline->SetRange( timeRange.start,timeRange.end );
	}

	int FindKey( float time )
	{
		// Find key with given time.
		int num = m_spline->num_keys();
		for (int i = 0; i < num; i++)
		{
			float keyt = m_spline->key(i).time;
			if (fabs(keyt-time) < MIN_TIME_PRECISION)
			{
				return i;
			}
		}
		return -1;
	}

	//! Create key at given time, and return its index.
	int CreateKey( float time )
	{
		ValueType value;
		
		int nkey = GetNumKeys();

		if (nkey > 0)
			GetValue( time,value );
		else
			value = m_defaultValue;

		typename Spline::ValueType tmp;
		m_spline->ToValueType(value, tmp);
		return m_spline->InsertKey(time, tmp);
	}

	int CloneKey( int srcKey )
	{
		return CopyKey(this, srcKey);
	}

	int CopyKey( IAnimTrack *pFromTrack, int nFromKey )
	{
		ITcbKey key;
		pFromTrack->GetKey( nFromKey,&key );
		int nkey = GetNumKeys();
		SetNumKeys( nkey+1 );
		SetKey( nkey,&key );
		return nkey;
	}

	//! Get key at given time,
	//! If key not exist adds key at this time.
	void SetKeyAtTime( float time,IKey *key )
	{
		assert( key != 0 );

		key->time = time;
		
		bool found = false;
		// Find key with given time.
		for (int i = 0; i < m_spline->num_keys(); i++)
		{
			float keyt = m_spline->key(i).time;
			if (fabs(keyt-time) < MIN_TIME_PRECISION)
			{
				key->flags = m_spline->key(i).flags;				// Reserve the flag value.
				SetKey( i,key );
				found = true;
				break;
			}
			//if (keyt > time)
				//break;
		}
		if (!found)
		{
			// Key with this time not found.
			// Create a new one.
			int keyIndex = CreateKey(time);
			// Reserve the flag value.
			key->flags = m_spline->key(keyIndex).flags;		// Reserve the flag value.
			SetKey( keyIndex,key );
		}
	}

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_spline);
	}
private:
	//! Spawns new instance of Tcb spline.
	void AllocSpline()
	{
		m_spline = new spline::TrackSplineInterpolator<ValueType>;
	}

	typedef spline::TrackSplineInterpolator<ValueType> Spline;
	Spline* m_spline;
	ValueType m_defaultValue;

	//! Keys of float track.
	int m_flags;
	AnimParamType m_nParamType;
};

template <class T>
inline bool TAnimSplineTrack<T>::Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks )
{
	if (bLoading)
	{
		int num = xmlNode->getChildCount();

		int flags = m_flags;
		xmlNode->getAttr( "Flags",flags );
		xmlNode->getAttr( "defaultValue",m_defaultValue );
		SetFlags( flags );

		T value;
		SetNumKeys( num );
		for (int i = 0; i < num; i++)
		{
			ITcbKey key; // Must be inside loop.

			XmlNodeRef keyNode = xmlNode->getChild(i);
			keyNode->getAttr( "time",key.time );
			
			if (keyNode->getAttr( "value",value ))
				key.SetValue( value );

			keyNode->getAttr( "tens",key.tens );
			keyNode->getAttr( "cont",key.cont );
			keyNode->getAttr( "bias",key.bias );
			keyNode->getAttr( "easeto",key.easeto );
			keyNode->getAttr( "easefrom",key.easefrom );
			keyNode->getAttr( "flags",key.flags );

			SetKey( i,&key );

			// In-/Out-tangent
			keyNode->getAttr("ds", m_spline->key(i).ds);
			keyNode->getAttr("dd", m_spline->key(i).dd);
		}

		if ((!num) && (!bLoadEmptyTracks))
			return false;
	}
	else
	{
		int num = GetNumKeys();
		xmlNode->setAttr( "Flags",GetFlags() );
		xmlNode->setAttr( "defaultValue",m_defaultValue );
		ITcbKey key;
		T value;
		for (int i = 0; i < num; i++)
		{
			GetKey( i,&key );
			XmlNodeRef keyNode = xmlNode->newChild( "Key" );
			keyNode->setAttr( "time",key.time );
			
			key.GetValue( value );
			keyNode->setAttr( "value",value );

			if (key.tens != 0)
				keyNode->setAttr( "tens",key.tens );
			if (key.cont != 0)
				keyNode->setAttr( "cont",key.cont );
			if (key.bias != 0)
				keyNode->setAttr( "bias",key.bias );
			if (key.easeto != 0)
				keyNode->setAttr( "easeto",key.easeto );
			if (key.easefrom != 0)
				keyNode->setAttr( "easefrom",key.easefrom );

			int flags = key.flags;
			// Just save the in/out mask part. Others are for editing convenience.
			flags &= (SPLINE_KEY_TANGENT_IN_MASK|SPLINE_KEY_TANGENT_OUT_MASK);
			if(flags != 0)
				keyNode->setAttr( "flags",flags );

			// We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
			keyNode->setAttr("ds", m_spline->key(i).ds);
			keyNode->setAttr("dd", m_spline->key(i).dd);
		}
	}
	return true;
}

template <class T>
inline bool TAnimSplineTrack<T>::SerializeSelection( XmlNodeRef &xmlNode,bool bLoading, bool bCopySelected,float fTimeOffset)
{
	if (bLoading)
	{
		int numCur = GetNumKeys();
		int num = xmlNode->getChildCount();

		int type;
		xmlNode->getAttr( "TrackType", type);

		if(type!=GetType())
			return false;

		T value;
		SetNumKeys( num + numCur);
		for (int i = 0; i < num; i++)
		{
			ITcbKey key; // Must be inside loop.

			XmlNodeRef keyNode = xmlNode->getChild(i);
			keyNode->getAttr( "time",key.time );
			key.time += fTimeOffset;

			if (keyNode->getAttr( "value",value ))
				key.SetValue( value );

			keyNode->getAttr( "tens",key.tens );
			keyNode->getAttr( "cont",key.cont );
			keyNode->getAttr( "bias",key.bias );
			keyNode->getAttr( "easeto",key.easeto );
			keyNode->getAttr( "easefrom",key.easefrom );
			keyNode->getAttr( "flags",key.flags );

			SetKey( i + numCur, &key );

			if (bCopySelected)
			{
				SelectKey(i+numCur, true);
			}

			// In-/Out-tangent
			keyNode->getAttr("ds", m_spline->key(i+numCur).ds);
			keyNode->getAttr("dd", m_spline->key(i+numCur).dd);
		}
		SortKeys();
	}
	else
	{
		int num = GetNumKeys();
		xmlNode->setAttr( "TrackType",GetType() );

		ITcbKey key;
		T value;
		for (int i = 0; i < num; i++)
		{
			GetKey( i,&key );

			if(!bCopySelected || IsKeySelected(i))
			{
				XmlNodeRef keyNode = xmlNode->newChild( "Key" );
				keyNode->setAttr( "time",key.time );

				key.GetValue( value );
				keyNode->setAttr( "value",value );

				if (key.tens != 0)
					keyNode->setAttr( "tens",key.tens );
				if (key.cont != 0)
					keyNode->setAttr( "cont",key.cont );
				if (key.bias != 0)
					keyNode->setAttr( "bias",key.bias );
				if (key.easeto != 0)
					keyNode->setAttr( "easeto",key.easeto );
				if (key.easefrom != 0)
					keyNode->setAttr( "easefrom",key.easefrom );

				int flags = key.flags;
				// Just save the in/out mask part. Others are for editing convenience.
				flags &= (SPLINE_KEY_TANGENT_IN_MASK|SPLINE_KEY_TANGENT_OUT_MASK);
				if(flags != 0)	
					keyNode->setAttr( "flags",flags );

				// We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
				keyNode->setAttr("ds", m_spline->key(i).ds);
				keyNode->setAttr("dd", m_spline->key(i).dd);
			}
		}
	}
	return true;
}

// This is the current main.
#include "AnimSplineTrack_Vec2Specialization.h"
typedef TAnimSplineTrack<Vec2>		C2DSplineTrack;

// Followings are deprecated and supported only for the backward compatibility.
#include "AnimSplineTrack_FloatSpecialization.h"
#include "AnimSplineTrack_Vec3Specialization.h"
#include "AnimSplineTrack_QuatSpecialization.h"
typedef TAnimSplineTrack<float>	CTcbFloatTrack;
typedef TAnimSplineTrack<Vec3>		CTcbVectorTrack;
typedef TAnimSplineTrack<Quat>		CTcbQuatTrack;

#endif // __animsplinetrack_h__
