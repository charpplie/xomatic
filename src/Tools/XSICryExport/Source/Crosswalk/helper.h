#ifndef _HELPER_H
#define _HELPER_H

extern void GetMaterialByName(CString& in_MaterialName, Material &out_Material);
extern void GetMaterialLibraryByMaterial(Material &in_Material, MaterialLibrary &out_MaterialLibrary);
extern LONG GetUnusedMaterialIDByMaterialLibrary(MaterialLibrary &in_MaterialLibrary);
extern bool IsFreeMaterialIDByMaterialLibrary(Material &in_Material, LONG in_MaterialID, MaterialLibrary &in_MaterialLibrary);
extern void UnhideAllClusters();
extern void HideClusters();

// Sokov: commented out because it's unused. Should we delete it?
//extern void ConvertObjectProperties(CString &in_Properties, std::vector<std::wstring> &out_Properties, CStringArray &in_NodeNames, CString &in_ModelName);

extern CRefArray GetAllCryExportNodes();
extern CString GetCryExportNodeListName(X3DObject in_Object, CString in_TypeString = CString());
extern short GetCryExportNodeFiletype(X3DObject in_Object);
extern X3DObject GetCryExportNodeByName(CString in_Name);

#endif
