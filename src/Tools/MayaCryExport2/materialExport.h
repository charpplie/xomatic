#ifndef __MATERIALEXPORT_H__
#define __MATERIALEXPORT_H__

#include "mayaIncludes.h"

class MayaCryMaterialEntry
{
public:
	std::string materialName;
	MObjectArray surfaceShaders;
};

class MayaCryMaterials
{
public:
	MayaCryMaterials() {};
	~MayaCryMaterials()
	{
		for( int i = 0;i<rootMaterials.size();i++ )
			delete rootMaterials[i];
		rootMaterials.clear();
	};

	MayaCryMaterialEntry *AddMaterial( const std::string &inMaterialName )
	{
		MayaCryMaterialEntry *newMaterial = new MayaCryMaterialEntry;
		newMaterial->materialName = inMaterialName;
		rootMaterials.push_back( newMaterial );

		return newMaterial;
	}

	std::vector< MayaCryMaterialEntry* > rootMaterials;
};

std::string getRootMaterialName( void );

void sortMaterialsByName( MObjectArray &surfaceShaders );
void gatherMaterialsFromSelected( MObjectArray &surfaceShaders );
void gatherUsedMaterials( MObjectArray &surfaceShaders );
void gatherMaterials( MObjectArray &surfaceShaders );

void buildMaterialArray( MayaCryMaterials &cryMaterials );

bool generateMaterialFiles( void );
bool readMaterialFile( const std::string &materialFileName );

#endif // __MATERIALEXPORT_H__