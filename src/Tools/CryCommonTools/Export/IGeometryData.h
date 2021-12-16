#ifndef __IGEOMETRYDATA_H__
#define __IGEOMETRYDATA_H__

class IGeometryData
{
public:
	virtual int AddPosition(float x, float y, float z) = 0;
	virtual int AddNormal(float x, float y, float z) = 0;
	virtual int AddTextureCoordinate(float u, float v) = 0;
	virtual int AddVertexColour(float r, float g, float b, float a) = 0;
	virtual int AddPolygon(const int* indices, int mtlID) = 0;

	virtual int GetNumberOfPositions() const = 0; 
	virtual int GetNumberOfNormals() const = 0;
	virtual int GetNumberOfTextureCoordinates() const = 0;
	virtual int GetNumberOfVertexColours() const = 0;
	virtual int GetNumberOfPolygons() const = 0;
};

#endif //__IGEOMETRYDATA_H__
