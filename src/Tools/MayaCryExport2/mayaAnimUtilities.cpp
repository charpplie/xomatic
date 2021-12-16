
#include "mayaAnimUtilities.h"
#include "MayaCryExport.h"


float getTimeFromFrame( int frame )
{
	MTime time;

	time.setUnit( time.uiUnit() );
	time.setValue( frame );

	return (float)time.as( MTime::kSeconds );
}

void GetAnimationRange( int &firstFrame, int &lastFrame, const cryExportOptions& exportOptions )
{
	firstFrame = exportOptions.animStart;
	lastFrame = exportOptions.animEnd;
}
