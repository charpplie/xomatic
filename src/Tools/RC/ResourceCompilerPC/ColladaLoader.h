////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   ColladaLoader.h
//  Version:     v1.00
//  Created:     3/4/2006 by Michael Smith
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __COLLADALOADER_H__
#define __COLLADALOADER_H__

#include "CGFContent.h"
#include "ICryXML.h"
#include "IXml.h"
#include "Export/HelperData.h"

#include <cctype>

class ColladaNode;

class IColladaLoaderListener
{
public:
	enum MessageType
	{
		MESSAGE_INFO,
		MESSAGE_WARNING,
		MESSAGE_ERROR
	};
	virtual void OnColladaLoaderMessage(MessageType type, const char* szMessage) = 0;
};

struct sMaterialLibrary;
struct ExportFile;
//class ColladaAnimClip;
class ColladaScene;
struct CInternalSkinningInfo;

class ColladaLoader
{
public:
	bool Load(std::vector<ExportFile>& exportFiles, std::vector<sMaterialLibrary>& materialLibraryList, const char* szFileName, ICryXML* pCryXML, IPakSystem* pPakSystem, IColladaLoaderListener* pListener);
};

enum ColladaArrayType
{
	TYPE_FLOAT,
	TYPE_MATRIX,
	TYPE_INT,
	TYPE_NAME,
	TYPE_BOOL,
	TYPE_ID
};

enum ColladaNodeType
{
	NODE_NULL,
	NODE_CAMERA,
	NODE_CONTROLLER,
	NODE_GEOMETRY,
	NODE_LIGHT,
	NODE_NODE,
};

enum ColladaInterpType
{
	INTERP_CONST,
	INTERP_LINEAR,
	INTERP_BEZIER
};

enum ColladaTransformType
{
	TRANS_TRANSLATION,
	TRANS_ROTATION_X,
	TRANS_ROTATION_Y,
	TRANS_ROTATION_Z,
	TRANS_SCALE,
	TRANS_NEAR,
};

enum ExportType
{
	EXPORT_CGF,
	EXPORT_CGA,
	EXPORT_CHR,
	EXPORT_ANM,
	EXPORT_CAF,
	EXPORT_CGA_ANM,
	EXPORT_CHR_CAF,
	EXPORT_UNKNOWN,
};

enum ColladaAuthorTool
{
	TOOL_UNKNOWN,
	TOOL_SOFTIMAGE_XSI,
	TOOL_AUTODESK_MAX,
	TOOL_MOTIONBUILDER,
	TOOL_MAYA
};

struct ExportFile
{
	ColladaAuthorTool authorTool;
	ExportType type;
	string name;
	CContentCGF* pCGF;
	CInternalSkinningInfo* pCtrlSkinningInfo;
};

struct SMeshFace32
{
	int v[3];
	int t[3];
	unsigned char nSubset;
	unsigned char dwFlags;
};

class CMesh32
{
public:
	std::vector<SMeshFace32> m_pFaces;
	std::vector<Vec3> m_pPositions;
	std::vector<Vec3> m_pNorms;
	std::vector<SMeshTexCoord> m_pTexCoord;
	std::vector<SMeshColor> m_pColor0;
	std::vector<SMeshColor> m_pColor1;

	AABB m_bbox;
	std::vector<SMeshSubset> m_subsets;

	void Clear()
	{
		m_pFaces.clear();
		m_pPositions.clear();
		m_pNorms.clear();
		m_pColor0.clear();
		m_pColor1.clear();
		m_pTexCoord.clear();
		m_bbox.Reset();
		m_subsets.clear();
	}

	int GetFacesCount() const { return m_pFaces.size(); }
	int GetVertexCount() const { return m_pPositions.size(); }
	int GetTexCoordsCount() const { return m_pTexCoord.size(); }
	int GetColor0Count() const { return m_pColor0.size(); }
	int GetColor1Count() const { return m_pColor1.size(); }

	void SetFacesCount(int nNewCount)
	{
		m_pFaces.resize(nNewCount);
	}

	void SetVertexCount(int nNewCount)
	{
		m_pPositions.resize(nNewCount);
		m_pNorms.resize(nNewCount);
	}

	void SetColor0Count(int nNewCount)
	{
		m_pColor0.resize(nNewCount);
	}

	void SetColor1Count(int nNewCount)
	{
		m_pColor1.resize(nNewCount);
	}

	void SetTexCoordsCount(int nNewCount)
	{
		m_pTexCoord.resize(nNewCount);
	}
};

#define CRYEXPORTNODE_LOWERCASE "cryexportnode_"
#define CRYEXPORTNODE_LENGTH 14
#define CRYPHYSBONESUFFIX "phys"
#define CRYPHYSBONESUFFIX_LENGTH 4
#define CRYPHYSPARENTFRAMESUFFIX "phys parentframe"
#define CRYPHYSPARENTFRAMESUFFIX_LENGTH 16
#define ROOT_JOINT "Bip01"

#define LINE_LOG "#%d"

void ReportWarning(IColladaLoaderListener* pListener, const char* szFormat, ...);
void ReportInfo(IColladaLoaderListener* pListener, const char* szFormat, ...);
void ReportError(IColladaLoaderListener* pListener, const char* szFormat, ...);

// float reader (same as std::strtod() but faster)
float read_float(const char*& buf, char** endptr);

template <typename T> class ResourceMap : public std::map<string, T*>
{
public:
	~ResourceMap()
	{
		for (iterator itEntry = this->begin(); itEntry != this->end(); ++itEntry)
			delete (*itEntry).second;
	}
};

template <typename T> class ResourceVector : public std::vector<T*>
{
public:
	~ResourceVector()
	{
		for (iterator itEntry = this->begin(); itEntry != this->end(); ++itEntry)
			delete (*itEntry);
	}
};

#pragma region Array

class IColladaArray
{
public:
	virtual ~IColladaArray() {}

	virtual bool ReadAsFloat(std::vector<float>& values, int stride, int offset, IColladaLoaderListener* pListener) = 0;
	virtual bool ReadAsInt(std::vector<int>& values, int stride, int offset, IColladaLoaderListener* pListener) = 0;
	virtual bool ReadAsName(std::vector<string>& values, int stride, int offset, IColladaLoaderListener* pListener) = 0;
	virtual bool ReadAsBool(std::vector<bool>& values, int stride, int offset, IColladaLoaderListener* pListener) = 0;
	virtual bool ReadAsID(std::vector<string>& values, int stride, int offset, IColladaLoaderListener* pListener) = 0;

	virtual void Reserve(int size) = 0;
	virtual bool Parse(const char* buf, IColladaLoaderListener* pListener) = 0;
	virtual int GetSize() = 0;
};

template <ColladaArrayType TypeCode> class ArrayType {};

template <> class ArrayType<TYPE_FLOAT>
{
public: 
	typedef float Type;
	static const char* GetName() {return "FLOAT";}
	static float Parse(const char*& buf, bool& success)
	{
		char* end;
		float value = read_float(buf, &end);
		if (end == buf)
			success = false;
		else
			buf = end;
		return value;
	}
};

template <> class ArrayType<TYPE_INT>
{
public: 
	typedef int Type; 
	static const char* GetName() {return "INT";}
	static int Parse(const char*& buf, bool& success)
	{
		char* end;
		int value = int(std::strtol(buf, &end, 0));
		if (end == buf)
			success = false;
		else
			buf = end;
		return value;
	}
};

template <> class ArrayType<TYPE_NAME>
{
public:
	typedef string Type;
	static const char* GetName() {return "NAME";}
	static string Parse(const char*& buf, bool& success)
	{
		const char* p = buf;
		while (std::isspace(*p))
		{
			++p;
			if (*p == 0)
			{
				success = false;
				return 0;
			}
		}

		string s;
		while (!std::isspace(*p) && *p != 0)
			s += *p++;

		buf = p;
		return s;
	}
};

template <> class ArrayType<TYPE_BOOL>
{
public: 
	typedef bool Type;
	static const char* GetName() {return "BOOL";}
	static bool Parse(const char*& buf, bool& success)
	{
		const char* p = buf;
		while (std::isspace(*p))
		{
			++p;
			if (*p == 0)
			{
				success = false;
				return 0;
			}
		}

		string s;
		while (!std::isspace(*p) && *p != 0)
			s += tolower(*p++);

		buf = p;
		return s != "false";
	}
};

template <> class ArrayType<TYPE_ID>
{
public:
	typedef string Type;
	static const char* GetName() {return "ID";}
	static string Parse(const char*& buf, bool& success)
	{
		const char* p = buf;
		while (std::isspace(*p))
		{
			++p;
			if (*p == 0)
			{
				success = false;
				return 0;
			}
		}

		string s;
		while (!std::isspace(*p) && *p != 0)
			s += *p++;

		buf = p;
		return s;
	}
};

template <ColladaArrayType TypeCode1, ColladaArrayType TypeCode2> class Converter
{
public:
	typedef typename ArrayType<TypeCode1>::Type DataType1;
	typedef typename ArrayType<TypeCode2>::Type DataType2;

	static void Convert(DataType1& out, DataType2 in) {assert(0);}
	static bool CheckConversion(IColladaLoaderListener* pListener)
	{
		ReportError(pListener, "Cannot convert values of type %s to type %s.", ArrayType<TypeCode2>::GetName(), ArrayType<TypeCode1>::GetName());
		return false;
	}
};

template <ColladaArrayType TypeCode1, ColladaArrayType TypeCode2> class ValidConverter
{
public:
	static bool CheckConversion(IColladaLoaderListener* pListener)
	{
		ReportInfo(pListener, "Converting values of type %s to %s.", ArrayType<TypeCode2>::GetName(), ArrayType<TypeCode1>::GetName());
		return true;
	}
};

template <ColladaArrayType TypeCode> class Converter<TypeCode, TypeCode>
{
public:
	typedef typename ArrayType<TypeCode>::Type DataType;

	static void Convert(DataType& out, DataType in)
	{
		out = in;
	}

	static bool CheckConversion(IColladaLoaderListener* pListener)
	{
		return true;
	}
};

template <> class Converter<TYPE_BOOL, TYPE_INT> : public ValidConverter<TYPE_BOOL, TYPE_INT>
{
public:
	static void Convert(bool& out, int in) {out = (in != 0);}
};

template <> class Converter<TYPE_INT, TYPE_BOOL> : public ValidConverter<TYPE_INT, TYPE_BOOL>
{
public:
	static void Convert(int& out, bool in) {out = int(in);}
};

template <> class Converter<TYPE_INT, TYPE_FLOAT> : public ValidConverter<TYPE_INT, TYPE_FLOAT>
{
public:
	static void Convert(int& out, float in) {out = int(in);}
};

template <> class Converter<TYPE_FLOAT, TYPE_INT> : public ValidConverter<TYPE_FLOAT, TYPE_INT>
{
public:
	static void Convert(float& out, int in) {out = float(in);}
};

template <ColladaArrayType TypeCode1, ColladaArrayType TypeCode2> class ArrayReader
{
public:
	typedef typename ArrayType<TypeCode1>::Type OutputType;
	typedef typename ArrayType<TypeCode2>::Type InputType;

	static bool Read(std::vector<OutputType>& values, int stride, int offset, IColladaLoaderListener* pListener, const std::vector<InputType> data)
	{
		if (!Converter<TypeCode1, TypeCode2>::CheckConversion(pListener))
			return false;
		values.reserve(data.size() / stride + 1);
		for (int index = offset; index < int(data.size()); index += stride)
		{
			OutputType value = OutputType();
			Converter<TypeCode1, TypeCode2>::Convert(value, data[index]);
			values.push_back(value);
		}
		return true;
	}
};

template <ColladaArrayType TypeCode> class ColladaArray : public IColladaArray
{
public:
	typedef typename ArrayType<TypeCode>::Type DataType;

	virtual bool ReadAsFloat(std::vector<float>& values, int stride, int offset, IColladaLoaderListener* pListener)
	{
		return ArrayReader<TYPE_FLOAT, TypeCode>::Read(values, stride, offset, pListener, this->data);
	}

	virtual bool ReadAsInt(std::vector<int>& values, int stride, int offset, IColladaLoaderListener* pListener)
	{
		return ArrayReader<TYPE_INT, TypeCode>::Read(values, stride, offset, pListener, this->data);
	}

	virtual bool ReadAsName(std::vector<string>& values, int stride, int offset, IColladaLoaderListener* pListener)
	{
		return ArrayReader<TYPE_NAME, TypeCode>::Read(values, stride, offset, pListener, this->data);
	}

	virtual bool ReadAsBool(std::vector<bool>& values, int stride, int offset, IColladaLoaderListener* pListener)
	{
		return ArrayReader<TYPE_BOOL, TypeCode>::Read(values, stride, offset, pListener, this->data);
	}

	virtual bool ReadAsID(std::vector<string>& values, int stride, int offset, IColladaLoaderListener* pListener)
	{
		return ArrayReader<TYPE_ID, TypeCode>::Read(values, stride, offset, pListener, this->data);
	}

	virtual void Reserve(int size)
	{
		this->data.reserve(size);
	}

	virtual bool Parse(const char* buf, IColladaLoaderListener* pListener)
	{
		this->data.clear();
		this->data.resize(100);

		// Loop until we have consumed the whole buffer.
		const char* p = buf;
		bool success = true;
		int index;
		for (index = 0; ; ++index)
		{
			while (isspace(*p))
				p++;

			if (*p == 0)
				break;

			if (size_t(index) >= this->data.size())
				this->data.resize(this->data.size() * 2);

			// Parse the next value.
			DataType value = ArrayType<TypeCode>::Parse(p, success);
			this->data[index] = value;
			if (!success)
				return false;
		}
		this->data.resize(index);
		return true;
	}

	virtual int GetSize()
	{
		return int(this->data.size());
	}

private:
	std::vector<DataType> data;
};

template class ColladaArray<TYPE_FLOAT>;
typedef ColladaArray<TYPE_FLOAT> ColladaFloatArray;

template class ColladaArray<TYPE_INT>;
typedef ColladaArray<TYPE_INT> ColladaIntArray;

template class ColladaArray<TYPE_NAME>;
typedef ColladaArray<TYPE_NAME> ColladaNameArray;

template class ColladaArray<TYPE_BOOL>;
typedef ColladaArray<TYPE_BOOL> ColladaBoolArray;

template class ColladaArray<TYPE_ID>;
typedef ColladaArray<TYPE_ID> ColladaIDArray;

#pragma endregion Array

class IntStream
{
public:
	IntStream(const char* text);
	int Read();

private:
	const char* position;
};

struct ColladaColor
{
	ColladaColor()
	{
		r = g = b = a = 0.0f;
	}
	ColladaColor(float r, float g, float b, float a)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	float r,g,b,a;
};

class ColladaAssetMetaData
{
public:
	enum EUpAxis 
	{
		eUA_X,
		eUA_Y,
		eUA_Z
	};

public:
	ColladaAssetMetaData()
		: m_upAxis(eUA_Z)
		, m_unitSizeInMeters(0.01f)
		, m_authorTool(TOOL_UNKNOWN)
	{
	}
	void set(EUpAxis const upAxis, float const unitSizeInMeters, const ColladaAuthorTool& tool) 
	{ 
		m_upAxis = upAxis;
		m_authorTool = tool; 
		m_unitSizeInMeters = unitSizeInMeters; 
	}
	ColladaAuthorTool GetAuthorTool() const
	{ 
		return m_authorTool; 
	}
	const EUpAxis GetUpAxis() const 
	{
		return m_upAxis;
	}
	float GetUnitSizeInMeters() const 
	{
		return m_unitSizeInMeters;
	}

private:
	EUpAxis m_upAxis;
	float m_unitSizeInMeters;
	ColladaAuthorTool m_authorTool;
};

class ColladaDataAccessor
{
public:
	ColladaDataAccessor(IColladaArray* array, int stride, int offset, int size);
	void AddParameter(const string& name, ColladaArrayType type, int offset);

	int GetSize(const string& param);
	bool ReadAsFloat(const string& param, std::vector<float>& values, IColladaLoaderListener* pListener);
	bool ReadAsMatrix(const string& param, std::vector<Matrix34>& values, IColladaLoaderListener* pListener);
	bool ReadAsInt(const string& param, std::vector<int>& values, IColladaLoaderListener* pListener);
	bool ReadAsName(const string& param, std::vector<string>& values, IColladaLoaderListener* pListener);
	bool ReadAsBool(const string& param, std::vector<bool>& values, IColladaLoaderListener* pListener);
	bool ReadAsID(const string& param, std::vector<string>& values, IColladaLoaderListener* pListener);

private:
	class Parameter
	{
	public:
		string name;
		ColladaArrayType type;
		int offset;
	};

	IColladaArray* array;
	typedef std::map<string, Parameter> ParameterMap;
	ParameterMap parameters;
	int stride;
	int offset;
	int size;
};

// ------------------------------------------------------
class ColladaDataSource
{
public:
	ColladaDataSource(ColladaDataAccessor* pDataAccessor);
	virtual ~ColladaDataSource();

	ColladaDataAccessor* GetDataAccessor() {return this->pDataAccessor;}

private:
	ColladaDataAccessor* pDataAccessor;
};
typedef ResourceMap<ColladaDataSource> DataSourceMap;

// ------------------------------------------------------
class ColladaVertexStream
{
public:
	ColladaVertexStream(ColladaDataSource* pPositionDataSource);
	bool ReadVertexPositions(Vec3* positions, int size, float scale, IColladaLoaderListener* pListener, const ColladaAssetMetaData::EUpAxis upAxis);
	int GetSize();

private:
	ColladaDataSource* pPositionDataSource;
};
typedef ResourceMap<ColladaVertexStream> VertexStreamMap;

// ------------------------------------------------------
class ColladaImage
{
public:
	ColladaImage(const string& name);
	const string& GetName() { return this->name; }
	const string& GetFileName() { return this->filename; }
	const string& GetRelativeFileName() { return this->rel_filename; }

	void SetFileName(const string& name);

private:
	string name;
	string filename;
	string rel_filename;
};
typedef ResourceMap<ColladaImage> ImageMap;

// ------------------------------------------------------
class ColladaEffect
{
public:
	ColladaEffect(const string& name);
	const string& GetName() { return this->name; }
	void SetShader(const string& shader) { this->shader = shader; }
	const string& GetShader() { return this->shader; }

	void SetAmbient(ColladaColor& color) { this->ambient = color; }
	void SetDiffuse(ColladaColor& color) { this->diffuse = color; }
	void SetEmission(ColladaColor& color) { this->emission = color; }
	void SetSpecular(ColladaColor& color) { this->specular = color; }
	void SetReflective(ColladaColor& color) { this->reflective = color; }
	void SetTransparent(ColladaColor& color) { this->transparent = color; }

	void SetShininess(float val) { this->shininess =  val; }
	void SetReflectivity(float val) { this->reflectivity =  val; }
	void SetTransparency(float val) { this->transparency = val; }

	void SetDiffuseImage(ColladaImage* image) { this->diffuseImage = image; }
	void SetSpecularImage(ColladaImage* image) { this->specularImage = image; }
	void SetNormalImage(ColladaImage* image) { this->normalImage = image; }

	ColladaColor& GetAmbient() { return this->ambient; }
	ColladaColor& GetDiffuse() { return this->diffuse; }
	ColladaColor& GetEmission() { return this->emission; }
	ColladaColor& GetSpecular() { return this->specular; }
	ColladaColor& GetReflective() { return this->reflective; }
	ColladaColor& GetTransparent() { return this->transparent; }

	float GetShininess() { return this->shininess; }
	float GetReflectivity() { return this->reflectivity; }
	float GetTransparency() { return this->transparency; }

	ColladaImage* GetDiffuseImage() { return this->diffuseImage; }
	ColladaImage* GetSpecularImage() { return this->specularImage; }
	ColladaImage* GetNormalImage() { return this->normalImage; }

private:
	string name;
	string shader;

	ColladaColor ambient;
	ColladaColor diffuse;
	ColladaColor emission;
	ColladaColor specular;
	ColladaColor reflective;
	ColladaColor transparent;

	float shininess;
	float reflectivity;
	float transparency;

	ColladaImage* diffuseImage;
	ColladaImage* specularImage;
	ColladaImage* normalImage;
};
typedef ResourceMap<ColladaEffect> EffectMap;

// ------------------------------------------------------
class ColladaMaterial
{
public:
	ColladaMaterial(const string& name);
	const string& GetFullName() const { return this->fullname; }
	const string& GetLibrary() const { return this->library; }
	const string& GetMatName() const { return this->matname; }

	void SetEffect(ColladaEffect* effect) { this->effect = effect; }
	ColladaEffect* GetEffect() { return this->effect; }


private:
	string library;
	string fullname;
	string matname;

	ColladaEffect* effect;
};
typedef ResourceMap<ColladaMaterial> MaterialMap;

// ------------------------------------------------------
class CGFMaterialMapEntry
{
public:
	CGFMaterialMapEntry(int id, CMaterialCGF* material) : id(id), material(material) {}
	int id;
	CMaterialCGF* material;
};
typedef std::map<const ColladaMaterial*, CGFMaterialMapEntry> CGFSubMaterialMap;
typedef std::map<CMaterialCGF*, CGFSubMaterialMap> CGFMaterialMap;
typedef std::map<ColladaNode*, CMaterialCGF*> NodeMaterialMap;

// ------------------------------------------------------
struct sTexture
{
	string type;
	string filename;
};

struct sMaterial
{
	string name;
	DWORD flag;
	string shader;
	string surfacetype;
	ColladaColor diffuse;
	ColladaColor specular;
	ColladaColor emissive;
	float shininess;
	float opacity;

	std::vector<sTexture> textures;
};

struct sMaterialLibrary
{
	string library;
	DWORD flag;
	std::vector<sMaterial> materials;
};

// ------------------------------------------------------

#pragma region Primitives
class IPrimitiveStream
{
public:
	virtual ~IPrimitiveStream() {}

	virtual bool ParseNode(XmlNodeRef& node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener) = 0;
	virtual bool CreateMesh(CMesh32* mesh, float scale, IColladaLoaderListener* pListener, std::vector<int>& newToOldPositionMapping, const ColladaAssetMetaData::EUpAxis upAxis) = 0;
	virtual ColladaMaterial* GetMaterial() = 0;
};

class BasePrimitiveStream : public IPrimitiveStream
{
public:
	BasePrimitiveStream();

	virtual bool ParseNode(XmlNodeRef& node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener);
	virtual ColladaMaterial* GetMaterial();
	virtual bool CreateMesh(CMesh32* mesh, float scale, IColladaLoaderListener* pListener, std::vector<int>& newToOldPositionMapping, const ColladaAssetMetaData::EUpAxis upAxis);

protected:
	void SetVertexStream(ColladaVertexStream* vertexStream);
	void SetNormalSource(ColladaDataSource* normalSource);
	void SetColourSource(ColladaDataSource* colourSource);
	void SetTexCoordSource(ColladaDataSource* colourSource);

	int numIndices;
	std::vector<int> vertexIndices;
	std::vector<int> normalIndices;
	std::vector<int> texcoordIndices;
	std::vector<int> colorIndices;

	int indexGroupSize;											// number of columns of <p> (ex. 3, if the polylist contains "VERTEX", "NORMAL", "TEXCOORD")
	ColladaVertexStream* vertexStream;
	int positionIndexOffset;
	ColladaDataSource* normalSource;
	int normalIndexOffset;
	ColladaDataSource* colourSource;
	int colourIndexOffset;
	ColladaDataSource* texCoordSource;
	int texCoordIndexOffset;
	ColladaMaterial* material;
};

class BasePolygonPrimitiveStream : public BasePrimitiveStream
{
public:
	virtual bool CreateMesh(CMesh32* mesh, float scale, IColladaLoaderListener* pListener, std::vector<int>& newToOldPositionMapping, const ColladaAssetMetaData::EUpAxis upAxis);

protected:
	std::vector<int> polygonSizes;				// content of <vcount> node

};

class PolyStripPrimitiveStream : public BasePolygonPrimitiveStream
{
public:
	virtual bool ParseNode(XmlNodeRef& node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener);
};

class TrianglesPrimitiveStream : public BasePolygonPrimitiveStream
{
public:
	virtual bool ParseNode(XmlNodeRef& node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener);
};

#pragma endregion Primitives

typedef ResourceMap<IColladaArray> ArrayMap;

// ------------------------------------------------------
class ColladaMesh
{
public:
	ColladaMesh(std::vector<IPrimitiveStream*>& streams, ColladaVertexStream* vertexStream);
	~ColladaMesh();

	CMesh* CreateMesh(CGFSubMaterialMap& cgfMaterials, float scale, IColladaLoaderListener* pListener, std::vector<int>& meshToVertexListIndexMapping, const string& nodeName, const std::vector<SMeshBoneMapping>& boneMapping, const ColladaAssetMetaData::EUpAxis upAxis);
	CMesh* CreateMeshFromMesh32(CMesh32* mesh32, const string& nodeName, IColladaLoaderListener* pListener);
	CMesh* GetMesh() { return pCMesh; }
	bool GetVertexPositions(std::vector<Vec3>& positions, float const scale, IColladaLoaderListener* pListener, const ColladaAssetMetaData::EUpAxis upAxis);

private:
	CMesh* pCMesh;
	std::vector<IPrimitiveStream*> streams;
	ColladaVertexStream* vertexStream;
};

// ------------------------------------------------------
class ColladaGeometry
{
public:
	ColladaGeometry(const string& name, ColladaMesh* pMesh);
	CMesh* CreateMesh(CGFSubMaterialMap& cgfMaterials, float scale, IColladaLoaderListener* pListener, std::vector<int>& meshToVertexListIndexMapping, const string& nodeName, const std::vector<SMeshBoneMapping>& boneMapping, const ColladaAssetMetaData::EUpAxis upAxis);
	bool GetVertexPositions(std::vector<Vec3>& positions, float const scale, IColladaLoaderListener* pListener, const ColladaAssetMetaData::EUpAxis upAxis);
	ColladaMesh* GetMesh() { return pMesh; }
	const string& GetName() const {return name;}

private:
	string name;
	ColladaMesh* pMesh;
};

typedef ResourceMap<ColladaGeometry> GeometryMap;

// ------------------------------------------------------
struct ColladaBoneMap
{
	std::vector<int> boneIDs;
	std::vector<float> weights;
};

class ColladaController
{
public:
	ColladaController(ColladaGeometry* pGeometry);

	bool ParseVertexWeights(XmlNodeRef node, DataSourceMap& dataSources, IColladaLoaderListener* pListener);
	bool ParseBoneMap(XmlNodeRef node, DataSourceMap& dataSources, IColladaLoaderListener* pListener);
	bool ParseBoneMatrices(XmlNodeRef node, DataSourceMap& dataSources, float scale, IColladaLoaderListener* pListener);
	void ReduceBoneMap(XmlNodeRef node, IColladaLoaderListener* pListener);
	void ReIndexBoneIndices(const string& rootJoint);
	Matrix34& GetShapeMatrix() { return shapeMatrix; }
	ColladaGeometry* GetGeometry() { return this->pGeometry; }
	void GetMeshBoneMapping(std::vector<SMeshBoneMapping>& meshbonemap) { meshbonemap = this->meshBoneMapping; }
	int GetNumJoints() { return this->jointList.size(); }
	string& GetJointID(int index) { return this->jointList[index]; }
	Matrix34& GetJointMatrix(int index) { return this->baseMatrices[index]; }
	int GetJointIndex(const string& name);

	void AddMorphTarget(const string& name, ColladaGeometry* geometry) {this->morphTargets.push_back(MorphTarget(name, geometry));}
	int GetMorphTargetCount() const {return this->morphTargets.size();}
	const string& GetMorphTargetName(int morphTargetIndex) {return this->morphTargets[morphTargetIndex].name;}
	ColladaGeometry* GetMorphTargetGeometry(int morphTargetIndex) {return this->morphTargets[morphTargetIndex].geometry;}

private:
	struct MorphTarget
	{
		MorphTarget(const string& name, ColladaGeometry* geometry): name(name), geometry(geometry) {}
		string name;
		ColladaGeometry* geometry;
	};

	int indexGroupSize;

	ColladaDataSource* jointListSource;						// joint list data source (only for error-check the references)

	Matrix34 shapeMatrix;									// shape matrix... for what? [MichaelS] This transforms the skin node into its bind pose position (can often be identity).
	std::vector<string> jointList;						// bone list for mesh
	std::vector<Matrix34> baseMatrices;						// bone base matrices
	std::vector<float> weightList;							// weight list
	std::vector<int> weightSizes;							// content of 'vcount' node
	int jointIndexOffset;
	int weightIndexOffset;
	std::vector<ColladaBoneMap> boneMapping;				// this size equal with positions size in collada mesh
	std::vector<SMeshBoneMapping> meshBoneMapping;			// same as boneMapping but reduced to 4 vertex per bone

	ColladaGeometry* pGeometry;

	std::vector<MorphTarget> morphTargets;
};

typedef ResourceMap<ColladaController> ControllerMap;

// ------------------------------------------------------
class ColladaNode
{
public:
	ColladaNode(const string& id, const string& name, ColladaNode* parent, int nodeIndex);

	void SetProperty(const string& prop) { this->prop = prop; }
	void SetHelperData(const SHelperData& helperData) { this->helperData = helperData; }
	void SetType(ColladaNodeType type) { this->type = type; }
	void SetTranslate(Vec3& translate) { this->translate = translate; }
	void SetRotationX(Vec4& rotate) { this->rotationX = rotate; }
	void SetRotationY(Vec4& rotate) { this->rotationY = rotate; }
	void SetRotationZ(Vec4& rotate) { this->rotationZ = rotate; }
	void SetScale(Vec3& scale) { this->scale = scale; }
	void SetTransform(Matrix34& transform) { this->transform = transform; }
	void SetGeometry(ColladaGeometry* geometry) { this->pGeometry = geometry; }
	void SetController(ColladaController* controller) { this->pController = controller; }

	const string& GetName() const { return this->name; }
	const string& GetID() const { return this->id; }
	const string& GetProperty() const { return this->prop; }
	const SHelperData& GetHelperData() const { return this->helperData; }
	ColladaNodeType GetType() { return this->type; }
	
	ColladaNode* GetParent() { return this->parent; }
	int GetNumChild() { return this->children.size(); }
	ColladaNode* GetChild(int index) { return this->children[index]; }
	void AddChild(ColladaNode* node) { this->children.push_back(node); }

	Vec3& GetTranslate() { return this->translate; }
	Vec4& GetRotationX() { return this->rotationX; }
	Vec4& GetRotationY() { return this->rotationY; }
	Vec4& GetRotationZ() { return this->rotationZ; }
	Vec3& GetScale() { return this->scale; }
	Matrix34& GetTransform() { return this->transform; }

	bool IsRoot() { return (parent ? true : false); }

	int GetIndex() { return nodeIndex; }

	void AddSubMaterial(ColladaMaterial* material);
	void GetSubMaterials(std::vector<ColladaMaterial*>& materials);
	ColladaGeometry* GetGeometry() { return this->pGeometry; }
	ColladaController* GetController() { return this->pController; }

	CNodeCGF* CreateNodeCGF(CMaterialCGF* rootMaterial, CGFSubMaterialMap& cgfMaterials, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener);

private:
	string id;
	string name;
	string prop;
	ColladaNodeType type;
	ColladaNode* parent;
	std::vector<ColladaNode*> children;
	Vec3 translate;
	Vec4 rotationX,rotationY,rotationZ;
	Vec3 scale;
	Matrix34 transform;

	ColladaGeometry* pGeometry;
	ColladaController* pController;
	std::vector<ColladaMaterial*> materials;

	SHelperData helperData;

	int nodeIndex;				// each collada node gets a new index
};

typedef std::map<string, ColladaNode*> NameNodeMap;

// ------------------------------------------------------
class ColladaAnimation;

class ColladaAnimClip
{
public:
	ColladaAnimClip(float startTime, float endTime, const string& animClipID, ColladaScene* targetScene);
	~ColladaAnimClip();

	float GetStartTime() { return startTime; }
	float GetEndTime() { return endTime; }
	string& GetAnimClipID() { return animClipID; }
	ColladaScene* GetTargetScene() { return targetScene; }

	int GetNumAnim() { return animations.size(); }
	ColladaAnimation* GetAnim(int index) { return animations[index]; }

	void AddAnim(ColladaAnimation* anim) { this->animations.push_back(anim); }

private:
	std::vector<ColladaAnimation*> animations;

	float startTime;
	float endTime;
	string animClipID;
	ColladaScene* targetScene;
};

typedef ResourceMap<ColladaAnimClip> AnimClipMap;

// ------------------------------------------------------
struct CryExportNodeParams
{
	CryExportNodeParams()
	{
		exportType = EXPORT_UNKNOWN;
		mergeAllNodes = false;
	}

	string fullName;
	string nodeName;
	ExportType exportType;
	bool mergeAllNodes;
};

enum ContentFlags
{
	ContentFlags_Geometry = 0x01,
	ContentFlags_Animation = 0x02
};

struct PhysBone
{
	enum Axis
	{
		AxisX,
		AxisY,
		AxisZ
	};
	enum Limit
	{
		LimitMin,
		LimitMax
	};
	typedef std::pair<Axis, Limit> AxisLimit;
	typedef std::map<AxisLimit, float> AxisLimitLimitMap;
	typedef std::map<Axis, float> AxisSpringTensionMap;
	typedef std::map<Axis, float> AxisSpringAngleMap;
	typedef std::map<Axis, float> AxisDampingMap;

	PhysBone(const string& name): name(name) {}
	string name;
	AxisLimitLimitMap limits;
	AxisSpringTensionMap springTensions;
	AxisSpringAngleMap springAngles;
	AxisDampingMap dampings;
};

typedef std::map<string, PhysBone> PhysBoneMap;

class ColladaScene
{
public:
	ColladaScene(CryExportNodeParams& nodeParams)
	{
		this->nodeParams = nodeParams;
	}

	~ColladaScene();
	void AddNode(ColladaNode* pNode);

	CContentCGF* CreateContentCGF(const string& filename, const NameNodeMap& nodes, NodeMaterialMap& nodeMaterialMap, const ColladaAssetMetaData& metaData, CGFMaterialMap& cgfMaterials, IColladaLoaderListener* pListener, int contentFlags);
	void CreateMaterial(CGFMaterialMap& cgfMaterials, const NameNodeMap& nameNodeMap, NodeMaterialMap& nodeMaterialMap, IColladaLoaderListener* pListener);
	
	// skinning
	void AddBoneRecursive(ColladaNode *node, int parent_id, CSkinningInfo *pSkinningInfo, ColladaController* controller, const NameNodeMap& nodes, const std::map<string, int>& boneNameIndexMap);
	bool SetSkinningInfoCGF(CContentCGF* cgf, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener, const NameNodeMap& nameNodeMap);
	bool SetBoneParentFrames(CContentCGF* cgf, IColladaLoaderListener* pListener);
	bool SetIKParams(CContentCGF* cgf, IColladaLoaderListener* pListener);
	bool AddCollisionMeshesCGF(CContentCGF* cgf, const ColladaAssetMetaData& metaData, const NameNodeMap& nameNodeMap, CGFMaterialMap& cgfMaterials, NodeMaterialMap& nodeMaterialMap, IColladaLoaderListener* pListener);
	bool AddMorphTargetsCGF(CContentCGF* cgf, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener);
	void GetAnimClipList(AnimClipMap& animclips, std::vector<ColladaAnimClip*>& animClipList, IColladaLoaderListener* pListener);
	CInternalSkinningInfo* CreateControllerTCBSkinningInfo(CContentCGF* cgf, ColladaAnimClip* animClip, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener);
	CInternalSkinningInfo* CreateControllerSkinningInfo(CContentCGF* cgf, ColladaAnimClip* animClip, const NameNodeMap& nodes, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener);

	NameNodeMap * GetNodesNameMap() { return &nodeNames; }
	const string& GetExportNodeName() { return nodeParams.nodeName; }
	ExportType GetExportType() { return nodeParams.exportType; }
	bool NeedToMerge()
	{
		// If the export type isn't CGF then this scene is not mergeable
		if (nodeParams.exportType != EXPORT_CGF)
			return false;

		return nodeParams.mergeAllNodes;
	}

	void SetPhysBones(PhysBoneMap& physBones);
	void SetPhysParentFrames(std::map<string, Matrix33>& physParentFrames);

private:
	std::vector<ColladaNode*> nodes;
	NameNodeMap nodeNames;
	CryExportNodeParams nodeParams;
	PhysBoneMap physBones;
	std::map<string, Matrix33> physParentFrames;
};

typedef ResourceMap<ColladaScene> SceneMap;

// ------------------------------------------------------

struct ColladaKey
{
	float time;
	float value;
	Vec2 inTangent;
	Vec2 outTangent;
	ColladaInterpType interp;
	Vec3 tcb;
	Vec2 easeInOut;
};

class ColladaAnimation
{
public:
	ColladaAnimation();
	~ColladaAnimation();

	bool ParseChannel(XmlNodeRef channelNode, XmlNodeRef samplerNode, SceneMap& scenes, DataSourceMap& dataSources, float scale, IColladaLoaderListener* pListener);
	void ParseName(const string& name);

	int GetKeys(void) { return inputData.size(); }
	void GetKey(int index, ColladaKey& colladaKey);
	float GetDuration(void);
	float GetValue(float time);

	ColladaNode* GetTargetNode() { return targetNode; }
	ColladaTransformType GetTargetTransform() { return targetTransform; }
	const string& GetTargetComponent() { return targetComponent; }

	enum Flags
	{
		Flags_NoExport = 1 << 0
	};
	void SetFlags(unsigned flags) {this->flags = flags;}
	unsigned GetFlags() const {return this->flags;}

private:
	std::vector<float> inputData;					// time (x) datas
	std::vector<float> outputData;					// value (y) datas
	std::vector<Vec2> inTangentData;				// in-tangents (absolute x,y coordinates) - only for bezier interpolation
	std::vector<Vec2> outTangentData;				// out-tangents (absolute x,y coordinates) - only for bezier interpolation
	std::vector<ColladaInterpType> interpData;		// interpolation types
	std::vector<Vec3> tcbData;						// TCB data
	std::vector<Vec2> easeInOutData;			// Ease-In/-Out data

	std::vector<ColladaKey> colladaKeys;			// compressed key format (time, value, in-out tangents, interpolation)

	ColladaNode* targetNode;						// animated node
	ColladaTransformType targetTransform;			// animated transformation (S,R,T)
	string targetComponent;					// animated transformation component (X,Y,Z,Angle)

	unsigned flags;                                 // Flags for curve - specified in name of curve, which is otherwise not used.

	bool ParseSamplerSource(XmlNodeRef node, DataSourceMap& dataSources, float scale, IColladaLoaderListener* pListener);
	bool ParseTransformTarget(XmlNodeRef node, SceneMap& scenes, IColladaLoaderListener* pListener);
};

typedef ResourceMap<ColladaAnimation> AnimationMap;

// ------------------------------------------------------

// Declare a stack to handle the recursion.
class NodeStackEntry
{
public:
	NodeStackEntry(XmlNodeRef parentXmlNode)
	{
		this->parentXmlNode = parentXmlNode;
		this->parentColladaNode = NULL;
		this->childIndex = 0;
		this->cryExportNode = false;
	}

	NodeStackEntry(const NodeStackEntry& other)
	{
		parentXmlNode = other.parentXmlNode;
		parentColladaNode = other.parentColladaNode;
		childIndex = other.childIndex;
		cryExportNode = other.cryExportNode;
	}
	
	~NodeStackEntry() {}

	NodeStackEntry& operator=(const NodeStackEntry& other)
	{
		parentXmlNode = other.parentXmlNode;
		parentColladaNode = other.parentColladaNode;
		childIndex = other.childIndex;
		cryExportNode = other.cryExportNode;
		return *this;
	}

	XmlNodeRef parentXmlNode;
	ColladaNode* parentColladaNode;
	int childIndex;
	bool cryExportNode;
};

class ColladaLoaderImpl
{
public:
	bool Load(std::vector<ExportFile>& exportFiles, std::vector<sMaterialLibrary>& materialLibraryList, const char* szFileName, ICryXML* pCryXML, IPakSystem* pPakSystem, IColladaLoaderListener* pListener);

private:
	XmlNodeRef LoadXML(const char* szFileName, IPakSystem* pPakSystem, ICryXML* pCryXML, IColladaLoaderListener* pListener);
	bool ParseMetaData(XmlNodeRef node, ColladaAssetMetaData* pMetadata);
	bool ParseElementIDs(XmlNodeRef node, std::map<string, XmlNodeRef>& idMap, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);

	bool ParseArrays(ArrayMap& arrays, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	IColladaArray* ParseArray(XmlNodeRef node, ColladaArrayType type, IColladaLoaderListener* pListener);

	bool ParseColladaVector(XmlNodeRef node, Vec2& param, IColladaLoaderListener* pListener);
	bool ParseColladaVector(XmlNodeRef node, Vec3& param, IColladaLoaderListener* pListener);
	bool ParseColladaVector(XmlNodeRef node, Vec4& param, IColladaLoaderListener* pListener);
	bool ParseColladaMatrix(XmlNodeRef node, Matrix34& param, IColladaLoaderListener* pListener);
	bool ParseColladaFloatList(XmlNodeRef node, int listSize, std::vector<float>& values, IColladaLoaderListener* pListener);
	bool ParseColladaColor(XmlNodeRef node, const string& parentTag, ColladaColor& color, IColladaLoaderListener* pListener);
	bool ParseColladaFloatParam(XmlNodeRef node, const string& parentTag, float& param, IColladaLoaderListener* pListener);

	bool ParseMaterials(MaterialMap& materials, EffectMap& effects, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	ColladaMaterial* ParseMaterial(XmlNodeRef node, EffectMap& effects, IColladaLoaderListener* pListener);

	bool ParseImages(ImageMap& images, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	ColladaImage* ParseImage(XmlNodeRef node, IColladaLoaderListener* pListener);

	bool ParseEffects(EffectMap& effects, ImageMap& images, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	ColladaEffect* ParseEffect(XmlNodeRef node, ImageMap& images, IColladaLoaderListener* pListener);

	bool ParseDataSources(DataSourceMap& sources, ArrayMap& arrays, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	ColladaDataSource* ParseDataSource(XmlNodeRef node, ArrayMap& arrays, IColladaLoaderListener* pListener);

	bool ParseVertexStreams(VertexStreamMap& vertexStreams, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	ColladaVertexStream* ParseVertexStream(XmlNodeRef node, DataSourceMap& dataSources, IColladaLoaderListener* pListener);

	bool ParseGeometries(GeometryMap& geometries, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	ColladaGeometry* ParseGeometry(XmlNodeRef node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener);

	bool ParseMorphControllers(ControllerMap& morphControllers, GeometryMap& geometries, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	ColladaController* ParseMorphController(XmlNodeRef node, GeometryMap& geometries, DataSourceMap& dataSources, IColladaLoaderListener* pListener);

	bool ParseSkinControllers(ControllerMap& skinControllers, ControllerMap& morphControllers, GeometryMap& geometries, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, float scale, IColladaLoaderListener* pListener);
	ColladaController* ParseSkinController(XmlNodeRef node, ControllerMap& morphControllers, GeometryMap& geometries, DataSourceMap& dataSources, float scale, IColladaLoaderListener* pListener);

	bool ParseAnimations(AnimationMap& animations, SceneMap& scenes, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, float scale, IColladaLoaderListener* pListener);
	ColladaAnimation* ParseAnimation(XmlNodeRef node, SceneMap& scenes, DataSourceMap& dataSources, float scale, IColladaLoaderListener* pListener);

	bool ParseAnimClips(AnimClipMap& animclips, AnimationMap& animations, SceneMap& scenes, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener);
	ColladaAnimClip* ParseAnimClip(XmlNodeRef node, AnimationMap& animations, SceneMap& scenes, IColladaLoaderListener* pListener);

	bool ParseSceneDefinitions(SceneMap& scenes, GeometryMap& geometries, ControllerMap& controllers, MaterialMap& materials, std::multimap<string, XmlNodeRef>& tagMap, float scale, IColladaLoaderListener* pListener);
	void ParseSceneDefinition(SceneMap& scenes, XmlNodeRef sceneNode, GeometryMap& geometries, ControllerMap& controllers, MaterialMap& materials, float scale, IColladaLoaderListener* pListener);
	void ParseSceneNode(SceneMap& scenes, XmlNodeRef sceneNode, NameNodeMap& nodes, GeometryMap& geometries, MaterialMap& materials, IColladaLoaderListener* pListener);

	void ReadXmlNodeTransform(ColladaNode* colladaNode, XmlNodeRef node, float scale, IColladaLoaderListener* pListener);

	string ReadXmlNodeUserProperties(XmlNodeRef node, IColladaLoaderListener* pListener);
	SHelperData ReadXmlNodeHelperData(XmlNodeRef node, float scale, IColladaLoaderListener* pListener);

	bool ParseInstanceNull(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, IColladaLoaderListener* pListener);
	bool ParseInstanceCamera(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, IColladaLoaderListener* pListener);
	bool ParseInstanceController(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, ControllerMap& controllers, MaterialMap& materials, IColladaLoaderListener* pListener);
	bool ParseInstanceGeometry(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, GeometryMap& geometries, MaterialMap& materials, IColladaLoaderListener* pListener);
	bool ParseInstanceLight(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, IColladaLoaderListener* pListener);
	bool ParseInstanceNode(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, IColladaLoaderListener* pListener);

	ColladaImage* GetImageFromTextureParam(XmlNodeRef profileNode, XmlNodeRef textureNode, ImageMap& images, IColladaLoaderListener* pListener);

	void CreateMaterialLibrariesContent(MaterialMap& materials, std::vector<sMaterialLibrary>& materialLibraryList);
	void EnsureRootHasIdentityTransform(CContentCGF* cgf, IColladaLoaderListener* pListener);
};

#endif //__COLLADALOADER_H__
