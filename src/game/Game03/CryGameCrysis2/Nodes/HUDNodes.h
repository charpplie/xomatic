#ifndef _HUDFADER_H_
#define _HUDFADER_H_

#include "HUD/HUDEventDispatcher.h"

static const int NUM_FADERS = 4;
static const int MFX_FADER_OFFSET = 0;
static const int MFX_FADER_END = 3;
static const int GAME_FADER_OFFSET = 0;

//////////////////////////////////////////////////////////////////////////
class CHUDFader
{
public:
	CHUDFader();
	virtual ~CHUDFader();

	const char* GetDebugName() const;
	ColorF GetCurrentColor() const;

	void Stop();
	void Reset();

	inline bool IsActive() const { return m_bActive; }
	inline bool IsPlaying(int ticket) const { return m_iTicket > 0 && m_iTicket==ticket; }

	int FadeIn(const ColorF& targetColor, float fDuration, bool bUseCurrentColor=true);
	int FadeOut(const ColorF& targetColor, float fDuration, const char* textureName="", bool bUseCurrentColor=true);

	virtual void Update(float fDeltaTime);
	virtual void Draw();

	static ITexture* LoadTexture(const char* textureName);
	inline void SetPosition(float fTopLeftX, float fTopLeftY) { m_fTopLeftX = fTopLeftX; m_fTopLeftY = fTopLeftY; }
	inline void SetDimension(float fWidth, float fHeight) { m_fWidth = fWidth; m_fHeight = fHeight; }
	inline void SetRotation(float fRotation) { m_fRotation = fRotation; }
	inline void SetZLevel(int iZLevel) { m_iZLevel = iZLevel; }
	inline int GetZLevel() const { return m_iZLevel; }
	inline bool IsDrawnBeforeHUD() const { return m_bDrawBeforeHUD; }
	inline void SetDrawBeforeHUD(bool drawBeforeHUD) { m_bDrawBeforeHUD = drawBeforeHUD; }
	inline float GetAlpha() const { return m_fAlpha; }
	inline void SetAlpha(float alpha) { m_fAlpha = clamp(alpha,0.0f,1.0f); }

	void SetTexture(const char* textureName);

	inline bool IsFading() const { return m_bIsFading; }

protected:

	IRenderer	  *m_pRenderer;
	ColorF         m_currentColor;
	ColorF         m_targetColor;
	ColorF         m_drawColor;

	CRY_FIXME( 23, 09, 2009, "Store the texture ID not a pointer to it." );
	ITexture      *m_pTexture;
	float          m_fDuration;
	float          m_fCurTime;
	int            m_iDirection;
	int            m_iTicket;
	bool           m_bActive;

	float		m_fTopLeftX;
	float		m_fTopLeftY;
	float		m_fWidth;
	float		m_fHeight;
	float		m_fRotation;
	float		m_fAlpha;
	int			m_iZLevel;

	bool			m_bIsFading;
	bool			m_bDrawBeforeHUD;

};

//////////////////////////////////////////////////////////////////////////

class CHUDFaderObject : public IHUDEventListener //: public CHUDObject
{
private:
	CHUDFaderObject();
	static CHUDFaderObject m_instance;

public:
	virtual ~CHUDFaderObject();

	static CHUDFaderObject* GetHUDFaderObject()
	{
		return &m_instance;
	}

	virtual void OnHUDEvent(const SHUDEvent& event);

	CHUDFader* GetHUDFader(int iZLevel);
	virtual void Update( float fDeltaTime );
	virtual void PostHUDDraw();
	virtual void OnHUDToBeDestroyed();

	void Reset();

	virtual void Serialize(TSerialize ser);
	void Register();
	void UnRegister();

private:
	bool m_bRegistered;
	std::list <CHUDFader*> m_listHUDFader;
};
extern CHUDFaderObject* g_pHUDFader;

#endif // _HUDFADER_H_