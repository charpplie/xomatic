#include "attribute.h"
#include "mayaUtilities.h"

bool strIsSame( std::string one, std::string two )
{
	if( strcmp( one.c_str(), two.c_str() ) == 0 )
		return true;
	return false;
}

static void strTokenise( const char* input, const char* delim, std::vector<std::string> &output )
{
	char *tempString;
	char *token;

	tempString = (char*)malloc( strlen(input)+5 );
	strcpy( tempString, input );
	if( tempString )
	{
		if( (token = strtok(tempString, delim)) )
		{
			output.push_back(std::string(token));
			while( (token = strtok(NULL, delim)) )
			{
				output.push_back(std::string(token));
			}
		}
		free(tempString);
		tempString = NULL;
	}
}

std::string convertFileType( std::string &typeString )
{
	std::string outString = "";
	if( strstr( typeString.c_str(), ".CGF" ) )
		outString = "cgf";
	else if( strstr( typeString.c_str(), ".CHR" ) )
		outString = "chr";
	else if( strstr( typeString.c_str(), ".CGA" ) )
		outString = "cga";
	else if( strstr( typeString.c_str(), ".ANM" ) )
		outString = "anm";

	return outString;
}

std::string getAttributeName( MPlug &plug )
{
	MString attributeNameString = plug.partialName( false, false, false, false, false, true );
	std::string attributeName = attributeNameString.asChar();

	return attributeName;
}

std::string getAttributeValue( MPlug &plug )
{
	std::string attributeName = getAttributeName( plug );
	std::string attributeValue = "0";

	MStatus status = MS::kFailure;
	MString valueString;
	if( plug.attribute().hasFn( MFn::kEnumAttribute ) )
	{
		MFnEnumAttribute attr( plug.attribute() );
		int iValue = plug.asInt();

		valueString = attr.fieldName( iValue );
		status = MS::kSuccess;
	}
	else if ( plug.attribute().hasFn( MFn::kNumericData ) || plug.attribute().hasFn( MFn::kNumericAttribute ) )
	{
		MFnNumericAttribute attr( plug.attribute() );
		if ( attr.unitType() == MFnNumericData::kDouble )
		{
			valueString += plug.asFloat();
		}
		else
		{
			valueString += plug.asInt();
		}
		status = MS::kSuccess;
	}
	else
	{
		valueString = plug.asString( );
		status = MS::kSuccess;
	}

	if( status == MS::kSuccess )
	{
		attributeValue.assign( valueString.asChar() );

		if( strIsSame( attributeName, "fileType" ) )
			attributeValue = convertFileType( attributeValue );
	}

	return attributeValue;
}

float getFloatAttributeValue( MPlug &plug )
{
	std::string attributeName = getAttributeName( plug );
	float attributeValue = 0.0f;

	if ( plug.attribute().hasFn( MFn::kNumericData ) || plug.attribute().hasFn( MFn::kNumericAttribute ) )
	{
		MFnNumericAttribute attr( plug.attribute() );
		attributeValue = plug.asFloat();
	}

	return attributeValue;
}

void getFloat3AttributeValue( MPlug &plug, float &outX, float &outY, float &outZ )
{
	if(plug.isCompound())
	{
		MPlug cPlugX = plug.child(0);
		MPlug cPlugY = plug.child(1);
		MPlug cPlugZ = plug.child(2);
		cPlugX.getValue(outX);
		cPlugY.getValue(outY);
		cPlugZ.getValue(outZ);
	}
}

std::string getAttributeValue( MObject shader, const char *attributeName )
{
	MStatus status;
	std::string outString = "";

	MPlug thePlug = MFnDependencyNode( shader ).findPlug( attributeName, &status );	
	if( status == MS::kSuccess )
	{
		outString = getAttributeValue( thePlug );
	}
	return outString;
}

float getFloatAttributeValue( MObject object, const char * attributeName )
{
	MStatus status;
	float out = 0.0f;

	MPlug thePlug = MFnDependencyNode( object ).findPlug( attributeName, &status );	
	if( status == MS::kSuccess )
	{
		out = getFloatAttributeValue( thePlug );
	}
	return out;
}

void getFloat3AttributeValue( MObject object, const char *attributeName, float &outX, float &outY, float &outZ )
{
	MStatus status;
	outX = outY = outZ = 0.0f;

	MPlug thePlug = MFnDependencyNode( object ).findPlug( attributeName, &status );	
	if( status == MS::kSuccess )
	{
		getFloat3AttributeValue( thePlug, outX, outY, outZ );
	}
}

std::string getTextureAttributeValue( MObject object, const char *attributeName )
{
	std::string texturePath = "";
	MStatus status;
	MObject texture;

	if( followPlug( object, attributeName, texture, MFn::kInvalid ) )
	{
		// If the texture node is not one of the types we need then search for a node that is.
		if( texture.apiType() != MFn::kFileTexture )
		{
			MItDependencyGraph *dgIt = new MItDependencyGraph( texture, MFn::kInvalid, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status );

			if( status ) 
			{
				while( !dgIt->isDone() )
				{
					if( dgIt->thisNode().apiType() == MFn::kFileTexture )
					{
						texture = dgIt->thisNode();
						break;
					}
					dgIt->next();
				}
			}
			delete dgIt;
		}
	}

	if( (!texture.isNull()) && texture.apiType() == MFn::kFileTexture )
	{
		MFnDependencyNode depNode( texture );
		MPlug fileNamePlug = depNode.findPlug( "fileTextureName", &status );
		if( status == MS::kSuccess )
			texturePath = fileNamePlug.asString().asChar();
	}
	
	return texturePath;
}

bool attributeIsBoolean( MPlug &plug )
{
	MStatus status = MS::kFailure;
	if ( plug.attribute().hasFn( MFn::kNumericData ) || plug.attribute().hasFn( MFn::kNumericAttribute ) )
	{
		MFnNumericAttribute attr( plug.attribute() );
		if( attr.unitType() == MFnNumericData::kBoolean )
			return true;
	}
	return false;
}

bool getBooleanValue( std::string valueStr )
{
	MString mString = valueStr.c_str();
	if( mString.asInt() )
		return true;
	else
		return false;
}

std::string getNodeProperties( MDagPath nodePath )
{
	MStatus status;
	int i;
	std::string properties;
	const std::string propDelimiter = "\n";

	bool addedProperty = false;
	MFnDependencyNode depNode( nodePath.node() );
	int numAttributes = depNode.attributeCount();
	for( i = 0;i<numAttributes;i++ )
	{
		MObject attribute = depNode.attribute( i );
		MPlug thePlug = depNode.findPlug( attribute, &status );
		if( status == MS::kSuccess )
		{
			if( thePlug.isDynamic() )
			{
				std::string attributeName = getAttributeName( thePlug );
				
				if( attributeName.size() > 0 )
				{
					std::string attributeValue = getAttributeValue( thePlug );

					// If the property is a boolean value and it is false, don't add it to the string.
					if( !(attributeIsBoolean( thePlug ) && !getBooleanValue( attributeValue )) )
					{
						if( addedProperty == true )
						{
							properties.append( propDelimiter );
						}
						properties.append( attributeName );
						properties.append( "=" );

						properties.append( attributeValue );
						addedProperty = true;
					}
				}
			}
		}
	}

	return properties;
}