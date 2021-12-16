#include "stdafx.h"
#include "LMGEditor/LMGInfo_Test.h"

int FindMappingFromString( const CString& mappingName );
int FindMotionParamIdFromString( const CString& motionParamIdName );

CLMGInfo_Test::CLMGInfo_Test()
{

}

CLMGInfo_Test::~CLMGInfo_Test()
{

}

const CString& CLMGInfo_Test::GetId() const
{
	return m_id;
}

const CString& CLMGInfo_Test::GetName() const
{
	return m_name;
}

size_t CLMGInfo_Test::GetJoystickCount() const
{
	return m_joysticks.size();
}

CLMGInfo_Test_Joystick* CLMGInfo_Test::GetJoystick( size_t index )
{
	return &m_joysticks[ index ];
}

size_t CLMGInfo_Test::GetSliderCount() const
{
	return m_sliders.size();
}

CLMGInfo_Test_Slider* CLMGInfo_Test::GetSlider( size_t index )
{
	return &m_sliders[ index ];
}

bool CLMGInfo_Test::ParseFromXmlNode( XmlNodeRef node )
{
	node->getAttr( "id", m_id );
	node->getAttr( "name", m_name );

	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef childNode = node->getChild( i );

		const char* childName = childNode->getTag();

		if ( strcmpi( childName, "joystick" ) == 0 )
		{
			CLMGInfo_Test_Joystick joystickInfo;
			bool parseSuccess = joystickInfo.ParseFromXmlNode( childNode );
			if ( parseSuccess )
			{
				m_joysticks.push_back( joystickInfo );
			}
		}

		bool generalSlider = ( strcmpi( childName, "slider" ) == 0 );
		bool verticalSlider = ( strcmpi( childName, "vertical_slider" ) == 0 );
		bool horizontalSlider = ( strcmpi( childName, "horizontal_slider" ) == 0 );
		bool slider = ( generalSlider || verticalSlider || horizontalSlider );
		if ( slider )
		{
			CLMGInfo_Test_Slider sliderInfo;
			bool parseSuccess = sliderInfo.ParseFromXmlNode( childNode );

			if ( horizontalSlider )
			{
				sliderInfo.defaultY = 0;
				sliderInfo.minY = 0;
				sliderInfo.maxY = 0;
			}

			if ( verticalSlider )
			{
				sliderInfo.defaultX = 0;
				sliderInfo.minX = 0;
				sliderInfo.maxX = 0;
			}

			if ( parseSuccess )
			{
				m_sliders.push_back( sliderInfo );
			}
		}
	}

	return true;
}


bool CLMGInfo_Test_Control::ParseFromXmlNode( XmlNodeRef node )
{
	node->getAttr( "name", name );
	node->getAttr( "x", x );
	node->getAttr( "y", y );
	node->getAttr( "width", width );
	node->getAttr( "height", height );

	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef child = node->getChild( i );

		CString what;
		CString to;

		bool mappingOk = true;
		mappingOk &= child->getAttr( "what", what );
		mappingOk &= child->getAttr( "to", to );

		if ( mappingOk )
		{
			int whatValue = FindMappingFromString( what );
			int toValue = FindMotionParamIdFromString( to );

			bool validValues = ( whatValue != -1 && toValue != -1 );
			if ( validValues )
			{
				MappingInfo mappingInfo( ( IUiMotionParamMapping::Mapping )( whatValue ), ( EMotionParamID )( toValue ) );
				mappings.push_back( mappingInfo );
			}
		}
	}

	return true;
}

bool CLMGInfo_Test_Joystick::ParseFromXmlNode( XmlNodeRef node )
{	
	CLMGInfo_Test_Control::ParseFromXmlNode( node );

	node->getAttr( "default_x", defaultX );
	node->getAttr( "default_y", defaultY );

	node->getAttr( "length_min", lengthMin );
	node->getAttr( "length_max", lengthMax );

	return true;
}

bool CLMGInfo_Test_Slider::ParseFromXmlNode( XmlNodeRef node )
{
	CLMGInfo_Test_Control::ParseFromXmlNode( node );

	node->getAttr( "default_x", defaultX );
	node->getAttr( "default_y", defaultY );

	node->getAttr( "min_x", minX );
	node->getAttr( "min_y", minY );

	node->getAttr( "max_x", maxX );
	node->getAttr( "max_y", maxY );

	return true;
}

int FindMotionParamIdFromString( const CString& motionParamIdName )
{
	if ( motionParamIdName.CompareNoCase( "TravelAngle" ) == 0 )
	{
		return eMotionParamID_TravelAngle;
	}
	if ( motionParamIdName.CompareNoCase( "TravelDistScale" ) == 0 )
	{
		return eMotionParamID_TravelDistScale;
	}
	if ( motionParamIdName.CompareNoCase( "TravelSpeed" ) == 0 )
	{
		return eMotionParamID_TravelSpeed;
	}
	if ( motionParamIdName.CompareNoCase( "TravelDist" ) == 0 )
	{
		return eMotionParamID_TravelDist;
	}
	if ( motionParamIdName.CompareNoCase( "TravelSlope" ) == 0 )
	{
		return eMotionParamID_TravelSlope;
	}
	if ( motionParamIdName.CompareNoCase( "WeightShift" ) == 0 )
	{
		return eMotionParamID_WeightShift;
	}
	if ( motionParamIdName.CompareNoCase( "TurnSpeed" ) == 0 )
	{
		return eMotionParamID_TurnSpeed;
	}
	if ( motionParamIdName.CompareNoCase( "TurnAngle" ) == 0 )
	{
		return eMotionParamID_TurnAngle;
	}
	if ( motionParamIdName.CompareNoCase( "Duration" ) == 0 )
	{
		return eMotionParamID_Duration;
	}
	if ( motionParamIdName.CompareNoCase( "Curving" ) == 0 )
	{
		return eMotionParamID_Curving;
	}
	if ( motionParamIdName.CompareNoCase( "Height" ) == 0 )
	{
		return eMotionParamID_Height;
	}
	if ( motionParamIdName.CompareNoCase( "Scale" ) == 0 )
	{
		return eMotionParamID_Scale;
	}

	return -1;
}

int FindMappingFromString( const CString& mappingName )
{
	if ( mappingName.CompareNoCase( "X" ) == 0 )
	{
		return IUiMotionParamMapping::X;
	}
	if ( mappingName.CompareNoCase( "X_TIMES_PARAM_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::X_TIMES_PARAM_RANGE;
	}
	if ( mappingName.CompareNoCase( "MAP_X_TO_PARAM_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::MAP_X_TO_PARAM_RANGE;
	}
	if ( mappingName.CompareNoCase( "X_TIMES_ASSET_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::X_TIMES_ASSET_RANGE;
	}
	if ( mappingName.CompareNoCase( "MAP_X_TO_ASSET_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::MAP_X_TO_ASSET_RANGE;
	}
	if ( mappingName.CompareNoCase( "Y" ) == 0 )
	{
		return IUiMotionParamMapping::Y;
	}
	if ( mappingName.CompareNoCase( "Y_TIMES_PARAM_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::Y_TIMES_PARAM_RANGE;
	}
	if ( mappingName.CompareNoCase( "MAP_Y_TO_PARAM_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::MAP_Y_TO_PARAM_RANGE;
	}
	if ( mappingName.CompareNoCase( "Y_TIMES_ASSET_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::Y_TIMES_ASSET_RANGE;
	}
	if ( mappingName.CompareNoCase( "MAP_Y_TO_ASSET_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::MAP_Y_TO_ASSET_RANGE;
	}
	if ( mappingName.CompareNoCase( "LENGTH_TIMES_PARAM_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::LENGTH_TIMES_PARAM_RANGE;
	}
	if ( mappingName.CompareNoCase( "LENGTH_TIMES_ASSET_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::LENGTH_TIMES_ASSET_RANGE;
	}
	if ( mappingName.CompareNoCase( "LENGTH_TIMES_RANGE" ) == 0 )
	{
		return IUiMotionParamMapping::LENGTH_TIMES_RANGE;
	}
	if ( mappingName.CompareNoCase( "ANGLE_Y_AXIS" ) == 0 )
	{
		return IUiMotionParamMapping::ANGLE_Y_AXIS;
	}

	return -1;
}