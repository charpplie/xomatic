#include "StdAfx.h"
#include "stdio.h"

#include "mayaIncludes.h"
#include "cryMayaSupportPlugin.h"
#include "materialExport.h"

// Note: The `cryEngineSettingsPlugin` and `cryMayaCheckDegradedFaces` will be moved here in the future.

cryMayaSupportPlugin::cryMayaSupportPlugin()
{
}

cryMayaSupportPlugin::~cryMayaSupportPlugin()
{
}

MStatus cryMayaSupportPlugin::doIt( const MArgList& args )
{
	if( args.length() > 0 )
	{
		if( args.asString(0) == MString( "readMaterial" ) )
		{
			if( args.length() >= 1 )
			{
				readMaterialFile( args.asString(1).asChar() );
			}
		}
		else
		{
			// Unknown option
		}
	}
	return MS::kSuccess;
}

bool cryMayaSupportPlugin::isUndoable() const
{
	return false;
}

void* cryMayaSupportPlugin::creator()
{
	return new cryMayaSupportPlugin();
}