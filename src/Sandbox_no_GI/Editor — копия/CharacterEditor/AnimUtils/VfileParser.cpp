////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  VfileParser.cpp
//  Version:    v1.00
//  Created:    22/5/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2005 Professional
//  Description: Vicon V-file parser

// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "VfileParser.h"

int32 CVfileRawParser::typeSize[];

CVfileRawParser::CVfileRawParser()
{
	//------------------------------------------------------------------------------
	// type size.

	typeSize[1] =1;
	typeSize[2] =1;
	typeSize[3] = 2;
	typeSize[4] = 4;
	typeSize[5] = 4;
	typeSize[6] = 8;
	typeSize[7] = 1;

}

CVfileRawParser::~CVfileRawParser()
{

}

void CVfileRawParser::ReadVFile(const char* path)
{
	//------------------------------------------------------------------------------
	// Open file

	ICryPak* g_pIPak = gEnv->pCryPak;

	FILE* stream =  g_pIPak->FOpen(path, "r+b");
	if (stream==0)
	{
		char szRes[ 128 ];
		sprintf( szRes, "Unable to load the following file: %s\n\nEither file does not exist or path is invalid",path );
		MessageBox(NULL, szRes, NULL, MB_OK);
		return;
	}

	//------------------------------------------------------------------------------
	// Get file size and read in all data

	g_pIPak->FSeek( stream, 0, SEEK_END );
	uint32 fileSize = g_pIPak->FTell(stream);
	uint8* pBuffer =	new uint8[fileSize];
	g_pIPak->FSeek( stream, 0, SEEK_SET );

	g_pIPak->FRead<uint8>( pBuffer, fileSize, stream );
	
	uint8* pCur = pBuffer;
	//------------------------------------------------------------------------------
	// Read header
	ReadHeader(pCur);
	pCur += 4;

	//------------------------------------------------------------------------------
	// Read static section
	int32 staticLen = *(int32*)(pCur); // 32-byte header not included
	const char* testGroupName = (const char*)(pCur +4);
	assert(strcmp(testGroupName, "DATAGROUP") == 0); // or maybe "PARAMETER"

	pCur += 32; // skip 32-byte header

	int32 passedLen = 0; // header not included
	const uint8* staticP = pCur;
	while(passedLen < staticLen)
	{
		int16 len;
		ReadStaticSection(staticP, len);
		staticP += len +2; // 2 is the length bytes
		passedLen += len + 2; // 2 is the length bytes
	}

	pCur += staticLen; // Skip all static records including the optional buffer
	pCur += 32; // Skip the static terminator

	//------------------------------------------------------------------------------
	// Read dynamic section
	int16 dynamicLen = *(int16*)(pCur);
	uint8* pEnd = pBuffer + fileSize;

	while(pCur < pEnd)
	{
		ReadDynamicSection(pCur);
		pCur += (2 + dynamicLen); // 2 bytes for the length itself
		dynamicLen = *(int16*)(pCur);
	}

	//------------------------------------------------------------------------------
	// Get dof num for each joint and marker
	CalcuDofEachJoint();

	SetRecordJointPureNames();

	//------------------------------------------------------------------------------
	// close file and clear memory
	g_pIPak->FClose( stream );
	
	delete[] pBuffer;
	pBuffer = NULL;

}

//------------------------------------------------------------------------------
//- Calculate the DOF for each joint (normally 6) or marker (normally 3)
//------------------------------------------------------------------------------
void CVfileRawParser::CalcuDofEachJoint()
{
	int32 recordCount = m_Records.size();
	for(int32 i=0; i<recordCount; ++i)
	{
		int32 dofNum = 1;
		int32 labelCount = m_Records[i].labelNames.size();
		string pureName = GetPureJointName(m_Records[i].labelNames[0]);
		for(int32 j=1; j<labelCount; ++j)
		{
			string pureNameB = GetPureJointName(m_Records[i].labelNames[j]);
			if(pureNameB == pureName)
				++dofNum;
		}
		m_DofEachJoint.push_back(dofNum);
	}
}

inline string CVfileRawParser::GetPureJointName(const string fullName)
{
	size_t s = fullName.find_last_of(":");
	size_t e = fullName.find_first_of(" ");
	string subName = fullName.substr(s+1, e-s-1);

	return subName;
}

void CVfileRawParser::SetRecordJointPureNames()
{
	m_JointPureNames.resize(m_Records.size());
	for(int32 i=0; i< m_Records.size(); ++i)
	{
		const SRecord& record = m_Records[i];
		int32 labelSize  = record.labelNames.size();
		for(int32 j =0; j<labelSize; j+= m_DofEachJoint[i])
		{
			string pureName = GetPureJointName(record.labelNames[j]);
			m_JointPureNames[i].push_back( pureName);
		}
	}

}
//------------------------------------------------------------------------------
//  recordName: e.g. "Global Bodies"
//  labelName: e.g. "pelvis <A-X>", subject: is omitted since it's not reliable
//  frameNum: the frame you want to get
//------------------------------------------------------------------------------
inline int32 CVfileRawParser::GetJointDataByName(const int32 recordIndex, const int32 labelIndex, const int32 frameNum, f64& data)
{
	if(recordIndex == -1 || labelIndex == -1)
		return -1;

	const CVfileRawParser::SRecord& record = m_Records[recordIndex];
	int32 frameCount = record.frameCount;
	
	const int32 typeSize = CVfileRawParser::typeSize[record.dataType];
	const int32 labelSize = record.labelNames.size();
	
	data = *(f64*) (&record.data[frameNum][labelIndex* typeSize]);
	
	return 1;
}

void CVfileRawParser::ReadHeader(const uint8* p)
{
	m_Header.vFileVersion = *(uint16*)(p+2);
}

void CVfileRawParser::ReadStaticSection(const uint8* p, int16& len)
{
	
	SRecord record;

	const uint8* pCur = p;

	//------------------------------------------------------------------------------
	// Length
	len = *(int16*)(pCur);
	if(len == 0) // empty record
		return;
	pCur += 2;
	
	//------------------------------------------------------------------------------
	// Group id
	int16 groupId = *(int16*)(pCur);
	pCur += 2;
	record.id = groupId;

	//------------------------------------------------------------------------------
	// Description field, e.g., "Markers", "Global Bodies", "Local Bodies"...
	uint8 descFieldLen = *(uint8*)(pCur);
	++ pCur;

	char* descField = NULL;
	if(descFieldLen !=0)
	{
		descField = (char*)(pCur);
		pCur += descFieldLen;
	}
	record.name = string(descField).MakeUpper();

	//------------------------------------------------------------------------------
	// Data type of each dof
	uint8 type = *(uint8*)(pCur);
	++ pCur;
	record.dataType = type;

	//------------------------------------------------------------------------------
	// Width
	++ pCur; // Skip "width", it's redundant

	//------------------------------------------------------------------------------
	// Frame Hertz
	f32 frameHerz = *(f32*)(pCur);
	pCur += 4;
	record.frameHertz = frameHerz;

	//------------------------------------------------------------------------------
	// Num of Dofs
	int16 numDofs = *(int16*)(pCur);
	pCur += 2;
	record.numDofsAll = numDofs;

	//------------------------------------------------------------------------------
	// Read label name of each dof
	for(int32 i=0; i<numDofs; ++i)
	{
		uint8 labelLen = *(uint8*)(pCur);
		++ pCur;
		const char* label = (const char*) (pCur);
		pCur += labelLen;
		string upperStr = string(label).MakeUpper();
		record.labelNames.push_back(upperStr );
		record.labelNameIndexMap[upperStr] = i;

		size_t ind = upperStr.find_first_of(":");
		size_t s = upperStr.size() - ind -1;
		string upperStrNoSubject = upperStr.substr( ind+1, s); // without "subject:"
		record.labelNameIndexMapNoSubject[upperStrNoSubject] = i;

	}

	m_Records.push_back(record);
	m_RecordNameIndexMap[record.name] = m_Records.size()-1;
	m_RecordNameIdMap[record.name] = record.id;
	m_RecordIdIndexMap[record.id] = m_Records.size()-1;
}

void CVfileRawParser::ReadDynamicSection(const uint8* p)
{
	int16 len = *(int16*)(p);
	if(len == 0)
		return;

	p += 2;

	int16 recordId = *(int16*)(p);
	p += 2;

	int32 frameNum = *(int32*)(p);
	p += 4;

	//------------------------------------------------------------------------------
	// Find record
	int32 ind = stl::find_in_map(m_RecordIdIndexMap, recordId, -1);
	SRecord& record = m_Records[ind];

	//------------------------------------------------------------------------------
	// Save current frame data
	std::vector<uint8> oneFrameData;
	int16 dataSize = len - 2 - 4; // 2: groupId, 4: FrameNum
	oneFrameData.resize(dataSize);
	memcpy(&oneFrameData[0], p, dataSize*sizeof(uint8));
	record.data.push_back(oneFrameData);

	++record.frameCount;
}

const uint32 CVfileRawParser::GetFrameCount() const
{
	if(m_Records.empty())
		return 0;
	else
		return m_Records[0].frameCount;
}

const f64 CVfileRawParser:: GetFrameHertz() const
{
	if(m_Records.empty())
		return 0;
	else
		return m_Records[0].frameHertz;
}

//------------------------------------------------------------------------------
// 
CVfileParser::CVfileParser(const char* path)
{
	m_VfileRawParser.ReadVFile(path);
}

CVfileParser::~CVfileParser()
{
}

int32 CVfileParser::GetMarkerPositions(std::vector< std::vector<Vec3r> >& markerPos, std::vector<string>& markerNames)
{
	const char* recordName = "MARKERS";
	
	int32 recordIndex = m_VfileRawParser.GetRecordIndexByName(recordName);
	if(recordIndex == -1)
		return -1;

	markerNames  = m_VfileRawParser.GetJointPureNames(recordIndex);
	uint32 frameCount = m_VfileRawParser.GetFrameCount();
	int32 numMarkers = markerNames.size();

	std::vector<int32> labelPxInd;
	std::vector<int32> labelPyInd;
	std::vector<int32> labelPzInd;


	string singleSpace = string(" ");
	for(int32 i=0; i< numMarkers; ++i)
	{
		labelPxInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, markerNames[i] + singleSpace + string("<P-X>") ) );
		labelPyInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, markerNames[i] + singleSpace + string("<P-Y>") ) );
		labelPzInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, markerNames[i] + singleSpace + string("<P-Z>") ) );
	}

	markerPos.clear();
	markerPos.resize(numMarkers);

	
	for(int32 i=0; i< frameCount; ++i)
	{
		for(int32 j=0; j< numMarkers; ++j)
		{
			Vec3r t;
			m_VfileRawParser.GetJointDataByName(recordIndex, labelPxInd[j], i, t.x); 
			m_VfileRawParser.GetJointDataByName(recordIndex, labelPyInd[j], i, t.y);
			m_VfileRawParser.GetJointDataByName(recordIndex, labelPzInd[j], i, t.z);
			t/=1000.0;
			markerPos[j].push_back(t);
		}
	}

	return 1;
}

int32 CVfileParser::GetJointData(const char* recordName, std::vector< std::vector<QuatT> >& bodyData, std::vector<string>& jointNames)
{
	int32 recordIndex = m_VfileRawParser.GetRecordIndexByName(recordName);
	if(recordIndex == -1)
		return -1;

	jointNames  = m_VfileRawParser.GetJointPureNames(recordIndex);
	uint32 frameCount = m_VfileRawParser.GetFrameCount();
	int32 numJoints = jointNames.size();

	bodyData.clear();
	bodyData.resize(numJoints);

	std::vector<int32> labelAxInd;
	std::vector<int32> labelAyInd;
	std::vector<int32> labelAzInd;
	std::vector<int32> labelTxInd;
	std::vector<int32> labelTyInd;
	std::vector<int32> labelTzInd;

	string singleSpace = string(" ");
	for(int32 i=0; i< numJoints; ++i)
	{
		labelAxInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, jointNames[i] + singleSpace + string("<A-X>") ) );
		labelAyInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, jointNames[i] + singleSpace + string("<A-Y>") ) );
		labelAzInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, jointNames[i] + singleSpace + string("<A-Z>") ) );
		labelTxInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, jointNames[i] + singleSpace + string("<T-X>") ) );
		labelTyInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, jointNames[i] + singleSpace + string("<T-Y>") ) );
		labelTzInd.push_back( m_VfileRawParser.GetLabelPosByName( recordIndex, jointNames[i] + singleSpace + string("<T-Z>") ) );
	}
	
	for(int32 i=0; i< frameCount; ++i)
	{
		for(int32 j=0; j< numJoints; ++j)
		{		
			Vec3r a;
			m_VfileRawParser.GetJointDataByName(recordIndex, labelAxInd[j], i, a.x); 
			m_VfileRawParser.GetJointDataByName(recordIndex, labelAyInd[j], i, a.y); 
			m_VfileRawParser.GetJointDataByName(recordIndex, labelAzInd[j], i, a.z); 
		
			Vec3r t;
			m_VfileRawParser.GetJointDataByName(recordIndex, labelTxInd[j], i, t.x); 
			m_VfileRawParser.GetJointDataByName(recordIndex, labelTyInd[j], i, t.y); 
			m_VfileRawParser.GetJointDataByName(recordIndex, labelTzInd[j], i, t.z); 
		
			QuatT qt;
			qt.q = exp( a*0.5 );
			qt.t = t/1000.0; // convert to meters

			bodyData[j].push_back(qt);
		}
	}

	return 1;
}

