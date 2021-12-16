/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Helper to register perception handlers to a factory
  
 -------------------------------------------------------------------------
  History:
  - 04:30:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __PERCEPTIONAUTOREG_H__
#define __PERCEPTIONAUTOREG_H__

struct IPerceptionHandler;

// Perception handler types
enum EPerceptionHandlerTypes
{
	ePHT_INVALID = -1,

	ePHT_Dummy,		// Dummy (empty) perception handling
	ePHT_Crysis,	// Standard perception used by Crysis
	ePHT_Warface,	// Perception for Warface
	ePHT_Crysis2,	// Standard perception used by Crysis2

	ePHT_COUNT,
};

// Perception handler registration
class CPerceptionHandlerAutoRegBase
{
public:
	CPerceptionHandlerAutoRegBase(int iTypeId);

	// Get handler from type Id
	static IPerceptionHandler* Create(int iTypeId, CWeakRef<CPuppet> refOwner);

protected:
	// Called to create a new instance
	virtual IPerceptionHandler* OnCreate(CWeakRef<CPuppet> refOwner) = 0;

private:
	int m_iTypeId;
	static CPerceptionHandlerAutoRegBase* m_Registery[ePHT_COUNT];
};

template <class T>
class CPerceptionHandlerAutoReg : public CPerceptionHandlerAutoRegBase
{
public:
	CPerceptionHandlerAutoReg(int iTypeId) : CPerceptionHandlerAutoRegBase(iTypeId) {}

protected:
	virtual IPerceptionHandler* OnCreate(CWeakRef<CPuppet> refOwner)
	{ 
		T *p = new T();
		CRY_ASSERT(p);
		CRY_ASSERT(refOwner.IsValid());
		p->SetOwner(refOwner);
		return p;
	}
};

// Register a perception handler with this helper
#define REGISTER_PERCEPTION_HANDLER(PerceptionHandlerClass) \
	CPerceptionHandlerAutoReg<PerceptionHandlerClass> g_AutoReg##PerceptionHandlerClass(PerceptionHandlerClass::TypeId);

#endif //__PERCEPTIONAUTOREG_H__
