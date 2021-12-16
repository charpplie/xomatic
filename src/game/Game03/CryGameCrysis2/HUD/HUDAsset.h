#ifndef __HUDASSET_H__
#define __HUDASSET_H__

//////////////////////////////////////////////////////////////////////////

#include "HUD/HUD.h"
#include "HUD/HUDEventDispatcher.h"
#include "HUD/ScreenLayoutManager.h"

#include "IFlashPlayer.h"
#include "Flash/Flash.h"

//////////////////////////////////////////////////////////////////////////
// Forward decs

// ...

//////////////////////////////////////////////////////////////////////////

struct IItemParamsNode;


struct SHUDSizeAndPositionInfo : IHUDEventListener
{
public:

	SHUDSizeAndPositionInfo();
	~SHUDSizeAndPositionInfo();

	void Init(const IItemParamsNode * asset, ScreenLayoutManager* pScreenLayout);
	void SetPositionAndSize(float x, float y, float sizeX, float sizeY);
	void SetAssetSize(int width, int height);

	void Recalculate();

	void CalculateSize();
	void CalculatePosition();

	//IHUDEventListener
	virtual void OnHUDEvent(const SHUDEvent& event);
	//~IHUDEventListener

	void DebugDraw( const char* label );

private:

	void SetCanvasSize(int width, int height);

public :

	int x;
	int y;
	int w;
	int h;

private :

	Vec2 m_canvasSize;
	Vec2 m_assetSize;

	Vec2 m_size;

	Vec2 m_relPos;

	Vec2i m_pivotAlignment;

	ScreenLayoutManager* m_screenLayout;
	ScreenLayoutStates   m_layoutFlags;

};


//////////////////////////////////////////////////////////////////////////


struct IHUDAsset
{

	virtual ~IHUDAsset() {}

	virtual void	SetPath(const string& path) = 0;
	virtual void	Init(const IItemParamsNode * asset, ScreenLayoutManager* pScreenLayout) = 0;
	virtual void	SetPositionAndSize(const float x, const float y, const float sizeX, const float sizeY) = 0;
	virtual void	Update(float frameTime) = 0;
	virtual void	Draw() = 0;

	virtual void	Invoke(const char* path, SFlashVarValue* pResult = 0) = 0;
	virtual void	Invoke(const char* path, int arg, SFlashVarValue* pResult = 0) = 0;
	virtual void	Invoke(const char* path, float arg, SFlashVarValue* pResult = 0) = 0;
	virtual void	Invoke(const char* path, bool arg, SFlashVarValue* pResult = 0) = 0;
	virtual void	Invoke(const char* path, const char* arg, SFlashVarValue* pResult = 0) = 0;
	virtual void	Invoke(const char* path, const wchar_t* arg, SFlashVarValue* pResult = 0) = 0;
	virtual void	Invoke(const char* path, const SFlashVarValue& arg, SFlashVarValue* pResult = 0) = 0;
	virtual void	Invoke(const char* path, SFlashVarValue* args,  unsigned int numArgs, SFlashVarValue* pResult = 0) = 0;

	virtual void	SetVariable(const char* path, int arg) = 0;
	virtual void	SetVariable(const char* path, float arg) = 0;
	virtual void	SetVariable(const char* path, bool arg) = 0;
	virtual void	SetVariable(const char* path, const char* arg) = 0;
	virtual void	SetVariable(const char* path, const wchar_t* arg) = 0;
	virtual void	SetVariable(const char* path, const SFlashVarValue& arg) = 0;

	virtual void	GetVariable(const char* path, int& arg) const = 0;
	virtual void	GetVariable(const char* path, float& arg) const = 0;
	virtual void	GetVariable(const char* path, string& arg) const = 0;
	virtual void	GetVariable(const char* path, SFlashVarValue& arg) const = 0;
	virtual void	GetVariable(const char* path, IFlashVariableObject*& arg) const = 0;

	virtual void	SetVariableArrayDouble(const char* path, std::vector<double> *doubleArray) = 0;
	virtual IFlashPlayer* GetFlashPlayer() = 0;

	virtual int		GetWidth() const = 0;
	virtual int		GetHeight() const = 0;

	virtual void	SetName(const char* name) {m_name = name; }
	virtual const char*	GetName() const { return m_name.c_str(); }

	virtual const SHUDSizeAndPositionInfo* GetInfo() { return &m_info; }

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const=0;
	// Handle HUD-Data parameter registration here - Must be done on load.
	//virtual void	RegisterParameters( void ) = 0;

	SHUDSizeAndPositionInfo m_info; 
	CryFixedStringT<16>			m_name;

};


//////////////////////////////////////////////////////////////////////////


class CFlashAsset : public IHUDAsset
{

public:

	CFlashAsset();
	CFlashAsset(IFlashPlayer* pPlayer);
	virtual ~CFlashAsset();

	virtual void	SetPath(const string& path);
	virtual void 	Init(const IItemParamsNode * params, ScreenLayoutManager* pScreenLayout);
	virtual void	SetPositionAndSize(const float x, const float y, const float sizeX, const float sizeY);
	virtual void	Update(float frameTime);
	virtual void	Draw();

	virtual void	Invoke(const char* path, SFlashVarValue* pResult = 0);
	virtual void	Invoke(const char* path, int arg, SFlashVarValue* pResult = 0);
	virtual void	Invoke(const char* path, float arg, SFlashVarValue* pResult = 0);
	virtual void	Invoke(const char* path, bool arg, SFlashVarValue* pResult = 0);
	virtual void	Invoke(const char* path, const char* arg, SFlashVarValue* pResult = 0);
	virtual void	Invoke(const char* path, const wchar_t* arg, SFlashVarValue* pResult = 0);
	virtual void	Invoke(const char* path, const SFlashVarValue& arg, SFlashVarValue* pResult = 0);
	virtual void	Invoke(const char* path, SFlashVarValue* args,  unsigned int numArgs, SFlashVarValue* pResult = 0);

	virtual void	SetVariable(const char* path, int arg);
	virtual void	SetVariable(const char* path, float arg);
	virtual void	SetVariable(const char* path, bool arg);
	virtual void	SetVariable(const char* path, const char* arg);
	virtual void	SetVariable(const char* path, const wchar_t* arg);
	virtual void	SetVariable(const char* path, const SFlashVarValue& arg);

	virtual void	GetVariable(const char* path, int& arg) const;
	virtual void	GetVariable(const char* path, float& arg) const;
	virtual void	GetVariable(const char* path, string& arg) const;
	virtual void	GetVariable(const char* path, SFlashVarValue& arg) const;
	virtual void	GetVariable(const char* path, IFlashVariableObject*& arg) const;

	virtual void	SetVariableArrayDouble(const char* path, std::vector<double> *doubleArray);
	virtual IFlashPlayer* GetFlashPlayer() { return m_pPlayer; }

	virtual int		GetWidth() const;
	virtual int		GetHeight() const;

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	IFlashPlayer* m_pPlayer;
	bool					m_externalPlayer;
};


//////////////////////////////////////////////////////////////////////////


class CHUDAssetNull : public IHUDAsset
{

public:

	CHUDAssetNull() {}
	virtual ~CHUDAssetNull() {}

	virtual void	SetPath(const string& path) {}
	virtual void 	Init(const IItemParamsNode * asset, ScreenLayoutManager* pScreenLayout) {};
	virtual void	SetPositionAndSize(const float x, const float y, const float sizeX, const float sizeY) {}
	virtual void	Update(float frameTime) {}
	virtual void	Draw() {}

	virtual void	Invoke(const char* path, SFlashVarValue* pResult = 0) {}
	virtual void	Invoke(const char* path, int arg, SFlashVarValue* pResult = 0) {}
	virtual void	Invoke(const char* path, float arg, SFlashVarValue* pResult = 0) {}
	virtual void	Invoke(const char* path, bool arg, SFlashVarValue* pResult = 0) {}
	virtual void	Invoke(const char* path, const char* arg, SFlashVarValue* pResult = 0) {}
	virtual void	Invoke(const char* path, const wchar_t* arg, SFlashVarValue* pResult = 0) {}
	virtual void	Invoke(const char* path, const SFlashVarValue& arg, SFlashVarValue* pResult = 0) {}
	virtual void	Invoke(const char* path, SFlashVarValue* args,  unsigned int numArgs, SFlashVarValue* pResult = 0) {}

	virtual void	SetVariable(const char* path, int arg) {}
	virtual void	SetVariable(const char* path, float arg) {}
	virtual void	SetVariable(const char* path, bool arg) {}
	virtual void	SetVariable(const char* path, const char* arg) {}
	virtual void	SetVariable(const char* path, const wchar_t* arg) {};
	virtual void	SetVariable(const char* path, const SFlashVarValue& arg) {}

	virtual void	GetVariable(const char* path, int& arg) const {}
	virtual void	GetVariable(const char* path, float& arg) const {}
	virtual void	GetVariable(const char* path, string& arg) const {}
	virtual void	GetVariable(const char* path, SFlashVarValue& arg) const {}
	virtual void	GetVariable(const char* path, IFlashVariableObject*& arg) const { arg = CHUD::s_pHUDFlashvariableObjectNULL; };

	virtual void	SetVariableArrayDouble(const char* path, std::vector<double> *doubleArray) {}
	virtual IFlashPlayer* GetFlashPlayer() { CFlash::Get(); return CFlash::s_pFlashPlayerNull; }

	virtual int		GetWidth() const { return 100; }
	virtual int		GetHeight() const { return 100; }

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};


//////////////////////////////////////////////////////////////////////////

#endif

