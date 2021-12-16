#include "StdAfx.h"
#include "NewBehaviourTree/TreeProfileManager.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

namespace NewBehaviourTree
{
	TreeProfileManager::TreeProfileManager()
	{}

	TreeProfileManager::~TreeProfileManager()
	{}

	bool TreeProfileManager::LoadProfiles(const string &path)
	{
		bool ret = true;

		bool bRet = false;

		ICryPak *pCryPak = gEnv->pCryPak;
		_finddata_t fd;

		string sSearchPath = PathUtil::Make(path, string("*"), string("xml"));
		intptr_t iHandle = pCryPak->FindFirst(sSearchPath, &fd);
		if (iHandle >= 0)
		{
			do
			{
				string sFile = PathUtil::Make(path, string(fd.name), string("xml"));
				XmlNodeRef pRoot = gEnv->pSystem->LoadXmlFile(sFile.c_str());
				if (pRoot)
				{
					TreeProfile temp(pRoot);

#if !defined(_RELEASE)
					if (m_Profiles.find(temp.GetProfileName()) != m_Profiles.end())
					{
						AIWarning("Warning: A profile with name %s already exists. Skipping this one (file = %s).", temp.GetProfileName().c_str(), fd.name);
					}
					else
#endif
					{
						m_Profiles[temp.GetProfileName()] = temp;
					}
				}
				else
				{
					ret = false;
					break;
				}

			} while( pCryPak->FindNext(iHandle, &fd) >= 0 );

			pCryPak->FindClose(iHandle);
		}

		return ret;
	}

	const TreeProfile*	TreeProfileManager::GetProfile(const string &name) const
	{
		const TreeProfile* ret = 0;

		ProfileContainer::const_iterator it = m_Profiles.find(name);

		if (m_Profiles.end() != it)
		{
			ret = &it->second;
		}

		return ret;
	}

}

