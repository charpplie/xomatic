#pragma once

class CDynamicPopupMenu;
class CPopupMenuItem0;
template<class Arg1> class CPopupMenuItem1;
template<class Arg1, class Arg2> class CPopupMenuItem2;
template<class Arg1, class Arg2, class Arg3> class CPopupMenuItem3;

class CPopupMenuItem : public _reference_target<int>
{
public:
	friend CDynamicPopupMenu;
	typedef std::vector<_smart_ptr<CPopupMenuItem> > Children;

	CPopupMenuItem(const char* text = "")
	: m_id(0)
	, m_parent(0)
	, m_text(text)
	, m_checked(false)
	, m_enabled(true)
	, m_default(false)
	{}

	virtual ~CPopupMenuItem();

	CPopupMenuItem& Check(bool checked = true){ m_checked = checked; return *this; }
	bool IsChecked() const{ return m_checked; }

	CPopupMenuItem& Enable(bool enabled = true){ m_enabled = enabled; return *this; }
	bool IsEnabled() const{ return m_enabled; }

	// CPopupMenuItem& SetHotkey(KeyPress key);
	// KeyPress Hotkey() const{ return m_hotkey; }

	void SetDefault(bool defaultItem = true){ m_default = defaultItem; }
	bool IsDefault() const{ return m_default; }

	const char* Text() { return m_text.c_str(); }
	CPopupMenuItem& AddSeparator();
	CPopupMenuItem0& Add(const char* text);

	CPopupMenuItem0& Add(const char* text, const Functor0& functor);

	template<class Arg1>
	CPopupMenuItem1<Arg1>& Add(const char* text, const Functor1<Arg1>& functor, const Arg1& arg1);

	template<class Arg1, class Arg2>
	CPopupMenuItem2<Arg1, Arg2>& Add(const char* text, const Functor2<Arg1, Arg2>& functor, const Arg1& arg1, const Arg2& arg2);

	template<class Arg1, class Arg2, class Arg3>
	CPopupMenuItem3<Arg1, Arg2, Arg3>& Add(const char* text, const Functor3<Arg1, Arg2, Arg3>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3);

	CPopupMenuItem* Find(const char* text);

	CPopupMenuItem* GetParent() { return m_parent; }
	Children& GetChildren() { return m_children; };
	const Children& GetChildren() const { return m_children; }
	bool Empty() const { return m_children.empty(); }
private:
	void AddChildren(CPopupMenuItem* item)
	{
		m_children.push_back(item);
		item->m_parent = this;
	}

	HMENU MenuHandle() const
	{
		ASSERT(!m_children.empty());
		return HMENU(m_id);
	}
	void SetMenuHandle(HMENU menu)
	{
		ASSERT(!m_children.empty());
		m_id = unsigned int(menu);
	}
	unsigned int MenuID() const
	{
		ASSERT(m_children.empty());
		return m_id;
	}
	void SetMenuID(unsigned int id)
	{
		ASSERT(m_children.empty());
		m_id = unsigned int(id);
	}

	virtual void Call() = 0;

	unsigned int m_id;
	bool m_default;
	bool m_checked;
	bool m_enabled;
	string m_text;
	CPopupMenuItem* m_parent;
	Children m_children;
	//KeyPress m_hotkey;
};

class CPopupMenuItem0 : public CPopupMenuItem
{
public:
	CPopupMenuItem0(const char* text = "", const Functor0& functor = Functor0())
	: CPopupMenuItem(text)
	, m_functor(functor)
	{}

	void Call(){ 
		if (m_functor)
			m_functor();
	}

protected:
	Functor0 m_functor;
};


template<class Arg1>
class CPopupMenuItem1 : public CPopupMenuItem
{
public:
	CPopupMenuItem1(const char* text, const Functor1<Arg1>& functor, const Arg1& arg1)
	: CPopupMenuItem(text)
	, m_functor(functor)
	, m_arg1(arg1)
	{}

	virtual void Call()
	{ 
		if (m_functor)
			m_functor(m_arg1); 
	}

protected:
	Functor1<Arg1> m_functor;
	Arg1 m_arg1;
};

template<class Arg1, class Arg2>
class CPopupMenuItem2 : public CPopupMenuItem
{
public:
	CPopupMenuItem2(const char* text, const Functor2<Arg1, Arg2>& functor, const Arg1& arg1, const Arg2& arg2)
	: CPopupMenuItem(text)
	, m_functor(functor)
	, m_arg1(arg1)
	, m_arg2(arg2)
	{}

	virtual void Call()
	{ 
		if (m_functor)
			m_functor(m_arg1, m_arg2); 
	}

protected:
	Functor2<Arg1, Arg2> m_functor;
	Arg1 m_arg1;
	Arg2 m_arg2;
};

template<class Arg1, class Arg2, class Arg3>
class CPopupMenuItem3 : public CPopupMenuItem
{
public:
	CPopupMenuItem3(const char* text, const Functor3<Arg1, Arg2, Arg3>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
	: CPopupMenuItem(text)
	, m_functor(functor)
	, m_arg1(arg1)
	, m_arg2(arg2)
	, m_arg3(arg3)
	{}

	virtual void Call()
	{
		if (m_functor)
			m_functor(m_arg1, m_arg2, m_arg3); 
	}

protected:
	Functor3<Arg1, Arg2, Arg3> m_functor;
	Arg1 m_arg1;
	Arg2 m_arg2;
	Arg3 m_arg3;
};

template<class Arg1>
CPopupMenuItem1<Arg1>& CPopupMenuItem::Add(const char* text, const Functor1<Arg1>& functor, const Arg1& arg1)
{
	CPopupMenuItem1<Arg1>* item = new CPopupMenuItem1<Arg1>(text, functor, arg1);
	AddChildren(item);
	return *item;
}

template<class Arg1, class Arg2>
CPopupMenuItem2<Arg1, Arg2>& CPopupMenuItem::Add(const char* text, const Functor2<Arg1, Arg2>& functor, const Arg1& arg1, const Arg2& arg2)
{
	CPopupMenuItem2<Arg1, Arg2>* item = new CPopupMenuItem2<Arg1, Arg2>(text, functor, arg1, arg2);
	AddChildren(item);
	return *item;
}

template<class Arg1, class Arg2, class Arg3>
CPopupMenuItem3<Arg1, Arg2, Arg3>& CPopupMenuItem::Add(const char* text, const Functor3<Arg1, Arg2, Arg3>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
{
	CPopupMenuItem3<Arg1, Arg2, Arg3>* item = new CPopupMenuItem3<Arg1, Arg2, Arg3>(text, functor, arg1, arg2, arg3);
	AddChildren(item);
	return *item;
}

class CDynamicPopupMenu
{
public:
	CDynamicPopupMenu();
	CPopupMenuItem0& GetRoot() { return m_root; };
	const CPopupMenuItem0& GetRoot() const { return m_root; };

	void Spawn(HWND atWindow);
	void Spawn(int x, int y, HWND parent);
	void Clear();

private:
	CPopupMenuItem* NextItem(CPopupMenuItem* item) const;
	CPopupMenuItem0 m_root;

	void AssignIDs();
};

