#ifndef __CRYMAYASUPPORTPLUGIN_H__ 
#define __CRYMAYASUPPORTPLUGIN_H__

#include <maya/MPxCommand.h>
#include "mayaIncludes.h"

class cryMayaSupportPlugin : public MPxCommand  
{
public:
	cryMayaSupportPlugin();
	~cryMayaSupportPlugin();

	virtual MStatus doIt( const MArgList& );

	bool isUndoable() const;
	static void* creator();
private:

	static char *readMaterialFileString;
};

#endif // __CRYMAYASUPPORTPLUGIN_H__