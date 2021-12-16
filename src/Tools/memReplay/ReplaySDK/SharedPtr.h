#pragma once

template <typename T>
class SharedPtr
{
public:
	typedef LONG RefCountType;
	typedef T ValueType;
	
	template <typename U> friend class SharedPtr;

public:
	SharedPtr()
		: m_pn(NULL)
		, m_px(NULL)
	{
	}

	SharedPtr(T* px)
	{
		if (px)
		{
			m_pn = new RefCountType(1);
			m_px = px;
		}
		else
		{
			m_pn = NULL;
			m_px = NULL;
		}
	}

	SharedPtr(const SharedPtr<T>& other)
		: m_pn(other.m_pn)
		, m_px(other.m_px)
	{
		if (m_pn)
			InterlockedIncrement(m_pn);
	}

	template <typename U>
	SharedPtr(const SharedPtr<U>& other)
		: m_pn(other.m_pn)
		, m_px(other.m_px)
	{
		if (m_pn)
			InterlockedIncrement(m_pn);
	}

	~SharedPtr()
	{
		Release();
	}

	SharedPtr<T>& operator = (const SharedPtr<T>& other)
	{
		if (other.m_pn)
			InterlockedIncrement(other.m_pn);

		Release();

		m_pn = other.m_pn;
		m_px = other.m_px;

		return *this;
	}

	template <typename U>
	SharedPtr<T>& operator = (const SharedPtr<U>& other)
	{
		if (other.m_pn)
			InterlockedIncrement(other.m_pn);

		Release();

		m_pn = other.m_pn;
		m_px = other.m_px;

		return *this;
	}

	SharedPtr<T>& operator = (T* other)
	{
		if (other != m_px)
		{
			Release();

			if (other)
			{
				m_pn = new RefCountType(1);
				m_px = other;
			}
		}

		return *this;
	}

	T& operator * () const			{ return *m_px; }
	T* operator -> () const			{ return m_px; }

	template <typename U>
	bool operator == (const SharedPtr<U>& other) const { return m_px == other.m_px; }

	template <typename U>
	bool operator != (const SharedPtr<U>& other) const { return m_px != other.m_px; }

	bool IsValid() const				{ return m_px != NULL; }

private:

	void Release()
	{
		if (m_pn)
		{
			if (InterlockedDecrement(m_pn) == 0)
			{
				delete m_pn;
				delete m_px;
				m_pn = NULL;
				m_px = NULL;
			}
		}
	}

private:
	RefCountType* volatile m_pn;
	T* m_px;
};