//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File Name        : AnimSplineTrack_QuatSpecialization.h
//  Author           : Jaewon Jung
//  Time of creation : 12/18/2009   16:19
//  Compilers        : VS2008
//  Description      : 'Quat' explicit specialization of the class template
//										 'TAnimSplineTrack'
//  Notice           : Should be included in AnimSplineTrack.h only
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

template <> 
inline TAnimSplineTrack<Quat>::TAnimSplineTrack()
{
	AllocSpline();
	//m_spline = (Spline*)new spline::TCBQuatSplineInterpolator;
	m_flags = 0;
	m_defaultValue.SetIdentity();
}

template <> inline void TAnimSplineTrack<Quat>::GetValue( float time,Quat &value ) 
{
	if(GetNumKeys() == 0)
		value = m_defaultValue;
	else
		m_spline->interpolate(time,value); 
}
template <> inline EAnimTrackType TAnimSplineTrack<Quat>::GetType() { return ATRACK_TCB_QUAT; }
template <> inline EAnimValue TAnimSplineTrack<Quat>::GetValueType() { return AVALUE_QUAT; }
template <>
inline void TAnimSplineTrack<Quat>::SetValue( float time,const Quat &value,bool bDefault )
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
template <> 
inline void TAnimSplineTrack<Quat>::GetKeyInfo( int index,const char* &description,float &duration )
{
	duration = 0;

	static char str[64];
	description = str;

	assert( index >= 0 && index < GetNumKeys() );
	Spline::key_type &k = m_spline->key(index);
	Ang3 Angles=RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(k.value)));
	sprintf_s( str,"%.2f  %.2f  %.2f",Angles.x, Angles.y, Angles.z );
}
