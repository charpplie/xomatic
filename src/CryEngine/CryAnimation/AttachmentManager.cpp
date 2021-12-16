//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:AttachmentManager.cpp
//  Implementation of AttachmentManager class
//
//	History:
//	August 16, 2004: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include <CryHeaders.h>

#include <Cry_Camera.h>
#include "ModelMesh.h"
#include "AttachmentManager.h"
#include "CharacterInstance.h"
#include "CharacterManager.h"

#include "IGame.h"
#include "IGameFramework.h"

uint32 CAttachmentManager::LoadAttachmentList(const char* pathname ) {

	XmlNodeRef nodeAttachList	= g_pISystem->LoadXmlFile(pathname);	
	const char* AttachListTag		= nodeAttachList->getTag();
	if (strcmp(AttachListTag,"AttachmentList")==0) 
	{
		int anum2 = GetAttachmentCount();
		RemoveAllAttachments();
		anum2 = GetAttachmentCount();

		uint32 num = nodeAttachList->getChildCount();
		for (uint32 i=0; i<num; i++) {

			XmlNodeRef nodeAttach = nodeAttachList->getChild(i);
			const char* AttachTag = nodeAttach->getTag();
			if (strcmp(AttachTag,"Attachment")==0) 
			{
				Quat WRotation;
				Vec3 WPosition;
				string AName = nodeAttach->getAttr( "AName" );
				string Type  = nodeAttach->getAttr( "Type" );
				nodeAttach->getAttr( "Rotation",WRotation );
				nodeAttach->getAttr( "Position",WPosition );
				string BoneName = nodeAttach->getAttr( "BName" );
				string ObjectFileName = nodeAttach->getAttr( "Binding" );

				IAttachment* pIAttachment=0;

				if (Type=="CA_BONE") 
				{
					pIAttachment = CreateAttachment(AName,CA_BONE,BoneName);
					if (pIAttachment==0) continue; 
					pIAttachment->SetAttAbsoluteDefault( QuatT(WRotation,WPosition) );

					IStatObj* pIStatObj = g_pI3DEngine->LoadStatObj( ObjectFileName );
					if (pIStatObj) 
					{
						CCGFAttachment* pStatAttachment = new CCGFAttachment();
						pStatAttachment->pObj  = pIStatObj;
						IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
						pIAttachment->AddBinding( pIAttachmentObject );
					}
				} 
				if (Type=="CA_FACE") 
				{
					pIAttachment = CreateAttachment(AName,CA_FACE,0);
					pIAttachment->SetAttAbsoluteDefault(QuatT(WRotation,WPosition));

					IStatObj* pIStatObj = g_pI3DEngine->LoadStatObj( ObjectFileName );
					if (pIStatObj) 
					{
						CCGFAttachment* pStatAttachment = new CCGFAttachment();
						pStatAttachment->pObj  = pIStatObj;
						IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
						pIAttachment->AddBinding( pIAttachmentObject );
					}
				}
				if (Type=="CA_SKIN") 
				{
					pIAttachment = CreateAttachment(AName,CA_SKIN,0);
					pIAttachment->SetAttAbsoluteDefault( QuatT(WRotation,WPosition) );

					IAttachmentObject* pIAttachmentObject=0;
					ICharacterInstance* pIChildCharacter=0;

					const char* fileExt = PathUtil::GetExt(ObjectFileName);

					bool IsCDF = (0 == stricmp(fileExt,"cdf"));
					if (IsCDF) 
					{
						if (pIChildCharacter) 
						{
							CCHRAttachment* pCharacterAttachment = new CCHRAttachment();
							pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
							pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
						}
						pIChildCharacter = g_pCharacterManager->LoadCharacterDefinition( ObjectFileName );
					}

					bool IsCHR = (0 == stricmp(fileExt,"chr"));
					if (IsCHR)
					{
						pIChildCharacter = g_pCharacterManager->CreateInstance( ObjectFileName );
						if (pIChildCharacter==0)
						{
							g_pILog->LogError ("CryAnimation: no character created: %s", pathname);
						} 
						else 
						{
							CCHRAttachment* pCharacterAttachment = new CCHRAttachment();
							pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
							pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
						}
					}

					pIAttachment->AddBinding(pIAttachmentObject);
				}

			}
		}
		uint32 count = GetAttachmentCount();
		assert(count==num);
		ProjectAllAttachment();
	}

	return 1;
};


uint32 CAttachmentManager::SaveAttachmentList(const char* pathname ) {

	uint32 count = GetAttachmentCount();

	if (count) {
		XmlNodeRef nodeAttachements		= g_pISystem->CreateXmlNode( "AttachmentList" );	

		for (uint32 i=0; i<count; i++) 
		{
			IAttachment*  pIAttachment = GetInterfaceByIndex(i);

			XmlNodeRef nodeAttach = nodeAttachements->newChild( "Attachment" );
			const char* AName = pIAttachment->GetName();
			Quat WRotation = pIAttachment->GetAttAbsoluteDefault().q;
			Vec3 WPosition = pIAttachment->GetAttAbsoluteDefault().t;
			uint32 Type = pIAttachment->GetType();

			uint32 BoneID = pIAttachment->GetBoneID();
			const char* BoneName	= m_pSkelInstance->m_SkeletonPose.GetJointNameByID(BoneID);	

			const char* BindingName = "";
			IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();

			if (Type==CA_BONE || Type==CA_FACE) 
			{
				if (pIAttachmentObject)
				{
					IStatObj* pIAttachmentStatObject = pIAttachmentObject->GetIStatObj();
					if (pIAttachmentStatObject)	
						BindingName=pIAttachmentStatObject->GetFilePath();
				}
			}

			if (Type==CA_SKIN || Type==CA_PART) 
			{
				if (pIAttachmentObject)
				{
					ICharacterInstance* pICharacterChild=pIAttachmentObject->GetICharacterInstance();
					if (pICharacterChild) 
					{
						BindingName = pICharacterChild->GetICharacterModel()->GetModelFilePath();
					}
				}
			}

			nodeAttach->setAttr( "AName",AName );
			if (Type==CA_BONE) nodeAttach->setAttr( "Type", "CA_BONE" );
			if (Type==CA_FACE) nodeAttach->setAttr( "Type", "CA_FACE" );
			if (Type==CA_SKIN) nodeAttach->setAttr( "Type", "CA_SKIN" );
			nodeAttach->setAttr( "Rotation",WRotation );
			nodeAttach->setAttr( "Position",WPosition );
			nodeAttach->setAttr( "BName",BoneName );
			nodeAttach->setAttr( "Binding", BindingName );
		}

		nodeAttachements->saveToFile(pathname);
	}
	return 1;
};

IAttachment* CAttachmentManager::CreateAttachment( const char* szName, uint32 type, const char* szBoneName,bool bCallProject,bool bAllowDuplicates, bool bNoDefaultPose ) 
{
	IAttachment* pIAttachment = GetInterfaceByName(szName);
	if (pIAttachment && !bAllowDuplicates )
		return 0;

	uint32 rm = m_pSkelInstance->GetResetMode();
	if (!bNoDefaultPose && (type==CA_BONE || type==CA_FACE))
		m_pSkelInstance->GetISkeletonPose()->SetDefaultPose();

	m_pSkelInstance->SetResetMode(1);

	int32 nBoneID=-1;
	if (type==CA_BONE) 
	{
		if(szBoneName==0)
		{
			m_pSkelInstance->SetResetMode(rm);
			return 0;
		}

		nBoneID = m_pSkelInstance->m_SkeletonPose.GetJointIDByName(szBoneName);
		if(nBoneID<0)
		{
			m_pSkelInstance->SetResetMode(rm);
			return 0;
		}


	}

	CAttachment* pAttachment = new CAttachment();

	pAttachment->m_Type=type;
	pAttachment->m_pAttachmentManager=this;
	pAttachment->m_Name = szName;


	if (type==CA_BONE) { 

		pAttachment->m_BoneID = nBoneID;//m_pSkelInstance->m_SkeletonAnim.GetIDByName(szBoneName);
		assert(pAttachment->m_BoneID<4096);

		pAttachment->m_pIAttachmentObject	=	0;
		pAttachment->m_AttAbsoluteDefault = m_pSkelInstance->m_SkeletonPose.GetPoseData().m_jointsAbsolute[pAttachment->m_BoneID];
		assert(pAttachment->m_AttAbsoluteDefault.IsValid());
		pAttachment->m_AttRelativeDefault.SetIdentity();
		pAttachment->m_FaceNr	=	0;
		pAttachment->m_AttFlags =	0;
		if( bCallProject )
		{
			pAttachment->ProjectAttachment();
		}
		m_arrAttachments.push_back(pAttachment);	
	}

	if (type==CA_FACE) 
	{ 
		pAttachment->m_pIAttachmentObject	=	0;
		if( bCallProject )
		{
			pAttachment->ProjectAttachment();
		}
		m_arrAttachments.push_back(pAttachment);	
		m_pSkinInstance->m_UpdateAttachmentMesh = true;
		m_pSkelInstance->m_UpdateAttachmentMesh = true;
	}

	if (type==CA_SKIN || type==CA_PART) 
	{ 
		m_arrAttachments.push_back(pAttachment);	
	}

	m_pSkelInstance->SetResetMode(rm);
	return pAttachment; 
}; 

void CAttachmentManager::ApplyVisibilityMask(int mask)
{
	int32 numAttachments = m_arrAttachments.size();
	for (int32 i=0; i<numAttachments; i++)
	{
		CAttachment& ainst	= *m_arrAttachments[i];

		if (ainst.GetType() == CA_PART)
		{
			// Apply visibility flags on attachment
			ICharacterPartAttachment* pPartAttachment = (ICharacterPartAttachment*)ainst.GetIAttachmentObject();
			if( !pPartAttachment )
				continue;

			int vis = mask & pPartAttachment->GetVisibilityMask();

			if ( vis & (~VISIBLE_SHADOW_GENERATION) )
				ainst.HideAttachment(false);
			else
				ainst.HideAttachment(true);

			if ( vis & VISIBLE_SHADOW_GENERATION )
			{
				ainst.HideInRecursion(false);
				ainst.HideInShadow(false);
			}
			else
				ainst.HideInShadow(true);
		}
	}
}

ICharacterInstance* CAttachmentManager::GetSkinInstance() const
{
	return m_pSkinInstance;
} 
ICharacterInstance* CAttachmentManager::GetSkelInstance() const
{
	return m_pSkelInstance;
} 

void CAttachmentManager::PhysicalizeAttachment(int idx, int nLod, IPhysicalEntity *pent, const Vec3 &offset)
{
	IStatObj *pStatObj;
	IAttachment *pAttachment=GetInterfaceByIndex(idx);

	if( !pAttachment )
		return;

	bool bWasPhysicalized = false;
	if (!(pAttachment=GetInterfaceByIndex(idx)) || !(pAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
		!pAttachment->GetIAttachmentObject() )
	{
		if( pAttachment && pAttachment->GetIAttachmentObject() )
		{
			bWasPhysicalized = pAttachment->GetIAttachmentObject()->PhysicalizeAttachment(this,idx,nLod,pent,offset);
		}
	}
	//
	if( bWasPhysicalized )
		return;

	// old path
	if (!(pAttachment=GetInterfaceByIndex(idx)) || pAttachment->GetType()!=CA_BONE || !(pAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
		!pAttachment->GetIAttachmentObject() || !(pStatObj=pAttachment->GetIAttachmentObject()->GetIStatObj()) || !pStatObj->GetPhysGeom() ||
		pAttachment->IsAttachmentHidden())
		return;

	int iBone = pAttachment->GetBoneID();
	pe_articgeomparams gp;
	Matrix34 mtx = Matrix34(m_pSkelInstance->m_SkeletonPose.GetPoseData().m_jointsAbsolute[iBone] * ((CAttachment*)pAttachment)->m_AttRelativeDefault);
	mtx.AddTranslation(offset);
	gp.pMtx3x4 = &mtx;
	//FIXME:
	gp.idbody = m_pSkelInstance->GetISkeletonPose()->getBonePhysParentOrSelfIndex(iBone,nLod);
	gp.flags = 0;
	if (pAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED_COLLISIONS)
		gp.flags = geom_colltype_solid|geom_colltype_solid|geom_floats|geom_colltype_explosion;
	if (pAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED_RAYS)
		gp.flags |= geom_colltype_ray;
	pent->AddGeometry(pStatObj->GetPhysGeom(),&gp,1000+idx);
	pAttachment->SetFlags(pAttachment->GetFlags() | FLAGS_ATTACH_WAS_PHYSICALIZED);
}

void CAttachmentManager::PhysicalizeAttachment( int idx, IPhysicalEntity *pent, int nLod )
{
	if (!pent)
		if (!(pent = m_pSkelInstance->GetISkeletonPose()->GetCharacterPhysics()))
			return;
	PhysicalizeAttachment(idx,nLod,pent,m_pSkelInstance->m_SkeletonPose.GetOffset());
}

void CAttachmentManager::DephysicalizeAttachment(int idx, IPhysicalEntity *pent)
{
	if (!pent)
		if (!(pent = m_pSkelInstance->GetISkeletonPose()->GetCharacterPhysics()))
			return;
	pent->RemoveGeometry(1000+idx);
	IAttachment *pAttachment = GetInterfaceByIndex(idx);
	if (pAttachment)
		pAttachment->SetFlags(pAttachment->GetFlags() & ~FLAGS_ATTACH_WAS_PHYSICALIZED);
}

int CAttachmentManager::UpdatePhysicalizedAttachment(int idx, IPhysicalEntity *pent, const QuatT &offset)
{
	IStatObj *pStatObj;
	IAttachment *pAttachment;

	bool WasHandled = false;
	if (!(pAttachment=GetInterfaceByIndex(idx)) || !(pAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
		!pAttachment->GetIAttachmentObject()/* || !(pStatObj=pAttachment->GetIAttachmentObject()->GetIStatObj()) || !pStatObj->GetPhysGeom()*/)
	{
		if( pAttachment && pAttachment->GetIAttachmentObject() )
		{
			WasHandled = pAttachment->GetIAttachmentObject()->UpdatePhysicalizedAttachment(this,idx,pent,offset);
		}
	}

	if( WasHandled )
		return 0;

	if (!(pAttachment=GetInterfaceByIndex(idx)) || pAttachment->GetType()!=CA_BONE || !(pAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
		!pAttachment->GetIAttachmentObject() || !(pStatObj=pAttachment->GetIAttachmentObject()->GetIStatObj()) || !pStatObj->GetPhysGeom())
		return 0;

	int changed = 0;
	if (pAttachment->IsAttachmentHidden() && pAttachment->GetFlags() & FLAGS_ATTACH_WAS_PHYSICALIZED)
		DephysicalizeAttachment(idx, pent), changed=1;
	else if (!pAttachment->IsAttachmentHidden() && !(pAttachment->GetFlags() & FLAGS_ATTACH_WAS_PHYSICALIZED))
		PhysicalizeAttachment(idx,pent), changed=1;

	int iBone = pAttachment->GetBoneID();
	pe_params_part pp;
	Matrix34 mtx = Matrix34(offset*pAttachment->GetAttModelRelative());
	pp.partid = 1000+idx;
	pp.pMtx3x4 = &mtx;
	pp.bRecalcBBox = 0;
	pent->SetParams(&pp);
	return changed;
}

int CAttachmentManager::UpdatePhysAttachmentHideState(int idx, IPhysicalEntity *pent)
{
	IStatObj *pStatObj;
	IAttachment *pAttachment;

	if (!(pAttachment=GetInterfaceByIndex(idx)) || pAttachment->GetType()!=CA_BONE || !(pAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
		!pAttachment->GetIAttachmentObject() || !(pStatObj=pAttachment->GetIAttachmentObject()->GetIStatObj()) || !pStatObj->GetPhysGeom())
		return 0;

	if (pAttachment->IsAttachmentHidden() && pAttachment->GetFlags() & FLAGS_ATTACH_WAS_PHYSICALIZED)
	{
		DephysicalizeAttachment(idx, pent);
		return 2;
	}
	if (!pAttachment->IsAttachmentHidden() && !(pAttachment->GetFlags() & FLAGS_ATTACH_WAS_PHYSICALIZED))
	{
		PhysicalizeAttachment(idx,pent);
		return 1;
	}
	return 3;
}


//////////////////////////////////////////////////////////////////////////
void CAttachmentManager::ResetAdditionalRotation()
{
	uint32 numAttachments = m_arrAttachments.size();
	for (uint32 i=0; i<numAttachments; i++)
		m_arrAttachments[i]->m_additionalRotation.SetIdentity();
}


int32 CAttachmentManager::RemoveAttachmentByInterface( const IAttachment* ptr )
{
	CAttachment* p=(CAttachment*)ptr;
	const char* name=p->m_Name;  
	return RemoveAttachmentByName(name);
}

int32 CAttachmentManager::RemoveAttachmentByName( const char* szName ) 
{

	int32 index = GetIndexByName( szName );
	//	assert(index!=-1);
	if (index==-1) return 0;

	RemoveAttachmentByIndex(index);

	return 1;
};


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

/*
namespace
{
class Evil : public CCharInstance
{
public:
Evil() : CCharInstance() { }

void GetSkeletonPose( int nLOD, const Matrix34& rRenderMat34, QuatTS*& pBoneQuatsL, QuatTS*& pBoneQuatsS, QuatTS*& pMBBoneQuatsL, QuatTS*& pMBBoneQuatsS, Vec4 shapeDeformationData[], uint32 &HWSkinningFlags )
{
CSkinInstance::GetSkeletonPose( nLOD, rRenderMat34, pBoneQuatsL, pBoneQuatsS,pMBBoneQuatsL,pMBBoneQuatsS, shapeDeformationData, HWSkinningFlags );
}
};
static Evil evil;
void Mystify(ICharacterInstance *const p)
{
*(void **)p = *(void **)&evil;
}
}
*/

uint32 CAttachment::AddBinding( IAttachmentObject* pModel ) 
{

	m_pAttachmentManager->m_pSkinInstance->SetHaveEntityAttachments(false);

	if (m_Type==CA_FACE || m_Type==CA_BONE)
	{
		if (m_pIAttachmentObject)
			m_pIAttachmentObject->Release();
		m_pIAttachmentObject=pModel;	

		if (pModel)
		{
			uint32 IsFastUpdateType = m_pAttachmentManager->m_pSkinInstance->IsFastUpdateType( pModel->GetAttachmentType() );
			if(IsFastUpdateType)
				m_pAttachmentManager->m_pSkinInstance->SetHaveEntityAttachments(true);
		}
		return 1;	
	}

	//----------------------------------------------------------------------------------------------
	if (m_Type==CA_SKIN || m_Type==CA_PART)
	{
		CSkinInstance* pInstanceMaster = m_pAttachmentManager->m_pSkelInstance;

		if (pModel) 
		{
			CSkinInstance* pInstanceSlave=(CSkinInstance*)pModel->GetICharacterInstance();
			if (pInstanceSlave==0)
			{
				// we can attach slave only for character parts
				if( m_Type==CA_PART )
				{
					m_pIAttachmentObject=pModel;

					m_AttRelativeDefault.SetIdentity();
					m_AttAbsoluteDefault.SetIdentity();

					m_FaceNr = 0;
					m_BoneID = 0xffffffff;

					m_AttModelRelative.SetIdentity();
					m_AttModelRelativePrev.SetIdentity();
					m_AttWorldAbsolute.SetIdentity();
					return 1;
				}
				//This is an serious error. We cannot attach a non-character
				g_pILog->LogError ("Subskin is no character. Attaching not possible" );
				assert(0);
				return 0;
			}

			CCharacterModel* pModelMaster = ((CSkinInstance*)pInstanceMaster)->m_pModel;
			uint32 numBonesMaster = pModelMaster->m_ModelSkeleton.m_arrModelJoints.size();
			uint32 found=0;
			CCharacterModel* pModelSlave = pInstanceSlave->m_pModel;
			uint32 numBonesSlave = pModelSlave->m_ModelSkeleton.m_arrModelJoints.size();
			uint32 NotMatchingNames=0;
			//
			if (numBonesSlave>numBonesMaster)
			{
				// we can attach slave only for character parts
				if( m_Type!=CA_PART )
				{
					//This is an serious error. We cannot attach a slave to a master with less bones
		
//Diesel cut
				//	g_pILog->LogError ("SLAVE-skeleton needs to have less bone then MASTER-skeleton. SLAVE:%d MASTER:%d ", numBonesSlave, numBonesMaster );
				//	assert(!"SLAVE-skeleton needs to have less bone then MASTER-skeleton");
//Diesel cut					
					
					//return 0;
				}
			}
			//
			m_arrRemapTable.resize(numBonesSlave);
			// fill by zeroes
			for(uint32 s=0; s<numBonesSlave; s++) 
			{
				m_arrRemapTable[s] = 0;
			}
			for(uint32 js=0; js<numBonesSlave; js++) 
			{
				found=0;
				const uint32 sHash = pModelSlave->m_ModelSkeleton.m_arrModelJoints[js].GetJointNameHash();
				for(uint32 m=0; m<numBonesMaster; m++) 
				{
					const uint32 mHash = pModelMaster->m_ModelSkeleton.m_arrModelJoints[m].GetJointNameHash();
					if (sHash == mHash) 
					{
						assert(stricmp(pModelSlave->m_ModelSkeleton.m_arrModelJoints[js].GetJointName(),pModelMaster->m_ModelSkeleton.m_arrModelJoints[m].GetJointName()) == 0);

						found=1;
						m_arrRemapTable[js]=m;
						break;
					}
				}

				if (found==0)
				{
					bool bErrorMessage = true;
					if( m_Type==CA_PART )
					{
						// check is it leave node
						bool isLeave = true;
						for(uint32 s=0; s<numBonesSlave; s++) 
						{
							int16 parentID = pModelSlave->m_ModelSkeleton.m_arrModelJoints[s].m_idxParent;
							if( parentID==s )
							{
								isLeave = false;
								break;
							}
						}
						if( isLeave )
						{
							bErrorMessage = false;
							NotMatchingNames = 0;
						}

					}
					//bones name of slave not found in master
					if( bErrorMessage )
					{
//Diesel cut
						//g_pILog->LogError ("Bone-names of SLAVE-skeleton don't match with bone-names of MASTER-skeleton:  %s", 
						//	pModelSlave->m_ModelSkeleton.m_arrModelJoints[js].GetJointName());
//Diesel cut						
						NotMatchingNames++;
						
//Diesel cut
						//assert(!"Missmatching bone names between master and slave skeletons!");
						

						//return 0;
					}
				}
			} //for loop

			if (NotMatchingNames)
				return 0; //critical! incompatible skeletons. cant create skin-attachment

		}
		// face attachments helpers for part attachment
		if( m_Type==CA_PART )
		{
			assert(pModel);
			ICharacterInstance* pIChildCharacter = pModel->GetICharacterInstance();
			//
			uint32 numBones = pIChildCharacter->GetISkeletonPose()->GetJointCount();
			assert(numBones);
			//
			const char* modelName = pIChildCharacter->GetICharacterModel()->GetName();
			size_t modelNameLen = modelName ? strlen(modelName) : 0;
			if( modelNameLen!=0 )
			{
				for (uint32 i=0; i <numBones; i++)
				{
					const char * sn = pIChildCharacter->GetISkeletonPose()->GetJointNameByID(i);
					// if joint name contains model name if format $socket$MODELNAME$...
					if( sn==NULL || strncmp(sn,"$socket$", strlen("$socket$"))!=0 )
						continue;
					if( strnicmp(sn+strlen("$socket$"),modelName,modelNameLen)!=0 )
						continue;
					const char* smChar  = sn+strlen("$socket$")+modelNameLen;
					if( smChar==NULL )
						continue;
					if( *(smChar+0)!='$' )
						continue;
					if( *(smChar+1)==NULL )
						continue;
					//
					const char* name = smChar+1;
					// passed
					const QuatT& absPos = pIChildCharacter->GetISkeletonPose()->GetDefaultAbsJointByID(i);
					CCharacterModel* pObjectModel = static_cast<CCharacterModel*>(pIChildCharacter->GetICharacterModel());
					CloseInfo cf = pObjectModel->m_arrModelMeshes[pObjectModel->m_nBaseLOD].FindClosestPointOnMesh( absPos.t );
					Vec3 x = (cf.tv1-cf.tv0).GetNormalized();
					Vec3 z = ((cf.tv1-cf.tv0)%(cf.tv0-cf.tv2)).GetNormalized();
					Vec3 y = z%x;
					QuatT TriMat=QuatT::CreateFromVectors(x,y,z,cf.middle);
					ICharacterPartAttachment* pModelCharPart = static_cast<ICharacterPartAttachment*>(pModel);
					pModelCharPart->AddFaceAttachment(GetName(),name, TriMat.GetInverted() *  absPos, absPos, cf.FaceNr);
				}
			}
		}
		//
		m_pIAttachmentObject=pModel;

		m_AttRelativeDefault.SetIdentity();
		m_AttAbsoluteDefault.SetIdentity();

		m_FaceNr = 0;
		m_BoneID = 0xffffffff;

		m_AttModelRelative.SetIdentity();
		m_AttModelRelativePrev.SetIdentity();
		m_AttWorldAbsolute.SetIdentity();
		return 1; 
	}
	return 0; 
}

uint32 CAttachmentManager::ProjectAllAttachment() 
{
	uint32 rm = m_pSkelInstance->GetResetMode();

	m_pSkelInstance->GetISkeletonPose()->SetDefaultPose();
	m_pSkelInstance->SetResetMode(1);

	uint32 numAttachments = m_arrAttachments.size();
	for (uint32 i=0; i<numAttachments; i++)
	{
		CAttachment* pAttachment	= m_arrAttachments[i];
		uint32 type = pAttachment->m_Type; 

		if (type==CA_FACE) 
		{
			CloseInfo cf=m_pSkinInstance->m_pModel->m_arrModelMeshes[m_pSkinInstance->m_pModel->m_nBaseLOD].FindClosestPointOnMesh( pAttachment->m_AttAbsoluteDefault.t );
			Vec3 x = (cf.tv1-cf.tv0).GetNormalized();
			Vec3 z = ((cf.tv1-cf.tv0)%(cf.tv0-cf.tv2)).GetNormalized();
			Vec3 y = z%x;
			QuatT TriMat=QuatT::CreateFromVectors(x,y,z,cf.middle);
			pAttachment->m_AttRelativeDefault	= TriMat.GetInverted() *  pAttachment->m_AttAbsoluteDefault ;
			pAttachment->m_FaceNr			=	cf.FaceNr;
		}

		if (type==CA_BONE) 
		{
			//ICryBone* pBone		=	m_pSkelInstance->GetBoneByIndex(pAttachment->m_BoneID);
			assert(pAttachment->m_BoneID<4096);
			QuatT BoneMat	= m_pSkelInstance->m_SkeletonPose.GetPoseData().m_jointsAbsolute[pAttachment->m_BoneID];	
			pAttachment->m_AttRelativeDefault = BoneMat.GetInverted() * pAttachment->m_AttAbsoluteDefault;
		}
	}
	//
	m_pSkinInstance->m_UpdateAttachmentMesh = true;
	m_pSkelInstance->m_UpdateAttachmentMesh = true;
	UpdateLocationAttachments(QuatT(IDENTITY),0);
	m_pSkinInstance->m_UpdateAttachmentMesh = true;
	m_pSkelInstance->m_UpdateAttachmentMesh = true;
	//
	m_pSkelInstance->SetResetMode(rm);
	return 1;
}

SPU_NO_INLINE IAttachment* CAttachmentManager::GetInterfaceByName(  const char* szName ) const
{ 
	int32 idx = GetIndexByName( szName ); 
	if (idx==-1) return 0;
	return GetInterfaceByIndex( idx );
};


SPU_NO_INLINE int32 CAttachmentManager::GetIndexByName( const char* szName ) const
{ 
	int num = GetAttachmentCount();
	for (int i = 0; i < num; i++)	
	{
		IAttachment* pA = GetInterfaceByIndex(i);
		if(stricmp(pA->GetName(),szName)==0) 
			return i;
	}
	return -1;
}

SPU_NO_INLINE void CAttachmentManager::UpdateLocationAttachments(const QuatT &rPhysLocationNext,IAttachmentObject::EType *pOnlyThisType )
{

	CSkinInstance* pSkin = m_pSkinInstance;
	//this piece of code is only valueeable if asserts are enabled
#if !defined(PS3) || defined(_DEBUG)
	CAttachment* pCAttachment = m_pSkelInstance->m_pSkinAttachment;
	CCharInstance* pMaster = m_pSkelInstance;
	if (pCAttachment) 
	{
		uint32 type = pCAttachment->GetType();
		if (type==CA_SKIN || type==CA_PART) 
			pMaster = pCAttachment->m_pAttachmentManager->m_pSkelInstance; //this is a skin attachment. we need the skeleton-master
		else 
			assert(0);
	}
	assert(pSkin);
	assert(pMaster);
#endif

	CModelMesh* pSkinning = pSkin->m_pModel->GetModelMesh(m_pSkinInstance->m_pModel->m_nBaseLOD);

	PrefetchLine(m_arrAttachments.GetForPrecache(0), 0);
	PrefetchLine(m_arrAttachments.GetForPrecache(0), 128);
	PrefetchLine(m_arrAttachments.GetForPrecache(1), 0);
	PrefetchLine(m_arrAttachments.GetForPrecache(1), 128);

	if (pOnlyThisType==0)
	{
		//--------------------------------------------
		//--- create attachment mesh               ---
		//--------------------------------------------
		if (pSkin->m_UpdateAttachmentMesh && pSkinning)
		{
			m_arrAttSkin.resize(0);
			m_arrAttSkinnedStream.resize(0);

			MoveConnectedFacesToNewBuffer();

			if (  ((CMorphing*)pSkin->GetIMorphing())->NeedMorph() && Console::GetInst().ca_UseMorph)
			{
				CreateMorphTargetsForAllConnectedPhases();
			}

		}
		SkinningAttSW( pSkinning );
	}


	//update attachmnets 
	const uint32 numAttachments=m_arrAttachments.size();
	for (uint32 i=0; i<numAttachments; i++)
	{
		int iPrecache = i + 2;
		PrefetchLine(m_arrAttachments.GetForPrecache(iPrecache), 0);
		PrefetchLine(m_arrAttachments.GetForPrecache(iPrecache), 128);

		CAttachment* pAttachment	= m_arrAttachments[i];		
		if (pOnlyThisType && pAttachment->m_pIAttachmentObject && *pOnlyThisType != pAttachment->m_pIAttachmentObject->GetAttachmentType())
			continue;
		
		if (pAttachment->m_Type==CA_BONE ) 
		{
			UpdateBoneAttachment(rPhysLocationNext, pAttachment);
		}
		else if (pAttachment->m_Type==CA_FACE ) 
		{
			UpdateFaceAttachment(rPhysLocationNext, pAttachment);
		}
		
		if (pAttachment->m_Type==CA_BONE || pAttachment->m_Type==CA_FACE) 
		{
			UpdateDanglyAttachment(pAttachment);
		}
	}
}





SPU_NO_INLINE void CAttachmentManager::UpdateLocationAttachmentsFast(const QuatT &rPhysLocationNext)
{
	//update attachments
	const uint32 numAttachments=m_arrAttachments.size();
	for (uint32 i=0; i<numAttachments; i++)
	{
		CAttachment* pAttachment = m_arrAttachments[i];
		IAttachmentObject* pIAttachmentObject = pAttachment->m_pIAttachmentObject;

		if (pAttachment->m_Type==CA_FACE) 
		{
			pAttachment->m_AttModelRelative.SetIdentity();         
			pAttachment->m_AttWorldAbsolute = rPhysLocationNext;
		}
		else if (pAttachment->m_Type==CA_BONE ) 
		{
			if (pIAttachmentObject && pIAttachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_Entity)
			{
				//for normal bone-attachments we use this code
				QuatT AbsBoneMat	= m_pSkelInstance->m_SkeletonPose.GetPoseData().m_jointsAbsolute[pAttachment->m_BoneID];        
				QuatT attModelRelative = AbsBoneMat*pAttachment->m_AttRelativeDefault * pAttachment->m_additionalRotation;
				pAttachment->m_AttWorldAbsolute = rPhysLocationNext * attModelRelative;
				pAttachment->m_AttModelRelative = attModelRelative;
			}
			else
			{
				pAttachment->m_AttModelRelative.SetIdentity();         
				pAttachment->m_AttWorldAbsolute = rPhysLocationNext;
			}
		}
	}
}




//////////////////////////////////////////////////////////////////////////
// skinning of internal vertices 
//////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CAttachmentManager::SkinningAttSW( CModelMesh* pSkinning )
{

#ifdef DEFINE_PROFILER_FUNCTION
	DEFINE_PROFILER_FUNCTION();
#endif

	uint32 HowManyVertices = m_arrAttSkin.size();
	if (HowManyVertices==0) return;

	//-----------------------------------------------------------------------
	//----           Create the internal morph-target stream             ----
	//-----------------------------------------------------------------------
	uint32 numAttVertices = m_arrAttMorphStream.size();	
	assert(numAttVertices);
	memset(&m_arrAttMorphStream[0],0,numAttVertices*sizeof(Vec3));

	// Timur, Not used for now.
	///*
	if (  ((CMorphing*)m_pSkelInstance->GetIMorphing())->NeedMorph() && Console::GetInst().ca_UseMorph)
	{
		uint32 numMorphEffectors=((CMorphing*)m_pSkelInstance->GetIMorphing())->m_arrMorphEffectors.size();
		for (uint32 me=0; me<numMorphEffectors; ++me)
		{
			const CryModEffMorph& rMorphEffector = ((CMorphing*)m_pSkelInstance->GetIMorphing())->m_arrMorphEffectors[me];
			int nMorphTargetId = rMorphEffector.getMorphTargetId ();
			if (nMorphTargetId < 0)
				continue;
			const AttMorphTargets& rMorphSkin = m_arrMorphTargets[nMorphTargetId];
			float fBlending = rMorphEffector.getBlending();
			uint32 num = rMorphSkin.m_arrAttMorph.size();
			for(uint32 i=0; i<num; i++) 
			{
				// add all morph-values to the stream to use subsequently in skinning
				uint32 idx = rMorphSkin.m_arrAttMorph[i].nVertexId;
				m_arrAttMorphStream[idx]+=rMorphSkin.m_arrAttMorph[i].ptVertex*fBlending;
			}
		}
	}
	//*/

	//-----------------------------------------------------------------------
	//----    Create 8 Vec4 vectors that contain the blending values     ----
	//---                    to blend between 3 models                    ---
	//-----------------------------------------------------------------------
	int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);

	CCharInstance* pMaster = m_pSkelInstance;
	CAttachment* pAttachment = m_pSkinInstance->m_pSkinAttachment;
	if (pAttachment) 
	{
		uint32 type = pAttachment->GetType();
		if (type==CA_SKIN || type==CA_PART) 
			pMaster = pAttachment->m_pAttachmentManager->m_pSkelInstance; //this is a skin attachment. we need the skeleton-master
		else 
			assert(0);
	}
	assert(pMaster);

	uint32 rm = pMaster->GetResetMode();
	f32 MorphArray[8] = { 0,0,0,0,0,0,0,0};

	if (rm==0) 
	{ 
		f32* pMorphValues = pMaster->GetShapeDeformArray();
		MorphArray[0]=pMorphValues[0];
		MorphArray[1]=pMorphValues[1];
		MorphArray[2]=pMorphValues[2];
		MorphArray[3]=pMorphValues[3];
		MorphArray[4]=pMorphValues[4];
		MorphArray[5]=pMorphValues[5];
		MorphArray[6]=pMorphValues[6];
		MorphArray[7]=pMorphValues[7];
	}

	Vec4 VertexRegs[8];
	for (uint32 i=0; i<8; i++) 
	{
		if (MorphArray[i]<0.0f) 
		{
			VertexRegs[i].x	=	1-(MorphArray[i]+1);
			VertexRegs[i].y	=	MorphArray[i]+1;
			VertexRegs[i].z	=	0;
		} else {
			VertexRegs[i].x	=	0;
			VertexRegs[i].y	=	1-MorphArray[i];
			VertexRegs[i].z	=	MorphArray[i];
		}
	}

	//--------------------------------------------------------
	//----   this part of code belongs into hardware      ----
	//---  this loop is a simulation of the vertex-shader  ---
	//--------------------------------------------------------
	assert(m_arrAttSkinnedStream.size()==HowManyVertices);

	QuatTS arrNewSkinQuat[MAX_JOINT_AMOUNT+1];	//bone quaternions for skinning (entire skeleton)
//	uint32 iActiveFrame		=	pMaster->m_iActiveFrame;
	QuatTS *pArrnewSkinQuat = &pMaster->m_arrNewSkinQuat[nList][0];
	if(pAttachment)
	{
		uint32 numRemapJoints	= pAttachment->m_arrRemapTable.size();		
		uint8 *pRemapTable = &pAttachment->m_arrRemapTable[0];
		for (uint32 i=0; i<numRemapJoints; i++)	arrNewSkinQuat[i]=pArrnewSkinQuat[pRemapTable[i]];
	}
	else
	{
		uint32 numJoints	= pMaster->m_SkeletonPose.GetPoseData().m_jointsAbsolute.size();
		cryMemcpy(arrNewSkinQuat, pArrnewSkinQuat,	sizeof(QuatTS)*numJoints);
	}

	if (Console::GetInst().ca_SphericalSkinning) 
	{
		for(uint32 i=0; i<HowManyVertices; i++ ) 
		{
			const AttSkinVertex &rCurAttSkin = m_arrAttSkin[i];

			//spherical skinning
			//create the final vertex for skinning (blend between 3 characters)
#if defined(XENON) || defined(PS3)
			uint8 idx		=	rCurAttSkin.color.r; 
#else
			uint8 idx		=	rCurAttSkin.color.a; 
			assert(idx<8);
#endif			
			Vec3 v	= rCurAttSkin.wpos0*VertexRegs[idx].x + rCurAttSkin.wpos1*VertexRegs[idx].y + rCurAttSkin.wpos2*VertexRegs[idx].z;

			//apply morph-targets
			v+=m_arrAttMorphStream[i];

			//get indices for bones (always 4 indices per vertex)
			uint32 b0 = rCurAttSkin.boneIDs[0];
			uint32 b1 = rCurAttSkin.boneIDs[1];
			uint32 b2 = rCurAttSkin.boneIDs[2];
			uint32 b3 = rCurAttSkin.boneIDs[3];

			//get indices for vertices (always 4 weights per vertex)
			f32 w0 = rCurAttSkin.weights[0];
			f32 w1 = rCurAttSkin.weights[1];
			f32 w2 = rCurAttSkin.weights[2];
			f32 w3 = rCurAttSkin.weights[3];
			//assert(fabsf((w0+w1+w2+w3)-1.0f)<0.0001f);

			const QuatD& q0=(const QuatD&)arrNewSkinQuat[b0];
			const QuatD& q1=(const QuatD&)arrNewSkinQuat[b1];
			const QuatD& q2=(const QuatD&)arrNewSkinQuat[b2];
			const QuatD& q3=(const QuatD&)arrNewSkinQuat[b3];
			QuatD wquat = (q0*w0 +  q1*w1 + q2*w2 +	q3*w3);

			f32 l=1.0f/wquat.nq.GetLength();
			wquat.nq*=l;
			wquat.dq*=l;

			Vec3 bv			=	QuatT(wquat)*v;
			m_arrAttSkinnedStream[i] = bv;

		}
	}
	else
	{
		//linear skinning
		for(uint32 i=0; i<HowManyVertices; i++ ) 
		{
			const AttSkinVertex &rCurAttSkin = m_arrAttSkin[i];

			//create the final vertex for skinning (blend between 3 characters)
#if defined(XENON) || defined(PS3)
			uint8 idx		=	rCurAttSkin.color.r; 
#else
			uint8 idx		=	rCurAttSkin.color.a; 
			assert(idx<8);
#endif
			Vec3 v	= rCurAttSkin.wpos0*VertexRegs[idx].x + rCurAttSkin.wpos1*VertexRegs[idx].y + rCurAttSkin.wpos2*VertexRegs[idx].z;

			//apply morph-targets
			v+=m_arrAttMorphStream[i];

			//get indices for bones (always 4 indices per vertex)
			uint32 b0 = rCurAttSkin.boneIDs[0];
			uint32 b1 = rCurAttSkin.boneIDs[1];
			uint32 b2 = rCurAttSkin.boneIDs[2];
			uint32 b3 = rCurAttSkin.boneIDs[3];

			//get indices for vertices (always 4 weights per vertex)
			f32 w0 = rCurAttSkin.weights[0];
			f32 w1 = rCurAttSkin.weights[1];
			f32 w2 = rCurAttSkin.weights[2];
			f32 w3 = rCurAttSkin.weights[3];
			//assert(fabsf((w0+w1+w2+w3)-1.0f)<0.0001f);

			//do the skinning transformation 
			const QuatTS& m0 = arrNewSkinQuat[b0];
			const QuatTS& m1 = arrNewSkinQuat[b1];
			const QuatTS& m2 = arrNewSkinQuat[b2];
			const QuatTS& m3 = arrNewSkinQuat[b3];
			Vec3 v0=m0*v*w0;
			Vec3 v1=m1*v*w1;
			Vec3 v2=m2*v*w2;
			Vec3 v3=m3*v*w3;
			m_arrAttSkinnedStream[i] = v0+v1+v2+v3;
		}
	}
}

void CAttachmentManager::DrawAttachments(const SRendParams& rRendParams, Matrix34& WorldMat34)
{
#ifdef DEFINE_PROFILER_FUNCTION
	DEFINE_PROFILER_FUNCTION();
#endif

#if !defined(_RELEASE)
	if (Console::GetInst().ca_DrawAttachments==0)
		return;
#endif

	//CAttachments are > 1 cache line
#ifndef _DEBUG
	CryPrefetch(m_arrAttachments[0]);
	CryPrefetch(((char*)m_arrAttachments[0]) + 128);
#endif

	f32 fFOV = g_pIRenderer->GetCamera().GetFov();
	f32 fZoomFactor = 0.5f + 0.5f*(RAD2DEG(fFOV)/60.f);
	fZoomFactor *= fZoomFactor*fZoomFactor;
	fZoomFactor *= 4;

	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );


	uint32 InRecursion = (uint32)(UINT_PTR)g_pIRenderer->EF_Query(EFQ_RecurseLevel);
	bool InShadow = GetISystem()->GetI3DEngine()->GetRenderIntoShadowmap();


	uint32 numAttachments = m_arrAttachments.size();
	for (uint32 i=0; i<numAttachments; i++)
	{
		CAttachment& ainst	= *m_arrAttachments[i];

#ifndef _DEBUG
		int iNextIndex = i + 1;
		CryPrefetch(m_arrAttachments[iNextIndex]);
		CryPrefetch(((char*)m_arrAttachments[iNextIndex]) + 128);
		CryPrefetch(&m_arrAttachments[iNextIndex + 1]);
#endif

#if !defined(_RELEASE)
		if (Console::GetInst().ca_DrawFaceAttachments==0 && ainst.m_Type==CA_FACE)
			continue;
#endif

		if (InRecursion>1)
		{
			if (ainst.m_HideInRecursion) 
				continue;
		}
		else if (InShadow)
		{
			if (ainst.m_HideInShadowPass) 
				continue;
		}
		else 
		{
			if (ainst.m_HideInMainPass) 
				continue;
		}

		IAttachmentObject * pAttachmentObject = ainst.GetIAttachmentObject();

		if (pAttachmentObject)
		{
			AABB caabb =	pAttachmentObject->GetAABB();
			const f32 fRadiusSqr	=	(caabb.max-caabb.min).GetLengthSquaredFloat();

			//if radius is zero, then the object is most probably not visible and we can continue 			
			if (fRadiusSqr==0.0f)
				continue;

			const bool bFaceOrBone = ainst.m_Type <= CA_FACE;

			if (bFaceOrBone) 
			{
				f32 fScaledDist = rRendParams.fDistance*fZoomFactor;
				if (sqr(fScaledDist) > sqr(Console::GetInst().ca_AttachmentCullingRation) * fRadiusSqr) 
					continue;
			}

			if (ainst.GetType() == CA_PART)
			{
				ICharacterPartAttachment* pPartAttachment = (ICharacterPartAttachment*)pAttachmentObject;
				//
				if (!pPartAttachment->GetICharacterInstance())
					continue;
			}

			Matrix34 FinalMat34 = WorldMat34 * Matrix34(ainst.m_AttModelRelative);


#if !defined(_RELEASE)
			if (Console::GetInst().ca_DrawAttachmentOBB) 
			{
				OBB obb2=OBB::CreateOBBfromAABB( Matrix33(FinalMat34),caabb );
				if (ainst.m_Type==CA_BONE) 
					g_pAuxGeom->DrawOBB(obb2,FinalMat34.GetTranslation(),0,RGBA8(0x1f,0x00,0xff,0xff),eBBD_Extremes_Color_Encoded);
				if (ainst.m_Type==CA_FACE) 
					g_pAuxGeom->DrawOBB(obb2,FinalMat34.GetTranslation(),0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);
				//Vec3 WPos		= Matrix33(FinalMat34)*obb2.c+FinalMat34.GetTranslation();
				//g_pAuxGeom->DrawSphere( WPos,radius, RGBA8(0xff,0x00,0x1f,0xff) );
			}
#endif

			SRendParams rParams (rRendParams);

			Matrix34 pAttachPrevMat34 = Matrix34(ainst.m_AttModelRelativePrev);
			ainst.m_AttModelRelativePrev = QuatT(FinalMat34);
			if( rRendParams.pPrevMatrix && ((m_pSkelInstance->m_fFPWeapon!=0) == m_pSkelInstance->m_bPrevFrameFPWeapon) )
				rParams.pPrevMatrix = &pAttachPrevMat34;
			else
				rParams.pPrevMatrix = &FinalMat34;

			rParams.pMatrix = &FinalMat34;

			// this is required to avoid the attachments using the parent character material (this is the material that overrides the default material in the attachment)
			rParams.pMaterial = NULL;

			if (m_pSkelInstance->rp.m_nFlags & CS_FLAG_DRAW_NEAR)
			{
				rParams.dwFObjFlags |= FOB_NEAREST;
			}

			if( bFaceOrBone )
			{
				uint32 BoneID = ainst.GetBoneID();
				if( BoneID<m_pSkelInstance->m_bCustomScale.size() && m_pSkelInstance->m_bCustomScale[BoneID] )
				{
					rParams.pMatrix->Scale(Vec3(m_pSkelInstance->GetCustomScaleForJoint(BoneID)));
				}
				else if( BoneID<m_pSkelInstance->m_IgnoreScale.size() && !m_pSkelInstance->m_IgnoreScale[BoneID] )
				{
					rParams.pMatrix->Scale(Vec3(m_pSkelInstance->GetUniformScale()));
				}
				else 
				{
					f32 s = m_pSkelInstance->GetUniformScale();
					rParams.pMatrix->m00*=s; rParams.pMatrix->m01*=s; rParams.pMatrix->m02*=s; 
					rParams.pMatrix->m10*=s; rParams.pMatrix->m11*=s; rParams.pMatrix->m12*=s; 
					rParams.pMatrix->m20*=s; rParams.pMatrix->m21*=s; rParams.pMatrix->m22*=s; 
					if (ainst.GetType()==CA_FACE)
					{
						rParams.pMatrix->m03*=s;
						rParams.pMatrix->m13*=s;
						rParams.pMatrix->m23*=s;
					} 
				}
			}

			if(_finite(rParams.pMatrix->m00))
				ainst.m_pIAttachmentObject->RenderAttachment(rParams, m_arrAttachments[i] );
			else
				assert(!"Invalid matrix");
		}
		else
		{
#if !defined(_RELEASE)
			if ((ainst.m_Type==CA_BONE || ainst.m_Type==CA_FACE) && Console::GetInst().ca_DrawEmptyAttachments) 
			{
				Matrix34 FinalMat34 = WorldMat34 * Matrix34(ainst.m_AttModelRelative);
				Vec3 pos =  FinalMat34.GetTranslation();
				static Ang3 angle1(0,0,0); 
				static Ang3 angle2(0,0,0); 
				static Ang3 angle3(0,0,0);
				angle1 += Ang3(0.01f,+0.02f,+0.03f);
				angle2 -= Ang3(0.01f,-0.02f,-0.03f);
				angle3 += Ang3(0.03f,-0.02f,+0.01f);

				AABB aabb1 = AABB(Vec3( -0.05f, -0.05f, -0.05f),Vec3( +0.05f, +0.05f, +0.05f));
				AABB aabb2 = AABB(Vec3(-0.005f,-0.005f,-0.005f),Vec3(+0.005f,+0.005f,+0.005f));

				Matrix33 m33;	OBB obb;

				m33=Matrix33::CreateRotationXYZ(angle1);
				obb=OBB::CreateOBBfromAABB( m33,aabb1 );
				g_pAuxGeom->DrawOBB(obb,pos,0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);
				obb=OBB::CreateOBBfromAABB( m33,aabb1 );
				g_pAuxGeom->DrawOBB(obb,pos,0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);

				m33=Matrix33::CreateRotationXYZ(angle2);
				obb=OBB::CreateOBBfromAABB( m33,aabb1 );
				g_pAuxGeom->DrawOBB(obb,pos,0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);
				obb=OBB::CreateOBBfromAABB( m33,aabb2 );
				g_pAuxGeom->DrawOBB(obb,pos,0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);

				m33=Matrix33::CreateRotationXYZ(angle3);

				obb=OBB::CreateOBBfromAABB( m33,aabb1 );
				g_pAuxGeom->DrawOBB(obb,pos,0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);
				obb=OBB::CreateOBBfromAABB( m33,aabb2 );
				g_pAuxGeom->DrawOBB(obb,pos,0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);

				obb=OBB::CreateOBBfromAABB( ainst.m_AttModelRelative.q,aabb1 );
				g_pAuxGeom->DrawOBB(obb,pos,0,RGBA8(0xff,0xff,0xff,0xff),eBBD_Extremes_Color_Encoded);
			}
#endif
		}
	}  
}







IAttachment* CAttachmentManager::GetInterfaceByIndex( uint32 c) const
{ 
	size_t size=m_arrAttachments.size();
	if (size==0) return 0;
	if (size<=c) return 0;
	return m_arrAttachments[c];
};

void CAttachmentManager::RemoveAttachmentByIndex(uint32 n) 
{ 
	//
	CAttachment* pAttachment = (CAttachment*)GetInterfaceByIndex(n);
	assert(pAttachment);
	if (pAttachment==0) return;
	assert(pAttachment->m_Type==CA_BONE || pAttachment->m_Type==CA_FACE || pAttachment->m_Type==CA_SKIN || pAttachment->m_Type==CA_PART);
	//
	if( pAttachment->GetIAttachmentObject() )
		pAttachment->GetIAttachmentObject()->OnRemoveAttachment(this,n);
	//
	size_t size = m_arrAttachments.size();
	if (size==0) return;
	if (size<=n) return;
	m_arrAttachments[n]->ClearBinding();
	delete(m_arrAttachments[n]);
	m_arrAttachments[n]=m_arrAttachments[size-1];	
	m_arrAttachments.pop_back();
}

uint32 CAttachmentManager::RemoveAllAttachments() 
{
	uint32 counter = GetAttachmentCount();
	for (uint32 i=counter; i>0; i--) 
		RemoveAttachmentByIndex(i-1);
	return 1;
}

void CAttachmentManager::Serialize(TSerialize ser)
{

	if (ser.GetSerializationTarget() != eST_Network)
	{
		ser.BeginGroup("CAttachmentManager");
		int32 numAttachments=GetAttachmentCount();			
		for (int32 i=0; i<numAttachments; i++)
		{
			CAttachment* pCAttachment = (CAttachment*)GetInterfaceByIndex(i);
			pCAttachment->Serialize(ser);
		}
		ser.EndGroup();
	}
}



//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------


uint32 CAttachment::SetType(uint32 type, const char* szBoneName ) 
{ 
	m_Type		=	type;
	m_BoneID	=	0xffffffff;

	if (type==CA_BONE) 
	{
		if(!szBoneName)	{ assert(0); return 0; }
		int BoneId = m_pAttachmentManager->m_pSkelInstance->m_SkeletonPose.GetJointIDByName(szBoneName);
		if(BoneId < 0)
		{
			// this is a severe bug if the bone name is invalid and someone tries to attach something
			g_pILog->LogError ("AttachObjectToBone is called for bone \"%s\", which is not in the model \"%s\". Ignoring, but this may cause a crash because the corresponding object won't be detached after it's destroyed", szBoneName, m_pAttachmentManager->m_pSkelInstance->m_pModel->GetModelFilePath());
			return 0;
		}

		if (m_AlignBoneAttachment)
		{
			QuatT m34 = m_pAttachmentManager->m_pSkelInstance->m_SkeletonPose.GetPoseData().m_jointsAbsolute[BoneId];
			m_AttAbsoluteDefault=m34;
			m_AttRelativeDefault.SetIdentity();
		}

		m_BoneID			=	BoneId;
		ProjectAttachment();
	}

	if (type==CA_FACE) 
	{
		m_AttAbsoluteDefault.SetIdentity();
		ProjectAttachment();
	}

	if (type==CA_SKIN || type==CA_PART) 
	{
		m_AttAbsoluteDefault.SetIdentity();
		m_AttRelativeDefault.SetIdentity();
	}

	return 1;
}; 


uint32 CAttachment::ProjectAttachment() 
{ 

	static ColorB color = RGBA8(0xff,0xff,0xff,0x00);
	color.r+=0x13;
	color.g+=0x23;
	color.b+=0x33;

	CSkinInstance*					pSkinInstance = m_pAttachmentManager->m_pSkinInstance;
	CCharInstance*	pSkelInstance = m_pAttachmentManager->m_pSkelInstance;

	uint32 rm = pSkelInstance->GetResetMode();
	pSkelInstance->GetISkeletonPose()->SetDefaultPose();
	pSkelInstance->SetResetMode(1);

	pSkinInstance->m_UpdateAttachmentMesh = true;
	pSkelInstance->m_UpdateAttachmentMesh = true;

	//#pragma message("TODO: CAttachment::ProjectAttachment() shouldn't render debug helpers by default. Ivo, please fix.")
	//#pragma message("Yeahhh, I'll do it later! Good work needs time, ya know?")


	SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	renderFlags.SetFillMode( e_FillModeWireframe );
	renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
	g_pAuxGeom->SetRenderFlags( renderFlags );

	if (m_Type==CA_FACE) 
	{

		CCharacterModel* pModel = pSkinInstance->m_pModel;
		if (pModel->m_ObjectType==CHR)
		{
			CloseInfo cf=pModel->m_arrModelMeshes[pModel->m_nBaseLOD].FindClosestPointOnMesh( m_AttAbsoluteDefault.t );
			Vec3 x = (cf.tv1-cf.tv0).GetNormalized();
			Vec3 z = ((cf.tv1-cf.tv0)%(cf.tv0-cf.tv2)).GetNormalized();
			Vec3 y = z%x;
			QuatT TriMat=QuatT::CreateFromVectors(x,y,z,cf.middle);
			m_AttRelativeDefault	= TriMat.GetInverted() * m_AttAbsoluteDefault;
			m_FaceNr			=	cf.FaceNr;
			if (pSkelInstance->m_SkeletonAnim.GetCharEditMode())
			{
				//float fColor[4] = {0,1,0,1};
				//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_FaceNr: %d", m_FaceNr );	g_YLine+=16.0f;
				//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"v0 (%f %f %f)", x.x,x.y,x.z );	g_YLine+=16.0f;
				//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"v1 (%f %f %f)", y.x,y.y,y.z );	g_YLine+=16.0f;
				//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"v2 (%f %f %f)", z.x,z.y,z.z );	g_YLine+=16.0f;

				g_pAuxGeom->DrawLine( cf.middle, color, m_AttAbsoluteDefault.t,color );
				g_pAuxGeom->DrawLine( cf.tv0,RGBA8(0xff,0x00,0x00,0x00), cf.tv1,RGBA8(0x00,0xff,0x00,0x00) );
				g_pAuxGeom->DrawLine( cf.tv1,RGBA8(0x00,0xff,0x00,0x00), cf.tv2,RGBA8(0x00,0x00,0xff,0x00) );
				g_pAuxGeom->DrawLine( cf.tv2,RGBA8(0x00,0x00,0xff,0x00), cf.tv0,RGBA8(0xff,0x00,0x00,0x00) );
			}
		}

	}

	if (m_Type==CA_BONE) 
	{
		QuatT BoneMat = pSkelInstance->m_SkeletonPose.GetPoseData().m_jointsAbsolute[m_BoneID];
		m_AttRelativeDefault = BoneMat.GetInverted() * m_AttAbsoluteDefault;
		m_AttWorldAbsolute = BoneMat;
		if (pSkelInstance->m_SkeletonAnim.GetCharEditMode())
			g_pAuxGeom->DrawLine(BoneMat.t,color,  m_AttAbsoluteDefault.t,color );
	}

	pSkelInstance->SetResetMode(rm);
	return 1;
}

void CAttachment::ClearBinding() 
{ 
	if (m_pIAttachmentObject)
		m_pIAttachmentObject->Release();
	m_pIAttachmentObject = 0;
	if (m_pAttachmentManager && m_pAttachmentManager->m_pSkinInstance)
		m_pAttachmentManager->m_pSkinInstance->SetHaveEntityAttachments(false);
}; 

void CAttachment::GetHingeParams(int &idx,float &limit,float &damping) 
{
	if (!m_pHinge) 
	{ 
		idx=-1; limit=120.0f; damping=2.0f; 
	} else 
	{ 
		idx=m_pHinge->idxEdge; limit=m_pHinge->maxAng; damping=m_pHinge->damping; 
	}
}

void CAttachment::SetHingeParams(int idx,float limit,float damping)
{
	if (idx==-1)
	{
		if (m_pHinge) delete m_pHinge;
		m_pHinge = 0;
	}	else
	{
		if (!m_pHinge) 
		{
			m_pHinge = new SAttachmentHinge;
			m_pHinge->wcenter.zero();
			m_pHinge->angle = 0;
			m_pHinge->velAng = 0;
		}
		m_pHinge->idxEdge = idx;
		m_pHinge->maxAng = limit;
		m_pHinge->damping = damping;
	}
}



void CAttachment::Serialize(TSerialize ser)
{
	if (ser.GetSerializationTarget() != eST_Network)
	{
		ser.BeginGroup("CAttachment");
		ser.Value("m_HideInMainPass", m_HideInMainPass);
		ser.EndGroup();
	}
}

size_t CAttachmentManager::SizeOfThis()
{
	size_t nSize = 0;
	nSize += m_arrAttachments.get_alloc_size();
	nSize += m_arrAttSkin.get_alloc_size();
	nSize += m_arrAttSkinnedStream.get_alloc_size();
	nSize += m_arrMorphTargets.get_alloc_size();
	nSize += m_morphTargets.get_alloc_size();
	nSize += m_arrAttMorphStream.get_alloc_size(); 

	uint32 numAttachments= m_arrAttachments.size();
	for(uint32 i=0; i<numAttachments; i++)
		nSize += m_arrAttachments[i]->SizeOfThis();

	nSize += sizeof(EVisibilityMask);
	return nSize;
}

size_t CAttachment::SizeOfThis()
{
	size_t nSize = sizeof(CAttachment) + sizeofVector(m_Name);
	nSize += m_arrRemapTable.get_alloc_size();
	if (m_pHinge)
		nSize += sizeof(SAttachmentHinge);
	return  nSize;
}

void CAttachmentManager::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject( m_arrAttachments );
	pSizer->AddObject( m_arrAttSkin );
	pSizer->AddObject( m_arrAttSkinnedStream );
	pSizer->AddObject( m_arrMorphTargets );
	pSizer->AddObject( m_morphTargets );
	pSizer->AddObject( m_arrAttMorphStream );	
}

void CAttachment::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject( m_Name );
	pSizer->AddObject( m_pHinge );
	pSizer->AddObject( m_arrRemapTable );	
}

namespace 
{
	struct MtlInfo
	{
		MtlInfo(IMaterial* _mtl,int _nFirstVertId,int _nNumVerts)
		{
			mtl = _mtl;
			nFirstVertId = _nFirstVertId;
			nNumVerts = _nNumVerts;
		}
		IMaterial* mtl;
		int nFirstVertId;
		int nNumVerts;
		ColorB colors;
	};

	struct MeshInfo
	{
		MeshInfo(CModelMesh* _modelMesh)
		{
			modelMesh = _modelMesh;
		}
		CModelMesh* modelMesh;
		//
		std::vector<MtlInfo> mtlInfo;
	};
};

void CAttachmentManager::BakeMaterials()
{
	/*
	float rscale = 0.8;  float rbias = 0.2;
	float gscale = 1;  float gbias = 0;
	float bscale = 0.7;  float bbias = 0.3;
	int offset = cry_rand();
	for(size_t n=0;n<colors.size();n++)
	{
	size_t i = (n+offset)%64;
	colors[n].r = (uint8)((((float)((i&(0x3<<0))>>0)/3.0f)*255.0f)*rscale+rbias);
	colors[n].g = (uint8)((((float)((i&(0x3<<2))>>2)/3.0f)*255.0f)*gscale+gbias);
	colors[n].b = (uint8)((((float)((i&(0x3<<4))>>4)/3.0f)*255.0f)*bscale+bbias);
	//
	colors[n].a = 0;
	}
	*/
	std::vector<ColorB> colors;
	colors.push_back(Col_Black);
	colors.push_back(Col_White);
	colors.push_back(Col_Aquamarine);
	colors.push_back(Col_Blue);
	colors.push_back(Col_BlueViolet);
	colors.push_back(Col_Brown);
	colors.push_back(Col_CadetBlue);
	colors.push_back(Col_Coral);
	colors.push_back(Col_CornflowerBlue);
	colors.push_back(Col_Cyan);
	colors.push_back(Col_DarkGray);
	colors.push_back(Col_DarkGreen);
	colors.push_back(Col_DarkOliveGreen);
	colors.push_back(Col_DarkOrchild);
	colors.push_back(Col_DarkSlateBlue);
	colors.push_back(Col_DarkSlateGray);
	colors.push_back(Col_DarkSlateGrey);
	colors.push_back(Col_DarkTurquoise);
	colors.push_back(Col_DarkWood);
	colors.push_back(Col_DimGray);
	colors.push_back(Col_DimGrey);
	colors.push_back(Col_FireBrick);
	colors.push_back(Col_ForestGreen);
	colors.push_back(Col_Gold);
	colors.push_back(Col_Goldenrod);
	colors.push_back(Col_Gray);
	colors.push_back(Col_Green);
	colors.push_back(Col_GreenYellow);
	colors.push_back(Col_Grey);
	colors.push_back(Col_IndianRed);
	colors.push_back(Col_Khaki);
	colors.push_back(Col_LightBlue);
	colors.push_back(Col_LightGray);
	colors.push_back(Col_LightSteelBlue);
	colors.push_back(Col_LightWood		);
	colors.push_back(Col_LimeGreen		);
	colors.push_back(Col_Magenta		);
	colors.push_back(Col_Maroon		);
	colors.push_back(Col_MedianWood		);
	colors.push_back(Col_MediumAquamarine	);
	colors.push_back(Col_MediumBlue		);
	colors.push_back(Col_MediumForestGreen	);
	colors.push_back(Col_MediumGoldenrod	);
	colors.push_back(Col_MediumOrchid	);
	colors.push_back(Col_MediumSeaGreen	);
	colors.push_back(Col_MediumSlateBlue	);
	colors.push_back(Col_MediumSpringGreen	);
	colors.push_back(Col_MediumTurquoise	);
	colors.push_back(Col_MediumVioletRed	);
	colors.push_back(Col_MidnightBlue);
	colors.push_back(Col_Navy				  );	
	colors.push_back(Col_NavyBlue			);
	colors.push_back(Col_Orange				);
	colors.push_back(Col_OrangeRed		);
	colors.push_back(Col_Orchid				);
	colors.push_back(Col_PaleGreen		);
	colors.push_back(Col_Pink					);
	colors.push_back(Col_Plum					);
	colors.push_back(Col_Red					);
	colors.push_back(Col_Salmon				);
	colors.push_back(Col_SeaGreen			);
	colors.push_back(Col_Sienna				);
	colors.push_back(Col_SkyBlue			);
	colors.push_back(Col_SlateBlue		);
	colors.push_back(Col_SpringGreen	);
	colors.push_back(Col_SteelBlue		);
	colors.push_back(Col_Tan					);
	colors.push_back(Col_Thistle			);
	colors.push_back(Col_Turquoise		);
	colors.push_back(Col_Violet				);
	colors.push_back(Col_VioletRed		);
	colors.push_back(Col_Wheat				);
	colors.push_back(Col_Yellow				);
	colors.push_back(Col_YellowGreen	);
	for(size_t n=0;n<colors.size();n++)
	{
		colors[n].a = 0;
	}
	//
	int numColors = colors.size();
	size_t colorIndex = cry_rand();

	CryLog("---------------- MergeInfo Issues ----------------------");
	/*
	struct MeshMaterialInfo
	{
	MtlInfo(CModelMesh* _modelMesh)
	{
	modelMesh = _modelMesh;
	}
	CModelMesh* modelMesh;
	//
	std::vector<MtlInfo> mtlInfo;
	};
	*/

	std::vector<IMaterial*> mtlList;
	std::vector<MeshInfo> meshInfoList;
	int numAttached = this->GetAttachmentCount();
	for (int part = 0; part < numAttached; ++part)
	{
		IAttachment* att =  this->GetInterfaceByIndex(part);
		if( att->GetType()==CA_PART )
		{
			ICharacterPartAttachment* pCharAttachment = static_cast<ICharacterPartAttachment*>(att->GetIAttachmentObject());
			if( pCharAttachment==NULL )
				continue;

			if( pCharAttachment->GetICharacterInstance()==NULL )
				continue;

			int nLOD = 0;

			CCharacterModel* pCharModel = static_cast<CCharacterModel*>(pCharAttachment->GetICharacterInstance()->GetICharacterModel());

			IMaterial* meshMtl = pCharAttachment->GetMaterial();// pCharModel->GetMaterial();

			IRenderMesh* renderMesh = pCharModel->GetRenderMesh(nLOD);

			CModelMesh* modelMesh = pCharModel->GetModelMesh(nLOD);
			MeshInfo mesh(modelMesh);

			PodArray<CRenderChunk> * chunks = renderMesh->GetChunks().Count() ? &renderMesh->GetChunks() : renderMesh->GetChunksSkinned();
			// add materials to mtl list (used render chunks)
			for(int m=0;m<chunks->Count();m++)
			{
				IMaterial* mtl = NULL;
				if( meshMtl->GetSubMtlCount()==0 )
				{
					mtl = meshMtl;
				}
				else
				{
					mtl = meshMtl->GetSubMtl(chunks->GetAt(m).m_nMatID%meshMtl->GetSubMtlCount());
				}
				mesh.mtlInfo.push_back(MtlInfo(mtl,chunks->GetAt(m).nFirstVertId,chunks->GetAt(m).nNumVerts));
				int colIndex = (colorIndex+m)%numColors;
				mesh.mtlInfo.back().colors = colors[colIndex];
				//
				mtlList.push_back(mtl);
			}
			colorIndex+=mesh.mtlInfo.size();
			//
			meshInfoList.push_back(mesh);
			//
			/*
			modelMesh->LockFullRenderMesh(nLOD);
			uint32 numExtVertices	=	pCharModel->GetRenderMesh(nLOD)->GetVertCount();
			//      m_pColors = pMesh->GetStridedColorPtr(m_ColorStride, 0);
			for(uint32 e=0; e<numExtVertices; e++) 
			{	
			ColorB* color = (ColorB*)&((struct_VERTEX_FORMAT_2xP3F_INDEX4UB*)(modelMesh->m_pMorphingInfo + e * modelMesh->m_ShapeStride))->index;
			int mi=0;
			for(;mi<meshMtlList.size();mi++)
			{
			if( e>=meshMtlList[mi].nFirstVertId && e<meshMtlList[mi].nFirstVertId+meshMtlList[mi].nNumVerts )
			{
			break;
			}
			}
			int colIndex = (colorIndex+mi)%numColors;
			*color = colors[colIndex];//ColorB(Vec3(1,0,0),0);
			meshMtlList[mi].colors = *color;
			}
			//
			modelMesh->UnlockFullRenderMesh(nLOD);
			*/
		}
	}
	//
	// try to collapse materials
	//
	std::vector<MtlInfo*> mtlInfoList;
	for(size_t mesh=0;mesh<meshInfoList.size();mesh++)
	{
		for(size_t mtl=0;mtl<meshInfoList[mesh].mtlInfo.size();mtl++)
		{
			mtlInfoList.push_back(&meshInfoList[mesh].mtlInfo[mtl]);
		}
	}
	// step 01: collapse all equal materials on different meshes
	for(size_t m=0;m<mtlInfoList.size();m++)
	{
		for(size_t m1=m+1;m1<mtlInfoList.size();m1++)
		{
			if( mtlInfoList[m1]->mtl==mtlInfoList[m]->mtl )
			{
				mtlInfoList[m1]->colors=mtlInfoList[m]->colors;

			}
		}
	} 
	// step 02: find mtls to collapse matrials which diffes only by textures
	for(size_t m=0;m<mtlInfoList.size();m++)
	{
		for(size_t m1=m+1;m1<mtlInfoList.size();m1++)
		{
			IMaterial* mtl = mtlInfoList[m]->mtl;
			IMaterial* mtl1 = mtlInfoList[m1]->mtl;
			if( gEnv->p3DEngine->CanBeMerged(mtl,mtl1,true,IMaterialMergePipe::mfDefaut) )
			{
				mtlInfoList[m1]->colors=mtlInfoList[m]->colors;
			}
		}
	} 
	//
	for(size_t n=0;n<meshInfoList.size();n++)
	{
		int nLOD = 0;
		CModelMesh* modelMesh = meshInfoList[n].modelMesh;
		modelMesh->LockFullRenderMesh(nLOD);
		int numExtVertices = (int)meshInfoList[n].modelMesh->GetVertextCount();
		for(int e=0; e<numExtVertices; e++) 
		{	
			ColorB* color = (ColorB*)&((SVF_P3F_P3F_I4B*)(modelMesh->m_pMorphingInfo + e * modelMesh->m_ShapeStride))->index;
			for(size_t mi=0;mi<meshInfoList[n].mtlInfo.size();mi++)
			{
				if( e>=meshInfoList[n].mtlInfo[mi].nFirstVertId && e<meshInfoList[n].mtlInfo[mi].nFirstVertId+meshInfoList[n].mtlInfo[mi].nNumVerts )
				{
					*color = meshInfoList[n].mtlInfo[mi].colors;
					break;
				}
			}
		}
		modelMesh->UnlockFullRenderMesh(nLOD);
	}
	// count batch
	std::set<unsigned int> uniqueColors;
	for(size_t n=0;n<mtlInfoList.size();n++)
	{
		uniqueColors.insert(mtlInfoList[n]->colors.pack_abgr8888());
	}
	CryLog("Batched Materials Statistics: Before %d, After %d",mtlInfoList.size(),uniqueColors.size());
}

SPU_NO_INLINE void CAttachmentManager::MoveConnectedFacesToNewBuffer()
{
	uint32 AttIndex=0;

	m_arrAttSkin.resize(0);
	m_arrAttSkinnedStream.resize(0);

	CSkinInstance* pSkin = m_pSkinInstance;
	CCharacterModel *pModel = pSkin->m_pModel;
	CModelMesh* pSkinning = pModel->GetModelMesh(pModel->m_nBaseLOD);

	uint32 numMergedSkins = pSkinning->m_arrExtVerticesCached.size();
	//move all connected faces into a new buffer
	uint32 numAttachments=m_arrAttachments.size();
	for (uint32 i=0; i<numAttachments; i++)
	{
		CAttachment* pAttachment	= m_arrAttachments[i];
		if (pAttachment->m_Type==CA_FACE ) 
		{
			uint16 i0 = pAttachment->m_FaceNr*3+0;
			uint16 i1 = pAttachment->m_FaceNr*3+1;
			uint16 i2 = pAttachment->m_FaceNr*3+2;
			assert(i0<numMergedSkins);
			assert(i1<numMergedSkins);
			assert(i2<numMergedSkins);

			AttSkinVertex iv0(pSkinning->m_arrExtVerticesCached[ i0 ]);
			AttSkinVertex iv1(pSkinning->m_arrExtVerticesCached[ i1 ]);
			AttSkinVertex iv2(pSkinning->m_arrExtVerticesCached[ i2 ]);

			//IRenderAuxGeom* pRenAux = gEnv->pRenderer->GetIRenderAuxGeom();
			//pRenAux->DrawTriangle( rPhysLocationNext*iv0.wpos0 ,RGBA8(0xff,0x00,0x00,0xff), rPhysLocationNext*iv1.wpos0 ,RGBA8(0xff,0x00,0x00,0xff),rPhysLocationNext*iv2.wpos0 ,RGBA8(0xff,0x00,0x00,0xff));

			//	float fColor[4] = {0,1,0,1};
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_FaceNr: %d", pAttachment->m_FaceNr );	g_YLine+=16.0f;
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"iv0 (%f %f %f)", iv0.wpos1.x,iv0.wpos1.y,iv0.wpos1.z );	g_YLine+=16.0f;
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"iv1 (%f %f %f)", iv1.wpos1.x,iv1.wpos1.y,iv1.wpos1.z );	g_YLine+=16.0f;
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"iv2 (%f %f %f)", iv2.wpos1.x,iv2.wpos1.y,iv2.wpos1.z );	g_YLine+=16.0f;
			///*
			//
			m_arrAttSkin.push_back(iv0);
			m_arrAttSkin.push_back(iv1);
			m_arrAttSkin.push_back(iv2);
			pAttachment->m_AttFaceNr=AttIndex;
			AttIndex+=3;
		}
	}
	m_arrAttSkinnedStream.resize(AttIndex);
	m_arrAttMorphStream.resize(AttIndex);

	m_pSkinInstance->m_UpdateAttachmentMesh=0;

}

SPU_NO_INLINE void CAttachmentManager::CreateMorphTargetsForAllConnectedPhases()
{
	CSkinInstance* pSkin = m_pSkinInstance;
	CCharacterModel *pModel = pSkin->m_pModel;
	CModelMesh* pSkinning = pModel->GetModelMesh(pModel->m_nBaseLOD);

	//Timur, Not used for now.
	//create morph-tragets for all connected faces
	uint32 numMorphStreams = pSkinning->m_morphTargets.size();

	m_morphTargets.resize(numMorphStreams);
	m_arrMorphTargets.resize(numMorphStreams);

	for (uint32 m=0; m<numMorphStreams; m++)
	{
		/*
		float fBlending = 0.0f;
		// calcl morph effect
		uint32 num=((CMorphing*)pSkin->GetIMorphing())->m_arrMorphEffectors.size();
		for (uint32 nMorphEffector=0; nMorphEffector<num; ++nMorphEffector)
		{
		const CryModEffMorph& rMorphEffector = ((CMorphing*)m_pSkelInstance->GetIMorphing())->m_arrMorphEffectors[nMorphEffector];
		int nMorphTargetId = rMorphEffector.getMorphTargetId ();
		if (nMorphTargetId < 0)
		continue;
		if( nMorphTargetId==m )
		{
		fBlending += rMorphEffector.getBlending();
		}
		}
		*/
		//
		m_arrMorphTargets[m].m_arrAttMorph.resize(0);
		m_morphTargets[m] = new CMorphTarget;

		uint32 numMorphVerts = pSkinning->m_morphTargets[m]->m_vertices.size();
		uint32 numAttVertices = m_arrAttSkin.size();

		for (uint32 a=0; a<numAttVertices; a++)
		{
			for (uint32 i=0; i<numMorphVerts; i++)
			{
				CMorphTarget *pCurMorphTarget = pSkinning->m_morphTargets[m];
				Vec3 &rCurMTExtension = pCurMorphTarget->m_MTExtensions;
				MorphTargetVertex& mvtx = pCurMorphTarget->m_vertices[i];

				f32 ex = rCurMTExtension.x/255.0f;
				f32 ey = rCurMTExtension.y/255.0f;
				f32 ez = rCurMTExtension.z/255.0f;

				if( mvtx.m_nVertexId==m_arrAttSkin[a].org_vertex_index )
				{
					Vec3 &rCurMTNegBasis = pCurMorphTarget->m_MTNegBasis;

					SMeshMorphTargetVertex vtx;
					vtx.nVertexId = a;
					//
					f32 x=f32(mvtx.m_MTVertexX);
					f32 y=f32(mvtx.m_MTVertexY);
					f32 z=f32(mvtx.m_MTVertexZ);
					Vec3 unpackecd;
					unpackecd.x = x*ex+rCurMTNegBasis.x;
					unpackecd.y = y*ey+rCurMTNegBasis.y;
					unpackecd.z = z*ez+rCurMTNegBasis.z;

					vtx.ptVertex = unpackecd;
					m_arrMorphTargets[m].m_arrAttMorph.push_back( vtx );
					break;
				}
				/*
				Vec3 morph = pSkinning->GetSkinVertexNoInd(mvtx.nVertexId).wpos1-mvtx.m_MTVertex*fBlending;
				Vec3 diff = m_arrAttSkin[a].wpos1-morph;
				if ( cry_fabsf(diff.x)<0.00001f && cry_fabsf(diff.y)<0.00001f && cry_fabsf(diff.z)<0.00001f )
				{
				}
				*/
			}
			/*
			if( !bFound )
			{
			int y=0;
			}
			*/
		}
	}	
}

SPU_NO_INLINE void CAttachmentManager::UpdateFaceAttachment(const QuatT &rPhysLocationNext, CAttachment *pAttachment)
{
	CSkinInstance* pSkin = m_pSkinInstance;
	CCharacterModel *pModel = pSkin->m_pModel;
	CModelMesh* pSkinning = pModel->GetModelMesh(pModel->m_nBaseLOD);

	if (pSkinning)
	{
		const Vec3& tv0 = m_arrAttSkinnedStream[ pAttachment->m_AttFaceNr+0 ];	
		const Vec3& tv1 = m_arrAttSkinnedStream[ pAttachment->m_AttFaceNr+1 ];	
		const Vec3& tv2 = m_arrAttSkinnedStream[ pAttachment->m_AttFaceNr+2 ];
		//IRenderAuxGeom* pRenAux = gEnv->pRenderer->GetIRenderAuxGeom();
		//  pRenAux->DrawTriangle( rPhysLocationNext*tv0+Vec3(0,0,0) ,RGBA8(0xff,0x00,0x00,0xff), rPhysLocationNext*tv1+Vec3(0,0,0) ,RGBA8(0xff,0x00,0x00,0xff),rPhysLocationNext*tv2+Vec3(0,0,0) ,RGBA8(0xff,0x00,0x00,0xff));
		//  pRenAux->DrawSphere( rPhysLocationNext*tv0 ,0.01,RGBA8(0xff,0x00,0x00,0xff));//, rPhysLocationNext*iv1.wpos0 ,RGBA8(0xff,0x00,0x00,0xff),rPhysLocationNext*iv2.wpos0 ,RGBA8(0xff,0x00,0x00,0xff));
		//  pRenAux->DrawSphere( rPhysLocationNext*tv1 ,0.01,RGBA8(0xff,0x00,0x00,0xff));//, rPhysLocationNext*iv1.wpos0 ,RGBA8(0xff,0x00,0x00,0xff),rPhysLocationNext*iv2.wpos0 ,RGBA8(0xff,0x00,0x00,0xff));
		//  pRenAux->DrawSphere( rPhysLocationNext*tv2 ,0.01,RGBA8(0xff,0x00,0x00,0xff));//, rPhysLocationNext*iv1.wpos0 ,RGBA8(0xff,0x00,0x00,0xff),rPhysLocationNext*iv2.wpos0 ,RGBA8(0xff,0x00,0x00,0xff));
		//float fColor[4] = {0,1,0,1};
		//uint32 numAttSkinnedStream = m_arrAttSkinnedStream.size();
		//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"numAttSkinnedStream: %d", numAttSkinnedStream );	g_YLine+=16.0f;
		//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"pAttachment->m_AttFaceNr: %d", pAttachment->m_AttFaceNr );	g_YLine+=16.0f;
		//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"tv0 (%f %f %f)", tv0.x,tv0.y,tv0.z );	g_YLine+=16.0f;
		//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"tv1 (%f %f %f)", tv1.x,tv1.y,tv1.z );	g_YLine+=16.0f;
		//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"tv2 (%f %f %f)", tv2.x,tv2.y,tv2.z );	g_YLine+=16.0f;

		//		g_pAuxGeom->DrawLine( tv0, RGBA8(0x00,0x00,0x00,0xff), tv1, RGBA8(0xff,0xff,0xff,0xff));
		//		g_pAuxGeom->DrawLine( tv1, RGBA8(0x00,0x00,0x00,0xff), tv2, RGBA8(0xff,0xff,0xff,0xff));
		//		g_pAuxGeom->DrawLine( tv2, RGBA8(0x00,0x00,0x00,0xff), tv0, RGBA8(0xff,0xff,0xff,0xff));

		Vec3 middle=(tv0+tv1+tv2)/3.0f;	
		Vec3 x = (tv1-tv0).GetNormalized();
		Vec3 z = ((tv1-tv0)%(tv0-tv2)).GetNormalized();
		Vec3 y = z%x;
		QuatT TriMat=QuatT::CreateFromVectors(x,y,z,middle);
		//FIXME: this creates a lot of assertion because TriMat was constructed out of a too small triangle
#ifdef PS3
		if(!TriMat.IsValid())TriMat.SetIdentity();
#endif
		pAttachment->m_AttModelRelative = TriMat * pAttachment->m_AttRelativeDefault * pAttachment->m_additionalRotation;         
		pAttachment->m_AttWorldAbsolute = rPhysLocationNext * pAttachment->m_AttModelRelative;
	}

}

SPU_NO_INLINE void CAttachmentManager::UpdateBoneAttachment(const QuatT &rPhysLocationNext, CAttachment *pAttachment)
{		
	CCharInstance* pMaster = m_pSkelInstance;

	if (pAttachment->m_BoneID<4096)
	{
		//for normal bone-attachments we use this code
		QuatT AbsBoneMat	= pMaster->m_SkeletonPose.GetPoseData().m_jointsAbsolute[pAttachment->m_BoneID];   
		QuatT rel = pAttachment->m_AttRelativeDefault;
		rel.t*=m_pSkelInstance->GetUniformScale();
		
		QuatT attModelRelative = AbsBoneMat* rel * pAttachment->m_additionalRotation;         
		pAttachment->m_AttModelRelative = attModelRelative;
		pAttachment->m_AttWorldAbsolute = rPhysLocationNext * attModelRelative;

#if !defined(__SPU__) && !defined(RELEASE)
		//Eyes-attachments are something very special
		if (Console::GetInst().ca_DrawLookIK)
		{
			uint32 lei=pMaster->m_pModel->m_ModelSkeleton.GetIdx(eIM_LeftEyeIdx);
			uint32 rei=pMaster->m_pModel->m_ModelSkeleton.GetIdx(eIM_RightEyeIdx);
			if (lei==pAttachment->m_BoneID || rei==pAttachment->m_BoneID)
			{
				Vec3 EyePos = pAttachment->m_AttWorldAbsolute.t;
				Vec3 EyeVec = pAttachment->m_AttWorldAbsolute.q.GetColumn1()*0.1f;
				g_pAuxGeom->DrawLine(EyePos, RGBA8(0x00,0x00,0x00,0xff), EyePos+EyeVec, RGBA8(0xff,0xff,0xff,0xff));
#ifdef _DEBUG
				Quat RelBoneMat	= pMaster->m_SkeletonPose.GetPoseData().m_jointsRelative[pAttachment->m_BoneID].q;
				assert(RelBoneMat.IsIdentity()); //if not, then something went wrong
#endif
				//AbsBoneMat	= AbsBoneMat * !RelBoneMat;
				//pAttachment->m_AttRelativeDefault.q=RelBoneMat;          
				//pAttachment->m_AttModelRelative = AbsBoneMat*pAttachment->m_AttRelativeDefault * pAttachment->m_additionalRotation;
				//pAttachment->m_AttWorldAbsolute = rPhysLocationNext * pAttachment->m_AttModelRelative;
				//	float fColor[4] = {0,1,0,1};
				//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"RelBoneMat: %f (%f %f %f)",RelBoneMat.w,RelBoneMat.v.x,RelBoneMat.v.y,RelBoneMat.v.z );	g_YLine+=16.0f;
				//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"AbsBoneMat: %f (%f %f %f)",AbsBoneMat.q.w,AbsBoneMat.q.v.x,AbsBoneMat.q.v.y,AbsBoneMat.q.v.z );	g_YLine+=16.0f;
			}
		}
#endif
	}
}

SPU_NO_INLINE void CAttachmentManager::UpdateDanglyAttachment( CAttachment *pAttachment)
{	
	if (pAttachment->m_pHinge && !m_pSkelInstance->GetPhysicsRelinquished())
	{	// dangly attachments 
		// store center point in world coords, hinge angle speed
		// each step:
		//   calculate prev center point pos in the new coord frame, compute the new angle
		//   add gravity to angular vel, angular vel to angle
		//   if the angle is out of range, snap it and 0 the velocity
		//   (the gravity is queried from the physical entity and projected into local space
		//   re-calculate the matrix basing on the angle
		AABB bbox=AABB(Vec3(ZERO),Vec3(ZERO));
		Vec3 size,sz,ptHinge,ptloc,axisz,axisx,rotax,sg,gravity(0,0,-9.81f);
		Vec3i ic;
		Vec2 ptc,gravity2d;
		float angle,velBody,gravityAng,dt=gEnv->pTimer->GetFrameTime(); 
		SAttachmentHinge &hinge = *pAttachment->m_pHinge;

		IAttachmentObject* pIAttachmentObject = pAttachment->GetIAttachmentObject();
		if (pIAttachmentObject)
			bbox = pIAttachmentObject->GetAABB();

		ic.z = hinge.idxEdge & 3;
		ic.x = ic.z+1+(hinge.idxEdge>>3&1); ic.x -= 3 & (2-ic.x)>>31;
		ic.y = 3-ic.x-ic.z;
		//		bFlipped = 1^hinge.idxEdge>>3&1^hinge.idxEdge>>2&1^hinge.idxEdge>>4&1;
		sz=size = bbox.GetSize()*0.5f; sz[ic.y] = 0;
		sz[ic.x] *= (sg[ic.x]=(hinge.idxEdge>>3&2)-1.0f);
		sz[ic.z] *= (sg[ic.z]=(hinge.idxEdge>>1&2)-1.0f);
		ptHinge = bbox.GetCenter()+sz;
		axisz.zero()[ic.z] = -sg[ic.z];
		axisx.zero()[ic.x] = -sg[ic.x];
		rotax = axisx%axisz;
		//	gravity = pAttachment->m_AttAbsolute.GetInvertedFast().TransformVector(gravity);
		gravity = !pAttachment->m_AttWorldAbsolute.q * gravity;

		ptloc = pAttachment->m_AttWorldAbsolute.GetInverted()*hinge.wcenter;
		ptc.set((ptloc[ic.x]-ptHinge[ic.x])*-sg[ic.x], (ptloc[ic.z]-ptHinge[ic.z])*-sg[ic.z]);
		gravity2d.set(gravity[ic.x]*-sg[ic.x], gravity[ic.z]*-sg[ic.z]);

		f32 fLength = ptc.GetLength2();
		if (fLength==0.0f) 
			fLength=0.00001f;

		gravityAng = (ptc^gravity2d)/fLength;
		angle = atan2f(ptc.y, ptc.x);
		velBody = iszero(dt) ? 0.0f : (hinge.angle-angle)/dt;
		angle += hinge.velAng*dt; 
		hinge.velAng = (hinge.velAng+gravityAng*dt)*max(0.0f,1-hinge.damping*dt);
		if (angle<0)
			angle=0, hinge.velAng=max(velBody,hinge.velAng);
		else if (angle>hinge.maxAng*(gf_PI/180.0f))
			angle=hinge.maxAng*(gf_PI/180.0f), hinge.velAng=min(velBody,hinge.velAng);
		hinge.angle = angle;

		Quat R = Quat::CreateRotationAA(angle,rotax);
		QuatT Rt(R,ptHinge-R*ptHinge);      
		pAttachment->m_AttModelRelative = pAttachment->m_AttModelRelative*Rt; 
		pAttachment->m_AttWorldAbsolute = pAttachment->m_AttWorldAbsolute*Rt;
		sz.zero()[ic.z] = size[ic.z]*sg[ic.z];
		hinge.wcenter = pAttachment->m_AttWorldAbsolute*(bbox.GetCenter()+sz);
	}
}


#include UNIQUE_VIRTUAL_WRAPPER(IAttachmentManager)
#include UNIQUE_VIRTUAL_WRAPPER(IAttachment)
