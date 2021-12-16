#include "Expected.h"
#include <windows.h>

bool ExpectedIsDebuggerPresent()
{
	return IsDebuggerPresent() ? true : false;
}
