//////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   TestExtensions.cpp
//  Version:     v1.00
//  Created:     02/25/2009 by CarstenW
//  Description: Part of CryEngine's extension framework.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TestExtensions.h"

#ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES

#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/Impl/ICryFactoryRegistryImpl.h>
#include <CryExtension/CryCreateClassInstance.h>

//////////////////////////////////////////////////////////////////////////

class CFoobar : public IFoobar
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IFoobar)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CFoobar, "Foobar", 0x76c8dd6d16634531, 0x95d3b1cfabcf7ef4)

public:
	virtual void Foo();
};

CRYREGISTER_CLASS(CFoobar)

CFoobar::CFoobar()
{
}

CFoobar::~CFoobar()
{
}

void CFoobar::Foo()
{
	printf("Inside CFoobar::Foo()\n");
}

static void TestFoobar()
{
	cryshared_ptr<CFoobar> p = CFoobar::CreateClassInstance();
	{
		CryInterfaceID iid = cryiidof<IFoobar>();
		CryClassID clsid = p->GetFactory()->GetClassID();
		int t = 0;
	}

	{
		IAPtr sp_ = cryinterface_cast<IA>(p); // sp_ == NULL

		ICryUnknownPtr sp1 = cryinterface_cast<ICryUnknown>(p);
		IFoobarPtr sp = cryinterface_cast<IFoobar>(sp1);
		sp->Foo();
	}

	{
		CFoobar* pF = reinterpret_cast<CFoobar*>(p.get());
		pF->Foo();
		ICryUnknown* p1 = cryinterface_cast<ICryUnknown>(pF);
	}

	IFoobar* pFoo = cryinterface_cast<IFoobar>(p.get());
	ICryFactory* pF1 = pFoo->GetFactory();
	pFoo->Foo();

	int t = 0;
}

//////////////////////////////////////////////////////////////////////////

class CRaboof : public IRaboof
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IRaboof)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CRaboof, "Raabof", 0xba482ce12b2e4309, 0x8238ed8b52cb1f1e)

public:
	virtual void Rab();
};

CRYREGISTER_CLASS(CRaboof)

CRaboof::CRaboof()
{
}

CRaboof::~CRaboof()
{
}

void CRaboof::Rab()
{
	printf("Inside CRaboof::Rab()\n");
}

static void TestRaboof()
{
	cryshared_ptr<CRaboof> pFoo0_ = CRaboof::CreateClassInstance();
	IRaboofPtr pFoo0 = cryinterface_cast<IRaboof>(pFoo0_);
	ICryUnknownPtr p0 = cryinterface_cast<ICryUnknown>(pFoo0);

	CryInterfaceID iid = cryiidof<IRaboof>();
	CryClassID clsid = p0->GetFactory()->GetClassID();

	cryshared_ptr<CRaboof> pFoo1 = CRaboof::CreateClassInstance();

	pFoo0->Rab();
	pFoo1->Rab();
}

//////////////////////////////////////////////////////////////////////////

class CAB : public IA, public IB
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IA)
		CRYINTERFACE_ADD(IB)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CAB, "AB", 0xb9e54711a64448c0, 0xa4819b4ed3024d04)

public:
	virtual void A();
	virtual void B();

private:
	int i;
};

CRYREGISTER_CLASS(CAB)

CAB::CAB()
{
	i = 0x12345678;
}

CAB::~CAB()
{
}

void CAB::A()
{
	printf("Inside CAB::A()\n");
}

void CAB::B()
{
	printf("Inside CAB::B()\n");
}

//////////////////////////////////////////////////////////////////////////

class CABC : public CAB, public IC
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IC)
	CRYINTERFACE_ENDWITHBASE(CAB)

	CRYGENERATE_CLASS(CABC, "ABC", 0x4e61feae11854be7, 0xa16157c5f8baadd9)

public:
	virtual void C();

private:
	int a;
};

CRYREGISTER_CLASS(CABC)

CABC::CABC()
//: CAB()
{
	a = 0x87654321;
}

CABC::~CABC()
{
}

void CABC::C()
{
	printf("Inside CABC::C()\n");
}

//////////////////////////////////////////////////////////////////////////

class CCustomC : public ICustomC
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IC)
		CRYINTERFACE_ADD(ICustomC)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CCustomC, "CustomC", 0xee61760b98a44b71, 0xa05e7372b44bd0fd)

public:
	virtual void C();
	virtual void C1();

private:
	int a;
};

CRYREGISTER_CLASS(CCustomC)

CCustomC::CCustomC()
{
	a = 0x87654321;
}

CCustomC::~CCustomC()
{
}

void CCustomC::C()
{
	printf("Inside CCustomC::C()\n");
}

void CCustomC::C1()
{
	printf("Inside CCustomC::C1()\n");
}

//////////////////////////////////////////////////////////////////////////

class CMultiBase : public CAB, public CCustomC
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ENDWITHBASE2(CAB, CCustomC)

	CRYGENERATE_CLASS(CMultiBase, "MultiBase", 0x75966b8f98644d42, 0x8fbdd489e94cc29e)

public:
	virtual void A();
	virtual void C1();

	int i;
};

CRYREGISTER_CLASS(CMultiBase)

CMultiBase::CMultiBase()
{
	i = 0x87654321;
}

CMultiBase::~CMultiBase()
{
}

void CMultiBase::C1()
{
	printf("Inside CMultiBase::C1()\n");
}

void CMultiBase::A()
{
	printf("Inside CMultiBase::A()\n");
}

//////////////////////////////////////////////////////////////////////////

static void TestComplex()
{
	{
		ICPtr p;
		if (CryCreateClassInstance(MAKE_CRYGUID(0x75966b8f98644d42, 0x8fbdd489e94cc29e), p))
		{
			p->C();
		}
	}

	{
		ICustomCPtr p;
		if (CryCreateClassInstance("MultiBase", p))
		{
			p->C();
		}
	}

	{
		IFoobarPtr p;
		if (CryCreateClassInstance(MAKE_CRYGUID(0x75966b8f98644d42, 0x8fbdd489e94cc29e), p))
		{
			p->Foo();
		}
	}

	{
		cryshared_ptr<CMultiBase> p = CMultiBase::CreateClassInstance();

		ICPtr pC = cryinterface_cast<IC>(p);
		ICustomCPtr pCC = cryinterface_cast<ICustomC>(pC);

		p->C();
		p->C1();

		pC->C();
		pCC->C1();

		IAPtr pA = cryinterface_cast<IA>(p);
		pA->A();
		p->A();
	}

	{
		cryshared_ptr<CCustomC> p = CCustomC::CreateClassInstance();

		ICPtr pC = cryinterface_cast<IC>(p);
		ICustomCPtr pCC = cryinterface_cast<ICustomC>(pC);

		p->C();
		p->C1();

		pC->C();
		pCC->C1();
	}
	{
		CryInterfaceID ia = cryiidof<IA>();
		CryInterfaceID ib = cryiidof<IB>();
		CryInterfaceID ic = cryiidof<IC>();
		CryInterfaceID ico = cryiidof<ICryUnknown>();
	}

	{
		cryshared_ptr<CAB> p = CAB::CreateClassInstance();
		CryClassID clsid = p->GetFactory()->GetClassID();

		IAPtr pA = cryinterface_cast<IA>(p);
		IBPtr pB = cryinterface_cast<IB>(p);

		IBPtr pB1 = cryinterface_cast<IB>(pA);
		IAPtr pA1 = cryinterface_cast<IA>(pB);

		pA->A();
		pB->B();

		ICryUnknownPtr p1 = cryinterface_cast<ICryUnknown>(pA);
		ICryUnknownPtr p2 = cryinterface_cast<ICryUnknown>(pB);
		const ICryUnknown* p3 = cryinterface_cast<const ICryUnknown>(pB.get());

		int t = 0;
	}

	{
		cryshared_ptr<CABC> pABC = CABC::CreateClassInstance();
		CryClassID clsid = pABC->GetFactory()->GetClassID();

		ICryFactory* pFac = pABC->GetFactory();
		pFac->ClassSupports(cryiidof<IA>());
		pFac->ClassSupports(cryiidof<IRaboof>());

		IAPtr pABC0 = cryinterface_cast<IA>(pABC);
		IBPtr pABC1 = cryinterface_cast<IB>(pABC0);
		ICPtr pABC2 = cryinterface_cast<IC>(pABC1);

		pABC2->C();
		pABC1->B();

		pABC2->GetFactory();

		const IC* pCconst = pABC2.get();
		const ICryUnknown* pOconst = cryinterface_cast<const ICryUnknown>(pCconst);
		const IA* pAconst = cryinterface_cast<const IA>(pOconst);
		const IB* pBconst = cryinterface_cast<const IB>(pAconst);

		//const IA* pA11 = cryinterface_cast<IA>(pOconst);

		pCconst = cryinterface_cast<const IC>(pBconst);

		IC* pC = reinterpret_cast<IC*>(pABC1.get());
		pC->C(); // calls IB::B()

		int t = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
// use of extension system without any of the helper macros/templates

class CDontLikeMacrosFactory : public ICryFactory
{
	// ICryFactory
public:
	virtual const char* GetClassName() const
	{
		return "DontLikeMacros";
	}
	virtual const CryClassID& GetClassID() const
	{
		static const CryClassID cid = {0x73c3ab0042e6488aull, 0x89ca1a3763365565ull};
		return cid;
	}
	virtual bool ClassSupports(const CryInterfaceID& iid) const
	{
		return iid == cryiidof<ICryUnknown>() || iid == cryiidof<IDontLikeMacros>();
	}
	virtual void ClassSupports(const CryInterfaceID*& pIIDs, size_t& numIIDs) const
	{
		static const CryInterfaceID iids[2] = {cryiidof<ICryUnknown>(), cryiidof<IDontLikeMacros>()};
		pIIDs = iids;
		numIIDs = 2;
	}
	virtual ICryUnknownPtr CreateClassInstance() const;

public:
	static CDontLikeMacrosFactory& Access()
	{
		static CDontLikeMacrosFactory s_factory;
		return s_factory;
	}

private:
	CDontLikeMacrosFactory() {}
	~CDontLikeMacrosFactory() {}
};

class CDontLikeMacros : public IDontLikeMacros
{
	// ICryUnknown
public:
	virtual ICryFactory* GetFactory() const
	{
		return &CDontLikeMacrosFactory::Access();
	};

	template <class Dst, class Src> friend Dst* InterfaceCastSemantics::cryinterface_cast(Src*);
	template <class Dst, class Src> friend Dst* InterfaceCastSemantics::cryinterface_cast(const Src*);
	template <class Dst, class Src> friend cryshared_ptr<Dst> InterfaceCastSemantics::cryinterface_cast(const cryshared_ptr<Src>&);

	template<class X> friend class sp_counted_impl;

protected:
	virtual void* QueryInterface(const CryInterfaceID& iid) const
	{
		if (iid == cryiidof<ICryUnknown>())
			return (void*) (ICryUnknown*) this;
		else if (iid == cryiidof<IDontLikeMacros>())
			return (void*) (IDontLikeMacros*) this;
		else
			return 0;
	}

	// IDontLikeMacros
public:
	virtual void CallMe()
	{
		printf("Yey, no macros...\n");
	}

	CDontLikeMacros() {}

protected:
	virtual ~CDontLikeMacros() {}
};

ICryUnknownPtr CDontLikeMacrosFactory::CreateClassInstance() const
{
	return ICryUnknownPtr(reinterpret_cast<ICryUnknown*>(new CDontLikeMacros));
}

static SRegFactoryNode g_dontLikeMacrosFactory(&CDontLikeMacrosFactory::Access());

//////////////////////////////////////////////////////////////////////////

static void TestDontLikeMacros()
{
	ICryFactory* f = &CDontLikeMacrosFactory::Access();

	f->ClassSupports(cryiidof<ICryUnknown>());
	f->ClassSupports(cryiidof<IDontLikeMacros>());

	const CryInterfaceID* pIIDs = 0;
	size_t numIIDs = 0;
	f->ClassSupports(pIIDs, numIIDs);

	ICryUnknownPtr p = f->CreateClassInstance();
	IDontLikeMacrosPtr pp = cryinterface_cast<IDontLikeMacros>(p);

	ICryUnknown* pi = cryinterface_cast<ICryUnknown>(reinterpret_cast<CDontLikeMacros*>(p.get()));

	pp->CallMe();
}

//////////////////////////////////////////////////////////////////////////

void TestExtensions(ICryFactoryRegistryImpl* pReg)
{
	struct MyCallback : public ICryFactoryRegistryCallback
	{
		virtual void OnNotifyFactoryRegistered(ICryFactory* pFactory)
		{
			int test = 0;
		}
		virtual void OnNotifyFactoryUnregistered(ICryFactory* pFactory)
		{
			int test = 0;
		}
	};

	//pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x4);
	//pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x1);
	//pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x3);
	//pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x3);
	//pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x2);

	//pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x2);
	//pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x2);
	//pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x4);
	//pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x3);
	//pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x1);

	//MyCallback callback0;
	//pReg->RegisterCallback(&callback0);
	//pReg->RegisterFactories(g_pHeadToRegFactories);

	//pReg->RegisterFactories(g_pHeadToRegFactories);
	//pReg->UnregisterFactories(g_pHeadToRegFactories);

	ICryFactory* pF[4];
	size_t numFactories = 4;
	pReg->IterateFactories(cryiidof<IA>(), pF, numFactories);



	pReg->IterateFactories(MAKE_CRYGUID(-1, -1), pF, numFactories);




	numFactories = (size_t) -1;
	pReg->IterateFactories(cryiidof<ICryUnknown>(), 0, numFactories);




	MyCallback callback1;
	pReg->RegisterCallback(&callback1);
	pReg->UnregisterCallback(&callback1);

	ICryFactory* p;
	p = pReg->GetFactory(MAKE_CRYGUID(0xee61760b98a44b71, 0xa05e7372b44bd0fd));
	p = pReg->GetFactory("CustomC");
	p = pReg->GetFactory("ABC");
	p = pReg->GetFactory((const char*)0);

	p = pReg->GetFactory("DontLikeMacros");
	p = pReg->GetFactory(MAKE_CRYGUID(0x73c3ab0042e6488a, 0x89ca1a3763365565));

	TestFoobar();
	TestRaboof();
	TestComplex();
	TestDontLikeMacros();
}

#endif // #ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES
