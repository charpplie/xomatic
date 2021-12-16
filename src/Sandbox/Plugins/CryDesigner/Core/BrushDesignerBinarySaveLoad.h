#pragma once

class CBrushDesignerBinarySaveLoad
{
public:

	void Save();
	void Load();

	void SaveMeshes();
	bool LoadMeshes();

	void DeleteDesignerBinaryFiles();

	void UpdateAllDesignerObjects( bool bForce );

};