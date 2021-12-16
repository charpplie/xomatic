

#include "stdafx.h"
#include "FaceAnim.h"

#include "FaceAnimDoc.h"
#include "FaceAnimView.h"
#include "CalibrationAVI4.h"

//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::LoadOBJ(const char *szFilename,tFaceDetect *pFace)
{
	FILE *fp=fopen(szFilename,"rt");

	if (!fp)
		return (false);

	char szText[256]; 
	char szBuff[256];
	char szObjectName[256];
	szObjectName[0]=0;
	bool bFace=false;
	vector3f	vCenter;
	
	pFace->lst_vModel_3D=NULL;
	pFace->lst_vModel_2D=NULL;
	pFace->nNumVertices=0;

	pFace->nNumTris=0;
	pFace->lst_Tris=NULL;

	std::vector<vector3f> lstVerts;
	std::vector<int> lstTris;

	pFace->vMins.SetMax();
	pFace->vMaxs.SetMin();
	pFace->vRef_3DCenter.Clear();

	int nFeatures=0;

	while (1)
	{
		char *szRes=fgets(szText,256,fp);
		if (!szRes)
			break;
		if (szText[0]=='g')
		{
			// get next line and see			
			szRes=fgets(szBuff,256,fp);
			if (!szRes)
				break;

			if (szBuff[0]=='#')
			{
				// g
				// # object Face to come ...
				// get object's name
				sscanf(szBuff,"# object %s to come",szObjectName);				
				lstVerts.clear();
				vCenter.Clear();

				continue;
			}
			else
			{
				// g ObjectName
				sscanf(szText,"g %s\n",szBuff);
				if (strcmp(szBuff,szObjectName)!=0)
					MyError("Error during OBJ loading");
				if (stricmp(szObjectName,"Face")==0)
					bFace=true;
				else
					bFace=false;

				if (bFace)
				{
					pFace->nNumVertices=(int)(lstVerts.size());
					pFace->lst_vModel_3D=new vector3f [pFace->nNumVertices];
					pFace->lst_vModel_2D=new vector3f [pFace->nNumVertices];
					pFace->v3DMins.SetMax();pFace->v3DMaxs.SetMin();
					for (int k=0;k<pFace->nNumVertices;k++)
					{
						pFace->lst_vModel_3D[k]=lstVerts[k];
						pFace->v3DMins.CheckMin(pFace->lst_vModel_3D[k]);
						pFace->v3DMaxs.CheckMax(pFace->lst_vModel_3D[k]);
					} //k					
				}
				else
				{					
					int k;
					for (k=0;k<FA_NUM_MARKERS;k++)
					{
						if (stricmp(pFace->lst_ref_Markers[k].szName,szObjectName)==0)
						{						
							pFace->lst_ref_Markers[k].v3DPos=vCenter/(float)(lstVerts.size());

							// store the original
							//pFace->lst_3d_points=pFace->pFace->lst_ref_Markers[k].v3DPos;
							break;
						}
					} //k
					if (k==FA_NUM_MARKERS)
						MyError("Error loading OBJ");

					pFace->vRef_3DCenter+=pFace->lst_ref_Markers[k].v3DPos;

					pFace->vMins.CheckMin(pFace->lst_ref_Markers[k].v3DPos);
					pFace->vMaxs.CheckMax(pFace->lst_ref_Markers[k].v3DPos);

					nFeatures++;
				}

				continue;
			}
		} //g

		if (szText[0]=='v' && szText[1]==' ')
		{			
			if (szObjectName[0]==0)
				MyError("Error during OBJ loading");

			float x,y,z;
			sscanf(szText,"v %f %f %f",&x,&y,&z);
			
			//vector3f vPos(-x,-y,z); // faceaa 1
			
			// note: the FOV is wrong, must be specified directly in degrees, however this 
			// gives better results, probably because the face is very close when recording the videos 
			// - see also strcutruefrommotion.cpp
			
			vector3f vPos(-x,-z,-y); // faceaa 2
			//vector3f vPos(x,z,y); // faceaa 2

			vCenter+=vPos; // 3d center for marker position
			lstVerts.push_back(vPos);

		} //v

		if (szText[0]=='f' && szText[1]==' ')
		{			
			if (!bFace)
				continue;
			int i1,i2,i3,iDummy;
			sscanf(szText,"f %d/%d/%d %d/%d/%d %d/%d/%d",&i1,&iDummy,&iDummy,&i2,&iDummy,&iDummy,&i3,&iDummy,&iDummy);
			lstTris.push_back(i1);
			lstTris.push_back(i2);
			lstTris.push_back(i3);
		}
	}

	assert(nFeatures==FA_NUM_MARKERS);
	pFace->vRef_3DCenter/=(ftype)FA_NUM_MARKERS;

	pFace->nNumTris=(int)(lstTris.size())/3;
	pFace->lst_Tris=new int [pFace->nNumTris*3];
	for (int k=0;k<pFace->nNumTris*3;k++)
	{
		pFace->lst_Tris[k]=lstTris[k];
	}

	fclose(fp);

	return (true);
}