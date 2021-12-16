#ifndef __EXPORTSTUB_H__
#define __EXPORTSTUB_H__

#include "max.h"
#include "guplib.h"

ClassDesc* GetExportStubDesc();

class ExportStubGUP : public GUP
{
public:
	ExportStubGUP();
	virtual ~ExportStubGUP();

	virtual DWORD Start();
	virtual void Stop();
	virtual DWORD_PTR Control(DWORD parameter);
	virtual void DeleteThis();

	void DoExport(const char* parameters, INode** nodes, DWORD nodeCount);

private:
	HMODULE m_exportLibrary;
};

#endif //__EXPORTSTUB_H__
