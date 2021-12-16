#include "StdAfx.h"
#include "GeometryData.h"

GeometryData::GeometryData()
{
}

int GeometryData::AddPosition(float x, float y, float z)
{
	int positionIndex = int(this->positions.size());
	this->positions.push_back(Vector(x, y, z));
	return positionIndex;
}

int GeometryData::AddNormal(float x, float y, float z)
{
	int normalIndex = int(this->normals.size());
	this->normals.push_back(Vector(x, y, z));
	return normalIndex;
}

int GeometryData::AddTextureCoordinate(float u, float v)
{
	int textureCoordinateIndex = int(this->textureCoordinates.size());
	this->textureCoordinates.push_back(TextureCoordinate(u, v));
	return textureCoordinateIndex;
}

int GeometryData::AddVertexColour(float r, float g, float b, float a)
{
	int vertexColourIndex = int(this->vertexColours.size());
	this->vertexColours.push_back(VertexColour(r, g, b, a));
	return vertexColourIndex;
}

int GeometryData::AddPolygon(const int* indices, int mtlID)
{
	int polygonIndex = int(this->polygons.size());
	this->polygons.push_back(Polygon(mtlID,
		Polygon::Vertex(indices[0], indices[1], indices[2], indices[3]),
		Polygon::Vertex(indices[4], indices[5], indices[6], indices[7]),
		Polygon::Vertex(indices[8], indices[9], indices[10], indices[11])));
	return polygonIndex;
}

int GeometryData::GetNumberOfPositions() const
{
	return (int)this->positions.size();
}

int GeometryData::GetNumberOfNormals() const
{
	return (int)this->normals.size();
}

int GeometryData::GetNumberOfTextureCoordinates() const
{
	return (int)this->textureCoordinates.size();
}

int GeometryData::GetNumberOfVertexColours() const
{
	return (int)this->vertexColours.size();
}

int GeometryData::GetNumberOfPolygons() const
{
	return (int)this->polygons.size();
}
