// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once
#include "AudioControl.h"
#include "common/ACBTypes.h"


enum EItemType
{
	// order of the enum also means
	// order in which items are sorted
	eIT_FOLDER = 0,
	eIT_SWITCH,
	eIT_ITEM ,
	eIT_INVALID ,
};


// --------- Audio System specific data ---------
struct SAudioSystemControlMimeData
{
	SAudioSystemControlMimeData()
		: m_id(AudioControls::ACB_INVALID_ID)
		, m_type(eIT_INVALID)
		, m_controlType(AudioControls::EACBControlType::eACBT_NUM_TYPES)
		, m_connected(true)
	{}

	EItemType m_type;
	AudioControls::EACBControlType m_controlType;
	AudioControls::CID m_id;
	bool m_connected;
};

Q_DECLARE_METATYPE(EItemType)
Q_DECLARE_METATYPE(SAudioSystemControlMimeData)

// Operators needed to support custom data to be passed with drag and drop
inline QDataStream& operator<<(QDataStream& stream, const SAudioSystemControlMimeData& obj)
{
	stream << (AudioControls::CID)obj.m_id;
	stream << obj.m_type;
	stream << obj.m_controlType;
	stream << obj.m_connected;
	return stream;
}
inline QDataStream& operator >> (QDataStream& stream, SAudioSystemControlMimeData& obj)
{
	stream >> (AudioControls::CID)obj.m_id;
	int type;
	stream >> type;
	obj.m_type = (EItemType)type;
	int controlType;
	stream >> controlType;
	obj.m_controlType = (AudioControls::EACBControlType)controlType;
	stream >> (bool)obj.m_connected;
	return stream;
}

// --------- Audio Implementation specific data ---------
struct SAudioImplControlMimeData
{
	SAudioImplControlMimeData()
		: m_id(AudioControls::ACB_INVALID_ID)
		, m_type(eIT_INVALID)
		, m_controlType(AudioControls::AUDIO_IMPL_INVALID_TYPE)
		, m_connected(true)
		, m_localised(false)
	{}

	AudioControls::CID m_id;
	EItemType m_type;
	AudioControls::TImplControlType m_controlType;
	bool m_localised;
	bool m_connected;
};

Q_DECLARE_METATYPE(SAudioImplControlMimeData)

// Operators needed to support custom data to be passed with drag and drop
inline QDataStream& operator<<(QDataStream& stream, const SAudioImplControlMimeData& obj)
{
	stream << (AudioControls::CID)obj.m_id;
	stream << obj.m_type;
	stream << obj.m_controlType;
	stream << obj.m_connected;
	stream << obj.m_localised;
	return stream;
}
inline QDataStream& operator >> (QDataStream& stream, SAudioImplControlMimeData& obj)
{
	stream >> (AudioControls::CID)obj.m_id;
	int type;
	stream >> type;
	obj.m_type = (EItemType)type;
	stream >> (AudioControls::TImplControlType)obj.m_controlType;
	stream >> (bool)obj.m_connected;
	stream >> (bool)obj.m_localised;
	return stream;
}