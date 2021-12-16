#ifndef __MAYACRYEXPORT_H__ 
#define __MAYACRYEXPORT_H__

//#define MAYA_SRC

#include "mayaIncludes.h"

enum EExportType
{
	kExportType_anim,
	kExportType_geom,
	kExportType_material,
	kExportType_unknown,
};

class cryExportOptions
{
public:
	cryExportOptions( void );
	~cryExportOptions( void );

	void setDefaultOptions( void );
	bool parseOptionsString( const MString &optionsString );

public:
	std::string optionsString;

	EExportType exportType;
	std::string customName;
	std::string customFilePath;
	std::string customMaterialName;
	int animStart;
	int animEnd;
	std::string animRoot;
	bool selectedMaterialsOnly;
};

// TODO: This shouldn't really be exposed like this.
extern cryExportOptions currentExporterOptions;

class MayaCryExport : public MPxFileTranslator 
{
public:
	MayaCryExport(void) { };
	virtual ~MayaCryExport(void) { };
    
	static void *creator(void) { return new MayaCryExport(); };
    
    MStatus reader(const MFileObject& file,const MString& optionsString,MPxFileTranslator::FileAccessMode mode) { return MS::kSuccess; };
    MStatus writer(const MFileObject& file,const MString& optionsString,MPxFileTranslator::FileAccessMode mode);

    bool haveReadMethod(void) const { return false; };
    bool haveWriteMethod(void) const { return true; };
    bool canBeOpened() const { return false; };
   
    MString defaultExtension(void) const { return "cgf"; };
    MFileKind identifyFile(const MFileObject& fileName,const char *buffer,short size) const { return kNotMyFileType; };

private:
};



#include <maya/MPxCommand.h>
#include "mayaIncludes.h"

class cryMayaCheckDegradedFaces : public MPxCommand  
{
public:
	cryMayaCheckDegradedFaces();
	~cryMayaCheckDegradedFaces();

	virtual MStatus doIt( const MArgList& );

	MStatus DegradedFacesIterateNodes(MDagPath node);
	MStatus VertexSharingPointCheckIterateNodes(MDagPath node);

	bool isUndoable() const;
	static void* creator();

};

#endif // __MAYACRYEXPORT_H__
