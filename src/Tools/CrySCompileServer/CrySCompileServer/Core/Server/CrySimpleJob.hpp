#ifndef __CRYSIMPLEJOB__
#define __CRYSIMPLEJOB__

#include <vector>

class TiXmlDocument;

class CCrySimpleJob
{
	tdHash						m_ID;
	uint32_t					m_RequestIP;

	bool							Compile(const TiXmlDocument& rReqParsed,std::vector<uint8_t>& rVec);
public:
										CCrySimpleJob(uint32_t requestIP, std::vector<uint8_t>& rVec);


	bool				      Execute(const std::string& rCmd,std::string &outError);
};

#endif
