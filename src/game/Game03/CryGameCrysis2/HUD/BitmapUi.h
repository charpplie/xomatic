#ifndef __Bitmap_Crosshair__h__
#define __Bitmap_Crosshair__h__

#include <ILevelSystem.h>

struct IGameFramework;
struct IUIDraw;

class CBitmapUi
	: public ILevelSystemListener
{
private:
	enum EShowScreen
	{
		eSS_NONE = 0,
		eSS_LOADING,
		eSS_START,
		eSS_CREDITS
	};

public:
	CBitmapUi();
	virtual ~CBitmapUi();

	void	Draw() const;

	// ILevelSystemListener
	VIRTUAL void OnLevelNotFound(const char* levelName) {}
	VIRTUAL void OnLoadingStart(ILevelInfo* pLevel) {}
	VIRTUAL void OnLoadingComplete(ILevel* pLevel) {}
	VIRTUAL void OnLoadingError(ILevelInfo* pLevel, const char* error) {}
	VIRTUAL void OnLoadingProgress(ILevelInfo* pLevel, int progressAmount);
	// ~ILevelSystemListener

protected:
	bool	CanDraw() const;

	bool	CanDrawCrosshair() const;
	void	DrawCrosshair() const;	

	void	DrawFullscreen() const;
	void	DrawPauseScreen() const;

	void	DrawCentered(int texId, float width, float height) const;

	void	UpdateCVars() const;

private:	
	IGameFramework*	m_pGameFramework;
	IUIDraw*				m_pUiDraw;

	int							m_iTexCrosshair;
	int							m_iTexLoadingScreen;
	int							m_iTexStartScreen;
	int							m_iTexCreditsScreen;
	int							m_iTexPauseScreen;
};

#endif