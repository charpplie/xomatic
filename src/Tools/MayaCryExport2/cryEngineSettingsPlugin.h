#ifndef __CRYENGINESETTINGSPLUGIN_H__ 
#define __CRYENGINESETTINGSPLUGIN_H__

#include <maya/MPxCommand.h>
#include "mayaIncludes.h"

class cryEngineSettingsPlugin : public MPxCommand  
{
public:
	cryEngineSettingsPlugin();
	~cryEngineSettingsPlugin();

	virtual MStatus doIt( const MArgList& );

	bool isUndoable() const;
	static void* creator();
private:

	static char *buildPathString;
};

#endif // __CRYENGINESETTINGSPLUGIN_H__