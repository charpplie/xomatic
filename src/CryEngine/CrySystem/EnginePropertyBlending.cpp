//////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   EnginePropertyBlending.cpp
//  Version:     v1.00
//  Created:     03/09/2009 by CarstenW
//  Description: Engine property blending extension.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "EnginePropertyBlending.h"
#include <ISystem.h>
#include <IXml.h>

#if defined(INCLUDE_DEBUGGER)
#	include <IConsole.h>
#	include <IRenderer.h>
#	include <algorithm>
#endif


static const char c_serValDelimiter[] = ", ";
static const char c_serNodeTag[] = "EnginePropertySet";
static const char c_serNameAttrib[] = "Name";
static const char c_serChildNodeTag[] = "Properties";


void CEngineProperty::BeginUpdate()
{
	EnginePropertyValue* pAccum = (EnginePropertyValue*) m_accum;
	for (size_t i=0; i<IEnginePropertySet::NumLayers; ++i)
	{
		pAccum[i].n = m_defval.n;
		for (uint32 j=0; j<pAccum[i].n; ++j)
			pAccum[i].data[j] = 0;
		m_weights[i] = 0;
		m_numaccum[i] = 0;
	}
}


void CEngineProperty::Accumulate(IEnginePropertySet::ELayerTypes layer, const EnginePropertyValue& val, float weight)
{
	assert(val.n == m_defval.n);
	assert(weight >= 0);
	EnginePropertyValue& accum = ((EnginePropertyValue*) m_accum)[layer];
	assert(accum.n == m_defval.n);
	for (uint32 i=0; i<val.n; ++i)
		accum.data[i] += val.data[i] * weight;
	m_weights[layer] += weight;
	++m_numaccum[layer];
}


void CEngineProperty::FinalizeUpdate()
{
	EnginePropertyValue* pAccum = (EnginePropertyValue*) m_accum;
	EnginePropertyValue newVal = m_defval;
	for (size_t i=0; i<IEnginePropertySet::NumLayers; ++i)
	{
		assert(pAccum[i].n == m_defval.n);
		if (m_numaccum[i] && m_weights[i] > 1e-4f)
		{
			const float s = clamp_tpl(m_weights[i], 0.0f, 1.0f);
			const float oneMinusS = (1.0f - s);
			const float sByWeights = s / m_weights[i];
			for (uint32 j=0; j<pAccum[i].n; ++j)
				newVal.data[j] = oneMinusS * newVal.data[j] + sByWeights * pAccum[i].data[j];
		}
	}
	EnginePropertyValue oldVal = m_val;
	m_val = newVal;
	if (m_pCallback)
		m_pCallback->OnPropertyUpdated(this, oldVal, newVal);
}

//////////////////////////////////////////////////////////////////////////

CEnginePropertySetDesc::CEnginePropertySetDesc()
: m_entries()
, m_check()
{
}


CEnginePropertySetDesc::~CEnginePropertySetDesc()
{
}


void CEnginePropertySetDesc::Release()
{
	delete this;
}


void CEnginePropertySetDesc::Reserve(size_t numElements)
{
	m_entries.reserve(numElements);
	m_entries.reserve((numElements >> BitSetShift) + 1);
}


void CEnginePropertySetDesc::Clear()
{
	m_entries.resize(0, Entry(0, EnginePropertyValue()));
	m_check.resize(0);
}


bool CEnginePropertySetDesc::Add(IEngineProperty* pProperty, const EnginePropertyValue& val)
{
	if (!pProperty || pProperty->GetDefValue().n != val.n)
		return false;
	CEngineProperty* p = (CEngineProperty*) pProperty;
	if (AlreadyAdded(p))
		return false;
	m_entries.push_back(Entry(p, val));
	MarkAsAdded(p);
	return true;
}


size_t CEnginePropertySetDesc::Size() const
{
	return m_entries.size();
}


void CEnginePropertySetDesc::Get(size_t idx, IEngineProperty*& pProperty, EnginePropertyValue& val) const
{
	assert(idx < m_entries.size());
	pProperty = m_entries[idx].m_ref;
	val = m_entries[idx].m_val;
}


bool CEnginePropertySetDesc::AlreadyAdded(const CEngineProperty* pProperty) const
{
	CEngineProperty::PropertyID id = pProperty->GetID();
	size_t idx = id >> BitSetShift;
	if (idx >= m_check.size())
		return false;
	return 0 != (m_check[idx] & (1 << (id & BitSetMask)));
}


void CEnginePropertySetDesc::MarkAsAdded(const CEngineProperty* pProperty)
{
	CEngineProperty::PropertyID id = pProperty->GetID();
	size_t idx = id >> BitSetShift;
	if (idx >= m_check.size())
		m_check.resize(idx+1, 0);
	m_check[idx] |= 1 << (id & BitSetMask);
}

//////////////////////////////////////////////////////////////////////////

CEnginePropertySet::CEnginePropertySet(const char* pName, const CEnginePropertySetDesc* pDesc)
: m_entries()
, m_name(pName)
{
	assert(m_name.c_str());
	assert(pDesc);

	if (pDesc)
	{
		size_t numEntries = pDesc->Size();
		m_entries.reserve(numEntries);
		for (size_t i=0; i<numEntries; ++i)
		{
			IEngineProperty* pProperty = 0;
			EnginePropertyValue val;
			pDesc->Get(i, pProperty, val);
			assert(pProperty && pProperty->GetDefValue().n == val.n); // validation already happened in CEnginePropertySetDesc::Add()
			CEngineProperty* p = (CEngineProperty*) pProperty;
			m_entries.push_back(Entry(p, val));
		}
	}
}


CEnginePropertySet::~CEnginePropertySet()
{
}


CEnginePropertySet::CEnginePropertySet(const CEnginePropertySet& rhs)
: m_entries(rhs.m_entries)
, m_name(rhs.m_name)
{
}


CEnginePropertySet& CEnginePropertySet::operator =(const CEnginePropertySet& rhs)
{
	if (this != &rhs)
	{
		m_entries = rhs.m_entries;
		m_name = rhs.m_name;
	}
	return *this;
}


void CEnginePropertySet::Release()
{
	delete this;
}


void CEnginePropertySet::Copy(const IEnginePropertySet* pSrc)
{
	if (pSrc)
		*this = *(CEnginePropertySet*) pSrc;
}


IEnginePropertySet* CEnginePropertySet::Clone() const
{
	return new CEnginePropertySet(*this);
}


const char* CEnginePropertySet::GetName() const
{
	return m_name.c_str();
}


bool CEnginePropertySet::SetName(const char* pName)
{
	assert(pName);
	if (pName)
		m_name = pName;
	return pName != 0;
}


size_t CEnginePropertySet::Size() const
{
	return m_entries.size();
}


bool CEnginePropertySet::Set(size_t idx, const EnginePropertyValue& val)
{
	if (idx >= m_entries.size())
		return false;
	if (m_entries[idx].m_ref->GetDefValue().n != val.n)
		return false;
	m_entries[idx].m_val = val;
	return true;
}


bool CEnginePropertySet::Get(size_t idx, EnginePropertyValue& val, const char** pPropertyName) const
{
	if (idx >= m_entries.size())
		return false;
	val = m_entries[idx].m_val;
	if (pPropertyName)
		*pPropertyName = m_entries[idx].m_ref->GetName();
	return true;
}


void CEnginePropertySet::OnPushPropertySet(IEnginePropertySet::ELayerTypes layer, float weight) const
{
	for (EntriesConstIt it = m_entries.begin(), itEnd = m_entries.end(); it != itEnd; ++it)
		(*it).m_ref->Accumulate(layer, (*it).m_val, weight);
}


static void ValueToStr(const EnginePropertyValue& val, char* buf, size_t bufLen, int precision)
{
	assert(buf && bufLen);
	if (!buf || !bufLen)
		return;
	buf[0] = '\0';
	char* p = buf;
	for (uint32 i=0; i<val.n; ++i)
	{
		int written = _snprintf(p, bufLen, "%s%.*g", i != 0 ? c_serValDelimiter : "", precision > 0 ? precision : 0, val.data[i]);
		if (written == bufLen || written < 0)
		{
			p[bufLen - 1] = '\0';
			break;
		}
		p += written;
		bufLen -= written;
	}
}


bool CEnginePropertySet::SaveToXMLInt(XmlNodeRef pNode) const
{
	pNode->setAttr(c_serNameAttrib, m_name.c_str());

	XmlNodeRef pProperties = pNode->newChild(c_serChildNodeTag);
	if (pProperties)
	{
		for (EntriesConstIt it = m_entries.begin(), itEnd = m_entries.end(); it != itEnd; ++it)
		{
			char buf[128];
			const EnginePropertyValue& val = (*it).m_val;
			ValueToStr(val, buf, sizeof(buf), 8);
			pProperties->setAttr((*it).m_ref->GetName(), buf);
		}
		return true;
	}

	return false;
}


bool CEnginePropertySet::SaveToXML(XmlNodeRef pNode) const
{
	XmlNodeRef pChild = pNode->newChild(c_serNodeTag);
	if (pChild)
		return SaveToXMLInt(pChild);

	return false;
}


bool CEnginePropertySet::SaveToXML(const char* pFilePath) const
{
	if (!pFilePath || !pFilePath[0])
		return false;

	XmlNodeRef pNode = gEnv->pSystem->CreateXmlNode(c_serNodeTag);
	if (pNode && SaveToXMLInt(pNode))
		return pNode->saveToFile(pFilePath);

	return false;
}


static inline bool VerifyTag(XmlNodeRef pNode, const char* pTag)
{
	assert(pNode && pTag);
	const char* pNodeTag = pNode->getTag();
	return pNodeTag && strcmp(pNodeTag, pTag) == 0;
}


CEnginePropertySet* CEnginePropertySet::LoadFromXML(const XmlNodeRef pNode, const CEnginePropertyBlender* pPropertyBlender)
{
	if (!pNode || !pPropertyBlender)
		return 0;

	if (!VerifyTag(pNode, c_serNodeTag))
		return 0;

	const char* pName = pNode->getAttr(c_serNameAttrib);
	if (!pName)
		return 0;

	XmlNodeRef pChildNode = pNode->getChild(0);
	if (!pChildNode)
		return 0;

	if (!VerifyTag(pChildNode, c_serChildNodeTag))
		return 0;

	CEnginePropertySetDesc* pSetDesc = new CEnginePropertySetDesc();
	if (pSetDesc)
	{
		int numProperties = pChildNode->getNumAttributes();
		pSetDesc->Reserve(numProperties > 0 ? (size_t) numProperties : 0);
		for (int i=0; i<numProperties; ++i)
		{
			const char* pPropertyName = 0; const char* pPropertyValue = 0;
			pChildNode->getAttributeByIndex(i, &pPropertyName, &pPropertyValue);

			assert(pPropertyName && pPropertyValue);
			if (pPropertyName && pPropertyValue)
			{
				IEngineProperty* pProperty = pPropertyBlender->GetProperty(pPropertyName);
				assert(pProperty);
				if (pProperty)
				{
					const char* pValueStr = pPropertyValue;

					EnginePropertyValue val;

					bool valExtracted = false;
					while (!valExtracted)
					{
						if (sscanf(pValueStr, "%f", &val.data[val.n]) == 1)
							++val.n;
						else
							break;

						pValueStr = strstr(pValueStr, c_serValDelimiter);
						if (pValueStr && val.n == EnginePropertyValue::MaxDimension)
							break;
						if (pValueStr)
							pValueStr += sizeof(c_serValDelimiter) - 1;
						else
							valExtracted = true;
					}

					if (!valExtracted || !pSetDesc->Add(pProperty, val))
					{
						assert(0);
					}
				}
			}
		}

		CEnginePropertySet* pSet = new CEnginePropertySet(pName, pSetDesc);
		pSetDesc->Release();
		return pSet;
	}

	return 0;
}


CEnginePropertySet* CEnginePropertySet::LoadFromXML(const char* pFilePath, const CEnginePropertyBlender* pPropertyBlender)
{
	if (!pFilePath || !pFilePath[0] || !pPropertyBlender)
		return 0;

	XmlNodeRef pNode = gEnv->pSystem->LoadXmlFile(pFilePath);
	return LoadFromXML(pNode, pPropertyBlender);
}

//////////////////////////////////////////////////////////////////////////

CRYREGISTER_CLASS(CEnginePropertyBlender)


CEnginePropertyBlender::CEnginePropertyBlender()
: m_properties()
#if defined(INCLUDE_DEBUGGER)
, m_pDbgCtx(0)
#endif
{
#if defined(INCLUDE_DEBUGGER)
	m_pDbgCtx = new CDebugContext(this);
#endif
}


CEnginePropertyBlender::~CEnginePropertyBlender()
{
	for (PropertiesIt it = m_properties.begin(), itEnd = m_properties.end(); it != itEnd; ++it)
		delete (*it).m_ptr;
#if defined(INCLUDE_DEBUGGER)
	SAFE_DELETE(m_pDbgCtx);
#endif
}


IEngineProperty* CEnginePropertyBlender::CreateProperty(const char* pName, const EnginePropertyValue& defval, bool copyName, IEnginePropertyUpdateCallback* pCallback)
{
	if (!pName)
		return 0;

	Property search(pName);
	PropertiesIt it = std::lower_bound(m_properties.begin(), m_properties.end(), search);
	if (it == m_properties.end() || (search < *it))
	{
		CEngineProperty* pProperty = new CEngineProperty(pName, defval, m_properties.size(), copyName, pCallback);
		if (pProperty)
			m_properties.insert(it, Property(pProperty->GetName(), pProperty));
		return pProperty;
	}
	else
		return 0;
}


IEnginePropertySetDesc* CEnginePropertyBlender::CreatePropertySetDesc() const
{
	return new CEnginePropertySetDesc();
}

IEnginePropertySet* CEnginePropertyBlender::CreatePropertySet(const char* pName, const IEnginePropertySetDesc* pDesc) const
{
	if (!pName || !pDesc)
		return 0;

	return new CEnginePropertySet(pName, static_cast<const CEnginePropertySetDesc*>(pDesc));
}


IEnginePropertySet* CEnginePropertyBlender::CreatePropertySetFrom(const XmlNodeRef pNode) const
{
	return CEnginePropertySet::LoadFromXML(pNode, this);
}


IEnginePropertySet* CEnginePropertyBlender::CreatePropertySetFrom(const char* pXmlFilePath) const
{
	return CEnginePropertySet::LoadFromXML(pXmlFilePath, this);
}


size_t CEnginePropertyBlender::GetNumProperties() const
{
	return m_properties.size();
}


IEngineProperty* CEnginePropertyBlender::GetProperty(size_t idx) const
{
	return idx < m_properties.size() ? m_properties[idx].m_ptr : 0;
}


IEngineProperty* CEnginePropertyBlender::GetProperty(const char* pName) const
{
	if (!pName)
		return 0;

	Property search(pName);
	PropertiesConstIt it = std::lower_bound(m_properties.begin(), m_properties.end(), search);
	return it != m_properties.end() && !(search < *it) ? (*it).m_ptr : 0;
}


void CEnginePropertyBlender::BeginPropertyUpdate() const
{
#if defined(INCLUDE_DEBUGGER)
	m_pDbgCtx->OnUpdateKeyInput();
	if (!m_pDbgCtx->IsPaused())
	{
#endif
		for (PropertiesConstIt it = m_properties.begin(), itEnd = m_properties.end(); it != itEnd; ++it)
			(*it).m_ptr->BeginUpdate();
#if defined(INCLUDE_DEBUGGER)
	}
	m_pDbgCtx->OnBeginPropertyUpdate();
#endif
}


bool CEnginePropertyBlender::PushPropertySet(const IEnginePropertySet* pSet, IEnginePropertySet::ELayerTypes layer, float weight, const char* pDebugComment) const
{
	if (!pSet || layer < 0 || layer >= IEnginePropertySet::NumLayers || weight < 0)
		return false;
#if defined(INCLUDE_DEBUGGER)
	if (!m_pDbgCtx->IsPaused())
	{
#endif
		static_cast<const CEnginePropertySet*>(pSet)->OnPushPropertySet(layer, weight);
#if defined(INCLUDE_DEBUGGER)
	}
	m_pDbgCtx->OnPushPropertySet(pSet, layer, weight, pDebugComment);
#endif
	return true;
}


void CEnginePropertyBlender::FinalizePropertyUpdate() const
{
#if defined(INCLUDE_DEBUGGER)
	if (!m_pDbgCtx->IsPaused())
	{
#endif
		for (PropertiesConstIt it = m_properties.begin(), itEnd = m_properties.end(); it != itEnd; ++it)
			(*it).m_ptr->FinalizeUpdate();
#if defined(INCLUDE_DEBUGGER)
	}
	m_pDbgCtx->OnFinalizePropertyUpdate();
#endif
}


#if defined(INCLUDE_DEBUGGER)
void CEnginePropertyBlender::DisplayDebugger()
{
	m_pDbgCtx->OnDisplayDebugger();
}


static int cv_sys_propblend_info = 0;
static ICVar* cv_sys_propblend_prop = 0;


struct EnginePropertyAutoComplete : IConsoleArgumentAutoComplete
{
	virtual int GetCount() const
	{
		cryshared_ptr<CEnginePropertyBlender> p = CEnginePropertyBlender::CreateClassInstance();
		return p ? (int) p->GetNumProperties() : 0;
	}
	virtual const char* GetValue(int nIndex) const
	{
		cryshared_ptr<CEnginePropertyBlender> p = CEnginePropertyBlender::CreateClassInstance();
		if (!p || nIndex >= (int) p->GetNumProperties() || nIndex < 0)
			return 0;

		IEngineProperty* pProp = p->GetProperty((size_t) nIndex);
		return pProp ? pProp->GetName() : 0;
	}
};
static EnginePropertyAutoComplete s_enginePropAutoComplete;


CEnginePropertyBlender::CDebugContext::CDebugContext(CEnginePropertyBlender* pEnginePropertyBlender)
: m_curSortTab(0)
, m_sortDescending(false)
, m_isPaused(false)
, m_listStartIdx(0)
, m_listSelIdx(0)
, m_numPushedSets(0)
, m_pushedSets()
, m_pushedSetsSorted()
, m_propBlendStack()
, m_blendStackForProp()
, m_pEnginePropertyBlender(pEnginePropertyBlender)
{
	InitCVars();
}


CEnginePropertyBlender::CDebugContext::~CDebugContext()
{
	for (size_t i=0, size=m_pushedSets.size(); i<size; ++i)
	{
		SAFE_RELEASE(m_pushedSets[i].m_pSetCopy);
	}
}


void CEnginePropertyBlender::CDebugContext::OnUpdateKeyInput()
{
	if (!cv_sys_propblend_info)
		return;

	if (CryGetAsyncKeyState(VK_UP) & 1)
	{
		if (CryGetAsyncKeyState(VK_CONTROL) & 0x8000)
			m_sortDescending = false;
		else
			m_listSelIdx = m_listSelIdx > 0 ? m_listSelIdx - 1 : 0;
	}
	if (CryGetAsyncKeyState(VK_DOWN) & 1)
	{
		if (CryGetAsyncKeyState(VK_CONTROL) & 0x8000)
			m_sortDescending = true;
		else
			++m_listSelIdx;
	}
	if (CryGetAsyncKeyState(VK_LEFT) & 1)
	{
		if (CryGetAsyncKeyState(VK_CONTROL) & 0x8000)
			m_curSortTab = m_curSortTab > 0 ? m_curSortTab - 1 : NumSortTabs - 1;
	}
	if (CryGetAsyncKeyState(VK_RIGHT) & 1)
	{
		if (CryGetAsyncKeyState(VK_CONTROL) & 0x8000)
			m_curSortTab = m_curSortTab < NumSortTabs - 1 ? m_curSortTab + 1 : 0;
	}

	if (CryGetAsyncKeyState(VK_SCROLL) & 1)
		m_isPaused = !m_isPaused;
}


void CEnginePropertyBlender::CDebugContext::OnBeginPropertyUpdate()
{
	if (!cv_sys_propblend_info)
		return;

	const char* pPropFilter = cv_sys_propblend_prop->GetString();
	if (pPropFilter && *pPropFilter)
	{
		if (!m_pEnginePropertyBlender->GetProperty(pPropFilter))
			cv_sys_propblend_prop->ForceSet("");
	}

	if (m_isPaused)
		return;

	m_numPushedSets = 0;
	m_pushedSetsSorted.resize(0);
	m_propBlendStack.Reset();
	m_blendStackForProp = cv_sys_propblend_prop->GetString();
}


void CEnginePropertyBlender::CDebugContext::OnPushPropertySet(const IEnginePropertySet* pSet, IEnginePropertySet::ELayerTypes layer, float weight, const char* pDebugComment)
{
	assert(pSet);

	if (!cv_sys_propblend_info || m_isPaused)
		return;

	if (m_numPushedSets == m_pushedSets.size())
		m_pushedSets.push_back(PushedSetInfo());

	PushedSetInfo& info = m_pushedSets[m_numPushedSets];
	info.m_pSet = pSet;
	info.m_layer = layer;
	info.m_weight = weight;
	info.m_dbgComment = pDebugComment;
	if (info.m_pSetCopy)
		info.m_pSetCopy->Copy(pSet);
	else
		info.m_pSetCopy = pSet->Clone();

	m_pushedSetsSorted.push_back(m_numPushedSets);

	const char* pPropFilter = cv_sys_propblend_prop->GetString();
	if (pPropFilter && *pPropFilter)
	{
		for (size_t i=0, size = pSet->Size(); i<size; ++i)
		{
			EnginePropertyValue val; const char* pPropName = 0;
			pSet->Get(i, val, &pPropName);

			if (strcmp(pPropFilter, pPropName) == 0)
				m_propBlendStack.m_layer[layer].push_back(PropertyBlendTraceInfo(val, m_numPushedSets));
		}
	}

	++m_numPushedSets;
}


void CEnginePropertyBlender::CDebugContext::OnFinalizePropertyUpdate()
{
	if (!cv_sys_propblend_info)
		return;

	if (m_curSortTab == 0)
		std::sort(m_pushedSetsSorted.begin(), m_pushedSetsSorted.end(), SortByPushIdx(m_pushedSets, m_sortDescending));
	else if (m_curSortTab == 1)
		std::sort(m_pushedSetsSorted.begin(), m_pushedSetsSorted.end(), SortByName(m_pushedSets, m_sortDescending));
	else if (m_curSortTab == 2)
		std::sort(m_pushedSetsSorted.begin(), m_pushedSetsSorted.end(), SortBySetPtr(m_pushedSets, m_sortDescending));
	else if (m_curSortTab == 3)
		std::sort(m_pushedSetsSorted.begin(), m_pushedSetsSorted.end(), SortByLayer(m_pushedSets, m_sortDescending));
	else if (m_curSortTab == 4)
		std::sort(m_pushedSetsSorted.begin(), m_pushedSetsSorted.end(), SortByWeight(m_pushedSets, m_sortDescending));
	else if (m_curSortTab == 5)
		std::sort(m_pushedSetsSorted.begin(), m_pushedSetsSorted.end(), SortByDbgComment(m_pushedSets, m_sortDescending));

	if (m_listSelIdx > m_pushedSetsSorted.size() - 1)
		m_listSelIdx = m_pushedSetsSorted.size() - 1;

	if (m_listStartIdx > m_listSelIdx)
		m_listStartIdx = m_listSelIdx;
	else if (m_listStartIdx + (NumListElementsShown - 1) < m_listSelIdx)
		m_listStartIdx = m_listSelIdx - (NumListElementsShown - 1);
}


static void DbgDrawText(float x, float y, const float* pColor, const char* pFormat, ...)
{
	char buffer[512];
	const size_t cnt = sizeof(buffer);

	va_list args;
	va_start(args, pFormat);
	int written = vsnprintf(buffer, cnt, pFormat, args);
	if (written < 0 || written == cnt)
		buffer[cnt-1] = '\0';
	va_end(args);

	gEnv->pRenderer->Draw2dLabel(x, y, 1.15f, pColor, false, "%s", buffer);
}


static void DbgDrawTextLimit(float x, float y, const float* pColor, size_t maxChars, const char* pFormat, ...)
{
	char buffer[512];
	const size_t cnt = sizeof(buffer) - 4;

	va_list args;
	va_start(args, pFormat);
	int written = vsnprintf(buffer, cnt, pFormat, args);
	va_end(args);

	if (written < 0 || written == cnt)
	{
		written = written < 0 ? cnt + 1 : cnt;
		buffer[cnt] = '\0';
	}

	maxChars = maxChars > cnt ? cnt : maxChars;

	if (written > (int)maxChars)
	{
		buffer[maxChars+0] = '.';
		buffer[maxChars+1] = '.';
		buffer[maxChars+2] = '.';
		buffer[maxChars+3] = '\0';
	}

	gEnv->pRenderer->Draw2dLabel(x, y, 1.15f, pColor, false, "%s", buffer);
}


static string DbgFormat(const EnginePropertyValue& val)
{
	char buf[128];
	ValueToStr(val, buf, sizeof(buf), 6);
	return string(buf);
}


static EnginePropertyValue DbgNormalize(const EnginePropertyValue& val, float accumWeight)
{
	const float s = 1.0f / accumWeight;

	EnginePropertyValue res;
	res.n = val.n;
	for (uint32 i=0; i<val.n; ++i)
		res.data[i] = val.data[i] * s;

	return res;
}


void CEnginePropertyBlender::CDebugContext::OnDisplayDebugger() const
{
	if (!cv_sys_propblend_info)
		return;

	// define colors
//	static const float color[4] = {1, 1, 1, 1};
	static const float colorWarning[4] = {1, 0, 0, 1};
	static const float colorInactive[4] = {0.75f, 0.75f, 0.75f, 1};

	static const float colorSel[4] = {0, 1, 1, 1};

	static const float colorItem[4] = {0.5f, 1, 0.5f, 1};
	static const float colorInfo[4] = {1, 1, 0.5f, 1};

	static const float colorItem2[4] = {0.75f, 1, 0.5f, 1};
	static const float colorInfo2[4] = {1, 0.75f, 0.5f, 1};

	static const char* layerNames[IEnginePropertySet::NumLayers] = {"Environment", "FlowGraph", "Game"};

	Vec2 overscanBorders = *(Vec2*)gEnv->pRenderer->EF_Query(EFQ_OverscanBorders);
	float xAdj = gEnv->pRenderer->GetWidth()  * overscanBorders.x;
	float yAdj = gEnv->pRenderer->GetHeight() * overscanBorders.y;

	float xpos = 10.0f + xAdj;
	float ypos = 10.0f + yAdj;

	if (cv_sys_propblend_info == 1)
	{
		assert(m_pushedSets.size() == m_pushedSetsSorted.size());

		static const char* sortCriteria[NumSortTabs] = { "push index", "set name", "set pointer", "layer set was pushed to", "weight of set", "debug comment"};
		DbgDrawText(xpos, ypos, colorItem, "%sList of all pushed sets (%d in total, sorted %s by %s)...", 
			m_isPaused ? "[paused] " : "", m_pushedSetsSorted.size(), m_sortDescending ? "descending" : "ascending", sortCriteria[m_curSortTab]);
		ypos += 20.0f;

		if (!m_pushedSetsSorted.empty())
		{
			const float* colors[NumSortTabs] = 
			{
				m_curSortTab == 0 ? colorItem : colorInfo, m_curSortTab == 1 ? colorItem : colorInfo, m_curSortTab == 2 ? colorItem : colorInfo, 
				m_curSortTab == 3 ? colorItem : colorInfo, m_curSortTab == 4 ? colorItem : colorInfo, m_curSortTab == 5 ? colorItem : colorInfo,
			};

			if (m_listStartIdx > 0)
				DbgDrawText(xpos, ypos, colorInactive, "^");

			static const float cpu64PtrAdj = sizeof(void*) == 8 ? 40.0f : 0.0f;

			float yposSaved = ypos;

			for (size_t i=m_listStartIdx, size=min(m_listStartIdx + NumListElementsShown, m_pushedSetsSorted.size()); i<size; ++i)
			{
				const PushedSetInfo& info = m_pushedSets[m_pushedSetsSorted[i]];
				DbgDrawText(xpos + 10.0f, ypos, i == m_listSelIdx ? colorSel : colors[0], "Idx: %.3d", m_pushedSetsSorted[i]);
				DbgDrawTextLimit(xpos + 70.0f, ypos, i == m_listSelIdx ? colorSel : colors[1], 25, "Name: \"%s\"", info.m_pSetCopy->GetName());
				DbgDrawText(xpos + 290.0f, ypos, i == m_listSelIdx ? colorSel : colors[2], "Ptr: 0x%p", info.m_pSet);
				DbgDrawTextLimit(xpos + 400.0f+ cpu64PtrAdj, ypos, i == m_listSelIdx ? colorSel : colors[3], 18, "Layer: %s", layerNames[info.m_layer]);
				DbgDrawText(xpos + 530.0f + cpu64PtrAdj, ypos, i == m_listSelIdx ? colorSel : colors[4], "Weight: %.4g", info.m_weight);
				DbgDrawTextLimit(xpos + 630.0f + cpu64PtrAdj, ypos, i == m_listSelIdx ? colorSel : colors[5], 40, "Comment: %s", !info.m_dbgComment.empty() ? info.m_dbgComment.c_str() : "no debug comment");
				ypos += 15.0f;
			}

			if (m_listStartIdx + NumListElementsShown < m_pushedSetsSorted.size())
				DbgDrawText(xpos, ypos - 15.0f, colorInactive, "v");

			ypos = yposSaved + NumListElementsShown * 15.0f + 5.0f;

			DbgDrawText(xpos + 10.0f, ypos, colorItem, "Content for selected set...");
			ypos += 15.0f;

			const PushedSetInfo& info = m_pushedSets[m_pushedSetsSorted[m_listSelIdx]];
			DbgDrawText(xpos + 30.0f, ypos, colorInfo, "Name: \"%s\"", info.m_pSetCopy->GetName());
			ypos += 12.0f;
			DbgDrawText(xpos + 30.0f, ypos, colorInfo, "Ptr: 0x%p", info.m_pSet);
			ypos += 12.0f;
			DbgDrawText(xpos + 30.0f, ypos, colorInfo, "Layer: %s", layerNames[info.m_layer]);
			ypos += 12.0f;
			DbgDrawText(xpos + 30.0f, ypos, colorInfo, "Weight: %.4g", info.m_weight);
			ypos += 12.0f;
			DbgDrawText(xpos + 30.0f, ypos, colorInfo, "Debug comment: %s", !info.m_dbgComment.empty() ? info.m_dbgComment.c_str() : "no debug comment");
			ypos += 15.0f;
			DbgDrawText(xpos + 30.0f, ypos, colorInfo, "Content:");
			ypos += 12.0f;

			const IEnginePropertySet* pSet = info.m_pSetCopy;
			for (size_t j=0, size=pSet->Size(); j<size; ++j)
			{
				EnginePropertyValue val;
				const char* pPropertyName = 0;
				pSet->Get(j, val, &pPropertyName);

				DbgDrawText(xpos + 40.0f, ypos, colorInfo2, "\"%s\" = %s", pPropertyName, DbgFormat(val).c_str());
				ypos += 12.0f;
			}
		}
	}
	else if (cv_sys_propblend_info == 2)
	{
		const char* pPropFilter = cv_sys_propblend_prop->GetString();

		DbgDrawText(xpos, ypos, colorItem, "%sBlend stack for property \"%s\"...", m_isPaused ? "[paused] " : "", pPropFilter ? pPropFilter : "");
		ypos += 15.0f;

		if (m_blendStackForProp != cv_sys_propblend_prop->GetString())
		{
			DbgDrawText(xpos, ypos, colorWarning, "Blend stack is stale as \"sys_propblend_prop\" changed while in pause mode. Briefly un-pause to update the blend stack!");
		}
		else
		{
			const CEngineProperty* pProperty = pPropFilter ? static_cast<const CEngineProperty*>(m_pEnginePropertyBlender->GetProperty(pPropFilter)) : 0;
			if (pProperty)
			{
				ypos += 5.0f;

				DbgDrawText(xpos + 10.0f, ypos, colorItem, "Layer: Default");
				ypos += 12.0f;
				DbgDrawText(xpos + 20.0f, ypos, colorInfo, "Value: %s", DbgFormat(pProperty->GetDefValue()).c_str());
				ypos += 15.0f;

				for (size_t l=0; l<IEnginePropertySet::NumLayers; ++l)
				{
					const PropertyBlendTraces& pushedLayerData = m_propBlendStack.m_layer[l];
					if (!pushedLayerData.empty())
					{
						const IEnginePropertySet::ELayerTypes curLayer = (IEnginePropertySet::ELayerTypes) l;

						const uint32 numAccums = pProperty->DbgGetNumAccums(curLayer);
						const float accumWeight = pProperty->DbgGetAccumWeight(curLayer);

						if (numAccums && accumWeight > 1e-4f)
						{
							DbgDrawText(xpos + 10.0f, ypos, colorItem, "Layer: %s", layerNames[l]);
							ypos += 12.0f;

							for (size_t i=0, size=pushedLayerData.size(); i<size; ++i)
							{
								const EnginePropertyValue& val = pushedLayerData[i].m_val;
								const PushedSetInfo& info = m_pushedSets[pushedLayerData[i].m_refSetIdx];

								DbgDrawText(xpos + 30.0f, ypos, colorItem2, "From set: \"%s\" (0x%p, %s)", info.m_pSetCopy->GetName(), info.m_pSet, !info.m_dbgComment.empty() ? info.m_dbgComment.c_str() : "no debug comment");
								ypos += 12.0f;
								DbgDrawText(xpos + 40.0f, ypos, colorInfo2, "Value: %s", DbgFormat(val).c_str());
								ypos += 12.0f;
								DbgDrawText(xpos + 40.0f, ypos, colorInfo2, "Weight: %.4g", info.m_weight);
								ypos += 15.0f;
							}

							DbgDrawText(xpos + 20.0f, ypos, colorInfo, "Value: %s", DbgFormat(DbgNormalize(pProperty->DbgGetAccumValue(curLayer), accumWeight)).c_str());
							ypos += 15.0f;
							DbgDrawText(xpos + 20.0f, ypos, colorInfo, "Blend: %.4g", clamp_tpl(accumWeight, 0.0f, 1.0f));
							ypos += 15.0f;
						}
					}
				}

				DbgDrawText(xpos + 10.0f, ypos, colorItem, "Final:");
				ypos += 12.0f;
				DbgDrawText(xpos + 20.0f, ypos, colorInfo, "Value: %s", DbgFormat(pProperty->GetValue()).c_str());
				ypos += 15.0f;
			}
			else
				DbgDrawText(xpos, ypos, colorWarning, "Property not found in blender. Use \"sys_propblend_prop\" to specify name of an existing engine property!");
		}
	}
}


void CEnginePropertyBlender::CDebugContext::InitCVars()
{
	gEnv->pConsole->Register("sys_propblend_info", &cv_sys_propblend_info, 0, 
		0, "Enables debugging of engine property blending...\n"
			 "0 - off (default)\n"
			 "1 - display all pushed sets this frame. Use UP/DOWN to scroll and display details on selected set.\n"
			 "    Use CTRL + LEFT/RIGHT to select tab for sorting.\n"
			 "    Use CTRL + UP/DOWN to toggle ascending/descending sort order for selected tab.\n"
			 "2 - display information about blending computations performed for a given property this frame.\n"
			 "    Use \"sys_propblend_prop\" to specify property's name.");

	cv_sys_propblend_prop = gEnv->pConsole->RegisterString("sys_propblend_prop", "", 0,
		"Argument for engine property blending debugger.\n"
		"Use TAB to auto-complete property name.\n"
		"Refer to \"sys_propblend_info\" for additional information.");
	gEnv->pConsole->RegisterAutoComplete("sys_propblend_prop", &s_enginePropAutoComplete);
}
#endif // #if defined(INCLUDE_DEBUGGER)

//////////////////////////////////////////////////////////////////////////

//#define TEST_DEBUGGER

#if defined(TEST_DEBUGGER)
static void FillBlender(IEnginePropertyBlender* p)
{
	static bool runOnce = false;

	static IEnginePropertySet* pSet0 = 0;
	static IEnginePropertySet* pSet1 = 0;
	static IEnginePropertySet* pSet2 = 0;
	static IEnginePropertySet* pSet3 = 0;
	static IEnginePropertySet* pSet4 = 0;
	static IEnginePropertySet* pSet5 = 0;

	if (!runOnce)
	{
		IEngineProperty* prop1 = p->CreateProperty("FloatProperty", 1.23f, false);
		IEngineProperty* prop2 = p->CreateProperty("Vec3Property", Vec3(1, 2, 3), true);
		IEngineProperty* prop3 = p->CreateProperty("ColorProperty", ColorF(0, 0.5, 1, 1), true);

		IEnginePropertySetDesc* pDesc = p->CreatePropertySetDesc();
		if (pDesc)
		{
			pDesc->Clear();
			pDesc->Add(prop1, 1.0f);
			pDesc->Add(prop2, Vec3(3.0f, 3.0f, 3.0f));
			pSet0 = p->CreatePropertySet("TestSet0", pDesc);

			pDesc->Clear();
			pDesc->Add(prop2, Vec3(1.0f, 3.0f, 1.0f));
			pSet1 = p->CreatePropertySet("TestSet1", pDesc);

			pDesc->Clear();
			pDesc->Add(prop3, ColorF(0.5f, 0.5f, 0.5f, 1.0f));
			pSet2 = p->CreatePropertySet("TestSet2", pDesc);

			pDesc->Clear();
			pDesc->Add(prop3, ColorF(0.75f, 0.75f, 0.25f, 1.0f));
			pSet3 = p->CreatePropertySet("TestSet3", pDesc);

			pDesc->Clear();
			pDesc->Add(prop2, Vec3(1.0f, 0.0f, 0.0f));
			pDesc->Add(prop3, ColorF(1.0f, 0.0f, 0.0f, 1.0f));
			pSet4 = p->CreatePropertySet("TestSet4", pDesc);

			pDesc->Clear();
			pDesc->Add(prop1, 5);
			pSet5 = p->CreatePropertySet("TestSet5", pDesc);

			pDesc->Release();
		}
		runOnce = true;
	}

	//static float f = 0.7f;
	//f += 0.012;
	//pSet0->Set(0, f);

	p->BeginPropertyUpdate();

	p->PushPropertySet(pSet0, IEnginePropertySet::Environment, 0.7f);
	p->PushPropertySet(pSet1, IEnginePropertySet::FlowGraph, 0.5f);
	p->PushPropertySet(pSet2, IEnginePropertySet::Game, 0.75f);

	p->PushPropertySet(pSet3, IEnginePropertySet::Environment, 1.5f);
	p->PushPropertySet(pSet4, IEnginePropertySet::FlowGraph, 0.25f);
	p->PushPropertySet(pSet5, IEnginePropertySet::Game, 0.25f);

	p->FinalizePropertyUpdate();
}
#endif // #if defined(TEST_DEBUGGER)

void RenderEnginePropertyBlenderInfo()
{
#if defined(INCLUDE_DEBUGGER)
	cryshared_ptr<CEnginePropertyBlender> pImpl = CEnginePropertyBlender::CreateClassInstance();
#	if defined(TEST_DEBUGGER)
	FillBlender(pImpl.get());
#	endif
	pImpl->DisplayDebugger();
#endif
}

//////////////////////////////////////////////////////////////////////////

#if defined(CRY_UNIT_TESTING)

#include <CryUnitTest.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ICryFactoryRegistry.h>


CRY_UNIT_TEST_SUITE(EnginePropertyBlenderTesting)
{
	static inline bool IsEqual(double x, double ref)
	{
		return fabs(x - ref) < 1e-6;
	}

	CRY_UNIT_TEST(CUT_General)
	{
		// get engine property blender
		IEnginePropertyBlenderPtr p;
		{
			// check system
			CRY_UNIT_TEST_ASSERT(gEnv && gEnv->pSystem);

			// get factory registry
			ICryFactoryRegistry* pFR = gEnv->pSystem->GetCryFactoryRegistry();
			CRY_UNIT_TEST_ASSERT(pFR != 0);

			// get factory
			ICryFactory* pF = pFR->GetFactory("EnginePropertyBlender");
			CRY_UNIT_TEST_ASSERT(pF != 0);

			// instantiate class
			ICryUnknownPtr pUnk = pF->CreateClassInstance();
			p = cryinterface_cast<IEnginePropertyBlender>(pUnk);
		}
		CRY_UNIT_TEST_ASSERT(p);

		//////////////////////////////////////////////////////////////////////////
		// the actual tests start here

		// test creation of properties
		struct UpdateCallback : public IEnginePropertyUpdateCallback
		{
			virtual void OnPropertyUpdated(const IEngineProperty* pProperty, const EnginePropertyValue& oldValue, const EnginePropertyValue& newValue)
			{
			}
		};
		UpdateCallback callbackTest;

		IEngineProperty* prop1 = p->CreateProperty("FloatProperty", 1.23f, false, &callbackTest);
		CRY_UNIT_TEST_ASSERT(prop1 != 0);

		IEngineProperty* prop2 = p->CreateProperty("Vec3Property", Vec3(1, 2, 3), true/*, &callbackTest*/);
		CRY_UNIT_TEST_ASSERT(prop2 != 0);

		IEngineProperty* prop3 = p->CreateProperty("ColorProperty", ColorF(0, 0.5, 1, 1), true);
		CRY_UNIT_TEST_ASSERT(prop3 != 0);

		CRY_UNIT_TEST_ASSERT(p->CreateProperty("FloatProperty", 0, true) == 0);
		CRY_UNIT_TEST_ASSERT(p->CreateProperty("Vec3Property", Vec3(0, 0, 0)) == 0);
		CRY_UNIT_TEST_ASSERT(p->CreateProperty("ColorProperty", ColorF(0, 0, 0, 0), true) == 0);

		CRY_UNIT_TEST_ASSERT(p->GetNumProperties() == 3);
		CRY_UNIT_TEST_ASSERT(p->GetProperty("FloatProperty") != 0);

		// test value assignment (not part of the actual unit test but will throw asserts in debug
		{
			EnginePropertyValue val = prop3->GetValue(); // ColorF property so val.n = 4
			ColorF c = val;
			float f = val; // should throw assert in debug!
			val = Vec3(2,0,1);
		}

		// test engine property set creation
		IEnginePropertySetDesc* pDesc = p->CreatePropertySetDesc();
		CRY_UNIT_TEST_ASSERT(pDesc != 0);
		//if (pDesc)
		{
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop1, Vec3(1, 2, 3)) == false);
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop2, Vec3(3, 2, 1)) == true);
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop3, ColorF(3, 2, 1, 0)) == true);
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop1, 1) == true);

			IEnginePropertySet* pSet = p->CreatePropertySet("TestSet", pDesc);
			CRY_UNIT_TEST_ASSERT(pSet !=0);
			if (pSet)
			{
				CRY_UNIT_TEST_ASSERT(pSet->Size() == 3);

				IEnginePropertySet* pSet2 = pSet->Clone();
				CRY_UNIT_TEST_ASSERT(pSet2 != 0);

				pSet->Copy(pSet2);
				pSet->Copy(pSet);

				CRY_UNIT_TEST_ASSERT(pSet->SaveToXML("test.xml"));

				pSet->Release();
				pSet2->Release();

				pSet = p->CreatePropertySetFrom("test.xml");
				CRY_UNIT_TEST_ASSERT(pSet != 0);
				CRY_UNIT_TEST_ASSERT(pSet->Size() == 3);
				EnginePropertyValue val;
				const char* pPropName = 0;
				CRY_UNIT_TEST_ASSERT(pSet->Get(0, val, &pPropName) == true && strcmp(pPropName, prop2->GetName()) == 0 && val.n == 3 && IsEqual(val.data[0], 3) && IsEqual(val.data[1], 2) && IsEqual(val.data[2], 1));
				CRY_UNIT_TEST_ASSERT(pSet->Get(1, val, &pPropName) == true && strcmp(pPropName, prop3->GetName()) == 0 && val.n == 4 && IsEqual(val.data[0], 3) && IsEqual(val.data[1], 2) && IsEqual(val.data[2], 1) && IsEqual(val.data[3], 0));
				CRY_UNIT_TEST_ASSERT(pSet->Get(2, val, &pPropName) == true && strcmp(pPropName, prop1->GetName()) == 0 && val.n == 1 && IsEqual(val.data[0], 1));
				SAFE_RELEASE(pSet);
			}
		}

		// test engine property blending
		//if (pDesc)
		{
			pDesc->Clear();
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop1, 1.0f));
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop2, Vec3(3.0f, 3.0f, 3.0f)));
			IEnginePropertySet* pSet0 = p->CreatePropertySet("TestSet0", pDesc);
			CRY_UNIT_TEST_ASSERT(pSet0 != 0);

			pDesc->Clear();
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop2, Vec3(1.0f, 3.0f, 1.0f)));
			IEnginePropertySet* pSet1 = p->CreatePropertySet("TestSet1", pDesc);
			CRY_UNIT_TEST_ASSERT(pSet1 != 0);

			pDesc->Clear();
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop3, ColorF(0.5f, 0.5f, 0.5f, 1.0f)));
			IEnginePropertySet* pSet2 = p->CreatePropertySet("TestSet2", pDesc);
			CRY_UNIT_TEST_ASSERT(pSet2 != 0);

			pDesc->Clear();
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop3, ColorF(0.75f, 0.75f, 0.25f, 1.0f)));
			IEnginePropertySet* pSet3 = p->CreatePropertySet("TestSet3", pDesc);
			CRY_UNIT_TEST_ASSERT(pSet3 != 0);

			pDesc->Clear();
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop2, Vec3(1.0f, 0.0f, 0.0f)));
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop3, ColorF(1.0f, 0.0f, 0.0f, 1.0f)));
			IEnginePropertySet* pSet4 = p->CreatePropertySet("TestSet4", pDesc);
			CRY_UNIT_TEST_ASSERT(pSet4 != 0);

			pDesc->Clear();
			CRY_UNIT_TEST_ASSERT(pDesc->Add(prop1, 5));
			IEnginePropertySet* pSet5 = p->CreatePropertySet("TestSet5", pDesc);
			CRY_UNIT_TEST_ASSERT(pSet5 != 0);

			p->BeginPropertyUpdate();

			CRY_UNIT_TEST_ASSERT(p->PushPropertySet(pSet0, IEnginePropertySet::Environment, 0.7f) == true);
			CRY_UNIT_TEST_ASSERT(p->PushPropertySet(pSet1, IEnginePropertySet::FlowGraph, 0.5f) == true);
			CRY_UNIT_TEST_ASSERT(p->PushPropertySet(pSet2, IEnginePropertySet::Game, 0.75f) == true);

			CRY_UNIT_TEST_ASSERT(p->PushPropertySet(pSet3, IEnginePropertySet::Environment, 1.5f) == true);
			CRY_UNIT_TEST_ASSERT(p->PushPropertySet(pSet4, IEnginePropertySet::FlowGraph, 0.25f) == true);
			CRY_UNIT_TEST_ASSERT(p->PushPropertySet(pSet5, IEnginePropertySet::Game, 0.25f) == true);

			p->FinalizePropertyUpdate();

			float f = p->GetProperty("FloatProperty")->GetValue();
			CRY_UNIT_TEST_ASSERT(IsEqual(f, 2.05175));

			Vec3 v = p->GetProperty("Vec3Property")->GetValue();
			CRY_UNIT_TEST_ASSERT(IsEqual(v.x, 1.35) && IsEqual(v.y, 2.175) && IsEqual(v.z, 1.25));

			ColorF c = p->GetProperty("ColorProperty")->GetValue();
			CRY_UNIT_TEST_ASSERT(IsEqual(c.r, 0.578125) && IsEqual(c.g, 0.515625) && IsEqual(c.b, 0.421875) && IsEqual(c.a, 1.0));

			pSet0->Release();
			pSet1->Release();
			pSet2->Release();
			pSet3->Release();
			pSet4->Release();
			pSet5->Release();
		}

		pDesc->Release();
	}
}

#endif // #if defined(CRY_UNIT_TESTING)

#include UNIQUE_VIRTUAL_WRAPPER(IEngineProperty)
#include UNIQUE_VIRTUAL_WRAPPER(IEnginePropertySet)