#ifndef __CGAMEMECHANISMBASE_H__
#define __CGAMEMECHANISMBASE_H__

#define REGISTER_GAME_MECHANISM(classType) CGameMechanismBase(# classType)

class CGameMechanismBase
{
	public:
	struct SLinkedListPointers
	{
		CGameMechanismBase * m_nextMechanism;
		CGameMechanismBase * m_prevMechanism;
	};

	CGameMechanismBase(const char * className);
	virtual ~CGameMechanismBase();
	virtual void Update(float dt) = 0;

	ILINE SLinkedListPointers * GetLinkedListPointers()
	{
		return & m_linkedListPointers;
	}

	ILINE const char * GetName()
	{
		return m_className;
	}

	private:
	SLinkedListPointers m_linkedListPointers;
	const char * m_className;
};

#endif //__CGAMEMECHANISMBASE_H__