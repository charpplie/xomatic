#include "stdafx.h"
#include "InternalCommon.h"
#include "LayerInfoManager.h"

#include "SegmentedWorldDoc.h"
#include "Objects/ObjectLayer.h"
#include "Objects/ObjectLayerManager.h"

using namespace sw;

// Currently this function is also responsible for filling extra informations after the creation of a layer
// TODO: We should either give it a better name to reflect the fact or extract the extra responsibility to 
// another function. -RL
int CLayerInfoManager::RegisterNewLayer( REFGUID guid, const char *szLayerName, REFGUID guidParent )
{
	if (!szLayerName || szLayerName[0] == '\0')
	{
		assert(0);
		return -1;
	}

	if (guid == GUID_NULL)
	{
		assert(0);
		return -1;
	}

	int nLayerId = -1;
	TLayerInfoMap::iterator iter = FetchLayer(guid);

	//insert new layerID
	if (iter == m_layerInfoMap.end())
	{
		nLayerId = m_layerInfoMap.size() + 1;

		ObjectLayersInfo_Add(nLayerId, guid, szLayerName);
		iter = m_layerInfoMap.find(guid);
	}

	if (iter == m_layerInfoMap.end())
		return -1;

	nLayerId = iter->second.m_nID;

	if (guidParent != GUID_NULL)
	{
		iter->second.m_guidParent = guidParent;
	}

	return nLayerId;

}

void CLayerInfoManager::ObjectLayersInfo_Add(int nID, REFGUID guid, const char *chName)
{
	TLayerInfoMap::iterator it;
	AUTO_LOCK(g_cs);
	it = m_layerInfoMap.find(guid);
	if(it != m_layerInfoMap.end())
		return;

	//TLayerInfo* pLi = new TLayerInfo();
	//pLi->m_nID = nID;
	//pLi->m_guid = guid;
	//pLi->m_strName = chName;
	m_layerInfoMap.insert(std::pair<GUID,TLayerInfo>(guid, TLayerInfo(nID,guid,chName)));
}

void CLayerInfoManager::ObjectLayersInfo_Init()
{
	bool bMainLayerCreated = false;

	AUTO_LOCK(g_cs);
	for (TLayerInfoMap::iterator it = m_layerInfoMap.begin(); it != m_layerInfoMap.end(); ++it)
	{
		TLayerInfo* pLi = &it->second;
		if (!bMainLayerCreated && !strcmpi(pLi->m_strName.c_str(), "Main"))
		{
			GetIEditor()->GetObjectManager()->GetLayersManager()->CreateMainLayer(&pLi->m_guid);
			bMainLayerCreated = true;
			continue;
		}

		CObjectLayer* pLayer;
		pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->CreateLayer(&pLi->m_guid);
		if (pLayer)
		{
			const string& s = pLi->m_strAttribXML;
			pLayer->Serialize(XmlHelpers::LoadXmlFromBuffer(s.c_str(),s.size()),true);
		}
	}

	GetIEditor()->GetObjectManager()->GetLayersManager()->ResolveLayerLinks(); //NotifyListeners( ON_LAYER_UPDATEALL,0 ) is private
}

void CLayerInfoManager::ObjectLayersInfo_Unload()
{
	//AUTO_LOCK(g_cs);
	m_layerInfoMap.clear();
}

bool CLayerInfoManager::ObjectLayersInfo_GetGUID(int nLayerID, GUID &guid, string &strName)
{
	assert(nLayerID != -1);
	{
		AUTO_LOCK(g_cs);
		for (TLayerInfoMap::iterator it = m_layerInfoMap.begin(); it != m_layerInfoMap.end(); ++it)
		{
			TLayerInfo* pLi = &it->second;
			if (pLi->m_nID == nLayerID)
			{
				guid = pLi->m_guid;
				strName = pLi->m_strName;
				return true;
			}
		}
	}
	ObjectLayersInfo_Add(nLayerID, guid, strName);
	return true;
}

int CLayerInfoManager::ObjectLayersInfo_GetID(REFGUID guid)
{
	TLayerInfoMap::iterator it = FetchLayer(guid);

	if (it != m_layerInfoMap.end())
		return it->second.m_nID;

	return -1;
}

CLayerInfoManager::TLayerInfoMap::iterator
	CLayerInfoManager::FetchLayer(REFGUID guid)
{
	assert(guid != GUID_NULL);
	TLayerInfoMap::iterator it;

	{
		AUTO_LOCK(g_cs);
		it = m_layerInfoMap.find(guid);
		if(it != m_layerInfoMap.end())
			return it;
	}

	return m_layerInfoMap.end();
}

int CLayerInfoManager::CacheOne( REFGUID guid )
{
	int64 guidLo;
	int64 guidHi;
	SplitGuid(guid, guidLo, guidHi);


	//TArgStack args;args.push_back(guidLo);args.push_back(guidHi);
	//DBPrepStmtResultSet pResultSet = g_db->ExecPreparedStatement(OBJECT_LAYER_GET_LAYER_BY_GUID,args);

	int nLayerId = -1;
	//const char* szName = NULL;
	//if (!pResultSet || !pResultSet->Next() || !DBAPIHelper::Get(pResultSet, nLayerId,szName))
	//{
	//	//char chLog[256]; sprintf(chLog, "%s query returned 0 rows.\n", CDBManager::GetStatementName(OBJECT_LAYER_GET_LAYER_BY_GUID));
	//	//SWLog(chLog);
	//	//assert(0);
	//	return -1; // no such item
	//}

	//if (nLayerId > 0 && szName )
	//	ObjectLayersInfo_Add(nLayerId,guid,szName);

	return nLayerId;
}

bool CLayerInfoManager::SaveLayerXML(const CString& sPath)
{
	CString sXML = sPath + SW_LayerInfoFile;
	
	XmlNodeRef xml = XmlHelpers::CreateXmlNode("Layers");
	if (!xml)
		return false;

	int index = 0;
	for (TLayerInfoMap::iterator it = m_layerInfoMap.begin(); it != m_layerInfoMap.end(); ++it)
	{
		TLayerInfo *pLayerInfo = &it->second;

		XmlNodeRef layer = XmlHelpers::CreateXmlNode("Layer");
		if (!layer)
			return false;

		layer->setAttr("ID", pLayerInfo->m_nID);

		XmlNodeRef attrib = XmlHelpers::LoadXmlFromBuffer(pLayerInfo->m_strAttribXML.c_str(), pLayerInfo->m_strAttribXML.size());
		attrib->setAttr("FullName", pLayerInfo->m_strFullName);
		layer->insertChild(0, attrib);

		// Check consistency
		{
			GUID guidInAttr;
			const char* nameInAttr;
			GUID parentInAttr;
			assert(attrib->getAttr("GUID", guidInAttr) && guidInAttr == pLayerInfo->m_guid); guidInAttr;
			assert((nameInAttr = attrib->getAttr("Name")) && pLayerInfo->m_strName == nameInAttr); nameInAttr;
			assert(!attrib->getAttr("ParentGUID", parentInAttr) || parentInAttr == pLayerInfo->m_guidParent); parentInAttr;
		}

		xml->insertChild(index, layer);
		index++;
	}

	if (!xml->saveToFile(sXML))
		return false;

	return true;
}

bool CLayerInfoManager::LoadLayerXML(const CString& sPath)
{
	CString sXML = sPath + SW_LayerInfoFile;
	if(!CFileUtil::FileExists(sXML))
		return false;

	XmlNodeRef xml = XmlHelpers::LoadXmlFromFile(sXML);
	if(!xml)
		return false;

	if(xml->isTag("Layers"))
	{
		m_layerInfoMap.clear();

		for(int i = 0; i < xml->getChildCount(); i++)
		{
			XmlNodeRef obj = xml->getChild(i);
			if(!obj->isTag("Layer"))
				continue;

			TLayerInfo layerInfo;
			obj->getAttr("ID", layerInfo.m_nID);

			assert(obj->getChildCount() == 1);
			XmlNodeRef attribs = obj->getChild(0);
			assert(attribs && attribs->isTag("LayerAttribs"));

			layerInfo.m_strAttribXML = attribs->getXML();
			attribs->getAttr("GUID", layerInfo.m_guid);
			assert(layerInfo.m_guid != GUID_NULL);

			layerInfo.m_strName = attribs->getAttr("Name");
			assert(!layerInfo.m_strName.empty());

			layerInfo.m_strFullName = attribs->getAttr("FullName");
			assert(!layerInfo.m_strFullName.empty());

			attribs->getAttr("ParentGUID", layerInfo.m_guidParent);

			std::pair<TLayerInfoMap::iterator,bool> ib
				= m_layerInfoMap.insert(TLayerInfoPair(layerInfo.m_guid, layerInfo));

			if (!ib.second)
			{
				assert(0&&"two layers have same guid");
				ib.first->second = layerInfo;
			}
		}
	}
	return true;
}

CLayerInfoManager::TLayerInfo* CLayerInfoManager::FindLayer( REFGUID guid )
{
	TLayerInfoMap::iterator iter = m_layerInfoMap.find(guid);
	if (iter != m_layerInfoMap.end())
		return &iter->second;

	return NULL;
}

// Remember that the layers that we are sync-ing now is just a part of the whole set.
void CLayerInfoManager::SyncLayers( std::vector<CObjectLayer*> const& vLayers )
{
	for (int i = 0; i < vLayers.size(); i++)
	{
		CObjectLayer* pOLayer = vLayers[i];
		//TLayerInfo li;
		//li.m_guid = vLayers[i]->GetGUID();
		//li.m_nID = -1;
		//li.m_strName = vLayers[i]->GetName();
		//li.m_nID = m_layerInfoMgr.CreateObjectLayer(li.m_guid, li.m_strName); // create object layer on DB only if not exists

		REFGUID guid = pOLayer->GetGUID();
		const GUID* guidParent = &GUID_NULL;
		if ( CObjectLayer* pParentLayer = pOLayer->GetParent())
			guidParent = &pParentLayer->GetGUID();

		// it's safe to call register if it already exits
		RegisterNewLayer(guid,pOLayer->GetName(),*guidParent);

		CLayerInfoManager::TLayerInfo* pInfo = FindLayer(guid);
		XmlNodeRef xmlnode = XmlHelpers::CreateXmlNode("LayerAttribs");
		pOLayer->Serialize(xmlnode,false);
		pInfo->m_strAttribXML = xmlnode->getXML();
	}

	// rebuild layer hierarchy
	for (TLayerInfoMap::iterator iter = m_layerInfoMap.begin(); iter != m_layerInfoMap.end(); ++iter)
	{
		TLayerInfoMap::iterator itParent = m_layerInfoMap.find(iter->second.m_guidParent);
		iter->second.m_pParentPtrCache = (itParent == m_layerInfoMap.end()? NULL : &itParent->second);
	}
	// RebuildFullNames();
	for (TLayerInfoMap::iterator iter = m_layerInfoMap.begin(); iter != m_layerInfoMap.end(); ++iter)
	{
		TLayerInfo& info = iter->second;
		info.m_strFullName.clear();
		info.m_strFullName.reserve(64);
		info._MakeFullName(info.m_strFullName);
	}
}

//////////////////////////////////////////////////////////////////////////
// struct TLayerInfo
//////////////////////////////////////////////////////////////////////////

void CLayerInfoManager::TLayerInfo::_MakeFullName( string& oss ) const
{
	if (m_pParentPtrCache)
	{
		m_pParentPtrCache->_MakeFullName(oss);
		oss.push_back('/');
	}
	oss.append(m_strName);
}
