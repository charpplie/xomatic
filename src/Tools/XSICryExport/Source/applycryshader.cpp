#include "stdafx.h"
#include "plugin_stub.h"

#include "Crosswalk/helper.h"

_XSI_EXTERN_ CStatus ApplyCryShaderToMaterials_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	LONG l_lApply = 0;
	CRefArray l_MaterialLibraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
	for (LONG i = 0; i < l_MaterialLibraries.GetCount(); i++)
	{
		MaterialLibrary l_MatLib(l_MaterialLibraries[i]);
		CRefArray l_Materials = l_MatLib.GetItems();
		for (LONG j=0; j < l_Materials.GetCount(); j++)
		{
			Material l_Material(l_Materials[j]);

			Selection l_Selection( app.GetSelection() );
			LONG l_lCount = l_Selection.GetCount();
			for (LONG l = 0; l < l_lCount; l++ )
			{
				SIObject l_Obj(l_Selection[l]);
// 				app.LogMessage(l_Obj.GetFullName());
// 				app.LogMessage(l_Material.GetFullName());
// 				app.LogMessage(l_Obj.GetType());
				if (l_Obj.GetType() != L"material")
					continue;
				Material l_ObjMat = (Material)l_Obj;
				MaterialLibrary l_ObjMatLib;
				GetMaterialLibraryByMaterial( l_ObjMat, l_ObjMatLib );
				//				if (l_Obj.GetFullName() == l_Material.GetFullName())
				if (l_ObjMat.GetName() == l_Material.GetName() && l_ObjMatLib.GetName() == l_MatLib.GetName())
				{
//					app.LogMessage( l_Obj.GetFullName() );

					Parameter l_parameter = l_Material.GetParameter(L"surface");
					// get the shader itself
					Shader l_PhongShader = l_parameter.GetSource();

					bool l_bValid;
					l_parameter = l_PhongShader.GetParameter(L"diffuse");

					l_bValid = l_parameter.IsValid();

					CRefArray			l_SubParameterCollection;
					l_SubParameterCollection = l_parameter.GetParameters();

					Parameter l_DiffuseR = l_SubParameterCollection.GetItem(0);
					Parameter l_DiffuseG = l_SubParameterCollection.GetItem(1);
					Parameter l_DiffuseB = l_SubParameterCollection.GetItem(2);
					Parameter l_DiffuseA = l_SubParameterCollection.GetItem(3);

					float l_fDiffuseR = l_DiffuseR.GetValue();
					float l_fDiffuseG = l_DiffuseG.GetValue();
					float l_fDiffuseB = l_DiffuseB.GetValue();
					float l_fDiffuseA = l_DiffuseA.GetValue();

					l_parameter = l_PhongShader.GetParameter(L"transparency");
					l_bValid = l_parameter.IsValid();
					l_SubParameterCollection = l_parameter.GetParameters();

					Parameter l_TransR = l_SubParameterCollection.GetItem(0);
					Parameter l_TransG = l_SubParameterCollection.GetItem(1);
					Parameter l_TransB = l_SubParameterCollection.GetItem(2);
					Parameter l_TransA = l_SubParameterCollection.GetItem(3);

					float l_fTransR = l_TransR.GetValue();
					float l_fTransG = l_TransG.GetValue();
					float l_fTransB = l_TransB.GetValue();
					float l_fTransA = l_TransA.GetValue();

					l_parameter = l_PhongShader.GetParameter(L"refract_inuse");
					l_bValid = l_parameter.IsValid();
					bool l_bTransparency = l_parameter.GetValue();

					CRefArray &l_Shaders = l_Material.GetShaders();
					for (LONG k = 0; k < l_Shaders.GetCount(); k++)
					{
						Shader l_Shader(l_Shaders[k]);

						CRefArray l_ImageClips = l_Shader.GetImageClips();
						CString l_Image;
						ImageClip l_ImageClip;
						for (int kk = 0; kk < l_ImageClips.GetCount(); kk++)
						{
							l_ImageClip = l_ImageClips[kk];
							l_Image = l_ImageClip.GetName();
						}

						CString l_ProgID = l_Shader.GetProgID();
						char* l_strProgID = (char*)l_ProgID.GetAsciiString();
						if (!strstr(l_strProgID, "CryShader.1"))
						{
							MaterialLibrary l_ActMatLib;
							GetMaterialLibraryByMaterial(l_Material, l_ActMatLib);
							LONG l_MaterialID = GetUnusedMaterialIDByMaterialLibrary(l_ActMatLib);

							CString l_MatName = l_Material.GetName();
							char l_strMatName[MAX_PATH];
							strcpy(l_strMatName, l_MatName.GetAsciiString());
							Material l_NewMaterial = l_MatLib.CreateMaterial(L"CryShader", L"CryShader");

							Parameter l_CryParameter = l_NewMaterial.GetParameter(L"surface");
							Shader l_CryShader = l_CryParameter.GetSource();
							l_CryShader.PutParameterValue( L"MaterialID", l_MaterialID );

							CRefArray l_UsedObjects = l_Material.GetUsedBy();
							CString l_Args = l_NewMaterial.GetFullName();
							for (LONG m = 0; m < l_UsedObjects.GetCount(); m++)
							{
								SceneItem l_UsedObj = l_UsedObjects[m];
//								app.LogMessage(l_UsedObj.GetFullName());
								l_Args += L"," + l_UsedObj.GetFullName();
							}

//							app.LogMessage( l_Args );
							if (l_UsedObjects.GetCount())
								XSI::COMMANDS::AssignMaterial(l_Args);
							XSI::COMMANDS::DeleteObj(l_Material.GetFullName());

							if (strlen(l_strMatName) > 4)
							{
								char l_strID[3];
								l_strID[0] = l_strMatName[1];
								l_strID[1] = l_strMatName[2];
								l_strID[2] = 0;
								LONG l_ID = atoi(l_strID);
								if (l_strMatName[0] == '_' && l_strMatName[3] == '_' && l_ID > 0 && l_ID < 33)
									l_MatName.PutAsciiString(&l_strMatName[4]);
							}

							CString l_MatIDName = L"_";
							if (l_MaterialID+1 < 10)
								l_MatIDName += L"0";
							l_MatIDName += CValue(l_MaterialID+1);
							l_MatIDName += L"_";
							l_MatName = l_MatIDName + l_MatName;
							l_NewMaterial.PutName(l_MatName);

							l_CryParameter = l_CryShader.GetParameter(L"diffuse");

							l_bValid = l_CryParameter.IsValid();

							l_SubParameterCollection = l_CryParameter.GetParameters();

							Parameter l_CryDiffuseR(l_SubParameterCollection.GetItem(0));
							Parameter l_CryDiffuseG(l_SubParameterCollection.GetItem(1));
							Parameter l_CryDiffuseB(l_SubParameterCollection.GetItem(2));
							Parameter l_CryDiffuseA(l_SubParameterCollection.GetItem(3));

							l_CryDiffuseR.PutValue(l_fDiffuseR);
							l_CryDiffuseG.PutValue(l_fDiffuseG);
							l_CryDiffuseB.PutValue(l_fDiffuseB);
							l_CryDiffuseA.PutValue(l_fDiffuseA);

							l_CryParameter = l_CryShader.GetParameter(L"transparency");
							l_bValid = l_parameter.IsValid();
							l_SubParameterCollection = l_CryParameter.GetParameters();

							Parameter l_CryTransR(l_SubParameterCollection.GetItem(0));
							Parameter l_CryTransG(l_SubParameterCollection.GetItem(1));
							Parameter l_CryTransB(l_SubParameterCollection.GetItem(2));
							Parameter l_CryTransA(l_SubParameterCollection.GetItem(3));

							l_CryTransR.PutValue(l_fTransR);
							l_CryTransG.PutValue(l_fTransG);
							l_CryTransB.PutValue(l_fTransB);
							l_CryTransA.PutValue(l_fTransA);

							l_CryParameter = l_CryShader.GetParameter(L"refract_inuse");
							l_bValid = l_parameter.IsValid();
							l_CryParameter.PutValue(l_bTransparency);

							// if there is an image, create an image clip
							if (l_ImageClip.IsValid())
							{
								l_parameter = l_CryShader.GetParameter(L"diffuse");

								// set default diffuse color
								l_CryDiffuseR.PutValue(0.7f);
								l_CryDiffuseG.PutValue(0.7f);
								l_CryDiffuseB.PutValue(0.7f);
								l_CryDiffuseA.PutValue(0.7f);

								// we connect the existing shader to the port
								CRef l_PreviousObject;

								// add the image clip	
								Shader l_ImageClipShader;
								l_parameter.ConnectFromProgID(L"Softimage.txt2d-image-explicit.1", l_PreviousObject, l_ImageClipShader);
								l_ImageClipShader.PutName(L"Image");

								l_parameter = l_ImageClipShader.GetParameter(L"tex");

								// This takes the existing shader and attaches it to the material or the previous shader
								l_parameter.Connect(l_ImageClip, l_PreviousObject);
							}
							l_lApply++;
							break;
						}
					}
				}
			}
		}
	}

	CString l_Text = L"CryShader applied to ";
	l_Text += CValue(l_lApply);
	l_Text += L" materials.";
	app.LogMessage(l_Text);

	return status;
}
