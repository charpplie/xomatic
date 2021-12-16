#ifndef __FlashHUD_H__
#define __FlashHUD_H__

#include "IFlashPlayer.h"
#include "IInput.h"
#include "ILevelSystem.h"

class CFlashHUD :	public IInputEventListener, 
					public ILevelSystemListener
{
public:
	CFlashHUD();
	~CFlashHUD();

	void Draw(float fDeltaTime);

	// IInputEventListener
	virtual bool OnInputEvent( const SInputEvent &event );
	// ~IInputEventListener

	// ILevelSystemListener
	virtual void OnLevelNotFound(const char *levelName) {};
	virtual void OnLoadingStart(ILevelInfo *pLevel) {};
	virtual void OnLoadingComplete(ILevel *pLevel);
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error) {};
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount) {};
	// ~ILevelSystemListener


private:
	IFlashPlayer* m_pFlashPlayer;
	float m_fCurrScale;
	int m_iWidth;
	int m_iHeight;
	bool m_bScaleEnabled;

	struct LevelMapInfo {
		LevelMapInfo() : fStartX(0), fStartY(0), fEndX(1), fEndY(1), fDimX(1), fDimY(1) {}
		string sMinimapName;
		float fStartX;
		float fStartY;
		float fEndX;
		float fEndY;
		float fDimX;
		float fDimY;
	};

	LevelMapInfo m_LevelMapInfo;

	enum ERadarEntityType {
		eRET_Item = 0,
		eRET_Actor = 1,
		eRET_Vehicle = 2,
	};

	struct RadarEntity
	{
		RadarEntity(int id, ERadarEntityType type, float posx, float posy, int rotation, int health = 0) : iId(id), eType(type), fPosX(posx), fPosY(posy), iRotation(rotation), iHealth(health) {}
		int iId;
		ERadarEntityType eType;
		float fPosX;
		float fPosY;
		int iRotation;
		int iHealth;
	};

	std::list<RadarEntity> m_EntitiesInProximity;

	void ScanProximity(IEntity *pPlayerEntity, float radius);
	void updateViewPort();
	bool CanDrawCrosshair() const;
};

#endif // #ifndef  __FlashHUD_H__