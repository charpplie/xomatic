#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
#include "IConvertor.h"							// IConvertor
#include "ImageCompiler.h"
#include "TextureFallbackSystem.h"

class CImageConvertor : public IConvertor
{
public:

	//! constructor
	CImageConvertor();
	//! destructor
	~CImageConvertor();

	// interface IConvertor ----------------------------------------------------

	virtual void Release();
	virtual void Init(IConfig* config, const char* exePath);
	virtual void DeInit();
	virtual ICompiler* CreateCompiler();
	virtual bool SupportsMultithreading() const;
	virtual int GetNumPlatforms() const;
	virtual EPlatform GetPlatform( int index ) const;
	virtual int GetNumExt() const;
	virtual const char* GetExt( int index ) const;
	virtual DWORD GetTimestamp() const;
	virtual IConvertContext* CreateConvertContext() const {return new ConvertContext;}

	// -------------------------------------------------------------------------

private:
	CTextureFallbackSystem m_TextureFallbackSystem;
	bool m_bInitialized;
};
