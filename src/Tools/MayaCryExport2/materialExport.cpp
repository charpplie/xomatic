
#include "StdAfx.h"

#include "MayaCryExport.h"
#include "mayaUtilities.h"
#include "mayaError.h"
#include "materialExport.h"
#include "attribute.h"
#include "PathHelpers.h"
#include "sharedExport.h"

#include <sstream>

// The list of object types for the materials we are interested in exporting.
MFn::Type exportShaderTypes[] = 
{
	MFn::kLambert,
	MFn::kPluginHwShaderNode,
};
int numExportShaderTypes = sizeof(exportShaderTypes) / sizeof(MFn::Type);

// Sort the materials. 
void sortMaterialsByName( MObjectArray &surfaceShaders )
{
	int numMaterials = surfaceShaders.length();
	for( int i = 1;i<numMaterials;i++ )
	{
		bool swap = false;
		for( int j = 0;j<numMaterials-i;j++ )
		{
			MString name1 = MFnDependencyNode( surfaceShaders[j] ).name();
			MString name2 = MFnDependencyNode( surfaceShaders[j+1] ).name();
			if( strcmp(name1.asChar(),name2.asChar()) > 0 )
			{
				MObject temp = surfaceShaders[j];
				surfaceShaders.set( surfaceShaders[j+1], j );
				surfaceShaders.set( temp, j+1 );
				swap = true;
			}	
		}
		if( !swap )
			break;
	}
}

static void addMeshShadersToList( const MObject &meshObject, MSelectionList &surfaceShaders )
{
	int index;
	MObjectArray shaderObs;
	MIntArray shaderIndices;
	MFnMesh mesh( meshObject );
	mesh.getConnectedShaders( 0, shaderObs, shaderIndices );
	std::vector< bool > shaderIsUsed;
	shaderIsUsed.resize( shaderObs.length() );
	for( index = 0;index<shaderObs.length();index++ )
		shaderIsUsed[index] = false;
	for( index = 0;index<shaderIndices.length();index++ )
		if( shaderIndices[index] >= 0 && shaderIndices[index] < shaderObs.length() )
			shaderIsUsed[shaderIndices[index]] = true;

	for( index = 0;index<shaderObs.length();index++ )
	{
		if( shaderIsUsed[index] && shaderObs[index].hasFn( MFn::kShadingEngine ) )
		{
			MObject surfaceShader;
			followPlug( shaderObs[index], "surfaceShader", surfaceShader, MFn::kInvalid );
			surfaceShaders.add( surfaceShader, true );
		}
	}
}

// Get a list of materials that are either selected themselves or applied to selected objects
void gatherMaterialsFromSelected( MObjectArray &surfaceShaders )
{
	MStatus status;
	MSelectionList selected;
	MSelectionList surfaceShadersList;

	MGlobal::getActiveSelectionList( selected );
	expandSelectionList( selected );

	for( int i = 0;i<selected.length();i++ )
	{
		MObject dagObject;
		status = selected.getDependNode( i, dagObject );
		if( status == MS::kSuccess )
		{
			if( dagObject.hasFn( MFn::kLambert ) || dagObject.hasFn( MFn::kPhong ) || dagObject.hasFn( MFn::kBlinn ) ) // Selected surface shaders
			{
				surfaceShadersList.add( dagObject, true );
			}
			else if( dagObject.hasFn( MFn::kShadingEngine ) ) // Selected shading groups
			{
				MObject surfaceShader;
				followPlug( dagObject, "surfaceShader", surfaceShader, MFn::kInvalid );
				surfaceShadersList.add( surfaceShader, true );
			}
			else if( dagObject.hasFn( MFn::kMesh) && !dagObject.hasFn( MFn::kTransform ) ) // Selected meshes
			{
				addMeshShadersToList( dagObject, surfaceShadersList );
			}
		}
	}

	surfaceShaders.clear();
	for( int si = 0;si<surfaceShadersList.length();si++ )
	{
		MObject shaderOb;
		surfaceShadersList.getDependNode( si, shaderOb );
		surfaceShaders.append( shaderOb );
	}
}

// Gather a list of all the used materials in the scene
void gatherUsedMaterials( MObjectArray &surfaceShaders )
{
	MSelectionList surfaceShadersList;
	MItDependencyNodes *dnIt = new MItDependencyNodes( MFn::kMesh );
	for( ;!dnIt->isDone();dnIt->next() )
	{
		addMeshShadersToList( dnIt->item(), surfaceShadersList );
	}

	surfaceShaders.clear();
	for( int si = 0;si<surfaceShadersList.length();si++ )
	{
		MObject shaderOb;
		surfaceShadersList.getDependNode( si, shaderOb );
		surfaceShaders.append( shaderOb );
	}
}

// Gather all materials in the scene
void gatherMaterials( MObjectArray &surfaceShaders )
{
	for( int i = 0;i<numExportShaderTypes;i++ )
	{
		MItDependencyNodes *dnIt = new MItDependencyNodes( exportShaderTypes[i] );
		for ( ; ! dnIt->isDone(); dnIt->next() ) 
		{
			MObject surfaceshader = dnIt->item();
			surfaceShaders.append( surfaceshader );
		}
	}
}

std::string getRootMaterialName( )
{
	std::string materialName = "defaultParent";

	if( !currentExporterOptions.customMaterialName.empty() )
		materialName = currentExporterOptions.customMaterialName;

	return materialName;
}

std::string getMaterialExportDirectory()
{
	std::string path = currentExporterOptions.customFilePath;
	MFileIO fio;
	MString currentFilename = fio.currentFile();

	if (path.empty())
	{
		path = currentExporterOptions.customName;
	}

	if (path.empty())
	{
		path = PathHelpers::GetDirectory(currentFilename.asChar());
	}
	else if (PathHelpers::IsRelative(path))
	{
		path = PathHelpers::Join(PathHelpers::GetDirectory(currentFilename.asChar()), path);
		SharedCleanupPath( path );
	}

	return path;
}

MObject createCryMaterialsPartition()
{
	MStatus status;
	MObject partitionObject = MObject::kNullObj;

	MFnPartition newPartition;
	partitionObject = newPartition.create( false, &status );
	newPartition.setName( "cryMaterials", &status );

	return partitionObject;
}

MObject getCryMaterialsPartition()
{
	MStatus status;
	MObject partitionObject = MObject::kNullObj;

	MItDependencyNodes depNodeIt( MFn::kPartition );
	for( ;!depNodeIt.isDone();depNodeIt.next() )
	{
		MObject object = depNodeIt.item();
		MString partitionName = MFnPartition(object).name();
		if( MFnPartition(object).name() == "cryMaterials" )
		{
			partitionObject = object;
			break;
		}
	}

	return partitionObject;
}

void gatherRootMaterials( MObjectArray &rootMaterials )
{
	MStatus status;
	MObject partitionObject = getCryMaterialsPartition();

	if( !partitionObject.isNull() )
	{
		MFnPartition partition(partitionObject);
		MPlugArray plugArray;
		partition.getConnections( plugArray );

		for( int i = 0;i<plugArray.length();i++ )
		{
			MPlugArray connectedArray;
			plugArray[i].connectedTo( connectedArray, true, true, &status );

			if( connectedArray.length() > 0 )
			{
				MObject node = connectedArray[0].node();
				if( node.hasFn( MFn::kSet ) )
				{
					rootMaterials.append( node );
				}
			}
		}
	}
}

MObject getRootMaterialByName( const std::string &materialName )
{
	MStatus status;
	MObjectArray rootMaterials;

	gatherRootMaterials( rootMaterials );

	for( int i = 0;i<rootMaterials.length();i++ )
	{
		MFnSet setNode( rootMaterials[i], &status );
		//MFnDagNode dagNode( rootMaterials[i], &status );
		if( status == MS::kSuccess )
		{
			if( strIsSame(setNode.name().asChar(), materialName.c_str() ) )
				return rootMaterials[i];
		}
	}

	return MObject::kNullObj;
}

void gatherSubMaterials( MObject &rootMaterial, MObjectArray &surfaceShaders )
{
	MStatus status;
	// Gather the set members by iterating over the array plug rather than calling `getMembers` so we can preserve the order.
	MFnDependencyNode depNode( rootMaterial );
	MPlug membersPlug = depNode.findPlug( "dnSetMembers", &status );
	if( status == MS::kSuccess )
	{
		int numElements = membersPlug.numElements();
		for( int i = 0;i<numElements;i++ )
		{
			MPlug child = membersPlug.elementByPhysicalIndex( i );
			MPlugArray connectedArray;
			child.connectedTo( connectedArray, true, true, &status );

			if( connectedArray.length() > 0 )
			{
				MObject node = connectedArray[0].node();
				MFnDependencyNode depNode( node );
				surfaceShaders.append( node );
			}
		}
	}
}

MObject getSubMaterialByName( MObject &rootMaterial, const std::string &subMaterialName )
{
	MStatus status;
	MObjectArray surfaceShaders;
	gatherSubMaterials( rootMaterial, surfaceShaders );

	for( int i = 0;i<surfaceShaders.length();i++ )
	{
		MFnDependencyNode shaderNode( surfaceShaders[i], &status );
		if( status == MS::kSuccess && strIsSame( shaderNode.name().asChar(), subMaterialName.c_str() ) )
			return surfaceShaders[i];
	}

	return MObject::kNullObj;
}

MObject getSubMaterialByIndex( MObject &rootMaterial, int subMaterialIndex )
{
	MStatus status;
	MObjectArray surfaceShaders;
	gatherSubMaterials( rootMaterial, surfaceShaders );

	if( subMaterialIndex < surfaceShaders.length() )
	{
		return surfaceShaders[subMaterialIndex];
	}

	return MObject::kNullObj;
}

bool renameSubMaterialByIndex( MObject &rootMaterial, int subMaterialIndex, const std::string &newName )
{
	bool cleanRenameSuccess = false;
	MObject subMaterial = getSubMaterialByIndex( rootMaterial, subMaterialIndex );
	
	if( !subMaterial.isNull() )
	{
		MFnDependencyNode depNode(subMaterial);
		depNode.setName( newName.c_str() );
		if( strIsSame( newName, depNode.name().asChar() ) )
			cleanRenameSuccess = true;
	}
	return cleanRenameSuccess;
}

MObject createNewRootMaterial( const std::string &newMaterialName )
{
	MStatus status;
	MObject partitionObject = getCryMaterialsPartition();
	if( partitionObject.isNull() )
		partitionObject = createCryMaterialsPartition();
	MFnPartition cryMaterialsPartition(partitionObject, &status);
	MFnSet newSet;
	MSelectionList setMembers;
	MObject setObject = newSet.create( setMembers, MFnSet::kNone, &status );
	newSet.setName( newMaterialName.c_str(), &status );
	status = cryMaterialsPartition.addMember( setObject );

	return setObject;
}

MObject createNewSubMaterial( MObject &rootMaterial, const std::string &newSubMaterialName )
{
	MFnPhongShader phongShader;
	MObject shaderObject = phongShader.create();
	phongShader.setName( newSubMaterialName.c_str() );

	MFnSet materialSet( rootMaterial );
	materialSet.addMember( shaderObject );

	return shaderObject;
}

bool writeMaterial( const std::string &materialName, const MObjectArray &surfaceShaders )
{
	MStatus status;
	int i;
	std::string fileName;

	fileName.assign( getMaterialExportDirectory() );
	fileName.append( "/" );
	fileName.append( materialName );
	fileName.append( ".mtl" );

	FILE *checkMaterialFile = fopen( fileName.c_str(), "r" );
	if( checkMaterialFile )
	{
		fclose( checkMaterialFile );
		std::ostringstream messageStream;
		messageStream << "Material file `" << materialName << ".mtl` already exists. Overwrite it?";
		if( !displayYesNoQuestion( messageStream.str().c_str() ) )
			return false;
	}

	FILE *materialFile = fopen( fileName.c_str(), "w" );
	if( materialFile )
	{
		fprintf( materialFile, "<Material MtlFlags=\"524544\" >\n" );
		fprintf( materialFile, "   <SubMaterials>\n" );
		for( i = 0;i<surfaceShaders.length();i++ )
		{
			MObject surfaceshader = surfaceShaders[i];
			MString shaderName = MFnDependencyNode( surfaceshader ).name(&status);

			// Get diffuse colour
			float3 diffuseColour = { 1.0f, 1.0f, 1.0f };
			getFloat3AttributeValue( surfaceshader, "color", diffuseColour[0], diffuseColour[1], diffuseColour[2] );
			std::string diffuseTexture = getTextureAttributeValue( surfaceshader, "color" );
			// Get specular colour
			float3 specularColour = { 1.0f, 1.0f, 1.0f };
			getFloat3AttributeValue( surfaceshader, "specularColor", specularColour[0], specularColour[1], specularColour[2] );
			// Get emissive colour
			float3 emissiveColour = { 0.0f, 0.0f, 0.0f };
			getFloat3AttributeValue( surfaceshader, "incandescence", emissiveColour[0], emissiveColour[1], emissiveColour[2] );

			fprintf( materialFile, "      <Material Name=\"%s\" ", shaderName.asChar() );

			// TODO : These string shouldn't be redefined here. We need to get this sorted across all the plugins.
			// Each string should only really be defined once.
			std::string physicalise = getAttributeValue( surfaceshader, "physicalise" );
			if( strIsSame( physicalise, "ProxyNoDraw" ) )
				fprintf( materialFile, "MtlFlags=\"1152\" Shader=\"Nodraw\" GenMask=\"0\" " );
			else
				fprintf( materialFile, "MtlFlags=\"524416\" Shader=\"Illum\" GenMask=\"100000000\" " );

			fprintf( materialFile, "SurfaceType=\"\" MatTemplate=\"\" " );
			fprintf( materialFile, "Diffuse=\"%f,%f,%f\" ", diffuseColour[0], diffuseColour[1], diffuseColour[2] );
			fprintf( materialFile, "Specular=\"%f,%f,%f\" ", specularColour[0], specularColour[1], specularColour[2] );
			fprintf( materialFile, "Emissive=\"%f,%f,%f\" ", emissiveColour[0], emissiveColour[1], emissiveColour[2] );
			fprintf( materialFile, "Shininess=\"10\" " );
			fprintf( materialFile, "Opacity=\"1\" " );
			fprintf( materialFile, ">\n" );

			fprintf( materialFile, "         <Textures>\n" );

			// Write out diffuse texture.
			if( diffuseTexture.length() > 0 )
			{
				fprintf( materialFile, "            <Texture Map=\"Diffuse\" File=\"%s\" >\n", diffuseTexture.c_str() );
				fprintf( materialFile, "               <TexMod />\n" );
				fprintf( materialFile, "            </Texture>\n" );
			}

			fprintf( materialFile, "         </Textures>\n" );
			fprintf( materialFile, "      </Material>\n");
		}
		fprintf( materialFile, "   </SubMaterials>\n" );
		fprintf( materialFile, "</Material>\n" );
		fclose( materialFile );

		return true;
	}
	else
	{
		return false;
	}
}

void buildMaterialArray( MayaCryMaterials &cryMaterials )
{
	int i;
	MStatus status;

	MObjectArray rootMaterials;
	gatherRootMaterials( rootMaterials );

	// If we have a cryMaterials partition and shader groups use those for the material files.
	// If not gather the materials from the scene.
	if( rootMaterials.length() > 0 )
	{
		for( i = 0;i<rootMaterials.length();i++ )
		{
			MFnSet materialSet( rootMaterials[i] );
			std::string materialName = materialSet.name().asChar();
			MayaCryMaterialEntry *newMaterial = cryMaterials.AddMaterial( materialName );

			gatherSubMaterials( rootMaterials[i], newMaterial->surfaceShaders );
		}
	}
	else
	{
		MayaCryMaterialEntry *newMaterial = cryMaterials.AddMaterial( getRootMaterialName() );

		if( currentExporterOptions.selectedMaterialsOnly )
			gatherUsedMaterials( newMaterial->surfaceShaders );
		else
			gatherMaterials( newMaterial->surfaceShaders );
		sortMaterialsByName( newMaterial->surfaceShaders );
	}
}

bool generateMaterialFiles( )
{
	int i;
	MStatus status;

	MayaCryMaterials cryMaterials;
	buildMaterialArray( cryMaterials );

	int writtenMaterialFileCount = 0;
	for( i = 0;i<cryMaterials.rootMaterials.size();i++ )
	{
		if( writeMaterial( cryMaterials.rootMaterials[i]->materialName, cryMaterials.rootMaterials[i]->surfaceShaders ) )
			writtenMaterialFileCount++;
	}

	std::ostringstream messageStream;
	messageStream << writtenMaterialFileCount << " material file(s) written.";
	displayErrorMessage( messageStream.str().c_str() );

	return true;
}

///////////////////////////////////////////////////////
// Reading material files
static bool sg_parseError = false;
const char *xmlSkipWhiteSpace( const char *p )
{
	while( p && *p && isspace( *p ) ) p++;
	return p;
}

const char *xmlReadNextWord( const char *p, std::string &outWord )
{
	outWord.clear();
	p = xmlSkipWhiteSpace( p );
	while( p && *p && (isalnum( *p ) || *p == '_') )
		outWord.append( 1, *p++ );
	return p;
}

const char *xmlReadNextString( const char *p, std::string &outWord )
{
	outWord.clear();
	p = xmlSkipWhiteSpace( p );
	if( p && *p && *p == '\"' )
	{
		p++;
		while( p && *p && *p != '\"' )
			outWord.append( 1, *p++ );
		if( p && *p && *p == '\"' ) p++;
	}
	return p;
}

int xmlReadAttributes( const char *data, std::vector< std::string > &attrNames, std::vector< std::string > &attrValues )
{
	attrNames.clear();
	attrValues.clear();
	if( !data )
		return 0;
	const char *dataPtr = data;
	std::string readName, readValue;
	dataPtr = xmlSkipWhiteSpace( dataPtr );
	int attrCount = 0;
	while( !sg_parseError && dataPtr && *dataPtr && *dataPtr != '<' && *dataPtr != '>' && *dataPtr != '/' )
	{
		dataPtr = xmlReadNextWord( dataPtr, readName );
		dataPtr = xmlSkipWhiteSpace( dataPtr );
		if( dataPtr && *dataPtr && *dataPtr == '=' )
		{
			dataPtr++;
			dataPtr = xmlReadNextString( dataPtr, readValue );
			attrNames.push_back( readName );
			attrValues.push_back( readValue );
		}
		else
			sg_parseError = true;
		dataPtr = xmlSkipWhiteSpace( dataPtr );
	}
	return attrCount;
}

std::string xmlGetAttributeValue( const std::string &searchName, std::vector<std::string> &attrNames, std::vector<std::string> &attrValues )
{
	for( int i = 0;i<attrNames.size();i++ )
	{
		if( strIsSame( attrNames[i], searchName ) )
			return attrValues[i];
	}
	return "";
}

void readMaterial( MObject surfaceShader, std::vector< std::string > &attrNames, std::vector< std::string > &attrValues )
{
	MStatus status;

	if( !surfaceShader.isNull() )
	{
		MFnDependencyNode shaderNode( surfaceShader, &status );

		std::string extraString;
		for( int i = 0;i<attrNames.size();i++ )
		{
			// Iterate over the attributes and add them to the shader.
			if( strIsSame( attrNames[i], "Name" ) )
			{ 
				// Name already processed.
			}
			else
			{
				// If we don't know what an attribute is add it to the extraStore as a string so we can still get it later.
				if( extraString.size() > 0 )
					extraString.append( ";" );
				extraString += attrNames[i] + "=" + attrValues[i];
			}
		}

		if( status == MS::kSuccess )
		{
			// Find the extraStore. If it not there, create it.
			MPlug extraStorePlug = shaderNode.findPlug( "extraStore", &status );
			if( status != MS::kSuccess )
			{
				MFnTypedAttribute tAttr;
				MObject extraObject = tAttr.create( "extraStore", "es", MFnData::kString );
				shaderNode.addAttribute( extraObject );
				extraStorePlug = shaderNode.findPlug( "extraStore", &status );
			}

			if( status == MS::kSuccess  )
			{
				extraStorePlug.setString( extraString.c_str() );
			}
		}
	}
}

void readTexture( MObject surfaceShader, std::vector< std::string > &textureAttrNames, std::vector< std::string > &textureAttrValues )
{

}

bool readMaterialFile( const std::string &materialFileName )
{
	const char *CSubMaterialStartString = "<SubMaterials>";
	const char *CMaterialStartString = "<Material";
	const char *CMaterialEndString = "/Material";
	const char *CTexturesStartString = "<Textures";
	const char *CTexturesEndString = "/Textures>";
	const char *CTextureStartString = "<Texture";
	const char *CTextureEndString = "/Texture";


	if( materialFileName.length() > 0 )
	{
		FILE *inFile = fopen( materialFileName.c_str(), "r" );
		if( inFile )
		{
			fseek(inFile, 0, SEEK_END );
			int fileSize = ftell(inFile);
			fseek(inFile, 0, SEEK_SET );
			if( fileSize == 0 )
			{
				fclose(inFile);
				return false;
			}
			char *fileData = (char*)malloc(fileSize);
			fread( fileData, 1, fileSize, inFile );
			fclose(inFile);

			std::string materialName = PathHelpers::RemoveExtension( PathHelpers::GetFilename( materialFileName ) );
			MObject rootMaterial = getRootMaterialByName( materialName );
			if( rootMaterial.isNull() )
			{
				rootMaterial = createNewRootMaterial( materialName );
				if( rootMaterial.isNull() )
					return false;
			}

			MObjectArray surfaceShaders;
			gatherSubMaterials( rootMaterial, surfaceShaders );


			std::vector<std::string> subMaterialNames;

			sg_parseError = false;
			int subMaterialIndex = 0;
			const char *materialStart = strstr( fileData, CSubMaterialStartString );
			materialStart = strstr( materialStart, CMaterialStartString );
			char *materialEnd = fileData + (materialStart-fileData);
			while( materialStart )
			{
				materialEnd = strstr( materialEnd+1, CMaterialEndString );
				if( materialStart && materialEnd && *materialStart && *materialEnd && materialStart < materialEnd )
				{
					*materialEnd = '\0'; // Make sure we don't read past the end of the material

					MStatus status;
					std::string discard;
					materialStart = xmlReadNextWord( ++materialStart, discard ); // Skip the '<' and the node name
					std::vector< std::string > attrNames, attrValues;
					xmlReadAttributes( materialStart, attrNames, attrValues );
					std::string subMaterialName = xmlGetAttributeValue( "Name", attrNames, attrValues );

					MObject surfaceShader;
					if( subMaterialIndex < surfaceShaders.length() )
					{
						surfaceShader = surfaceShaders[subMaterialIndex];
					}
					else if( subMaterialIndex == surfaceShaders.length() )
					{
						surfaceShader = createNewSubMaterial( rootMaterial, subMaterialName );
						surfaceShaders.append( surfaceShader );
					}
					else
					{
						// Some sort of error has happened if we get here. TODO : Output error somewhere.
					}

					if( !surfaceShader.isNull() )
					{
						MFnDependencyNode shaderNode( surfaceShader, &status );
						if( !strIsSame( shaderNode.name().asChar(), subMaterialName ) )
						{
							renameSubMaterialByIndex( rootMaterial, subMaterialIndex, subMaterialName );
						}

						subMaterialNames.push_back( subMaterialName );
						readMaterial( surfaceShader, attrNames, attrValues );

						////////////////////////////////////////////////
						// Read textures
						const char *textureStart = strstr( materialStart, CTexturesStartString );
						if( textureStart )
						{
							textureStart = strstr( textureStart+1, CTextureStartString );
							char *textureEnd = fileData + (textureStart-fileData);
							while( textureStart )
							{
								textureEnd = strstr( textureEnd+1, CTextureEndString );
								if( textureStart && textureEnd && *textureStart && *textureEnd && textureStart < textureEnd )
								{
									*textureEnd = '\0'; // Make sure we don't read past the end of the texture

									MStatus status;
									std::string discard;
									textureStart = xmlReadNextWord( ++textureStart, discard ); // Skip the '<' and the node name
									std::vector< std::string > textureAttrNames, textureAttrValues;
									xmlReadAttributes( textureStart, textureAttrNames, textureAttrValues );

									readTexture( surfaceShader, textureAttrNames, textureAttrValues );

									textureStart = strstr( textureEnd+1, CTextureStartString );
								}
								else
								{
									textureStart = NULL;
								}
							}
						}
						////////////////////////////////////////////////
					}

					materialStart = strstr( materialEnd+1, CMaterialStartString );
				}
				else
				{
					materialStart = NULL;
				}
				subMaterialIndex++;
			}

			bool subMaterialNameClash = true;
			if( subMaterialNames.size() == subMaterialIndex )
			{
				// Rename the materials again to clean up any reordered names.
				for( int i =0;i<subMaterialNames.size();i++ )
				{
					if( !renameSubMaterialByIndex( rootMaterial, i, subMaterialNames[i] ) )
						subMaterialNameClash = true;
				}
			}
			else
			{
				// Some sort of error has occurred. We should never get here.
			}

			if( subMaterialNameClash )
			{
				// TODO : Output warning
			}

			free(fileData);
		}

	}
	return true;
}