#ifndef _FLASH_PLAYER_INSTANCE_H_
#define _FLASH_PLAYER_INSTANCE_H_

#pragma once


#include <IFlashPlayer.h>


#ifndef EXCLUDE_SCALEFORM_SDK

#if !defined(_RELEASE)
#	define ENABLE_FLASH_INFO
#endif

#pragma warning(push)
#pragma warning(disable : 6326) // Potential comparison of a constant with another constant
#pragma warning(disable : 6011) // Dereferencing NULL pointer
#include <GFxPlayer.h>
#pragma warning(pop)

#include "GRendererXRender.h"
#include "SharedResources.h"
#include <CryThread.h>
#include <CryName.h>
#include <CryExtension/CrySharedPtr.h>

struct ICVar;
#if defined(ENABLE_FLASH_INFO)
struct SFlashProfilerData;
#endif

template<typename T>
struct SLinkNode
{
	SLinkNode()
	{
		m_pHandle = 0;
		m_pPrev = this;
		m_pNext = this;
	};

	void Link(SLinkNode* pRoot)
	{
		m_pPrev = pRoot->m_pPrev;
		m_pNext = pRoot;
		pRoot->m_pPrev->m_pNext = this;
		pRoot->m_pPrev = this;
	}

	void Unlink()
	{
		m_pPrev->m_pNext = m_pNext;
		m_pNext->m_pPrev = m_pPrev;
		m_pNext = m_pPrev = 0;
	}

	SLinkNode* m_pPrev;
	SLinkNode* m_pNext;
	T* m_pHandle;
};


class CFlashPlayer : public IFlashPlayer, IFlashPlayer_RenderProxy
{
	friend struct FunctionHandlerAdaptor;

public:
	// IFlashPlayer interface
	virtual void AddRef();
	virtual void Release();

	virtual bool Load(const char* pFilePath, unsigned int options = IFlashPlayer::RENDER_EDGE_AA);

	virtual void SetBackgroundColor(const ColorB& color);
	virtual void SetBackgroundAlpha(float alpha);
	virtual float GetBackgroundAlpha() const;
	virtual void SetViewport(int x0, int y0, int width, int height, float aspectRatio = 1.0f);
	virtual void GetViewport(int& x0, int& y0, int& width, int& height, float& aspectRatio) const;
	virtual void SetViewScaleMode(EScaleModeType scaleMode);
	virtual EScaleModeType GetViewScaleMode() const;
	virtual void SetViewAlignment(EAlignType viewAlignment);
	virtual EAlignType GetViewAlignment() const;
	virtual void SetScissorRect(int x0, int y0, int width, int height);
	virtual void GetScissorRect(int& x0, int& y0, int& width, int& height) const;
	virtual void Advance(float deltaTime);
	virtual void Render(bool stereo = false);
	virtual void SetCompositingDepth(float depth);
	virtual void SetStereoPlaneDepth(float depth);

	virtual void Restart();
	virtual bool IsPaused() const;
	virtual void Pause(bool pause);
	virtual void GotoFrame(unsigned int frameNumber);
	virtual bool GotoLabeledFrame(const char* pLabel, int offset = 0);
	virtual unsigned int GetCurrentFrame() const;
	virtual bool HasLooped() const;

	virtual void SetFSCommandHandler(IFSCommandHandler* pHandler, void* pUserData = 0);
	virtual void SetExternalInterfaceHandler(IExternalInterfaceHandler* pHandler, void* pUserData = 0);
	virtual void SendCursorEvent(const SFlashCursorEvent& cursorEvent);
	virtual void SendKeyEvent(const SFlashKeyEvent& keyEvent);

	virtual void SetVisible(bool visible);
	virtual bool GetVisible() const;

	virtual bool SetVariable(const char* pPathToVar, const SFlashVarValue& value);
	virtual bool SetVariable(const char* pPathToVar, const IFlashVariableObject* pVarObj);
	virtual bool GetVariable(const char* pPathToVar, SFlashVarValue& value) const;
	virtual bool GetVariable(const char* pPathToVar, IFlashVariableObject*& pVarObj) const;
	virtual bool IsAvailable(const char* pPathToVar) const;
	virtual bool SetVariableArray(EFlashVariableArrayType type, const char* pPathToVar, unsigned int index, const void* pData, unsigned int count);
	virtual unsigned int GetVariableArraySize(const char* pPathToVar) const;
	virtual bool GetVariableArray(EFlashVariableArrayType type, const char* pPathToVar, unsigned int index, void* pData, unsigned int count) const;
	virtual bool Invoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult = 0);

	virtual bool CreateString(const char* pString, IFlashVariableObject*& pVarObj);
	virtual bool CreateStringW(const wchar_t* pString, IFlashVariableObject*& pVarObj);
	virtual bool CreateObject(const char* pClassName, const SFlashVarValue* pArgs, unsigned int numArgs, IFlashVariableObject*& pVarObj);
	virtual bool CreateArray(IFlashVariableObject*& pVarObj);
	virtual bool CreateFunction(IFlashVariableObject*& pFuncVarObj, IActionScriptFunction* pFunc, void* pUserData = 0);

	virtual unsigned int GetFrameCount() const;
	virtual float GetFrameRate() const;
	virtual int GetWidth() const;
	virtual int GetHeight() const;
	virtual size_t GetMetadata(char* pBuff, unsigned int buffSize) const;
	virtual const char* GetFilePath() const;

	virtual void ScreenToClient(int& x, int& y) const;
	virtual void ClientToScreen(int& x, int& y) const;

	// IFlashPlayer_RenderProxy interface
	virtual void RenderCallback(EFrameType ft, bool releaseOnExit = true);

public:
	CFlashPlayer();
	virtual ~CFlashPlayer();

	void DelegateFSCommandCallback(const char* pCommand, const char* pArgs);
	void DelegateExternalInterfaceCallback(const char* pMethodName, const GFxValue* pArgs, UInt numArgs);

public:
	static void RenderFlashInfo();
	static void SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler);
	static IFlashLoadMovieHandler* GetFlashLoadMovieHandler();
	static void InitCVars();
	static int GetWarningLevel();

	enum ELogOptions
	{
		LO_LOADING			= 0x01, // log flash loading
		LO_ACTIONSCRIPT = 0x02, // log flash action script execution
		LO_PEAKS				= 0x04, // log high-level flash function calls which cause peaks
	};
	static unsigned int GetLogOptions();

private:
	bool IsEdgeAaAllowed() const;
	void UpdateRenderFlags();
	void UpdateASVerbosity();
	void Link();
	void Unlink();

private:
	static bool IsFlashEnabled();

private:
#if defined(ENABLE_FLASH_INFO)
	static ICVar* CV_sys_flash_info_peak_exclude;
#endif
	static int ms_sys_flash;
	static int ms_sys_flash_edgeaa;
#if defined(ENABLE_FLASH_INFO)
	static int ms_sys_flash_info;
	static float ms_sys_flash_info_peak_tolerance;
	static float ms_sys_flash_info_histo_scale;
#endif
	static int ms_sys_flash_log_options;
	static float ms_sys_flash_curve_tess_error;
	static int ms_sys_flash_warning_level;

	static SLinkNode<CFlashPlayer> ms_rootNode;
	static IFlashLoadMovieHandler* ms_pLoadMovieHandler;

private:
	volatile int m_refCount;
	bool m_allowEgdeAA;
	float m_compDepth;
	float m_stereoPlaneDepth;
	GFxRenderConfig m_renderConfig;
	GFxActionControl m_asVerbosity;
	IFSCommandHandler* m_pFSCmdHandler;
	void* m_pFSCmdHandlerUserData;
	IExternalInterfaceHandler* m_pEIHandler;
	void* m_pEIHandlerUserData;
	GPtr<GFxMovieDef> m_pMovieDef;
	GPtr<GFxMovieView> m_pMovieView;
	GPtr<GFxLoader2> m_pLoader;
	GPtr<GRendererXRender> m_pRenderer;
	CCryName m_filePath;
	SLinkNode<CFlashPlayer> m_node;
#if defined(ENABLE_FLASH_INFO)
	mutable SFlashProfilerData* m_pProfilerData;
#endif
	cryshared_ptr<CryCriticalSection> m_lock;
	GFxValue m_retValRefHolder;
};


#endif // #ifndef EXCLUDE_SCALEFORM_SDK


#endif // #ifndef _FLASH_PLAYER_INSTANCE_H_
