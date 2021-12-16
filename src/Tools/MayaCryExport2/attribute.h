
#ifndef __ATTRIBUTE_H__
#define __ATTRIBUTE_H__

#include "mayaIncludes.h"

bool strIsSame( std::string one, std::string two );

std::string getAttributeValue( MObject shader, const char *attributeName );
float getFloatAttributeValue( MObject object, const char * attributeName );
void getFloat3AttributeValue( MObject object, const char *attributeName, float &outX, float &outY, float &outZ );
std::string getTextureAttributeValue( MObject object, const char *attributeName );
std::string getNodeProperties( MDagPath nodePath );
std::string getAttributeName( MPlug &plug );

#endif // __ATTRIBUTE_H__