#include "stdafx.h"


#include "AnimationLoader.h"
#include "AnimationManager.h"
#include "Wavelets\Compression.h"

#include <float.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



namespace ControllerHelper
{

	uint32 GetPositionsFormatSizeOf(uint32 format)
	{
		switch(format)
		{
		case eNoCompress:
		case eNoCompressVec3:
			{
				return sizeof (NoCompressVec3);
			}
		}
		return 0;
	}


	uint32 GetRotationFormatSizeOf(uint32 format)
	{
		switch(format)
		{
		case	eNoCompress:
		case eNoCompressQuat:
			return sizeof(NoCompressQuat);

			//case eSmallTreeDWORDQuat:
			//	return sizeof(Sma;

		case eSmallTree48BitQuat:
			return sizeof(SmallTree48BitQuat);

		case eSmallTree64BitQuat:
			return sizeof(SmallTree64BitQuat);

		case eSmallTree64BitExtQuat:
			return sizeof(SmallTree64BitExtQuat);

		}
		return 0;

	}

	uint32 GetKeyTimesFormatSizeOf(uint32 format)
	{
		switch(format)
		{

		case eF32:
			return sizeof(float);

		case eUINT16:
			return sizeof(uint16);

		case eByte:
			return sizeof(byte);

		case eF32StartStop:
			return sizeof(float);

		case eUINT16StartStop:
			return sizeof(uint16);

		case eByteStartStop:
			return sizeof(byte);

		case eBitset:
			return sizeof(uint16);
		}

		return 0;

	}


	ITrackPositionStorage * GetPositionControllerPtr(uint32 format)
	{

		switch(format)
		{
		case eNoCompress:
		case eNoCompressVec3:
			{
				//					return new NoCompressVec3PositionTrackInformation;
				//					IPositionInformation * pTrack = new PositionTrackInformation;
				//				ITrackPositionStorage * pStorage = new NoCompressPosition;
				//					pTrack->SetPositionStorage(TrackPositionStoragePtr(pStorage));
				//					return pTrack;
				return new NoCompressPosition;
			}


		}
		return 0;
		//return new 
	}

	ITrackRotationStorage * GetRotationControllerPtr(uint32 format)
	{
		switch(format)
		{

			/*
			typedef CTrackDataStorage<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotation;
			typedef _smart_ptr<NoCompressRotation> NoCompressRotationPtr;

			typedef CTrackDataStorage<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotation;
			typedef _smart_ptr<SmallTree48BitQuatRotation> SmallTree48BitQuatRotationPtr;

			typedef CTrackDataStorage<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotation;
			typedef _smart_ptr<SmallTree64BitQuatRotation> SmallTree64BitQuatRotationPtr;

			typedef CTrackDataStorage<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotation;
			typedef _smart_ptr<SmallTree64BitExtQuatRotation> SmallTree64BitExtQuatRotationPtr;

			*/
		case	eNoCompress:
		case eNoCompressQuat:
			return new NoCompressRotation;

			//case eShotInt3Quat:

			//	//{
			//	//	//					return new ShotInt3QuatRotationTrackInformation;
			//	//	IPositionInformation * pTrack = new PositionTrackInformation;
			//	//	ITrackPositionStorage * pStorage = new NoCompressPosition;
			//	//	pTrack->SetPositionStorage(TrackPositionStoragePtr(pStorage));

			//	//	return pTrack;
			//	//}



			//case eSmallTreeDWORDQuat:
			//	//return new SmallTreeDWORDQuatRotationTrackInformation;

		case eSmallTree48BitQuat:
			//return new SmallTree48BitQuatRotationTrackInformation;
			return new SmallTree48BitQuatRotation;

		case eSmallTree64BitQuat:
			//return new SmallTree64BitQuatRotationTrackInformation;
			return new SmallTree64BitQuatRotation;

			//case ePolarQuat:
			//	//return new PolarQuatRotationTrackInformation;

		case eSmallTree64BitExtQuat:
			//return new SmallTree64BitExtQuatRotationTrackInformation;
			return new SmallTree64BitExtQuatRotation;

		}
		return 0;
	}

	IKeyTimesInformation * GetKeyTimesControllerPtr(uint32 format)
	{
		switch(format)
		{
		case eF32:
			return new F32KeyTimesInformation;

		case eUINT16:
			return new UINT16KeyTimesInformation;

		case eByte:
			return new ByteKeyTimesInformation;

		//case eF32StartStop:
		//	return new F32SSKeyTimesInformation;

		//case eUINT16StartStop:
		//	return new UINT16SSKeyTimesInformation;

		//case eByteStartStop:
		//	return new ByteSSKeyTimesInformation;

		case eBitset:
			return new CKeyTimesInformationBitSet;

		}
		return 0;
	}

};



void CCompressedController::UnCompress()
{

	CWaveletData Rotations[3];
	CWaveletData Positions[3];

	Wavelet::UnCompress(m_Rotations0, Rotations[0]);
	Wavelet::UnCompress(m_Positions0, Positions[0]);
	Wavelet::UnCompress(m_Rotations1, Rotations[1]);
	Wavelet::UnCompress(m_Positions1, Positions[1]);
	Wavelet::UnCompress(m_Rotations2, Rotations[2]);
	Wavelet::UnCompress(m_Positions2, Positions[2]);

}

void CCompressedController::Compress(IController * uncompressedController)//TrackInformationPtr& uncompRotation, PositionInformationPtr& uncompPosition)
{

	CWaveletData Rotations[3];
	CWaveletData Positions[3];

	int rotSizes = 0;

	if (m_pRotationController->GetNumKeys() > 44)
	{
		Rotations[0].m_Data.resize(m_pRotationController->GetNumKeys());
		Rotations[1].m_Data.resize(m_pRotationController->GetNumKeys());
		Rotations[2].m_Data.resize(m_pRotationController->GetNumKeys());

		CControllerPQLog * pController = dynamic_cast<CControllerPQLog *>(uncompressedController);
		if (pController)
		{
			uint32 endOrig = pController->m_arrTimes.size();

			for (uint32 i = 0; i < m_pRotationController->GetNumKeys(); ++i)
			{
				Quat quat;

				uint32 key = uint32(m_pRotationController->GetTimeFromKey(i));
				for (uint32 j =0; j < endOrig; ++j)
				{
					if (key == pController->m_arrTimes[j]/* pCController->GetRotationController()->GetTimeFromKey(j)*/)
					{
						key = j;//pController->m_arrTimes[j]/*pCController->GetRotationController()->GetTimeFromKey(j)*/;
						break;
					}
				}

				// Get uncompressed value to increase precision
				quat = !exp(pController->m_arrKeys[key].vRotLog);
				// Convert to euler angles
				Ang3 angles = Ang3::GetAnglesXYZ(quat);

				Rotations[0].m_Data[i] = angles.x;
				Rotations[1].m_Data[i] = angles.y;
				Rotations[2].m_Data[i] = angles.z;
			}
		}
		else
		{
			CController * pCController  = dynamic_cast<CController*>(uncompressedController);

			if (!pCController)
			{
				assert(false);
				return;
			}
			assert(m_pRotationController->GetNumKeys() != 34);

			uint32 endOrig = pCController->GetRotationController()->GetNumKeys();

			for (uint32 i = 0; i < m_pRotationController->GetNumKeys(); ++i)
			{

				Quat quat;
				//m_pRotationController->GetValueFromKey(i, quat);
				uint32 key = uint32(m_pRotationController->GetTimeFromKey(i));
				// Get uncompressed value to increase precision
				for (uint32 j =0; j < endOrig; ++j)
				{
					if (key == pCController->GetRotationController()->GetTimeFromKey(j))
					{
						key = j;//pCController->GetRotationController()->GetTimeFromKey(j);
						break;
					}
				}
				pCController->GetRotationController()->GetValueFromKey(key, quat);
				// Convert to euler angles
				Ang3 angles = Ang3::GetAnglesXYZ(quat);

				Rotations[0].m_Data[i] = angles.x;
				Rotations[1].m_Data[i] = angles.y;
				Rotations[2].m_Data[i] = angles.z;
			}
		}

		if (m_pRotationController->GetNumKeys() & 1)
		{
			// Add last value for even number of samples
			Rotations[0].m_Data.push_back(Rotations[0].m_Data[Rotations[0].m_Data.size()-1]);
			Rotations[1].m_Data.push_back(Rotations[1].m_Data[Rotations[1].m_Data.size()-1]);
			Rotations[2].m_Data.push_back(Rotations[2].m_Data[Rotations[2].m_Data.size()-1]);
		}

		int wavelet = 22;
		float rotError = 0.00001f;//0.000001f

		Wavelet::Compress(Rotations[0], m_Rotations0, 0, 1, rotError, rotError, wavelet, wavelet);//0.000001f
		rotSizes += m_Rotations0.size() * sizeof(int);
		Wavelet::Compress(Rotations[1], m_Rotations1, 0, 1, rotError, rotError, wavelet, wavelet);//0.000001f
		rotSizes += m_Rotations1.size() * sizeof(int);
		Wavelet::Compress(Rotations[2], m_Rotations2, 0, 1, rotError, rotError, wavelet, wavelet);//0.000001f
		rotSizes += m_Rotations2.size() * sizeof(int);

		Wavelet::UnCompress(m_Rotations0, Rotations[0]);
		Wavelet::UnCompress(m_Rotations1, Rotations[1]);
		Wavelet::UnCompress(m_Rotations2, Rotations[2]);


		if (rotSizes > m_pRotationController->GetRotationStorage()->GetDataRawSize())
		{
			// Quantization format takes less memory. Don't use wavelets
			rotSizes = m_pRotationController->GetRotationStorage()->GetDataRawSize();
			m_Rotations0.clear();
			m_Rotations1.clear();
			m_Rotations2.clear();
		}
	}

	int posSizes = 0;

	if (m_pPositionController && m_pPositionController->GetNumKeys() > 44)
	{
		Positions[0].m_Data.resize(m_pPositionController->GetNumKeys());
		Positions[1].m_Data.resize(m_pPositionController->GetNumKeys());
		Positions[2].m_Data.resize(m_pPositionController->GetNumKeys());

		for (uint32 i = 0; i < m_pPositionController->GetNumKeys(); ++i)
		{
			Vec3 pos;
			m_pPositionController->GetValueFromKey(i, pos);
			Positions[0].m_Data[i] = pos.x;
			Positions[1].m_Data[i] = pos.y;
			Positions[2].m_Data[i] = pos.z;
		}

		if (m_pPositionController->GetNumKeys() & 1)
		{
			// Add last value for even number of samples
			Positions[0].m_Data.push_back(Positions[0].m_Data[Positions[0].m_Data.size()-1]);
			Positions[1].m_Data.push_back(Positions[1].m_Data[Positions[1].m_Data.size()-1]);
			Positions[2].m_Data.push_back(Positions[2].m_Data[Positions[2].m_Data.size()-1]);
		}

		float posError = 0.0001f; //0.000001f

		Wavelet::Compress(Positions[0], m_Positions0, 0, 1, posError, posError, 22, 22); //0.000001f
		posSizes += m_Positions0.size() * sizeof(int);
		Wavelet::Compress(Positions[1], m_Positions1, 0, 1, posError, posError, 22, 22); //0.000001f
		posSizes += m_Positions1.size() * sizeof(int);
		Wavelet::Compress(Positions[2], m_Positions2, 0, 1, posError, posError, 22, 22); //0.000001f
		posSizes += m_Positions2.size() * sizeof(int);

		if (posSizes > m_pPositionController->GetPositionStorage()->GetDataRawSize())
		{
			posSizes = m_pPositionController->GetPositionStorage()->GetDataRawSize();
			m_Positions0.clear();
			m_Positions1.clear();
			m_Positions2.clear();
		}
	}


	m_iRotationFormat = /*eSmallTree64BitExtQuat;//*/m_pRotationController->GetFormat();
	if (m_pPositionController)
		m_iPositionFormat = m_pPositionController->GetFormat();

	m_CompressedSize = rotSizes + posSizes;
}

byte CKeyTimesInformationBitSet::m_bTable[256] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,
4,4,5,4,5,5,6,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,
3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,1,2,2,3,2,3,3,4,2,3,3,
4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,
4,5,5,6,4,5,5,6,5,6,6,7,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,
6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};
