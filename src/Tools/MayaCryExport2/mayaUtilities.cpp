/******************************************************************************
** Andrew Rayson - December 2003
** General utility functions
******************************************************************************/

#define MAX_STRLEN (4096)

#include "mayaIncludes.h"
#include "mayaUtilities.h"
#include "mayaAssert.h"
#include "attribute.h"

MMatrix getNodeTransform( const MDagPath &bonePath, MSpace::Space space )
{
	const MDagPath nodePath = bonePath;
	MMatrix matrix;

	matrix.setToIdentity();
	if( space == MSpace::kWorld )
	{
		matrix = nodePath.inclusiveMatrix();
	}
	else
	{
		MFnTransform nodeTransform( nodePath );
		matrix = (nodeTransform.transformation()).asMatrix();
	}
	return matrix;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Wild card compare functions
static bool wc_ignorCase = true;
static bool wc_cmp(const char *s1,const char *s2,int len) 
{
	for (int i=0;i<len;++i)
	{
		char c1 = wc_ignorCase ? toupper(s1[i]) : s1[i];
		char c2 = wc_ignorCase ? toupper(s2[i]) : s2[i];
		if (c1 != c2 && c1 != '?')
			return false;
	}
	return true;
}

static bool find(const char *s1,const char *s2) 
{
	int srchlen=(int)strlen(s1);
	int cmplen=(int)strlen(s2);

	for (int i=0;i<cmplen;++i)
		if(wc_cmp(s1,s2+i,srchlen))
			return true;

	return false;
}

static bool EndCmp(char *src,const char *cmp) 
{
	int slen=(int)strlen(src);
	int clen=(int)strlen(cmp);

	int j=slen-1;
	for (int i=clen-1;i>0 && src[j] != '*';--i,--j)
	{
		char sc = wc_ignorCase ? toupper(src[j]) : src[j];
		char cc = wc_ignorCase ? toupper(cmp[i]) : cmp[i];
		if(sc != cc && sc != '?')  
			return false;                        
	}
	src[j] = '\0';

	return true;
}

static bool StartCmp(char *src,const char *cmp) 
{
	int len = (int)strlen(cmp);

	int i=0;
	for ( ;i<len && src[i] != '*';++i) 
	{
		char sc = wc_ignorCase ? toupper(src[i]) : src[i];
		char cc = wc_ignorCase ? toupper(cmp[i]) : cmp[i];
		if (sc != cc && sc != '?')
			return false;
	}

	if (i < len) 
		strncpy(src,src+i,strlen(src)-i+1); 

	return true;
}

bool wildcmp(const char *findstr,const char *cmp,bool ignoreCase = false);
bool wildcmp(const char *findstr,const char *cmp, bool ignoreCase ) 
{
	wc_ignorCase = ignoreCase;

	bool retval = true;
	int len=(int)strlen(findstr);

	char *srch = new char[len+1];
	strcpy(srch,findstr);

	if (*srch != '*' && !StartCmp(srch,cmp)) 
		retval = false;
	else if (srch[len-1] != '*' && !EndCmp(srch,cmp))
		retval = false;                                
	else 
	{
		len = (int)strlen(srch);

		char *s = new char[len+1];
		int j=0;

		for (int i=0;i<len && retval;++i)
		{
			if (srch[i] != '*') 
			{
				s[j++] = srch[i];
				s[j] = '\0';

				if (!find(s,cmp)) 
					retval = false;  

				if (srch[i+1] == '*') 
				{ 
					*s = '\0';
					j=0;
				}
			}
		}
		delete [] s;
	}
	delete [] srch;
	return retval;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

// Remove the namespace prefix from a name.
void removeNamespace( const std::string &inName, std::string &outName )
{
	if( inName.length() > 0 )
	{
		int searchPos = (int)inName.length()-1;

		while( searchPos > 0 && inName[searchPos] != ':' ) 
			searchPos--;
		if( inName[searchPos] == ':' ) searchPos++;

		if( searchPos < inName.length() )
		{
			outName.assign( inName.substr( searchPos, inName.length()-searchPos ) );
			return;
		}
	}
	outName = inName;
}

// Change '__' into a space
void convertToGameName( const std::string &inName, std::string &outName )
{
	std::string tempName;
	removeNamespace( inName, tempName );
	for( int i = 0;i<tempName.length();i++ )
	{
		if( i < tempName.length()-1 && tempName[i] == '_' && tempName[i+1] == '_' )
		{
			outName.append( " " );
			i++; // Skip the second '_'
		}
		else
		{
			outName.append( 1, tempName[i] );
		}
	}
}

// See if the node name matches the one we are searching for
bool checkNodeName(const char* checkName, MDagPath checkPath, int flags)
{
    MStatus status;
    MFnDagNode dagNode( checkPath, &status );
    Assert( status );

	if( !checkName )
		return true;

	// If the string contains a `.` or a `*` then we do wild compare
	// The `FNBN_FULLNAMEONLY` and `FNBN_IGNORECASE` are ignored when this happens
	if( strstr( checkName, "?" ) || strstr( checkName, "*" ) )
	{
		printf("checkName `%s`   nodeName `%s`\n",checkName,dagNode.name().asChar());
		if( wildcmp( checkName, dagNode.name().asChar() ) )
			return true;
		return false;
	}

	if( flags & FNBN_FULLNAMEONLY )
	{
		if( strlen( checkName ) != strlen( dagNode.name().asChar() ) )
			return false;
	}

	if( flags & FNBN_IGNORECASE )
	{
		// Compare names ignoring case
		if( strncmp( checkName, dagNode.name().asChar(), strlen( checkName ) ) != 0 )
			return false;
	}
	else
	{
		// Compare names respecting case
		if( strncmp( dagNode.name().asChar(), checkName, strlen( dagNode.name().asChar() ) ) != 0 )
			return false;
	}

	return true;
}

MDagPath findNodeUpStream( const char* searchName, MDagPath &path, int flags )
{
	MDagPath returnPath;
	MDagPath parentPath( path );

	while( parentPath.isValid() && parentPath.length() > 1 )
	{
		parentPath.pop(1);

		if( !checkNodeName( searchName, parentPath, flags ) )
			continue;
		returnPath = parentPath;
		break;
	}
	return returnPath;
}

MDagPath findNodeUpStream( std::vector<const char*> searchNames, MDagPath &path, int flags )
{
	unsigned int i;
	MDagPath returnPath;
	MDagPath parentPath( path );

	while( parentPath.isValid() && parentPath.length() > 1 )
	{
		parentPath.pop(1);
		bool found = false;
		for( i = 0;i<searchNames.size();i++ )
		{
			if( !checkNodeName( searchNames[i], parentPath, flags ) )
				continue;
			returnPath = parentPath;
			found = true;
			break;
		}
		if( found ) break;
	}
	return returnPath;
}

MDagPath findNodeInDagIterator(const char* searchName,MItDag &dagIterator, int flags, MFn::Type type)
{
	MStatus status;
	MDagPath dagPath, returnPath;

	for( ;!dagIterator.isDone();dagIterator.next() )
	{
		status = dagIterator.getPath(dagPath);
		Assert(status);
		MFnDagNode dagNode(dagPath, &status);
		Assert(status);
		if(dagNode.isIntermediateObject())
			continue;

		if( type != MFn::kInvalid && type != dagNode.object().apiType() )
			continue;

		if( !(flags&FNBN_NONAMECMP) )
		{
			if( !checkNodeName( searchName, dagPath, flags ) )
				continue;
		}
		// If we get here the names are the same, so return this path
		returnPath = dagPath;

		// Advance the iterator so if we call this function again with the same iterator we get the next match
		// not this one again.
		dagIterator.next();
		break;
	}
	return returnPath;
}

bool findNodesInHierarchy( const char *searchName, MDagPath &hierarchyRoot, MDagPathArray &resultArray, int flags, MFn::Type type )
{
	MStatus status;
	MDagPath foundPath;
	MItDag dagIterator(MItDag::kDepthFirst, MFn::kInvalid, &status);

	// Dont clear result array

	status = dagIterator.reset( hierarchyRoot, MItDag::kDepthFirst, MFn::kInvalid );
	Assert(status);

	// Search below this item
	do
	{
		foundPath = findNodeInDagIterator( searchName, dagIterator,flags,type );
		if( foundPath.isValid() )
		{
			if( (!(flags&FNBN_EXCLUDEROOT)) || (!(foundPath == hierarchyRoot)) )
				resultArray.append( foundPath );
		}
	} 
	while ( foundPath.isValid() );

	if( resultArray.length() > 0 )
		return true;
	else
		return false;
}

MDagPath findNodeInHierarchy(const char* searchName,MDagPath &path, int flags, MFn::Type type)
{
	MDagPathArray resultArray;

	findNodesInHierarchy( searchName, path, resultArray, flags, type );

	if( resultArray.length() > 0 )
		return resultArray[0];
	else
	{
		MDagPath invalidPath;
		return invalidPath;
	}
}

bool findNodesInList(const char* searchName,MSelectionList &searchList, MDagPathArray &resultArray, int flags, MFn::Type type)
{
	MStatus status;
	MDagPath foundPath,returnPath;
	MItDag dagIterator(MItDag::kDepthFirst, MFn::kInvalid, &status);
	Assert(status);

	// Clear the array
	resultArray.clear();

	MItSelectionList iter (searchList, MFn::kInvalid);
	for( ;!iter.isDone();iter.next() )
	{
		MDagPath startDagPath;
		iter.getDagPath(startDagPath);
		findNodesInHierarchy(searchName,startDagPath, resultArray, flags, type);
	}

	if( resultArray.length() > 0 )
		return true;
	else
		return false;
}

MDagPath findNodeInList(const char* searchName,MSelectionList &searchList, int flags, MFn::Type type)
{
	MDagPathArray resultArray;

	findNodesInList( searchName, searchList, resultArray, flags, type );

	if( resultArray.length() > 0 )
		return resultArray[0];
	else
	{
		MDagPath invalidPath;
		return invalidPath;
	}
}

bool findNodesInScene(const char* searchName, MDagPathArray &resultArray, int flags, MFn::Type type)
{
	MDagPath path;
	MSelectionList topList;

	// Clear the array
	resultArray.clear();

	// Get a list of all top level nodes
	MItDag dagIt (MItDag::kBreadthFirst);
	for (dagIt.next(); !dagIt.isDone(); dagIt.next()) 
	{
		if (dagIt.getPath (path) != MS::kSuccess) 
			continue;
		topList.add(path);
		dagIt.prune();
	}

	return findNodesInList( searchName, topList, resultArray, flags, type );
}

MDagPath findNodeInScene(const char* searchName, int flags, MFn::Type type)
{

	MDagPathArray resultArray;

	findNodesInScene( searchName, resultArray, flags, type );

	if( resultArray.length() > 0 )
		return resultArray[0];
	else
	{
		MDagPath invalidPath;
		return invalidPath;
	}    
}

int copyDagPathArray( MDagPathArray &destArray, MDagPathArray &sourceArray )
{
	int i, numSources;
	numSources = sourceArray.length();

	destArray.clear();
	for( i = 0;i<numSources;i++ )
	{
		destArray.append( sourceArray[i] );
	}

	return destArray.length();
}

int mergeDagPathArrays( MDagPathArray &destArray, MDagPathArray &sourceArray )
{
	int i, j, numSources, numDests;
	numSources = sourceArray.length();

	for( i = 0;i<numSources;i++ )
	{
		numDests = destArray.length();
		for( j = 0;j<numDests;j++ )
		{
			if( destArray[j] == sourceArray[i] )
				break;
		}
		if( j == numDests ) // Not found in dest array, so add it
		{
			destArray.append( sourceArray[i] );
		}
	}
	return destArray.length();
}

void flaternDagPathArray( MDagPathArray &pathArray )
{
	unsigned int i, j;
	MDagPath parentPath;
	bool removedNode = true;

	// Need the while loop because after removing a node we need to start the whole process again as the pathArray
	// will have changed
	while( removedNode )
	{
		removedNode = false;
		for( i = 0;i<pathArray.length();i++ )
		{
			parentPath = pathArray[i];
			while( parentPath.isValid() && parentPath.length() > 1 )
			{
				bool removeThisPath = false;
				parentPath.pop(1);
				for( j = 0;j<pathArray.length();j++ )
				{
					if( parentPath == pathArray[j] )
					{
						removeThisPath = true;
						break; // Break from the inner for loop
					}
				}

				if( removeThisPath )
				{
					pathArray.remove( i );
					removedNode = true;
					break; //  Break out of inner while loop
				}
			}
			if( removedNode )
			{
				break; // Break out of the outer for loop
			}
		}
	}
}

void dumpDagPathArray( MDagPathArray &pathArray )
{
	unsigned int i;
	MDagPath path;

	printf("Dag Path Array");
	for( i = 0;i<pathArray.length();i++ )
	{
		path = pathArray[i];
		printf( "%s\n", MFnDagNode( path ).name().asChar() );
	}
	fflush( stdout );
}

void gatherTopLevelNodes( MSelectionList &outputSelectionList )
{
	MDagPath path;
	outputSelectionList.clear();
	MItDag dagIt (MItDag::kBreadthFirst);
	for (dagIt.next(); !dagIt.isDone(); dagIt.next()) 
	{
		if (dagIt.getPath (path) != MS::kSuccess) 
			continue;
		outputSelectionList.add(path);
		dagIt.prune();
	}
}

float getInternalToMeterScale( void )
{
	MDistance distance;

	distance.setUnit( MDistance::internalUnit() );

	distance.setValue( 1.0f );
	return (float)distance.asMeters();
}

float getSceneScale( void )
{
	MStatus status;
	float globalScale = 1.0f;

	MDagPath scalePath = findNodeInScene( "exportscale", FNBN_FULLNAMEONLY|FNBN_IGNORECASE, MFn::kTransform );
	if( scalePath.isValid() )
	{
		MFnTransform transform( scalePath, &status );
		if( MS::kSuccess == status ) 
		{
			double scale[3];
			transform.getScale( scale );
			globalScale = (float)scale[0];
		}
	}

	return globalScale;
}    

// Find a plug in 'iRoot' called 'plugName' and follow it, set 'oObject' to the object found at the end 
bool followPlug( const MObject &iRoot, const char* plugName, MObject &oObject, MFn::Type filter )
{
	MStatus status;
	MPlug outPlug = MFnDependencyNode( iRoot ).findPlug( plugName, &status );

	if( status != MS::kSuccess )
		return false;

	MItDependencyGraph dgIt( outPlug, filter, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst,
		MItDependencyGraph::kNodeLevel, &status );
	if( status != MS::kSuccess )
		return false;

	dgIt.disablePruningOnFilter();

	if( filter == MFn::kInvalid )
		dgIt.next();

	if( dgIt.isDone() ) 
		return false;

	oObject = dgIt.thisNode();
	return true;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
// Some random functions that don't fit anywhere else
float getNodeFloatAttribute( const MDagPath &nodePath, const char* attributeName, const float defaultValue )
{
	MStatus status;
	float returnVal = defaultValue;

	MPlug thePlug = MFnDependencyNode( nodePath.node() ).findPlug( attributeName, &status );
	if( status == MS::kSuccess )
	{
		thePlug.getValue( returnVal );
	}

	return returnVal;
}

void getNodePosScaleRotation( const MDagPath &nodePath, float pos[3], float scale[3],float rotation[3], bool transformSpace )
{
	MSpace::Space space = transformSpace ? MSpace::kTransform : MSpace::kWorld;
	MMatrix obMatrix = getNodeTransform( nodePath, space );
	//MMatrix obMatrix = nodePath.inclusiveMatrix();
	double values[3];

	MTransformationMatrix tmat( obMatrix );
	MTransformationMatrix::RotationOrder rotorder = MTransformationMatrix::kXYZ; 

	if( pos )
	{
		MPoint outPoint( 0.0f, 0.0f, 0.0f );

		// Set outpoint to be the localPosition if the node has one
		MObject localPosAttr = MFnDependencyNode( nodePath.node() ).attribute( "localPosition" );
		if( !localPosAttr.isNull() )
		{
			MPlug posPlug( nodePath.node(), localPosAttr );
			MPlug plugX = posPlug.child(0);
			MPlug plugY = posPlug.child(1);
			MPlug plugZ = posPlug.child(2);

			plugX.getValue(outPoint.x);
			plugY.getValue(outPoint.y);
			plugZ.getValue(outPoint.z);	
		}

		outPoint *= obMatrix;
		pos[0] = (float)outPoint.x/100.0f;
		pos[1] = (float)outPoint.y/100.0f;
		pos[2] = (float)outPoint.z/100.0f;
	}

	if( rotation )
	{
		tmat.getRotation( values, rotorder );
		rotation[0] = (float)values[0];
		rotation[1] = (float)values[1];
		rotation[2] = (float)values[2];
	}

	if( scale )
	{
		tmat.getScale( values, space );
		scale[0] = (float)values[0];
		scale[1] = (float)values[1];
		scale[2] = (float)values[2];
	}
}

MObject getNoneMeshSurfaceShader( MDagPath& nodePath )
{
	MStatus status;
	MObject shadingGroup;
	MObject surfaceShader;
	MObject object = nodePath.node();

	// Search down stream util we find a shaderEngine
	MItDependencyGraph *dgIt = new MItDependencyGraph( object, MFn::kShadingEngine,
		MItDependencyGraph::kDownstream, MItDependencyGraph::kBreadthFirst,
		MItDependencyGraph::kNodeLevel, &status );

	if ( !status ) 
	{
		delete dgIt;
		return surfaceShader;
	}
	dgIt->disablePruningOnFilter();

	for ( ; ! dgIt->isDone(); dgIt->next() ) 
	{
		shadingGroup = dgIt->thisNode();
		followPlug( shadingGroup, "surfaceShader", surfaceShader, MFn::kInvalid );
		break;
	}

	return surfaceShader;
}

MString getNoneMeshSurfaceShaderName( MDagPath& nodePath )
{
	MStatus status;
	MObject shaderGroup;

	// Get the surfaceShader and return its name.
	shaderGroup = getNoneMeshSurfaceShader( nodePath );
	MString returnString = MFnDependencyNode( shaderGroup ).name( &status );

	return returnString;
}

static void recurseExpandSelectionList( MDagPath dagPath, MSelectionList &outList )
{
	if( !outList.hasItem( dagPath ) )
	{
		outList.add( dagPath );

		int childCount = dagPath.childCount();
		for( int childIndex = 0;childIndex < childCount;childIndex++ )
		{
			dagPath.push( dagPath.child( childIndex ) );
			recurseExpandSelectionList( dagPath, outList );
			dagPath.pop(1);
		}
	}
}

// Expands the selection list so that all nodes that are implicitly selected are explicitly selected.
void expandSelectionList( MSelectionList &inOutList )
{
	int index;
	MSelectionList newList;

	for( index = 0;index<(int)inOutList.length();index++ )
	{
		MDagPath rootPath;
		if( inOutList.getDagPath( index, rootPath ) == MS::kSuccess )
		{
			recurseExpandSelectionList( rootPath, newList );
		}
		else
		{
			MObject object;
			inOutList.getDependNode( index, object );
			newList.add( object, true );
		}
	}
	inOutList = newList;
}

bool plugHasExpression( const MObject &node, const char* plugName )
{
	MStatus status;
	MPlug plug = MFnDependencyNode( node ).findPlug( plugName, &status );
	if( status != MS::kFailure ) 
	{
		MItDependencyGraph expressionItr( plug, MFn::kExpression, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status );
		if( status != MS::kFailure ) 
		{
			expressionItr.disablePruningOnFilter();
			if( !expressionItr.isDone() ) 
			{
				return true;
			}
		}
	}
	return false;
}

bool getPlugExpressionString( const MObject &node, const char* plugName, MString &outString )
{
	unsigned int i;
	MStatus status;
	MPlug thisPlug = MFnDependencyNode( node ).findPlug( plugName, &status);
	MPlug outPlug = thisPlug;
	MString expressionString;

	if (status != MS::kSuccess)
		return false;

	while( !outPlug.node().hasFn(MFn::kExpression) && !outPlug.isNull() )
	{
		// Check for some nodes and handle them
		if(outPlug.node().hasFn(MFn::kUnitConversion)) // Unit Conversion (Skip it)
		{
			outPlug = MFnDependencyNode( outPlug.node() ).findPlug( "input", &status );
			if (status != MS::kSuccess)
			{
				printf("Internal error with unitConversion node\n" );
				return false;
			}
		}

		if( outPlug.isConnected() )
		{
			MPlugArray pArray;
			outPlug.connectedTo( pArray, true, false );
			if( pArray.length() > 0 )
			{
				outPlug = pArray[0];
			}
			else
			{
				break;
			}
		}
		else if( outPlug.isChild() )
		{
			if( outPlug.parent().isConnected() )
			{
				unsigned int id = 0;
				while( outPlug.parent().child( id ) != outPlug && id < outPlug.parent().numChildren() )
					id++;

				MPlugArray pArray;
				outPlug.parent().connectedTo(pArray,true,false);
				if( pArray.length() > 0 )
				{
					outPlug = pArray[0].child( id );
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if(outPlug.node().hasFn(MFn::kExpression))
	{
		int arrayIdx = 0;
		//ListAttrNames(outPlug.node());

		// Get the index into the output array that we came in on.
		if( outPlug.isElement() )
			arrayIdx = outPlug.logicalIndex();

		// Use the internal expression as the other expression seems less well defined
		MPlug expStrPlug = MFnDependencyNode( outPlug.node() ).findPlug( "internalExpression", &status );
		if( status != MS::kSuccess )
			return false;

		MString valueString;
		expStrPlug.getValue( valueString );

		// Use the array index to get the string that is driving our plug
		MStringArray valueStringArray;
		valueString.split( '\n', valueStringArray );
		expressionString = valueStringArray[arrayIdx];
	}
	else
	{
		float value;
		thisPlug.getValue( value );
		expressionString = value;
	}

	// Strip out chars that are going to cause a problem
	std::string tempString;
	tempString.clear();
	for( i = 0;i<expressionString.length();i++ )
	{
		char c = expressionString.asChar()[i];
		if( c != '\n' && c != '\r' )
		{
			tempString.append( &c, 1 );
		}
	}

	outString = tempString.c_str();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////
// Checks to see a node is visible in maya.
// Checks the `visibility` plug and the visibility of any layer the node is in.
bool nodeIsVisible( MDagPath &dagPath, bool ignoreLayer )
{
	MStatus status;
	if( !ignoreLayer )
	{
		// Check the lay the node is in
		MPlug drawPlug = MFnDependencyNode( dagPath.node()).findPlug("drawOverride", &status );
		if( status != MS::kFailure ) 
		{
			MItDependencyGraph drawitr( drawPlug, MFn::kDisplayLayer,MItDependencyGraph::kUpstream,MItDependencyGraph::kBreadthFirst,MItDependencyGraph::kNodeLevel,&status );
			if( status != MS::kFailure ) 
			{
				drawitr.disablePruningOnFilter();
				if( !drawitr.isDone() ) 
				{
					bool b = false;
					//ListAttrNames(drawitr.thisNode());
					MPlug visPlug = MFnDependencyNode( drawitr.thisNode()).findPlug( "visibility", &status );
					if( status == MS::kSuccess )
						visPlug.getValue( b );
					//printf("4 vis %sn",b ? "TRUE" : "FALSE");
					if( b == false )
						return false;
				}
			}
		}
	}

	// Now check the node and its parents
	bool result = true;
	MDagPath searchPath( dagPath );
	while( true ) 
	{
		MFnDagNode fnDN(searchPath);
		MPlug vPlug = fnDN.findPlug("visibility");
		MPlug iPlug = fnDN.findPlug("intermediateObject");
		bool visible, intermediate;
		vPlug.getValue(visible);
		iPlug.getValue(intermediate);

		if(!(visible && !intermediate))
		{
			result = false;
			break;
		}
		if(searchPath.length() == 1) 
			break;
		searchPath.pop();
	}

	return result;
}

MObject getPolygonShaderUsingMEL( MDagPath &nodePath, int polygonIndex )
{
	MStatus status;
	MFnDagNode dagNode( nodePath, &status );
	MObject returnShadingGroup;

	char command[512];
	sprintf( command, "listSets -type 1 -o %s.f[%d]", dagNode.fullPathName().asChar(), polygonIndex);

	MStringArray resultArray;
	MGlobal::executeCommand( command, resultArray, true, false );

	if( resultArray.length() > 0 )
	{
		MSelectionList shadingGroupList;
		MGlobal::getSelectionListByName ( resultArray[0], shadingGroupList );

		if( shadingGroupList.length() != 1 )
		{
			printf( "More than 1 shadingGroup on a polygon ???\n" );
		}
		else
		{
			shadingGroupList.getDependNode ( 0, returnShadingGroup ); 
		}
	}

	return returnShadingGroup;
}

bool shaderIsNeeded( const MObject &shader )
{
	if( shader.apiType() == MFn::kBlinn )
	{
		return false;
	}
	return true;
}

bool textureIsNeeded( const MObject &texture )
{
	unsigned int i, j;
	MStatus status;
	MPlugArray connectedPlugs, sourcePlugArray;
	bool connectedToLambert = false;
	bool connectedToBlinn = false;

	status = MFnDependencyNode( texture ).getConnections( connectedPlugs );
	if( status == MS::kSuccess )
	{
		for( i = 0;i<connectedPlugs.length();i++ )
		{
			connectedPlugs[i].connectedTo( sourcePlugArray, false, true );

			for( j = 0;j<sourcePlugArray.length();j++ )
			{
				MObject object = sourcePlugArray[j].node();
				if( object.apiType() == MFn::kLambert || object.apiType() == MFn::kPhong )
				{
					connectedToLambert = true;		
				}
				else if( object.apiType() == MFn::kBlinn )
				{
					connectedToBlinn = true;
				}
			}
		}
	}

	if( connectedToLambert )
		return true;
	if( connectedToBlinn )
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
// Get the blend shape connected to a mesh
MObject getBlendControllerFromBase( const MDagPath &inNodePath )
{
	MStatus status;
	MObject blendObject = MObject::kNullObj;
	MDagPath nodePath = inNodePath;

	if( !nodePath.hasFn( MFn::kMesh ) )
		return MObject::kNullObj;

	if( nodePath.hasFn( MFn::kTransform ) && nodePath.hasFn( MFn::kMesh ) )
		nodePath.extendToShapeDirectlyBelow( 0 );

	MFnDagNode dagNode( nodePath );
	std::string nName = dagNode.name().asChar();

	MPlug inMeshPlug = dagNode.findPlug( "inMesh", &status );
	if( status == MS::kSuccess )
	{
		MItDependencyGraph *dgIt = new MItDependencyGraph( inMeshPlug, MFn::kBlendShape, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status );
		if( status == MS::kSuccess && !dgIt->isDone() )
			blendObject = dgIt->thisNode();
	}

	return blendObject;
}

MObject getBlendControllerFromTarget( const MDagPath &inNodePath )
{
	MStatus status;
	MObject blendObject = MObject::kNullObj;
	MDagPath nodePath = inNodePath;

	if( !nodePath.hasFn( MFn::kMesh ) )
		return MObject::kNullObj;

	if( nodePath.hasFn( MFn::kTransform ) && nodePath.hasFn( MFn::kMesh ) )
		nodePath.extendToShapeDirectlyBelow( 0 );

	MFnDagNode dagNode( nodePath );
	MPlug worldMeshPlug = dagNode.findPlug( "worldMesh", &status );
	if( status != MS::kSuccess )
		return MObject::kNullObj;

	worldMeshPlug = worldMeshPlug.elementByPhysicalIndex( 0, &status );
	if( status == MS::kSuccess )
	{
		MItDependencyGraph *dgIt = new MItDependencyGraph( worldMeshPlug, MFn::kBlendShape, MItDependencyGraph::kDownstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status );
		if( status == MS::kSuccess && !dgIt->isDone() )
			blendObject = dgIt->thisNode();
	}

	return blendObject;
}

void ListAttrNames(MObject ob)
{
	MStatus status;
	int i;
	float f;
	MString str;
	MObject obj;
	bool b;

	printf("=======================\n");
	if(ob.isNull())
	{
		printf("ListAttrNames :: Object is null\n");
		printf("=======================\n");
		return;
	}
	printf("Attr count %d\n",MFnDependencyNode(ob).attributeCount());
	for(unsigned int ui = 0; ui < MFnDependencyNode(ob).attributeCount();ui++)
	{
		MObject pp = MFnDependencyNode(ob).attribute(ui);
		MPlug ppp(ob,pp);
		printf("Attr %d :: %s ::",ui,ppp.name().asChar());
		status = ppp.getValue(str);
		if(status == MS::kSuccess) 
			printf("(STR '%s') ",str.asChar());
		status = ppp.getValue(i);
		if(status == MS::kSuccess) 
			printf("(INT %d) ",i);
		status = ppp.getValue(f);
		if(status == MS::kSuccess) 
			printf("(FLOAT %f) ",f);
		status = ppp.getValue(b);
		if(status == MS::kSuccess) 
			printf("(BOOL %s) ",b?"TRUE":"FALSE");
		status = ppp.getValue(obj);
		if(status == MS::kSuccess) 
			printf("(OBJECTTYPE \"%s\") ",obj.apiTypeStr());

		printf("\n");
	}
	printf("=======================\n");
	fflush(stdout);
}
