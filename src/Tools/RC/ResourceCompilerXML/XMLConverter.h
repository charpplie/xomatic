//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __XMLCONVERTER_H__
#define __XMLCONVERTER_H__

#include "IConvertor.h"
#include "ICryXML.h"

class XMLConverter : public IConvertor, public ICompiler
{
public:
	XMLConverter(ICryXML* pCryXML);
	~XMLConverter();

	virtual void Release();

	virtual bool Process(ConvertContext &cc);
	virtual void ConstructAndSetOutputFile(ConvertContext &cc);
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const;

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

private:
	static const string sk_sInputExtensions[];
	int m_refCount;
	ICryXML* pCryXML;
};

#endif //__XMLCONVERTER_H__
