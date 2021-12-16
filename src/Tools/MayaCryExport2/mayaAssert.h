
#ifndef __MAYAASSERT_H__
#define __MAYAASSERT_H__

#include <maya/MStatus.h>
#include <maya/MGlobal.h>
#include <fstream>

inline void mayaAssertPrintMessage( const char *msg, const char *file, const int line )
{
    if( msg )
    {
		printf("Assert thrown. File : `%s` Line : `$s` :: %s\n", file, line, msg);
		fflush(stdout);
    }
    else
    {
		printf("Assert thrown. File : `%s` Line : `$s`\n", file, line);
		fflush(stdout);
    }
}

inline void _Assert( bool b, const char *msg, const char *file, int line )
{
    if(!b)
	{
		mayaAssertPrintMessage( msg, file, line );
		int i = 0;
		throw(i);
	}
}

inline void _Assert( MStatus &status, const char *msg, const char *file, int line )
{
	if(MS::kSuccess != status)
	{
		mayaAssertPrintMessage( msg, file, line );
		int i = 0;
		throw(i);
	}
}

#define Assert(x) _Assert( (x), #x, __FILE__, __LINE__ )
#define AssertMsg(x,y) _Assert( (x), (y), __FILE__, __LINE__ )

//#define Assert(x)
//#define AssertMsg(x,y)

#endif // __MAYAASSERT_H__
