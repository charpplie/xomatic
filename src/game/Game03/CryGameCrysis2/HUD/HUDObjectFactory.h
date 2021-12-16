#ifndef __HUDOBJECTFACTORY_H__
#define __HUDOBJECTFACTORY_H__


//////////////////////////////////////////////////////////////////////////

class CHUDObject;


//////////////////////////////////////////////////////////////////////////


class CHUDObjectFactory
{

public:

	CHUDObjectFactory();
	virtual ~CHUDObjectFactory();

	void RegisterHUDObject(const char *name, CHUDObject *(*)());
	CHUDObject* GetClassInstance(const char* name);

	typedef std::map<string, CHUDObject		*(*)()>								THUDObjectRegistry;

	THUDObjectRegistry m_objectRegistry;

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};


//////////////////////////////////////////////////////////////////////////

#endif

