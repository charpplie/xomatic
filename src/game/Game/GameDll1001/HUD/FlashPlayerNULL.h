/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 10:11:2006: Created by Julien Darre

*************************************************************************/
#ifndef __FLASHPLAYERNULL_H__
#define __FLASHPLAYERNULL_H__

//-----------------------------------------------------------------------------------------------------

#include "IFlashPlayer.h"

//-----------------------------------------------------------------------------------------------------

class CFlashPlayerNULL : public IFlashPlayer
{
public:

	// IFlashPlayer
	virtual void AddRef() { ++m_refCount; }
	virtual void Release() 
	{ 
		--m_refCount; 
		if (m_refCount <= 0) 
			delete this; 
	}
	virtual bool Load( const char* pSWFFilePath, unsigned int options = RENDER_EDGE_AA | INIT_FIRST_FRAME ) { return true; }
	virtual void SetBackgroundColor( const ColorB& color ) {}
	virtual void SetBackgroundAlpha( float alpha ) {}
	virtual float GetBackgroundAlpha() const { return 1.0f; }
	virtual void SetViewport( int x0, int y0, int width, int height, float aspectRatio = 1.0f ) {}
	virtual void GetViewport( int& x0, int& y0, int& width, int& height, float& aspectRatio ) const {}
	virtual void SetViewScaleMode(EScaleModeType scaleMode) {}
	virtual EScaleModeType GetViewScaleMode() const { return eSM_NoScale; }
	virtual void SetViewAlignment(EAlignType viewAlignment) {}
	virtual EAlignType GetViewAlignment() const { return eAT_Center; }
	virtual void SetScissorRect(int x0, int y0, int width, int height) {}
	virtual void GetScissorRect(int& x0, int& y0, int& width, int& height) const {}
	virtual void Advance( float deltaTime ) {}
	virtual void Render(bool stereo = false) {}
	virtual void SetCompositingDepth(float depth) {}
	virtual void SetStereoPlaneDepth(float depth) {}
	virtual void Restart() {}
	virtual bool IsPaused() const { return false; }
	virtual void Pause(bool pause) {}
	virtual void GotoFrame(unsigned int frameNumber) {}
	virtual bool GotoLabeledFrame(const char* pLabel, int offset = 0) { return true; }
	virtual unsigned int GetCurrentFrame() const { return 0; }
	virtual bool HasLooped() const { return false; }
	virtual void SetFSCommandHandler( IFSCommandHandler* pHandler, void* pUserData = 0) {}
	virtual void SetExternalInterfaceHandler(IExternalInterfaceHandler* pHandler, void* pUserData = 0) {}
	virtual void SetLoadMovieHandler(IFlashLoadMovieHandler* pHandler) {}
	virtual void SendCursorEvent( const SFlashCursorEvent& cursorEvent ) {}
	virtual void SendKeyEvent( const SFlashKeyEvent& keyEvent ) {}
	virtual void SetVisible(bool visible) {}
	virtual bool GetVisible() const { return false; }
	virtual bool SetVariable(const char* pPathToVar, const SFlashVarValue& value) { return true; }
	virtual bool SetVariable(const char* pPathToVar, const IFlashVariableObject* pVarObj) { return true; }
	virtual bool GetVariable(const char* pPathToVar, SFlashVarValue& value) const { return true; }
	virtual bool GetVariable(const char* pPathToVar, IFlashVariableObject*& pVarObj) const { pVarObj = 0; return true; }
	virtual bool IsAvailable(const char* pPathToVar) const { return true; }
	virtual bool SetVariableArray(EFlashVariableArrayType type, const char* pPathToVar, unsigned int index, const void* pData, unsigned int count) { return true; }
	virtual unsigned int GetVariableArraySize(const char* pPathToVar) const { return 0; }
	virtual bool GetVariableArray(EFlashVariableArrayType type, const char* pPathToVar, unsigned int index, void* pData, unsigned int count) const { return true; }
	virtual bool Invoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult) { return true; }
	virtual bool CreateString(const char* pString, IFlashVariableObject*& pVarObj) { pVarObj = 0; return true; }
	virtual bool CreateStringW(const wchar_t* pString, IFlashVariableObject*& pVarObj) { pVarObj = 0; return true; }
	virtual bool CreateObject(const char* pClassName, const SFlashVarValue* pArgs, unsigned int numArgs, IFlashVariableObject*& pVarObj) { pVarObj = 0; return true; }
	virtual bool CreateArray(IFlashVariableObject*& pVarObj) { pVarObj = 0; return true; }
	virtual bool CreateFunction(IFlashVariableObject*& pFuncVarObj, IActionScriptFunction* pFunc, void* pUserData = 0) { pFuncVarObj = 0; return true; };
	virtual unsigned int GetFrameCount() const { return 0; }
	virtual float GetFrameRate() const { return 0; }
	virtual int	GetWidth() const { return 0; }
	virtual int	GetHeight() const { return 0; }
	virtual size_t GetMetadata( char* pBuff, unsigned int buffSize ) const { return 0; }
	virtual int GetFlags() const { return 0; }
	virtual const char* GetFilePath() const { return 0; }
	virtual void ScreenToClient(int& x, int& y) const {}
	virtual void ClientToScreen(int& x, int& y) const {}
	// ~IFlashPlayer

	CFlashPlayerNULL() : m_refCount(1) {}
	~CFlashPlayerNULL() {}

private:
	int m_refCount;
};

//-----------------------------------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------------------------------