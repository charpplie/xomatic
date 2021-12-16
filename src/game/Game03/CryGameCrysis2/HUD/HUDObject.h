#ifndef __HUDOBJECT_H__
#define __HUDOBJECT_H__


//////////////////////////////////////////////////////////////////////////


#include "HUDEventDispatcher.h"
#include "HUDAsset.h"


//////////////////////////////////////////////////////////////////////////


class CHUDObject : public IHUDEventListener
{
public :
	typedef std::map<uint32, IHUDAsset*> TAssets;

public:

	CHUDObject();
	virtual ~CHUDObject();

	virtual void    Initialize(const IItemParamsNode * object);
	virtual void		Init() {}
	virtual void		PreDelete() {}

	virtual void		Update(float frameTime) {}
	virtual void		Draw() {}

	virtual void		OnHUDEvent(const SHUDEvent& event) {}

	const TAssets*	GetAssets( void ) const { return &m_assets; };

	void GetMemoryUsage(ICrySizer *pSizer) const
	{		
		pSizer->AddObject(m_assets);
	}
protected:

	IHUDAsset*	GetAsset( void );
	IHUDAsset*	GetAsset(const char* asset);
	IHUDAsset*	GetAsset2D( void );
	IHUDAsset*	GetFirstAsset( void );

private:

	void				InitializeAssets(const IItemParamsNode* assets);
	void				InitializeEvents(const IItemParamsNode* events);

	IHUDAsset*	GetAssetClassInstance(const char* classType);
	
	TAssets m_assets;

};


//////////////////////////////////////////////////////////////////////////


#endif

