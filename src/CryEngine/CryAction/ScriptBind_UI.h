/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Exposes basic UI functionality to the Script System.
  
 -------------------------------------------------------------------------
  History:
  - 16:8:2004   17:28 : Created by Márcio Martins

*************************************************************************/
#ifndef __SCRIPTBIND_UI_H__
#define __SCRIPTBIND_UI_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IScriptSystem.h>
#include <ScriptHelpers.h>


struct ISystem;
class CUISystem;
struct IGameFramework;

// <title UI>
// Syntax: UI
class CScriptBind_UI :
	public CScriptableBase
{
public:
	CScriptBind_UI(ISystem *pSystem, CUISystem *pUISystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_UI();

	void Release() { delete this; };

	// Script Events
	enum EUIScriptEvent
	{
		eUISE_OnInit = 0,
		eUISE_OnRelease,
		eUISE_OnPreUpdate,
		eUISE_OnPostUpdate,
		eUISE_OnIdle,
		eUISE_OnKeyDown,
		eUISE_OnKeyUp,
		eUISE_OnMouseDown,
		eUISE_OnMouseUp,
		eUISE_OnMouseMove,
		eUISE_OnLevelNotFound,
		eUISE_OnLoadingStart,
		eUISE_OnLoadingComplete,
		eUISE_OnLoadingError,
		eUISE_OnLoadingProgress,
		eUISE_Last,
	};
	
	void CallScriptEvent(EUIScriptEvent event)
	{
		if (m_onEventPtr[event])
		{
			Script::CallMethod(m_scriptTable, m_onEventPtr[event]);
		}
	};

	template <typename T1>
	void CallScriptEvent(EUIScriptEvent event, T1 p1)
	{
		if (m_onEventPtr[event])
		{
			Script::CallMethod(m_scriptTable, m_onEventPtr[event], p1);
		}
	};

	template <typename T1, typename T2>
	void CallScriptEvent(EUIScriptEvent event, T1 p1, T2 p2)
	{
		if (m_onEventPtr[event])
		{
			Script::CallMethod(m_scriptTable, m_onEventPtr[event], p1, p2);
		}
	};

	template <typename T1, typename T2, typename T3>
	void CallScriptEvent(EUIScriptEvent event, T1 p1, T2 p2, T3 p3)
	{
		if (m_onEventPtr[event])
		{
			Script::CallMethod(m_scriptTable, m_onEventPtr[event], p1, p2, p3);
		}
	};

	template <typename T1, typename T2, typename T3, typename T4>
	void CallScriptEvent(EUIScriptEvent event, T1 p1, T2 p2, T3 p3, T4 p4)
	{
		if (m_onEventPtr[event])
		{
			Script::CallMethod(m_scriptTable, m_onEventPtr[event], p1, p2, p3, p4);
		}
	};


	// Internal Events

	void OnInit();
	void OnRelease();


	// Script Functions

	// <title Reset>
	// Syntax: UI.Reset()
	// Description:
	//		Resets the UI.
	int Reset(IFunctionHandler *pH);
	// <title EnableInput>
	// Syntax: UI.EnableInput( bool enable )
	// Arguments:
	//		enable - True to enable the input, false to disable it.
	// Description:
	//		Enables/disables the input.
	int EnableInput(IFunctionHandler *pH, bool enable);

	// <title Begin2D>
	// Syntax: UI.Begin2D( int width, int height )
	// Arguments:
	//		width	- Width size.
	//		height	- Height size.
	// Description:
	//		Begins the 2D.
	int Begin2D(IFunctionHandler *pH, int width, int height);
	// <title Clear>
	// Syntax: UI.Clear(SmartScriptTable color)
	// Arguments:
	//		color - Color used to clear the UI.
	// Description:
	//		Clears the screen with the specified color.
	int Clear(IFunctionHandler *pH, SmartScriptTable color);
	// <title SetScissor>
	// Syntax: UI.SetScissor( int l, int t, int w, int h )
	// Arguments:
	//		l - X position.
	//		t - Y position.
	//		w - Width.
	//		h - Height.
	// Description:
	//		Sets scissor value.
	int SetScissor(IFunctionHandler *pH);
	// <title SetBlend>
	// Syntax: UI.SetBlend( int src, int dst )
	// Arguments:
	//		src - Source. Can be one of:
	//				GS_BLSRC_MASK
	//				GS_BLSRC_ZERO
	//				GS_BLSRC_ONE
	//				GS_BLSRC_DSTCOL
	//				GS_BLSRC_ONEMINUSDSTCOL
	//				GS_BLSRC_SRCALPHA
	//				GS_BLSRC_ONEMINUSSRCALPHA
	//				GS_BLSRC_DSTALPHA
	//				GS_BLSRC_ONEMINUSDSTALPHA
	//				GS_BLSRC_ALPHASATURATE
	//		dst - Destination. Can be one of:
	//				GS_BLDST_MASK
	//				GS_BLDST_ZERO
	//				GS_BLDST_ONE
	//				GS_BLDST_SRCCOL
	//				GS_BLDST_ONEMINUSSRCCOL
	//				GS_BLDST_SRCALPHA
	//				GS_BLDST_ONEMINUSSRCALPHA
	//				GS_BLDST_DSTALPHA
	//				GS_BLDST_ONEMINUSDSTALPHA
	// Description:
	//		Sets the blend.
	int SetBlend(IFunctionHandler *pH);
	// <title DrawLine>
	// Syntax: UI.DrawLine( float x0, float y0, float x1, float y1, SmartScriptTable colorTbl )
	// Arguments:
	//		x0 - X value of the start point of the line.
	//		y0 - Y value of the start point of the line.
	//		x1 - X value of the end point of the line.
	//		y1 - Y value of the end point of the line.
	//		colorTbl - color for the line.
	// Description:
	//		Draws a line on the screen.
	int DrawLine(IFunctionHandler *pH);
	// <title DrawTri>
	// Syntax: UI.DrawTri(float x0, float y0, float x1, float y1, float x2, float y2)
	// Arguments:
	//		x0 - X value of the first vertex of the triangle.
	//		y0 - Y value of the first vertex of the triangle.
	//		x1 - X value of the second vertex of the triangle.
	//		y1 - Y value of the second vertex of the triangle.
	//		x2 - X value of the third vertex of the triangle.
	//		y2 - Y value of the third vertex of the triangle.
	// Description:
	//		Draws a triangle on the screen.
	int DrawTri(IFunctionHandler *pH);
	// <title DrawQuad>
	// Syntax: UI.DrawQuad( float l, float t, float w, float h, ScriptHandle texture, SmartScriptTable	colorTbl, SmartScriptTable texCoordTbl)
	// Arguments:
	//		l			- X position.
	//		t			- Y position.
	//		w			- Quad width.
	//		h			- Quad height.
	//		texture		- Texture identifier.
	//		colorTbl	- Color.
	//		texCoordTbl	- Texture coordinates.
	// Description:
	//		Draws a quad.
	int DrawQuad(IFunctionHandler *pH);
	// <title DrawCircle>
	// Syntax: UI.DrawCircle( float x, float y, float radius, int slices, SmartScriptTable colorTbl )
	// Arguments:
	//		x			- X position.
	//		y			- Y position.
	//		radius		- Radius of the circle.
	//		slices		- Number of slices.
	//		colorTbl	- Circle color.
	// Description:
	//		Draws a circle.
	int DrawCircle(IFunctionHandler *pH);
	// <title Flush2D>
	// Syntax: UI.Flush2D()
	// Description:
	//		Flushes the 2D graphic.
	int Flush2D(IFunctionHandler *pH);
	// <title End2D>
	// Syntax: UI.End2D()
	// Description:
	//		Ends the 2D graphics.
	int End2D(IFunctionHandler *pH);

	// <title Render3DEntity>
	// Syntax: UI.Render3DEntity( ScriptHandle entityId )
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Renders a 3D entity.
	int Render3DEntity(IFunctionHandler *pH, ScriptHandle entityId);

	// <title GetScreenWidth>
	// Syntax: UI.GetScreenWidth()
	// Description:
	//		Gets the screen width.
	int GetScreenWidth(IFunctionHandler *pH);
	// <title GetScreenHeight>
	// Syntax: UI.GetScreenHeight()
	// Description:
	//		Gets the screen height.
	int GetScreenHeight(IFunctionHandler *pH);

	// <title GetWidth>
	// Syntax: UI.GetWidth()
	int GetWidth(IFunctionHandler *pH);
	// <title GetHeight>
	// Syntax: UI.GetHeight()
	int GetHeight(IFunctionHandler *pH);

	// <title PauseGame>
	// Syntax: UI.PauseGame( bool pause )
	// Arguments:
	//		pause - True to pause the game, false to resume.
	// Description:
	//		Pauses/resumes the game.
	int PauseGame(IFunctionHandler *pH, bool pause);
	// <title IsGamePaused>
	// Syntax: UI.IsGamePaused()
	// Description:
	//		Checks if the game is paused.
	int IsGamePaused(IFunctionHandler *pH);

	// <title Localize>
	// Syntax: UI.Localize()
	int Localize(IFunctionHandler *pH);
	// <title SetFont>
	// Syntax: UI.SetFont( const char *fontName, const char *fontEffect)
	// Arguments:
	//		fontName	- Font name.
	//		fontEffect	- Font effect.
	// Description:
	//		Sets a font name and effect.
	int SetFont(IFunctionHandler *pH);
	// <title SetFont>
	// Syntax: UI.SetFont(int x, int y, int text, int size, SmartScriptTable colorTbl, int halign, int valign)
	// Arguments:
	//		x			- X position.
	//		y			- Y position.
	//		text		- Text string.
	//		size		- Size of the text.
	//		colorTbl	- Text color.
	//		halign		- Horizontal align of the text. Can be one of:
	//							eUIA_Left,
	//							eUIA_Right,
	//							eUIA_Center,
	//							eUIA_Middle,
	//							eUIA_Top,
	//							eUIA_Bottom
	//		valign		- Vertical align of the text. Can be one of
	//							eUIA_Left,
	//							eUIA_Right,
	//							eUIA_Center,
	//							eUIA_Middle,
	//							eUIA_Top,
	//							eUIA_Bottom
	// Description:
	//		Draws a text.
	int DrawText(IFunctionHandler *pH);
	// <title SetFont>
	// Syntax: UI.SetFont( const char* text, float size )
	// Arguments:
	//		text - Text value.
	//		size - Size of the text.
	// Description:
	//		Gets the text size.
	int GetTextSize(IFunctionHandler *pH);

	// <title LoadImage>
	// Syntax: UI.LoadImage( const char *imageName, [optional] bool reload )
	// Arguments:
	//		imageName - Image name string.
	//		reload	  - True to reload, false otherwise. Default value is false.
	// Description:
	//		Loads an image.
	int LoadImage(IFunctionHandler *pH);
	// <title FreeImage>
	// Syntax: UI.FreeImage( ScriptHandle tex )
	// Arguments:
	//		tex - Texture identifier.
	// Description:
	//		Frees an image.
	int FreeImage(IFunctionHandler *pH);
	// <title GetImageSize>
	// Syntax: UI.GetImageSize( ScriptHandle tex )
	// Arguments:
	//		tex - Texture identifier.
	// Description:
	//		Gets the image size.
	int GetImageSize(IFunctionHandler *pH);

	// <title CreateRenderTarget>
	// Syntax: UI.CreateRenderTarget( int width, int height )
	// Arguments:
	//		width	- Width size.
	//		height	- Height size.
	// Description:
	//		Creates a render target.
	int CreateRenderTarget(IFunctionHandler *pH);
	// <title DestroyRenderTarget>
	// Syntax: UI.DestroyRenderTarget( ScriptHandle id )
	// Arguments:
	//		id - Render target identifier.
	// Description:
	//		Destroys a render target.
	int DestroyRenderTarget(IFunctionHandler *pH);
	// <title SetRenderTarget>
	// Syntax: UI.SetRenderTarget( ScriptHandle id )
	// Arguments:
	//		id - Render target identifier.
	// Description:
	//		Sets a render target.
	int SetRenderTarget(IFunctionHandler *pH);

	// <title GetCurrentLevelInfo>
	// Syntax: UI.GetCurrentLevelInfo()
	// Description:
	//		Gets the current level info.
	int GetCurrentLevelInfo(IFunctionHandler *pH);

private:
	
	void RegisterMethods();
	void RegisterGlobals();

	CUISystem					*m_pUISystem;
	IGameFramework		*m_pGameFramework;
	SmartScriptTable	m_scriptTable;
	SmartScriptTable	m_levelInfo;

	HSCRIPTFUNCTION		m_onEventPtr[eUISE_Last];
};

#endif //__SCRIPTBIND_UI_H__