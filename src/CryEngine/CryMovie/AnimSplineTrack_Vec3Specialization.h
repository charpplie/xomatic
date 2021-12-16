//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File Name        : AnimSplineTrack_Vec3Specialization.h
//  Author           : Jaewon Jung
//  Time of creation : 12/18/2009   16:19
//  Compilers        : VS2008
//  Description      : 'Vec3' explicit specialization of the class template
//										 'TAnimSplineTrack'
//  Notice           : Should be included in AnimSplineTrack.h only
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

template <>
inline TAnimSplineTrack<Vec3>::TAnimSplineTrack()
{
	AllocSpline();
	m_flags = 0;
	m_defaultValue = Vec3(0,0,0);
}
template <> inline void TAnimSplineTrack<Vec3>::GetValue( float time,Vec3 &value ) 
{ 
	if(GetNumKeys() == 0)
		value = m_defaultValue;
	else
		m_spline->interpolate(time,value); 
}
template <> inline EAnimTrackType TAnimSplineTrack<Vec3>::GetType() { return ATRACK_TCB_VECTOR; }
template <> inline EAnimValue TAnimSplineTrack<Vec3>::GetValueType() { return AVALUE_VECTOR; }
template <>
inline void TAnimSplineTrack<Vec3>::SetValue( float time,const Vec3 &value,bool bDefault )
{
	if (!bDefault)
	{
		ITcbKey key;
		key.SetValue( value );
		SetKeyAtTime( time,&key );
	}
	else
		m_defaultValue = value;
}
template <>
inline void TAnimSplineTrack<Vec3>::OffsetKeyPosition( const Vec3 &offset )
{
	// Iterate over all keys and offet them.
	ITcbKey key;
	for (int i = 0; i < GetNumKeys(); i++)
	{
		// Offset each key.
		GetKey(i,&key);
		key.SetVec3( key.GetVec3()+offset );
		SetKey(i,&key);
	}
}

//////////////////////////////////////////////////////////////////////////
template <>
inline void TAnimSplineTrack<Vec3>::GetKeyInfo( int index,const char* &description,float &duration )
{
	duration = 0;

	static char str[64];
	description = str;

	assert( index >= 0 && index < GetNumKeys() );
	Spline::key_type &k = m_spline->key(index);
	sprintf_s( str,"%.2f  %.2f  %.2f",k.value[0],k.value[1],k.value[2] );
}