#ifndef __SRFCOMPILER_H
#define __SRFCOMPILER_H

#pragma once

#include "IConvertor.h"					// IConvertor
#include "SRFData.h"						// CSRFData
#include "SRFProperties.h"			// CSRFProperties


// SRF is the output file format of the Polybump tool - here we compile the data to whatever we need
class CSRFCompiler :public ICompiler, public IConvertor
{
public:

	// constructor
	CSRFCompiler();
	// destructor
	~CSRFCompiler();

	bool IsHorizonInformationAvailable() const;

	// interface IConvertor -----------------------------------------------------------

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

	// interface ICompiler ------------------------------------------------------------

	virtual bool Process(ConvertContext &cc);
	virtual void ConstructAndSetOutputFile(ConvertContext &cc);
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const;

	// interface IConvertor + ICompiler -----------------------------------------------

	virtual void Release();

	// --------------------------------------------------------------------------------

private: 

	CSRFData                  m_InputSRFFile;       //
	CSRFProperties            m_Props;              //!< user settings
	ConvertContext *          m_pCC;                //!< pointer to the object give to Process (is 0 outside of the call)


	bool LoadConfigFile();

	bool UpdateAndSaveConfig();

	//! run with the user specified user properties in m_Prop
	//! /return true=success, false otherwise(e.g. compression failed because of non power of two)
	bool RunWithProperties();

	friend class CSRFUserDialog;
	int m_refCount;
};


#endif // __SRFCOMPILER_H