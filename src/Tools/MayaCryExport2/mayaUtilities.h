/******************************************************************************
** Andrew Rayson - December 2003
** General utility functions
******************************************************************************/

#ifndef __MAYAUTILITIES_H__ 
#define __MAYAUTILITIES_H__

#include "mayaIncludes.h"

MMatrix getNodeTransform( const MDagPath &bonePath, MSpace::Space space );

// Find node flags
enum
{
    FNBN_NULL = (0),

    FNBN_FULLNAMEONLY = (1<<0),	// Only match the whole name
    FNBN_IGNORECASE = (1<<1),	// Ignore case in name comparisons
    FNBN_EXCLUDEROOT = (1<<2),	// Dont include the root node in the result, even if it matches the name
    FNBN_NONAMECMP = (1<<3),	// Dont compare the name. Do only a type check. If type is MFn::kInvalid then all nodes will be matched
    
    FNBN_COUNT,
};

void removeNamespace( const std::string &inName, std::string &outName );
void convertToGameName( const std::string &inName, std::string &outName );
bool checkNodeName(const char* checkName, MDagPath checkPath, int flags);

MDagPath findNodeUpStream( const char* searchName, MDagPath &path, int flags );
MDagPath findNodeUpStream( std::vector<const char*> searchNames, MDagPath &path, int flags );
MDagPath findNodeInDagIterator(const char* searchName,MItDag &dagIterator, int flags, MFn::Type type = MFn::kInvalid);
bool findNodesInHierarchy( const char *searchName, MDagPath &hierarchyRoot, MDagPathArray &resultArray, int flags, MFn::Type type = MFn::kInvalid );
MDagPath findNodeInHierarchy(const char* searchName,MDagPath &path, int flags, MFn::Type type = MFn::kInvalid);
bool findNodesInList(const char* searchName,MSelectionList &searchList, MDagPathArray &resultArray, int flags, MFn::Type type = MFn::kInvalid);
MDagPath findNodeInList(const char* searchName,MSelectionList &searchList, int flags, MFn::Type type = MFn::kInvalid);
bool findNodesInScene(const char* searchName, MDagPathArray &resultArray, int flags, MFn::Type type = MFn::kInvalid);
MDagPath findNodeInScene(const char* searchName, int flags, MFn::Type type = MFn::kInvalid);

int copyDagPathArray( MDagPathArray &destArray, MDagPathArray &sourceArray );
int mergeDagPathArrays( MDagPathArray &destArray, MDagPathArray &sourceArray );
void flaternDagPathArray( MDagPathArray &pathArray );
void dumpDagPathArray( MDagPathArray &pathArray );
void gatherTopLevelNodes( MSelectionList &outputSelectionList );

char *distanceUnitToString( MDistance::Unit unit );
float getInternalToMeterScale( void );
float getSceneScale( void );

// Plug utilities
bool followPlug( const MObject &iRoot, const char* plugName, MObject &oObject, MFn::Type filter );

// Filename utilities
bool convertFilenameGetFullTexturePath( char *fname, MFileObject &currentFile, int maxlen );
void convertFilenameEx( char* texturename );

// Other utility functions
float getNodeFloatAttribute( const MDagPath &nodePath, const char* attributeName, const float defaultValue );
void getNodePosScaleRotation( const MDagPath &nodePath, float pos[3], float scale[3],float rotation[3], bool transformSpace = false );
MObject getNoneMeshSurfaceShader( MDagPath& nodePath );
MString getNoneMeshSurfaceShaderName( MDagPath& nodePath );

void expandSelectionList( MSelectionList &inOutList );

bool plugHasExpression( const MObject &node, const char* plugName );
bool getPlugExpressionString( const MObject &node, const char* plugName, MString &outString );

bool nodeIsVisible( MDagPath &dagPath, bool ignoreLayers = false );

MObject getPolygonShaderUsingMEL( MDagPath &nodePath, int polygonIndex );

bool shaderIsNeeded( const MObject &shader );
bool textureIsNeeded( const MObject &texture );

// Blend shapes
MObject getBlendControllerFromBase( const MDagPath &inNodePath );
MObject getBlendControllerFromTarget( const MDagPath &inNodePath );

void ListAttrNames(MObject ob);

// Some random defines
#define _PI  float(3.14159265358979323846264338327950288419716939937510)
#define DEG2RAD(x) ((x)*(_PI/180.0f))
#define RAD2DEG(x) ((x)*(180.0f/_PI))

#endif // __MAYAUTILITIES_H__
