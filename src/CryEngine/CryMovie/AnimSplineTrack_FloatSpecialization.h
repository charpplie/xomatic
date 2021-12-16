//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File Name        : AnimSplineTrack_FloatSpecialization.h
//  Author           : Jaewon Jung
//  Time of creation : 12/18/2009   16:19
//  Compilers        : VS2008
//  Description      : 'float' explicit specialization of the class template
//										 'TAnimSplineTrack'
//  Notice           : Should be included in AnimSplineTrack.h only
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

template <>
inline TAnimSplineTrack<float>::TAnimSplineTrack()
{
	AllocSpline();
	m_flags = 0;
	m_defaultValue = 0;
}
template <> inline void TAnimSplineTrack<float>::GetValue( float time,float &value ) 
{ 
	if(GetNumKeys() == 0)
		value = m_defaultValue;
	else
		m_spline->interpolate(time,value); 
}
template <> inline EAnimTrackType TAnimSplineTrack<float>::GetType() { return ATRACK_TCB_FLOAT; }
template <> inline EAnimValue TAnimSplineTrack<float>::GetValueType() { return AVALUE_FLOAT; }
template <>
inline void TAnimSplineTrack<float>::SetValue( float time,const float &value,bool bDefault )
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

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimSplineTrack<float>::GetKeyInfo( int index,const char* &description,float &duration )
{
	duration = 0;

	static char str[64];
	description = str;
	assert( index >= 0 && index < GetNumKeys() );
	Spline::key_type &k = m_spline->key(index);
	sprintf_s( str,"%.2f",k.value );
}