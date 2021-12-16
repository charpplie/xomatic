////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   MiniGUI.h
//  Created:     26/08/2009 by Timur.
//  Description: Interface to the Mini GUI subsystem
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MiniGUI_h__
#define __MiniGUI_h__

#include "ICryMiniGUI.h"
#include <Cry_Color.h>
#include <IHardwareMouse.h>
#include "IInput.h"

#include <CryExtension/Impl/ClassWeaver.h>

MINIGUI_BEGIN

class CMiniMenu;

//////////////////////////////////////////////////////////////////////////
// Root window all other controls derive from
class CMiniCtrl : public IMiniCtrl
{
public:

	CMiniCtrl() : 
	m_nFlags(0),
	m_id(0), 
	m_renderCallback(NULL), 
	m_fTextSize(12.f),
	m_prevX(0.f),
	m_prevY(0.f),
	m_moving(false),
	m_requiresResize(false),
	m_pCloseButton(NULL)
	{};

	//////////////////////////////////////////////////////////////////////////
	// IMiniCtrl interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Reset();
	virtual void SaveState();
	virtual void RestoreState();

	virtual void SetGUI( IMiniGUI *pGUI ) { m_pGUI = pGUI; };
	virtual IMiniGUI* GetGUI() const { return m_pGUI; };

	virtual int  GetId() const { return m_id; };
	virtual void SetId( int id ) { m_id = id; };

	virtual const char* GetTitle() const { return m_title; };
	virtual void        SetTitle( const char *title ) { m_title = title; };

	virtual Rect        GetRect() const { return m_rect; }
	virtual void        SetRect( const Rect &rc );

	virtual void        SetFlag( uint32 flag ) { set_flag(flag); }
	virtual void        ClearFlag( uint32 flag ) { clear_flag(flag); };
	virtual bool        CheckFlag( uint32 flag ) const { return is_flag(flag); }

	virtual void        AddSubCtrl( IMiniCtrl *pCtrl );
	virtual void        RemoveSubCtrl( IMiniCtrl *pCtrl );
	virtual void        RemoveAllSubCtrl();
	virtual int         GetSubCtrlCount() const;
	virtual IMiniCtrl*  GetSubCtrl( int nIndex ) const;
	virtual IMiniCtrl*  GetParent() const { return m_pParent; };

	virtual IMiniCtrl* GetCtrlFromPoint( float x,float y );
	
	virtual void SetVisible(bool state);
	
	virtual void OnEvent( float x,float y,EMiniCtrlEvent );
	
	virtual bool SetRenderCallback( RenderCallback callback ) { m_renderCallback = callback; return true; };


	// Not implemented in base control
	virtual bool SetControlCVar( const char *sCVarName,float fOffValue,float fOnValue ) { assert(0); return false; };
	virtual bool SetClickCallback( ClickCallback callback, void *pCallbackData ) { assert(0); return false; };
	virtual bool SetConnectedCtrl( IMiniCtrl* pConnectedCtrl ) { assert(0); return false; };
	//////////////////////////////////////////////////////////////////////////
	
	//////////////////////////////////////////////////////////////////////////
	virtual void AutoResize();
	
	//////////////////////////////////////////////////////////////////////////
	virtual void CreateCloseButton();

	void DrawCtrl( CDrawContext &dc );


protected:
	void set_flag( uint32 flag ) { m_nFlags |= flag; }
	void clear_flag( uint32 flag ) { m_nFlags &= ~flag; };
	bool is_flag( uint32 flag ) const { return (m_nFlags&flag) == flag; }

	//dynamic movement
	void StartMoving(float x, float y);
	void StopMoving();
	void Move(float x, float y);

protected:
	int m_id;
	IMiniGUI *m_pGUI;
	uint32 m_nFlags;
	CryFixedStringT<32> m_title;
	Rect m_rect;
	_smart_ptr<IMiniCtrl> m_pParent;
	std::vector<IMiniCtrlPtr> m_subCtrls;
	RenderCallback m_renderCallback;
	float m_fTextSize;
	
	//optional close 'X' button on controls
	IMiniCtrl *m_pCloseButton;

	//dynamic movement
	float m_prevX;
	float m_prevY;
	bool m_moving;
	bool m_requiresResize;
};

//////////////////////////////////////////////////////////////////////////
class CMiniGUI : public IMiniGUI, IHardwareMouseEventListener, public IInputEventListener
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IMiniGUI)
	CRYINTERFACE_END()
	CRYGENERATE_SINGLETONCLASS(CMiniGUI, "MiniGUI", 0x1a049b879a4e4b58, 0xac14026e17e6255e)

public:
	void InitMetrics();
	void ProcessInput();

	//////////////////////////////////////////////////////////////////////////
	// IMiniGUI interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Init();
	virtual void Done();
	virtual void Draw();
	virtual void Reset();
	virtual void SaveState();
	virtual void RestoreState();
	virtual void SetEnabled(bool status);
	virtual void SetInFocus(bool status);
	virtual bool InFocus() {return m_inFocus;}

	virtual void SetEventListener( IMiniGUIEventListener *pListener );

	virtual SMetrics& Metrics();

	virtual void OnCommand( SCommand &cmd );

	virtual void RemoveAllCtrl();
	virtual IMiniCtrl* CreateCtrl( IMiniCtrl *pParentCtrl,int nCtrlID,EMiniCtrlType type,int nCtrlFlags,const Rect &rc,const char *title );

	virtual IMiniCtrl* GetCtrlFromPoint( float x,float y ) const;

	void SetHighlight( IMiniCtrl *pCtrl, bool bEnable, float x, float y );
	void SetFocus( IMiniCtrl *pCtrl,bool bEnable );

	//////////////////////////////////////////////////////////////////////////
	// IHardwareMouseEventListener
	//////////////////////////////////////////////////////////////////////////
	virtual void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta = 0);
	//////////////////////////////////////////////////////////////////////////

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &rInputEvent);

protected:

	//DPad menu navigation
	void UpdateDPadMenu(const SInputEvent &rInputEvent);
	void SetDPadMenu(IMiniCtrl *pMenu);
	void CloseDPadMenu();

protected:
	bool m_bListenersRegistered;
	bool m_enabled;
	bool m_inFocus;

	SMetrics m_metrics;

	_smart_ptr<CMiniCtrl> m_pRootCtrl;

	_smart_ptr<IMiniCtrl> m_highlightedCtrl;
	_smart_ptr<IMiniCtrl> m_focusCtrl;

	IMiniGUIEventListener *m_pEventListener;

	CMiniMenu *m_pDPadMenu;
	std::vector<minigui::IMiniCtrl*> m_rootMenus;
};

MINIGUI_END

#endif __MiniGUI_h__