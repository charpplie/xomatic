#ifndef __FLASH_H__
#define __FLASH_H__

#include "IFlashPlayer.h"

//////////////////////////////////////////////////////////////////////////

class CFlashPlayerNull : public IFlashPlayer
{

public:

	virtual void AddRef() {}
	virtual void Release() {}

	// Summary:
	//	 Initialization.
	virtual bool Load(const char* pFilePath, unsigned int options = DEFAULT) {return true; }
	virtual void SetBackgroundColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {}
	virtual void SetBackgroundAlpha(float alpha) {}
	virtual void SetViewport(int x0, int y0, int width, int height, float aspectRatio = 1.0f) {}
	virtual void GetViewport(int& x0, int& y0, int& width, int& height, float& aspectRatio) const {}
	virtual void SetViewScaleMode(EScaleModeType scaleMode) {}
	virtual void SetScissorRect(int x0, int y0, int width, int height) {}
	virtual void GetScissorRect(int& x0, int& y0, int& width, int& height) const {}
	virtual void Advance(float deltaTime) {}
	virtual void Render(bool stereo = false) {}
	virtual void SetCompositingDepth(float depth) {}
	virtual void SetFSCommandHandler(IFSCommandHandler* pHandler, void* pUserData = 0) {}
	virtual void SetExternalInterfaceHandler(IExternalInterfaceHandler* pHandler, void* pUserData = 0){}
	virtual void SendCursorEvent(const SFlashCursorEvent& cursorEvent) {}
	virtual void SendKeyEvent(const SFlashKeyEvent& keyEvent) {}
	virtual void SetVisible(bool visible) {}
	virtual bool GetVisible() const {return false; }
	virtual bool SetVariable(const char* pPathToVar, const SFlashVarValue& value) {return true; }
	virtual bool SetVariable(const char* pPathToVar, const IFlashVariableObject* pVarObj) { return true; }
	virtual bool GetVariable(const char* pPathToVar, SFlashVarValue& value) const {return true; }
	virtual bool GetVariable(const char* pPathToVar, IFlashVariableObject*& pVarObj) const { pVarObj = 0; return true; }
	virtual bool IsAvailable(const char* pPathToVar) const {return true; }
	virtual bool SetVariableArray(EFlashVariableArrayType type, const char* pPathToVar, unsigned int index, const void* pData, unsigned int count) {return true; }
	virtual unsigned int GetVariableArraySize(const char* pPathToVar) const {return 0; }
	virtual bool GetVariableArray(EFlashVariableArrayType type, const char* pPathToVar, unsigned int index, void* pData, unsigned int count) const {return true; }
	virtual bool Invoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult = 0) {return true; }
	virtual bool CreateString(const char* pString, IFlashVariableObject*& pVarObj) { pVarObj = 0; return true; }
	virtual bool CreateStringW(const wchar_t* pString, IFlashVariableObject*& pVarObj) { pVarObj = 0; return true; }
	virtual bool CreateObject(const char* pClassName, const SFlashVarValue* pArgs, unsigned int numArgs, IFlashVariableObject*& pVarObj) { pVarObj = 0; return true; }
	virtual bool CreateArray(IFlashVariableObject*& pVarObj) { pVarObj = 0; return true; }
	virtual int GetWidth() const {return 0; }
	virtual int GetHeight() const {return 0; }
	virtual size_t GetMetadata(char* pBuff, unsigned int buffSize) const {return 0; }
	virtual const char* GetFilePath() const {return ""; }
	virtual void ScreenToClient(int& x, int& y) const {}
	virtual void ClientToScreen(int& x, int& y) const {}

	CFlashPlayerNull() {}
	virtual ~CFlashPlayerNull() {}

		virtual void SetBackgroundColor(const ColorB& color); //Diesel new


};

//////////////////////////////////////////////////////////////////////////

class CFlash
{

public:

	static CFlash*					Get()
	{
		if(!m_pInst)
			m_pInst = new CFlash();
		return m_pInst;
	}


	IFlashPlayer*					CreateSafeFlashPlayerInstance();
	static IFlashPlayer*			s_pFlashPlayerNull;

private:

	static CFlash*					m_pInst;

	CFlash();

	virtual ~CFlash();

};

//////////////////////////////////////////////////////////////////////////

#endif

