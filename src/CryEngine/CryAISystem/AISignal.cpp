#include "StdAfx.h"
#include "AISignal.h"
#include <crc32.h>

//====================================================================
// AISIGNAL_EXTRA_DATA 
//====================================================================
AISignalExtraData::SignalExtraDataAlloc AISignalExtraData::m_signalExtraDataAlloc;

AISignalExtraData::AISignalExtraData()
{
	point.zero();
	point2.zero();
	fValue = 0.0f;
	nID = 0;
	iValue = 0;
	iValue2 = 0;
	sObjectName = NULL;
}

AISignalExtraData::~AISignalExtraData()
{
	if ( sObjectName )
		delete [] sObjectName;
}

void AISignalExtraData::SetObjectName( const char* objectName )
{
	if ( sObjectName )
	{
		delete [] sObjectName;
		sObjectName = NULL;
	}
	if ( objectName && *objectName )
	{
		sObjectName = new char[strlen( objectName ) + 1];
		strcpy( sObjectName, objectName );
	}
}

void AISignalExtraData::Serialize( TSerialize ser )
{
	ser.Value("point", point);
	ser.Value("point2", point2);
	//	ScriptHandle nID;
	ser.Value("fValue", fValue);
	ser.Value("iValue", iValue);
	ser.Value("iValue2", iValue2);
	string	objectNameString(sObjectName);
	ser.Value("sObjectName", objectNameString);

	// (MATT) This change in #149616 will probably break save compatibility for Crysis - however, we may find a way to do without these strings anyway.
	// I believe it's the only part of the change that will actually impact the saves - the rest could be left as-is. {2008/01/14:18:14:18}
	string sStringData1( string1 );
	ser.Value("string1", sStringData1);

	string sStringData2( string2 );
	ser.Value("string2", sStringData2);

	if(ser.IsReading())
	{
		SetObjectName(objectNameString.c_str());
		string1 = sStringData1.c_str();
		string1 = sStringData2.c_str();
	}
}

AISignalExtraData& AISignalExtraData:: operator = ( const AISignalExtraData& other )
{
	point = other.point;
	point2 = other.point2;
	nID = other.nID;
	fValue = other.fValue;
	iValue = other.iValue;
	iValue2 = other.iValue2;
	string1 = other.string1;
	string2 = other.string2;
	SetObjectName( other.sObjectName );
	return *this;
};

void AISignalExtraData::ToScriptTable(SmartScriptTable &table) const
{
	// These may create tables on the fly
	CScriptVector oVec(gEnv->pScriptSystem);
	CScriptVector oVec2(gEnv->pScriptSystem);
	oVec.Set(point);
	oVec2.Set(point2);

	table->SetValue("id", nID);
	table->SetValue("fValue", fValue);
	table->SetValue("iValue", iValue);
	table->SetValue("iValue2", iValue2);
	table->SetValue("string1", string1);
	table->SetValue("string2", string2);	
	table->SetValue("point", oVec);
	table->SetValue("point2", oVec2);

	if (sObjectName && sObjectName[0])
		table->SetValue("ObjectName", sObjectName);
}

void AISignalExtraData::FromScriptTable(const SmartScriptTable &table)
{
	// These may create tables on the fly
	CScriptVector oVec(gEnv->pScriptSystem);
	CScriptVector oVec2(gEnv->pScriptSystem);

	table->GetValue("id",nID);
	table->GetValue("fValue",fValue);
	table->GetValue("iValue",iValue);
	table->GetValue("iValue2",iValue2);
	table->GetValue("string1",string1);
	table->GetValue("string2",string2);
	table->GetValue("point",oVec);
	table->GetValue("point2",oVec2);

	point = oVec.Get();
	point2 = oVec2.Get();

	const char* sTableObjectName;
	if (table->GetValue("ObjectName",sTableObjectName) && sTableObjectName[0])
		SetObjectName(sTableObjectName);
}


//====================================================================
// AISIGNAL Serialize 
//====================================================================
void AISIGNAL::Serialize( TSerialize ser, class CObjectTracker& objectTracker )
{
	ser.Value("nSignal",nSignal);

	string	textString(strText);
	ser.Value("strText", textString);
	ser.Value("senderID", senderID);

	if(ser.IsReading())
	{
		strcpy(strText, textString.c_str());
		m_nCrcText = gAIEnv.SignalCRCs.m_crcGen->GetCRC32(textString);
	}

	if (ser.IsReading())
	{
		if (pEData)
			delete (AISignalExtraData*) pEData;
		pEData = new AISignalExtraData;
	}

	if (pEData)
		pEData->Serialize(ser);
	else
	{
		AISignalExtraData dummy;
		dummy.Serialize(ser);
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IAISignalExtraData)
