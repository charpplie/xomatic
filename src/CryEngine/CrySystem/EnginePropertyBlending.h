//////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   EnginePropertyBlending.h
//  Version:     v1.00
//  Created:     03/09/2009 by CarstenW
//  Description: Engine property blending extension.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _ENGINE_PROPERTY_BLENDING_H_
#define _ENGINE_PROPERTY_BLENDING_H_

#pragma once

#include <IEnginePropertyBlending.h>
#include <CryExtension/Impl/ClassWeaver.h>
#include <vector>
#include <CryString.h>


#define INCLUDE_DEBUGGER


class CEnginePropertyBlender;


class CEngineProperty : public IEngineProperty
{
public:
	VIRTUAL const char* GetName() const {return m_pName;}
	VIRTUAL const EnginePropertyValue& GetValue() const {return m_val;}
	VIRTUAL const EnginePropertyValue& GetDefValue() const {return m_defval;}

public:
	typedef size_t PropertyID;

public:
	CEngineProperty(const char* pName, const EnginePropertyValue& defval, PropertyID id, bool copyName, IEnginePropertyUpdateCallback* pCallback)
	: m_val(defval)
	, m_defval(defval)
	, m_id(id)
	, m_pName(copyName ? 0 : pName)
	, m_pNameCopy(copyName ? new string(pName) : 0)
	, m_pCallback(pCallback)
	{
		if (m_pNameCopy)
			m_pName = m_pNameCopy->c_str();

		assert(m_pName);
	}

	~CEngineProperty()
	{
		SAFE_DELETE(m_pNameCopy);
	}

	PropertyID GetID() const {return m_id;}

	void BeginUpdate();
	void Accumulate(IEnginePropertySet::ELayerTypes layer, const EnginePropertyValue& val, float weight);
	void FinalizeUpdate();

#if defined(INCLUDE_DEBUGGER)
	const EnginePropertyValue& DbgGetAccumValue(IEnginePropertySet::ELayerTypes layer) const
	{
		EnginePropertyValue& accum = ((EnginePropertyValue*) m_accum)[layer];
		return accum;
	}
	float DbgGetAccumWeight(IEnginePropertySet::ELayerTypes layer) const
	{
		return m_weights[layer];
	}
	uint32 DbgGetNumAccums(IEnginePropertySet::ELayerTypes layer) const
	{
		return m_numaccum[layer];
	}
#endif

private:
	EnginePropertyValue m_val;
	const EnginePropertyValue m_defval;
	PropertyID m_id;
	const char* m_pName; // cannot be NULL, either points to valid name passed by (and managed by) client or to m_pNameCopy.c_str() if copying the name was requested
	string* m_pNameCopy;
	IEnginePropertyUpdateCallback* m_pCallback;

	char m_accum[sizeof(EnginePropertyValue) * IEnginePropertySet::NumLayers];
	uint32 m_numaccum[IEnginePropertySet::NumLayers];
	float m_weights[IEnginePropertySet::NumLayers];
};

//////////////////////////////////////////////////////////////////////////

class CEnginePropertySetDesc : public IEnginePropertySetDesc
{
public:
	virtual void Release();

	VIRTUAL void Reserve(size_t numElements);
	VIRTUAL void Clear();
	VIRTUAL bool Add(IEngineProperty* pProperty, const EnginePropertyValue& val);

public:
	CEnginePropertySetDesc();
	~CEnginePropertySetDesc();

	size_t Size() const;
	void Get(size_t idx, IEngineProperty*& pProperty, EnginePropertyValue& val) const;

private:
	struct Entry
	{
		IEngineProperty* m_ref;
		EnginePropertyValue m_val;

		Entry(IEngineProperty* ref, const EnginePropertyValue& val) : m_ref(ref), m_val(val) {}
	};
	typedef std::vector<Entry> Entries;
	typedef Entries::iterator EntriesIt;
	typedef Entries::const_iterator EntriesConstIt;

	typedef std::vector<uint32> InsertionCheckBitSet;
	enum EBitSetConstants
	{
		BitSetShift = 5, // uint32 can hold 2^5 bits
		BitSetMask = (1 << BitSetShift) - 1
	};

private:
	bool AlreadyAdded(const CEngineProperty* pProperty) const;
	void MarkAsAdded(const CEngineProperty* pProperty);

private:
	Entries m_entries;
	InsertionCheckBitSet m_check;
};

//////////////////////////////////////////////////////////////////////////

class CEnginePropertySet : public IEnginePropertySet
{
public:
	virtual void Release();

	VIRTUAL void Copy(const IEnginePropertySet* pSrc);
	VIRTUAL IEnginePropertySet* Clone() const;

	VIRTUAL const char* GetName() const;
	VIRTUAL bool SetName(const char* pName);

	VIRTUAL size_t Size() const;
	VIRTUAL bool Set(size_t idx, const EnginePropertyValue& val);
	VIRTUAL bool Get(size_t idx, EnginePropertyValue& val, const char** pPropertyName = 0) const;

	VIRTUAL bool SaveToXML(XmlNodeRef pNode) const;
	VIRTUAL bool SaveToXML(const char* pFilePath) const;

public:
	CEnginePropertySet(const char* pName, const CEnginePropertySetDesc* pDesc);
	~CEnginePropertySet();

	CEnginePropertySet(const CEnginePropertySet& rhs);
	CEnginePropertySet& operator =(const CEnginePropertySet& rhs);

	void OnPushPropertySet(IEnginePropertySet::ELayerTypes layer, float weight) const;

	static CEnginePropertySet* LoadFromXML(const XmlNodeRef pNode, const CEnginePropertyBlender* pPropertyBlender);
	static CEnginePropertySet* LoadFromXML(const char* pFilePath, const CEnginePropertyBlender* pPropertyBlender);

private:
	struct Entry
	{
		CEngineProperty* m_ref;
		EnginePropertyValue m_val;

		Entry(CEngineProperty* ref, const EnginePropertyValue& val) : m_ref(ref), m_val(val) {}
	};
	typedef std::vector<Entry> Entries;
	typedef Entries::iterator EntriesIt;
	typedef Entries::const_iterator EntriesConstIt;

private:
	bool SaveToXMLInt(XmlNodeRef pNode) const;

private:
	Entries m_entries;
	string m_name;
};

//////////////////////////////////////////////////////////////////////////

class CEnginePropertyBlender : public IEnginePropertyBlender
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IEnginePropertyBlender)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CEnginePropertyBlender, "EnginePropertyBlender", 0xa450e48c2aef45d3, 0xb27a7a2824457a77)

public:
	virtual IEngineProperty* CreateProperty(const char* pName, const EnginePropertyValue& defval, bool copyName = false, IEnginePropertyUpdateCallback* pCallback = 0);

	VIRTUAL IEnginePropertySetDesc* CreatePropertySetDesc() const;

	VIRTUAL IEnginePropertySet* CreatePropertySet(const char* pName, const IEnginePropertySetDesc* pDesc) const;
	VIRTUAL IEnginePropertySet* CreatePropertySetFrom(const XmlNodeRef pNode) const;
	VIRTUAL IEnginePropertySet* CreatePropertySetFrom(const char* pXmlFilePath) const;

	VIRTUAL size_t GetNumProperties() const;
	VIRTUAL IEngineProperty* GetProperty(size_t idx) const;
	VIRTUAL IEngineProperty* GetProperty(const char* pName) const;

	VIRTUAL void BeginPropertyUpdate() const;
	VIRTUAL bool PushPropertySet(const IEnginePropertySet* pSet, IEnginePropertySet::ELayerTypes layer, float weight, const char* pDebugComment = 0) const;
	VIRTUAL void FinalizePropertyUpdate() const;

private:
	struct Property
	{
		const char* m_name;
		CEngineProperty* m_ptr;

		Property(const char* name, CEngineProperty* ptr = 0) : m_name(name), m_ptr(ptr) {assert(m_name);}
		bool operator <(const Property& rhs) const {return strcmp(m_name, rhs.m_name) < 0;}
	};
	typedef std::vector<Property> Properties;
	typedef Properties::iterator PropertiesIt;
	typedef Properties::const_iterator PropertiesConstIt;

private:
	Properties m_properties;

#if defined(INCLUDE_DEBUGGER)
public:
	void DisplayDebugger();

private:
	class CDebugContext
	{
	public:
		CDebugContext(CEnginePropertyBlender* pEnginePropertyBlender);
		~CDebugContext();

		void OnUpdateKeyInput();
		void OnBeginPropertyUpdate();
		void OnPushPropertySet(const IEnginePropertySet* pSet, IEnginePropertySet::ELayerTypes layer, float weight, const char* pDebugComment);
		void OnFinalizePropertyUpdate();

		void OnDisplayDebugger() const;

		bool IsPaused() const {return m_isPaused;}

	private:
		void InitCVars();

	private:
		struct PushedSetInfo
		{
			const void* m_pSet;
			IEnginePropertySet::ELayerTypes m_layer;
			float m_weight;
			string m_dbgComment;

			IEnginePropertySet* m_pSetCopy;

			PushedSetInfo(): m_pSet(0), m_layer(IEnginePropertySet::Environment), m_weight(0), m_dbgComment(), m_pSetCopy(0) {}
		};
		typedef std::vector<PushedSetInfo> PushedSets;
		typedef std::vector<size_t> PushedSetsSorted;

		struct PropertyBlendTraceInfo
		{
			EnginePropertyValue m_val;
			size_t m_refSetIdx;

			PropertyBlendTraceInfo() {}
			PropertyBlendTraceInfo(const EnginePropertyValue& val, size_t refSetIdx) : m_val(val), m_refSetIdx(refSetIdx) {}
		};
		typedef std::vector<PropertyBlendTraceInfo> PropertyBlendTraces;

		struct PropertyBlendStack
		{
			PropertyBlendTraces m_layer[IEnginePropertySet::NumLayers];

			void Reset()
			{
				for (size_t i=0; i<IEnginePropertySet::NumLayers; ++i)
					m_layer[i].resize(0);
			}
		};

		enum
		{
			NumSortTabs = 6,
			NumListElementsShown = 4
		};

	private:
		struct SortByPushIdx
		{
			const PushedSets& m_pushedSets;
			const bool m_sortDescending;

			SortByPushIdx(const PushedSets& pushedSets, bool sortDescending) : m_pushedSets(pushedSets), m_sortDescending(sortDescending) {}
			bool operator()(size_t lhs, size_t rhs) const
			{
				if (!m_sortDescending)
					return lhs < rhs;
				else
					return lhs > rhs;
			}
		};

		struct SortByName
		{
			const PushedSets& m_pushedSets;
			const bool m_sortDescending;

			SortByName(const PushedSets& pushedSets, bool sortDescending) : m_pushedSets(pushedSets), m_sortDescending(sortDescending) {}
			bool operator()(size_t lhs, size_t rhs) const
			{
				if (!m_sortDescending)
					return m_pushedSets[lhs].m_pSetCopy->GetName() < m_pushedSets[rhs].m_pSetCopy->GetName();
				else
					return m_pushedSets[lhs].m_pSetCopy->GetName() > m_pushedSets[rhs].m_pSetCopy->GetName();
			}
		};

		struct SortBySetPtr
		{
			const PushedSets& m_pushedSets;
			const bool m_sortDescending;

			SortBySetPtr(const PushedSets& pushedSets, bool sortDescending) : m_pushedSets(pushedSets), m_sortDescending(sortDescending) {}
			bool operator()(size_t lhs, size_t rhs) const
			{
				if (!m_sortDescending)
					return m_pushedSets[lhs].m_pSet < m_pushedSets[rhs].m_pSet;
				else
					return m_pushedSets[lhs].m_pSet > m_pushedSets[rhs].m_pSet;
			}
		};

		struct SortByLayer
		{
			const PushedSets& m_pushedSets;
			const bool m_sortDescending;

			SortByLayer(const PushedSets& pushedSets, bool sortDescending) : m_pushedSets(pushedSets), m_sortDescending(sortDescending) {}
			bool operator()(size_t lhs, size_t rhs) const
			{
				if (!m_sortDescending)
					return m_pushedSets[lhs].m_layer < m_pushedSets[rhs].m_layer;
				else
					return m_pushedSets[lhs].m_layer > m_pushedSets[rhs].m_layer;
			}
		};

		struct SortByWeight
		{
			const PushedSets& m_pushedSets;
			const bool m_sortDescending;

			SortByWeight(const PushedSets& pushedSets, bool sortDescending) : m_pushedSets(pushedSets), m_sortDescending(sortDescending) {}
			bool operator()(size_t lhs, size_t rhs) const
			{
				if (!m_sortDescending)
					return m_pushedSets[lhs].m_weight < m_pushedSets[rhs].m_weight;
				else
					return m_pushedSets[lhs].m_weight > m_pushedSets[rhs].m_weight;
			}
		};

		struct SortByDbgComment
		{
			const PushedSets& m_pushedSets;
			const bool m_sortDescending;

			SortByDbgComment(const PushedSets& pushedSets, bool sortDescending) : m_pushedSets(pushedSets), m_sortDescending(sortDescending) {}
			bool operator()(size_t lhs, size_t rhs) const
			{
				if (!m_sortDescending)
					return m_pushedSets[lhs].m_dbgComment < m_pushedSets[rhs].m_dbgComment;
				else
					return m_pushedSets[lhs].m_dbgComment > m_pushedSets[rhs].m_dbgComment;
			}
		};

	private:
		int m_curSortTab;
		bool m_sortDescending;
		
		bool m_isPaused;

		size_t m_listStartIdx;
		size_t m_listSelIdx;

		size_t m_numPushedSets;
		PushedSets m_pushedSets;
		PushedSetsSorted m_pushedSetsSorted;

		PropertyBlendStack m_propBlendStack;

		string m_blendStackForProp;

		CEnginePropertyBlender* m_pEnginePropertyBlender;
	};

	CDebugContext* m_pDbgCtx;
#endif // #if defined(INCLUDE_DEBUGGER)
};


#endif // #ifndef _ENGINE_PROPERTY_BLENDING_H_