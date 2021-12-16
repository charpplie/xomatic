////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   ColladaCompiler.h
//  Version:     v1.00
//  Created:     3/4/2006 by Michael Smith
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __COLLADACOMPILER_H__
#define __COLLADACOMPILER_H__

#include "IConvertor.h"

class ICryXML;
class CContentCGF;
class CPhysicsInterface;
struct sMaterialLibrary;
enum ColladaAuthorTool;
struct CInternalSkinningInfo;
class IPakSystem;

class ColladaCompiler : public IConvertor, public ICompiler
{
public:
	ColladaCompiler(ICryXML* pCryXML, IPakSystem* pPakSystem);
	virtual ~ColladaCompiler();

	// IConvertor methods.
	virtual void Init(IConfig* config, const char* exePath) {}
	virtual void DeInit() {}
	virtual ICompiler* CreateCompiler();
	virtual bool SupportsMultithreading() const;
	virtual int GetNumPlatforms() const;
	virtual EPlatform GetPlatform(int index) const;
	virtual int GetNumExt() const;
	virtual const char* GetExt(int index) const;
	virtual DWORD GetTimestamp() const;
	virtual IConvertContext* CreateConvertContext() const { return new ConvertContext; }

	// ICompiler methods.
	virtual bool Process(ConvertContext &cc);
	virtual void ConstructAndSetOutputFile(ConvertContext &cc);
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const;

	// ICompiler + IConvertor methods.
	virtual void Release();

private:
	void WriteMaterialLibrary(sMaterialLibrary& materialLibrary, const string& matFileName);
	
	bool CompileToCGF(ConvertContext &cc, ColladaAuthorTool authorTool, CContentCGF* pCGF, CInternalSkinningInfo* pSkinninginfo, const string& exportFileName);
	bool CompileToCHR(ConvertContext &cc, ColladaAuthorTool authorTool, CContentCGF* pCGF, const string& exportFileName);
	bool CompileToANM(ConvertContext &cc, ColladaAuthorTool authorTool, CContentCGF* pCGF, CInternalSkinningInfo* pSkinninginfo, const string& exportFileName);
	bool CompileToCAF(ConvertContext &cc, ColladaAuthorTool authorTool, CContentCGF* pCGF, CInternalSkinningInfo* pSkinninginfo, const string& exportFileName);

	bool PrepareCGF(CContentCGF* pCGF);
	void FindUsedMaterialIDs(std::vector<int>& usedMaterialIDs, CContentCGF* pCGF);

	ICryXML* pCryXML;
	IPakSystem* pPakSystem;
	CPhysicsInterface* pPhysicsInterface;
	int m_refCount;
};

#endif //__COLLADACOMPILER_H__
