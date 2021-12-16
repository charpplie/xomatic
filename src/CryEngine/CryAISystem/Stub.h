#ifndef _AI_STUB
#define _AI_STUB


class CAIObject;

class CStub
{
friend class CObjectContainer;

public:
	CStub() 
	{
		Reset();
	}

	void Reset()
	{
		m_pObject = NULL;
	}

	CAIObject * GetAIObject(void) const { return m_pObject; }

protected:
	CAIObject *m_pObject;
};


/*
This will, AFAICS
Inevitably, a template be

template <class T = CAIObject> class CStub
{
public:
	CStub() 
	{
		Reset();
	}

	void Reset()
	{
		m_pObject = NULL;
	}

	T * GetAIObject(void) const { return m_pAIObject; }

protected:
	T *m_pAIObject;
};
*/

#endif //_AI_STUB
