////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  VfileParser.h
//  Version:    v1.00
//  Created:    22/5/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2005 Professional
//  Description: Vicon V-file parser

// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __VfileParser_h__
#define __VfileParser_h__

#if _MSC_VER > 1000
#pragma once
#endif

// Read in groups and their data directly
class CVfileRawParser
{
public:
	CVfileRawParser();
	~CVfileRawParser();
public:
	struct SRecord
	{
		SRecord():id(0), numLabels(0), numDofsAll(0), frameHertz(0.0f), dataType(0), frameCount(0){}
		uint32 id; // Record id.
		string name; // e.g. GlobalBodies, LocalBodies, Markers
		uint32 numLabels; // e.g., joints
		uint32 numDofsAll; // All the dofs: Num labels * dofs each label
		f32 frameHertz;
		std::vector<string> labelNames; // Label names in the group: e.g. "SUBJECT::LTIBIA <A-X>". All in upper case.
		uint8 dataType;
		int32 frameCount;
		std::vector< std::vector<uint8> > data; // The data in dynamic section, ordered frame by frame. Dof by dof in each frame.
		std::map<string, int32> labelNameIndexMap; // labelName -> index, SUBJECT: is removed since it's not reliable
		std::map<string, int32> labelNameIndexMapNoSubject; // labelName -> index, SUBJECT: is removed since it's not reliable
	};

	struct SHeader
	{
		uint16 vFileVersion;
	};

public:
	void ReadVFile(const char* path);
	inline string GetPureJointName(const string fullName);
	inline int32 GetJointDataByName(const int32 recordIndex, const int32 labelIndex, const int32 frameNum, f64& data);
	const std::vector<string>& GetJointPureNames(const int32 recordIndex)const {return m_JointPureNames[recordIndex];}
	const int32 GetRecordIndexByName(const string name) const {return stl::find_in_map(m_RecordNameIndexMap, name, -1);}
	const uint32 GetFrameCount() const;
	const f64 GetFrameHertz() const;
	const int32 GetLabelPosByName(const int32 recordIndex, const char* labelName) const {return stl::find_in_map(m_Records[recordIndex].labelNameIndexMapNoSubject, labelName, -1);}
	
private:
	static int32 typeSize[7]; // This is standard V-file definition
	std::vector< SRecord > m_Records; // E.g. "Global bodies", "Local Bodies", "markers" et. al.
	SHeader m_Header;
	std::map<string, int32> m_RecordNameIndexMap; // map record name to actual index in m_Records.
	std::map<string, int32> m_RecordNameIdMap; // map record name to record id.
	std::map<int32, int32> m_RecordIdIndexMap; // map record id to actual index in m_Records
	std::vector<uint32> m_DofEachJoint; // How many Dof for each joint or marker, the same size as m_Records
	std::vector< std::vector<string> > m_JointPureNames; // pure names for each record, e.g. "ltibia", no duplicated names

	void ReadHeader(const uint8* p);
	void ReadStaticSection(const uint8* p, int16& len);
	void ReadDynamicSection(const uint8* p);
	inline int32 GetTypeSize(uint8 typeInd) const;
	void CalcuDofEachJoint();
	void SetRecordJointPureNames();
};

// Get higher-level data
class CVfileParser
{
public:
	CVfileParser(const char* path);
	~CVfileParser();

	void ReafVFile(const char* path);
	int32 GetMarkerPositions(std::vector< std::vector<Vec3r> >& markerPos, std::vector<string>& markerNames);
	int32 GetJointData(const char* recordName, std::vector< std::vector<QuatT> >& globalBodies, std::vector<string>& jointNames);
	CVfileRawParser m_VfileRawParser;	
};
#endif