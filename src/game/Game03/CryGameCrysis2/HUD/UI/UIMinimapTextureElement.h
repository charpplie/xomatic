/************************************************************************/
/* UI Minimap Texture, Ben Parbury , 2009						  									*/
/************************************************************************/

#ifndef UI_MINIMAP_TEXTURE_ELEMENT_H
#define UI_MINIMAP_TEXTURE_ELEMENT_H

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventDispatcher.h"
#include <IHardwareMouse.h>
#include "IInput.h"

struct ILevelInfo;
struct IHUDAsset;

class CUIMinimapTextureElement : public CUIElement, public IHUDEventListener, IHardwareMouseEventListener, IInputEventListener
{
public:
	CUIMinimapTextureElement();
	virtual ~CUIMinimapTextureElement();

	//IHUDEventListener
	virtual void OnHUDEvent(const SHUDEvent& event);

	//IHardwareMouseEventListener
	virtual void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta = 0);

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &rInputEvent);
	virtual bool OnInputEventUI(const SInputEvent &rInputEvent) { return false; }

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		CUIElement::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
protected:
	void Update(float frameTime);
	void Draw(void) const;

	void LoadMinimap();
	void LoadMinimap(ILevelInfo *pLevel, const XmlNodeRef node);

	void SetRotation(float rot);
	void AttachToRadar(IHUDAsset* radar);
	void AdjustPosAndSize(float x, float y, float w, float h);

	float GetMinimapFraction(float start, float end, float current);
	float GetGameXFromMapFraction(float fraction);
	float GetGameYFromMapFraction(float fraction);
	
	static float GetPosFromFraction(float fraction, float start, float end);

	float GetMinimapFractionFromScreen(float posX, float mapPosX, float mapWidth);

	bool GetGameCoordFromMouse(float *gameX, float* gameY);
	void DeployStrikeAtGameCoord(float x, float y);

	Vec2 m_mapCenter;

	ITexture *m_minimapTexture;
	ITexture *m_minimapTextureMask;
	ITexture *m_mapTextureMask;
	IHUDAsset *m_attached;

	float m_startX;
	float m_startY;
	float m_endX;
	float m_endY;
	float m_scale;
	float m_mapTextureScale;
	float m_playerX;
	float m_playerY;
	float m_rotation;
	float m_hud_minimapOverrideScale;		//cvar
	float m_hud_minimapMapOverrideScale;//cvar
	float m_hud_mapDeployDebug;//cvar
	float m_blindAlpha;

	bool m_usingMenuMap;
	bool m_wantToUseMenu;
	bool m_enabled;

	float m_radar_x;
	float m_radar_y;
	float m_radar_w;
	float m_radar_h;
};

#endif	// UI_MINIMAP_TEXTURE_ELEMENT_H
