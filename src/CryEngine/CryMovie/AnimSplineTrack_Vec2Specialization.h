//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File Name        : AnimSplineTrack_Vec2Specialization.h
//  Author           : Jaewon Jung
//  Time of creation : 12/18/2009   16:19
//  Compilers        : VS2008
//  Description      : 'Vec2' explicit specialization of the class template
//										 'TAnimSplineTrack'
//  Notice           : Should be included in AnimSplineTrack.h only
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

template <>
inline TAnimSplineTrack<Vec2>::TAnimSplineTrack()
{
	AllocSpline();
	m_flags = 0;
	m_defaultValue = Vec2(0,0);
}
template <> inline void TAnimSplineTrack<Vec2>::GetValue( float time,float &value ) 
{ 
	if(GetNumKeys() == 0)
		value = m_defaultValue.y;
	else
	{
		Spline::ValueType tmp;
		m_spline->Interpolate(time,tmp);
		value = tmp[0];
	}
}
template <> inline EAnimTrackType TAnimSplineTrack<Vec2>::GetType() { return ATRACK_BEZIER_FLOAT; }
template <> inline EAnimValue TAnimSplineTrack<Vec2>::GetValueType() { return AVALUE_FLOAT; }
template <>
inline void TAnimSplineTrack<Vec2>::SetValue( float time,const float &value,bool bDefault )
{
	if (!bDefault)
	{
		I2DBezierKey key;
		key.value = Vec2(time, value);
		SetKeyAtTime( time,&key );
	}
	else
		m_defaultValue = Vec2(time, value);
}

template <>
inline void TAnimSplineTrack<Vec2>::GetKey( int index,IKey *key )
{
	assert( index >= 0 && index < GetNumKeys() );
	assert( key != 0 );
	Spline::key_type &k = m_spline->key(index);
	I2DBezierKey *bezierkey = (I2DBezierKey*)key;
	bezierkey->time = k.time;
	bezierkey->flags = k.flags;

	bezierkey->value = k.value;
}

template <>
inline void TAnimSplineTrack<Vec2>::SetKey( int index,IKey *key )
{
	assert( index >= 0 && index < GetNumKeys() );
	assert( key != 0 );
	Spline::key_type &k = m_spline->key(index);
	I2DBezierKey *bezierkey = (I2DBezierKey*)key;
	k.time = bezierkey->time;
	k.flags = bezierkey->flags;
	k.value = bezierkey->value;
	Invalidate();
}

//! Create key at given time, and return its index.
template <>
inline int TAnimSplineTrack<Vec2>::CreateKey( float time )
{
	float value;

	int nkey = GetNumKeys();

	if (nkey > 0)
		GetValue( time,value );
	else
		value = m_defaultValue.y;

	Spline::ValueType tmp;
	tmp[0] = value;
	tmp[1] = 0;
	return m_spline->InsertKey(time, tmp);
}

template <>
inline int TAnimSplineTrack<Vec2>::CopyKey( IAnimTrack *pFromTrack, int nFromKey )
{
	// This small time offset is applied to prevent the generation of singular tangents.
	float timeOffset = 0.01f;
	return CreateKey(pFromTrack->GetKeyTime(nFromKey) + timeOffset);
}

template <>
inline bool TAnimSplineTrack<Vec2>::Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks )
{
	if (bLoading)
	{
		int num = xmlNode->getChildCount();

		int flags = m_flags;
		xmlNode->getAttr( "Flags",flags );
		xmlNode->getAttr( "defaultValue",m_defaultValue );
		SetFlags( flags );

		SetNumKeys( num );
		for (int i = 0; i < num; i++)
		{
			I2DBezierKey key; // Must be inside loop.

			XmlNodeRef keyNode = xmlNode->getChild(i);
			if (!keyNode->getAttr( "time",key.time ))
			{
				CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing time information.");
				return false;
			}
			if (!keyNode->getAttr( "value",key.value ))
			{
				CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing value information.");
				return false;
			}
			assert(key.time == key.value.x);

			keyNode->getAttr( "flags",key.flags );

			SetKey( i,&key );

			// In-/Out-tangent
			if (!keyNode->getAttr("ds", m_spline->key(i).ds))
			{
				CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing ds spline information.");
				return false;
			}

			if (!keyNode->getAttr("dd", m_spline->key(i).dd))
			{
				CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:dd spline information.");
				return false;
			}
		}

		if ((!num) && (!bLoadEmptyTracks))
			return false;
	}
	else
	{
		int num = GetNumKeys();
		xmlNode->setAttr( "Flags",GetFlags() );
		xmlNode->setAttr( "defaultValue",m_defaultValue );
		I2DBezierKey key;
		for (int i = 0; i < num; i++)
		{
			GetKey( i,&key );
			XmlNodeRef keyNode = xmlNode->newChild( "Key" );
			assert(key.time == key.value.x);
			keyNode->setAttr( "time",key.time );
			keyNode->setAttr( "value",key.value );

			int flags = key.flags;
			// Just save the in/out/unify mask part. Others are for editing convenience.
			flags &= (SPLINE_KEY_TANGENT_IN_MASK|SPLINE_KEY_TANGENT_OUT_MASK|SPLINE_KEY_TANGENT_UNIFY_MASK);
			if(flags != 0)
				keyNode->setAttr( "flags",flags );

			// We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
			keyNode->setAttr("ds", m_spline->key(i).ds);
			keyNode->setAttr("dd", m_spline->key(i).dd);
		}
	}
	return true;
}

template <>
inline bool TAnimSplineTrack<Vec2>::SerializeSelection( XmlNodeRef &xmlNode,bool bLoading, bool bCopySelected,float fTimeOffset)
{
	if (bLoading)
	{
		int numCur = GetNumKeys();
		int num = xmlNode->getChildCount();

		int type;
		xmlNode->getAttr( "TrackType", type);

		if(type!=GetType())
			return false;

		SetNumKeys( num + numCur);
		for (int i = 0; i < num; i++)
		{
			I2DBezierKey key; // Must be inside loop.

			XmlNodeRef keyNode = xmlNode->getChild(i);
			keyNode->getAttr( "time",key.time );
			keyNode->getAttr( "value",key.value );
			assert(key.time == key.value.x);
			key.time += fTimeOffset;
			key.value.x += fTimeOffset;

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

		I2DBezierKey key;
		for (int i = 0; i < num; i++)
		{
			GetKey( i,&key );
			assert(key.time == key.value.x);

			if(!bCopySelected || IsKeySelected(i))
			{
				XmlNodeRef keyNode = xmlNode->newChild( "Key" );
				keyNode->setAttr( "time",key.time );
				keyNode->setAttr( "value",key.value );

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

template <>
inline void TAnimSplineTrack<Vec2>::SaveToColladaInFixedFPS( XmlNodeRef libraryAnimationsNode, const char *nodeName, const char *trackName, float fps )
{
	assert(fps > 0);
	if (fps <= 0 || GetNumKeys() == 0)
		return;

	/// Prepare data
	string inputData, outputData, interpData, timeString, valueString;
	int count = 0;
	float frameTime  = 1.0f/fps;
	float startTime = GetKeyTime(0);
	float endTime = GetKeyTime(GetNumKeys() - 1);
	for(float t=startTime; t <= endTime; t += frameTime, ++count)
	{
		float value;
		GetValue(t, value);
		if(count > 0)
		{
			inputData += " ";
			outputData += " ";
			interpData += " ";
		}
		timeString.Format("%f", t);
		valueString.Format("%f", value);
		inputData += timeString;
		outputData += valueString;
		interpData += "LINEAR";
	}
		
	/// <animation>
	XmlNodeRef sourceNode, arrayNode, techniqueNode, accessorNode, paramNode, samplerNode, inputNode, channelNode;
	string id, array, source, target;
	XmlNodeRef animationNode = libraryAnimationsNode->newChild("animation");
	animationNode->setAttr("id", trackName);
		// input source
		sourceNode = animationNode->newChild("source");
		id.Format("%s-input", trackName);
		sourceNode->setAttr("id", id.c_str());
			arrayNode = sourceNode->newChild("float_array");
			arrayNode->setAttr("count", count);
			array.Format("%s-array", id.c_str());
			arrayNode->setAttr("id", array.c_str());
			arrayNode->setContent(inputData.c_str());
			techniqueNode = sourceNode->newChild("technique_common");
				accessorNode = techniqueNode->newChild("accessor");
				accessorNode->setAttr("source", array.c_str());
				accessorNode->setAttr("count", count);
				accessorNode->setAttr("stride", 1);
					paramNode = accessorNode->newChild("param");
					paramNode->setAttr("name", "TIME");
					paramNode->setAttr("type", "float");
		// output source
		sourceNode = animationNode->newChild("source");
		id.Format("%s-output", trackName);
		sourceNode->setAttr("id", id.c_str());
			arrayNode = sourceNode->newChild("float_array");
			arrayNode->setAttr("count", count);
			array.Format("%s-array", id.c_str());
			arrayNode->setAttr("id", array.c_str());
			arrayNode->setContent(outputData.c_str());
			techniqueNode = sourceNode->newChild("technique_common");
				accessorNode = techniqueNode->newChild("accessor");
				accessorNode->setAttr("source", array.c_str());
				accessorNode->setAttr("count", count);
				accessorNode->setAttr("stride", 1);
					paramNode = accessorNode->newChild("param");
					paramNode->setAttr("name", "VALUE");
					paramNode->setAttr("type", "float");
		// interpolation source
		sourceNode = animationNode->newChild("source");
		id.Format("%s-interp", trackName);
		sourceNode->setAttr("id", id.c_str());
			arrayNode = sourceNode->newChild("Name_array");
			arrayNode->setAttr("count", count);
			array.Format("%s-array", id.c_str());
			arrayNode->setAttr("id", array.c_str());
			arrayNode->setContent(interpData.c_str());
			techniqueNode = sourceNode->newChild("technique_common");
				accessorNode = techniqueNode->newChild("accessor");
				accessorNode->setAttr("source", array.c_str());
				accessorNode->setAttr("count", count);
				accessorNode->setAttr("stride", 1);
					paramNode = accessorNode->newChild("param");
					paramNode->setAttr("name", "INTERPOLATION");
					paramNode->setAttr("type", "Name");

		// sampler
		samplerNode = animationNode->newChild("sampler");
		id.Format("%s-sampler", trackName);
		samplerNode->setAttr("id", id.c_str());
			inputNode = samplerNode->newChild("input");
			inputNode->setAttr("semantic", "INPUT");
			id.Format("#%s-input", trackName);
			inputNode->setAttr("source", id.c_str());
			inputNode = samplerNode->newChild("input");
			inputNode->setAttr("semantic", "OUTPUT");
			id.Format("#%s-output", trackName);
			inputNode->setAttr("source", id.c_str());
			inputNode = samplerNode->newChild("input");
			inputNode->setAttr("semantic", "INTERPOLATION");
			id.Format("#%s-interp", trackName);
			inputNode->setAttr("source", id.c_str());

		// channel
		channelNode = animationNode->newChild("channel");
		source.Format("#%s-sampler", trackName);
		channelNode->setAttr("source", source.c_str());
		target = nodeName;
		// So, at the moment, only following param types are formally supported.
		switch(GetParameterType())
		{
		case APARAM_POS_X:
			target += "/translate.X";
			break;
		case APARAM_POS_Y:
			target += "/translate.Y";
			break;
		case APARAM_POS_Z:
			target += "/translate.Z";
			break;
		case APARAM_ROT_X:
			target += "/rotateX.ANGLE";
			break;
		case APARAM_ROT_Y:
			target += "/rotateY.ANGLE";
			break;
		case APARAM_ROT_Z:
			target += "/rotateZ.ANGLE";
			break;
		case APARAM_FOV:
			target += "/yfov";
			break;
		}
		channelNode->setAttr("target", target.c_str());
}

//////////////////////////////////////////////////////////////////////////
template<>
inline bool TAnimSplineTrack<Vec2>::LoadFromCollada( XmlNodeRef animationNode )
{
	/// 1. Get the animation data.
	// 1.1 Find the input/output source names.
	XmlString inputSrcName, outputSrcName;
	XmlNodeRef samplerNode = animationNode->findChild("sampler");
	if (!samplerNode)
		return false;
	for (int i=0; i<samplerNode->getChildCount(); ++i)
	{
		XmlNodeRef inputNode = samplerNode->getChild(i);
		if (strcmp(inputNode->getTag(), "input") != 0)
			continue;
		XmlString semantic;
		if (inputNode->getAttr("semantic", semantic) == false)
			continue;
		if (semantic == "INPUT")
		{
			inputNode->getAttr("source", inputSrcName);
			// Remove the '#' on head.
			inputSrcName = inputSrcName.substr(1).c_str();
		}
		else if (semantic == "OUTPUT")
		{
			inputNode->getAttr("source", outputSrcName);
			// Remove the '#' on head.
			outputSrcName = outputSrcName.substr(1).c_str();
		}
	}
	// 1.2 Get actual input/output sources.
	XmlString inputData, outputData;
	int inputCount=0, outputCount=0;
	for (int i=0; i<animationNode->getChildCount(); ++i)
	{
		XmlNodeRef sourceNode = animationNode->getChild(i);
		if (strcmp(sourceNode->getTag(), "source") != 0)
			continue;
		XmlString id;
		sourceNode->getAttr("id", id);
		if (id == inputSrcName)
		{
			XmlNodeRef floatArrayNode = sourceNode->findChild("float_array");
			if (!floatArrayNode)
				return false;
			inputData = floatArrayNode->getContent();
			floatArrayNode->getAttr("count", inputCount);
		}
		else if (id == outputSrcName)
		{
			XmlNodeRef floatArrayNode = sourceNode->findChild("float_array");
			if (!floatArrayNode)
				return false;
			outputData = floatArrayNode->getContent();
			floatArrayNode->getAttr("count", outputCount);
		}
	}
	if (inputData.empty() || outputData.empty()
	|| (inputCount != outputCount))
		return false;

	/// 2. Parse them.
	std::vector<float> times, values;
	times.resize(inputCount);
	values.resize(outputCount);
	int pos = 0, prevPos = 0, count = 0;
	XmlString valueToken;
	// 2.1 Parse key times.
	const int VALID_DIGIT_COUNT_IN_MAX = 6;
	while (!(valueToken = inputData.Tokenize(" \r\n", pos).c_str()).empty())
	{
		// Max COLLADA exporter outputs the decimal point as ','.
		// So we have to replace it with '.'.
		valueToken.replace(',', '.');
		int decimalPos = valueToken.find_first_of('.');
		if (decimalPos > 0)
		{
			// Furthermore, it outputs float array to multiple lines, but
			// unfortunately our XML getContent() method removes all those new lines
			// return the rest.
			// So the hacky check and adjusting below are necessary.
			if (valueToken.length() != decimalPos + VALID_DIGIT_COUNT_IN_MAX + 1)
				pos = prevPos + (decimalPos + VALID_DIGIT_COUNT_IN_MAX + 1);
		}
		times[count] = (float)atof(valueToken.c_str());
		++count;
		prevPos = pos;
	}
	if (count != inputCount)
		return false;
	// 2.2 Parse key value.
	pos = prevPos = count = 0;
	while (!(valueToken = outputData.Tokenize(" \r\n", pos).c_str()).empty())
	{
		// Max COLLADA exporter outputs the decimal point as ','.
		// So we have to replace it with '.'.
		valueToken.replace(',', '.');
		int decimalPos = valueToken.find_first_of('.');
		if (decimalPos > 0)
		{
			// Furthermore, it outputs float array to multiple lines, but
			// unfortunately our XML getContent() method removes all those new lines
			// return the rest.
			// So the hacky check and adjusting below are necessary.
			if (valueToken.length() != decimalPos + VALID_DIGIT_COUNT_IN_MAX + 1)
				pos = prevPos + (decimalPos + VALID_DIGIT_COUNT_IN_MAX + 1);
		}
		values[count] = (float)atof(valueToken.c_str());
		++count;
		prevPos = pos;
	}
	if (count != outputCount)
		return false;

	/// 3. Add them to this track.
	for (int i=0; i<inputCount; ++i)
	{
		SetValue(times[i], values[i], false);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimSplineTrack<Vec2>::GetKeyInfo( int index,const char* &description,float &duration )
{
	duration = 0;

	static char str[64];
	description = str;
	assert( index >= 0 && index < GetNumKeys() );
	Spline::key_type &k = m_spline->key(index);
	sprintf_s( str,"%.2f",k.value.y );
}
