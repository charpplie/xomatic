#ifndef __LMG_Info__Test__h__
#define __LMG_Info__Test__h__

#include "..\LMGEditor/UiMotionParamMappings.h"
#include "ICryAnimation.h"

struct CLMGInfo_Test_Control
{
	CString name;
	int x;
	int y;
	int width;
	int height;

	typedef std::pair< IUiMotionParamMapping::Mapping, EMotionParamID > MappingInfo;
	std::vector< MappingInfo > mappings;

	CLMGInfo_Test_Control()
		: name( "" )
		, x( 0 )
		, y( 0 )
		, width( 128 )
		, height( 128 )
	{}

	virtual bool ParseFromXmlNode( XmlNodeRef node );
};

struct CLMGInfo_Test_Joystick
	: public CLMGInfo_Test_Control
{
	float defaultX;
	float defaultY;

	float lengthMin;
	float lengthMax;

	CLMGInfo_Test_Joystick()
		: defaultX( 0 )
		, defaultY( 0 )
		, lengthMin( 0 )
		, lengthMax( 1 )
	{}

	virtual bool ParseFromXmlNode( XmlNodeRef node );
};

struct CLMGInfo_Test_Slider
	: public CLMGInfo_Test_Control
{
	float defaultX;
	float defaultY;
	float minX;
	float minY;
	float maxX;
	float maxY;

	CLMGInfo_Test_Slider()
		: defaultX( 0 )
		, defaultY( 0 )
		, minX( -1 )
		, minY( -1 )
		, maxX( 1 )
		, maxY( 1 )
	{}

	virtual bool ParseFromXmlNode( XmlNodeRef node );
};

class CLMGInfo_Test
{
public:
	CLMGInfo_Test();
	virtual ~CLMGInfo_Test();

	const CString& GetId() const;
	const CString& GetName() const;

	bool ParseFromXmlNode( XmlNodeRef node );

	size_t GetJoystickCount() const;
	CLMGInfo_Test_Joystick* GetJoystick( size_t index );

	size_t GetSliderCount() const;
	CLMGInfo_Test_Slider* GetSlider( size_t index );

private:
	CString m_id;
	CString m_name;

	std::vector< CLMGInfo_Test_Joystick > m_joysticks;
	std::vector< CLMGInfo_Test_Slider > m_sliders;
};

#endif