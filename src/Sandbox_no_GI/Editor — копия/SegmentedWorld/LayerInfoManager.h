#ifndef __LayerInfoManager_h__
#define __LayerInfoManager_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "VersionCommon.h"

class CLayerInfoManager
{
public:
	struct TLayerInfo
	{
		int m_nID;					// ID of Layer
		GUID m_guid;				// guid of Layer
		string m_strName;			// name of Layer
		string m_strFullName;		// fullname of the layer, cached for loading-time usage
		GUID m_guidParent;			// guid for Parent Layer
		string m_strAttribXML;		// attrib of the layer

		TLayerInfo():m_nID(-1),m_guid(GUID_NULL),m_guidParent(GUID_NULL),m_pParentPtrCache(NULL){}
		TLayerInfo(int id,REFGUID guid, const char* szName)
			:m_nID(id),m_guid(guid),m_strName(szName),m_guidParent(GUID_NULL),m_pParentPtrCache(NULL)
		{}
	private:
		void _MakeFullName(string& oss) const;
		TLayerInfo* m_pParentPtrCache; // a cached pointer to the parent
		friend CLayerInfoManager;
	};

protected:
	typedef std::map<GUID, TLayerInfo, guid_less_predicate> TLayerInfoMap;
	typedef std::pair<GUID, TLayerInfo> TLayerInfoPair;
public:
	void ObjectLayersInfo_Init();
	void ObjectLayersInfo_Unload();
	int RegisterNewLayer(REFGUID guid, const char *szLayerName, REFGUID guidParent);
	void SyncLayers( std::vector<CObjectLayer*> const& vLayers );

	bool ObjectLayersInfo_GetGUID(int nLayerID, GUID &guid, string &strName);
	int  ObjectLayersInfo_GetID(REFGUID guid);
	bool SaveLayerXML(const char *path);
	bool LoadLayerXML(const char *path);
	TLayerInfo* FindLayer(REFGUID guid);
protected:
	int  CacheOne( REFGUID guid );
	void ObjectLayersInfo_Add(int nID, REFGUID guid, const char *chName);
	TLayerInfoMap::iterator FetchLayer(REFGUID guid);

private:
	TLayerInfoMap m_layerInfoMap;

	friend class CSWDebug;
};

#endif // __LayerInfoManager_h__