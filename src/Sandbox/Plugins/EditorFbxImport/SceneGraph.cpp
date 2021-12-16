// Copyright (c) 1999-2014 Crytek.

#include "StdAfx.h"
#include "SceneGraph.h"
#include "FbxImportPlugin.h"
#include "ResourceCompilerHelper.h"
#include "RCListener.h"
#include <QDir>
#include <IXml.h>

namespace
{
	// Helper struct to store a manifest in
	struct SManifest
	{
		static const size_t cInvalidIndex = (size_t)-1;

		struct SNode
		{
			const char *pName;
			size_t parentIndex;
			size_t meshIndex;
			bool bUnsupportedAttribute;
			SSceneNode *pConverted;

			SNode() : pName(nullptr), parentIndex(cInvalidIndex), meshIndex(cInvalidIndex), bUnsupportedAttribute(false), pConverted(nullptr) {}
		};

		struct SMesh
		{
			size_t numFaces;

			SMesh() : numFaces(0) {}
		};

		SManifest(size_t numNodes, size_t numMeshes) 
			: pNodes(numNodes ? new SNode[numNodes] : nullptr), numNodes(numNodes)
			, pMeshes(numMeshes ? new SMesh[numMeshes] : nullptr), numMeshes(numMeshes)
			, dUnitSizeInCm(-1.0) {}

		~SManifest()
		{
			delete [] pNodes;
			delete [] pMeshes;
		}

		SNode * const pNodes;
		size_t const numNodes;
		SMesh * const pMeshes;
		size_t const numMeshes;

		double dUnitSizeInCm;

	private:
		// Prevent accidental copy or assign
		SManifest(const SManifest &);
		void operator =(const SManifest &);
	};

	// Query the RC for the manifest to an FBX file
	XmlNodeRef GetRCManifest(const char *pFbxPath)
	{
		// Call RC to generate the manifest
		string manifestFile = string(pFbxPath) + "$manifest";
		string options = "/overwriteextension=fbx /refresh /manifest=\"" + manifestFile + "\"";
		CRcListener listener(IResourceCompilerListener::MessageSeverity_Warning);
		gEnv->pSystem->GetILog()->LogAlways(LOG_RC_FORMAT "%s", "Running RC manifest for ", pFbxPath);
		bool bSuccess = CResourceCompilerHelper::CallResourceCompiler(pFbxPath, options.c_str(), &listener, false, CResourceCompilerHelper::eRcExePath_currentFolder, true, true) == CResourceCompilerHelper::eRcCallResult_success;

		XmlNodeRef result = nullptr;
		if (bSuccess)
		{
			// Load XML file
			IXmlUtils *pXmlUtils = gEnv->pSystem->GetXmlUtils();
			result = pXmlUtils->LoadXmlFromFile(manifestFile.c_str());
		}

		// Delete temporary file
		// Note: we don't handle failure for this, if it fails we leave a small file behind
		QFile::remove(QtUtil::ToQString(manifestFile.c_str()));

		return result;
	}

	// Load a manifest file
	// Note: The XML node reference needs to be kept in scope as long as string values are needed
	static SManifest *LoadManifest(const char *pFbxPath, XmlNodeRef &ref)
	{
		// Parse the manifest XML
		XmlNodeRef manifest = GetRCManifest(pFbxPath);
		XmlNodeRef sceneNode = manifest && strcmp(manifest->getTag(), "scene") == 0 ? manifest : nullptr;
		if (sceneNode)
		{
			// Set up de-serialization object
			XmlNodeRef nodesListNode = sceneNode->findChild("nodes");
			size_t nodeCount = nodesListNode ? nodesListNode->getChildCount() : 0;
			XmlNodeRef meshesListNode = sceneNode->findChild("meshes");
			size_t meshCount = meshesListNode ? meshesListNode->getChildCount() : 0;
			SManifest *result = new SManifest(nodeCount, meshCount);

			// Global attributes
			sceneNode->getAttr("unitSizeInCm", result->dUnitSizeInCm);
			if (!(result->dUnitSizeInCm > 0.0))
			{
				// Assume something valid for this, afaik the RC should specify this always
				result->dUnitSizeInCm = 100.0;
			}

			// Parse nodes
			for (size_t i = 0; i < nodeCount; ++i)
			{
				XmlNodeRef nodeNode = nodesListNode->getChild(i);
				if (strcmp(nodeNode->getTag(), "node") == 0)
				{
					size_t nodeIndex;
					if (nodeNode->getAttr("index", nodeIndex) && nodeIndex < nodeCount)
					{
						SManifest::SNode *pTarget = result->pNodes + nodeIndex;
						if (nodeNode->getAttr("parentNodeIndex", pTarget->parentIndex)) // If no parentNodeIndex, skip the rest of this node, it's considered invalid
						{
							pTarget->pName = nodeNode->getAttr("name"); // If no name, this assigns nullptr, not a problem
							XmlNodeRef attributeNode = nodeNode->findChild("attribute");
							if (attributeNode)
							{
								const char *pAttributeType = attributeNode->getAttr("type");
								if (strcmp(pAttributeType, "mesh") == 0)
								{
									if (!attributeNode->getAttr("meshIndex", pTarget->meshIndex))
									{
										pTarget->bUnsupportedAttribute = true;
									}
								}
								else if (pAttributeType)
								{
									// Unknown attribute type, mark as unsupported, but don't reject
									// This is a "forward compatibility" feature, when RC gets more type support we can at least accept and ignore
									pTarget->bUnsupportedAttribute = true;
								}
							}
						}
					}
				}
			}

			// Parse meshes
			for (size_t i = 0; i < meshCount; ++i)
			{
				XmlNodeRef meshNode = meshesListNode->getChild(i);
				if (strcmp(meshNode->getTag(), "mesh") == 0)
				{
					size_t meshIndex;
					if (meshNode->getAttr("index", meshIndex) && meshIndex < meshCount)
					{
						SManifest::SMesh *pTarget = result->pMeshes + meshIndex;
						meshNode->getAttr("triFaceCount", pTarget->numFaces);
					}
				}
			}

			// Validate
			if (result->numNodes > 0)
			{
				bool bHasRoot = false;
				bool bValid = true;
				SManifest::SNode *pNode = result->pNodes;
				for (size_t i = 0; i < nodeCount; ++i, ++pNode)
				{
					if (pNode->parentIndex == SManifest::cInvalidIndex)
					{
						if (!bHasRoot)
						{
							bHasRoot = true;
						}
						else
						{
							// Two or more root nodes
							bValid = false;
						}
					}
					else if (pNode->parentIndex >= nodeCount)
					{
						// Invalid parent index
						bValid = false;
					}
					if (pNode->meshIndex >= meshCount)
					{
						if (pNode->meshIndex != SManifest::cInvalidIndex)
						{
							// Invalid mesh index
							bValid = false;
						}
					}
					else
					{
						SManifest::SMesh *pMesh = result->pMeshes + pNode->meshIndex;
						if (pMesh->numFaces == 0)
						{
							// Not an actual mesh, detach
							pNode->meshIndex = SManifest::cInvalidIndex;
							pNode->bUnsupportedAttribute = true;
						}
					}
				}
				if (bValid)
				{
					// Passed validation
					ref = manifest;
					return result;
				}
			}

			// Delete loaded data, it's invalid
			delete result;
		}

		// No such manifest, or invalid manifest
		return nullptr;
	}

	// Helper to create a root SSceneNode
	static SSceneNode *CreateRootNode(const QString &name)
	{
		SSceneNode *pResult = new SSceneNode();
		pResult->model.pParent = nullptr;
		pResult->model.name = name;
		pResult->model.type = eNC_None;
		return pResult;
	}

	// Convert a single linear node to a tree node
	static SSceneNode *ConvertNodeToTreeRecursive(SManifest::SNode *pNode, SManifest *pManifest)
	{
		// Return if already converted
		if (pNode->pConverted)
		{
			return pNode->pConverted;
		}

		// Convert parent node, if applicable
		SSceneNode *pParent = nullptr;
		if (pNode->parentIndex != SManifest::cInvalidIndex)
		{
			SManifest::SNode *pUnconvertedParent = pManifest->pNodes + pNode->parentIndex;
			pParent = ConvertNodeToTreeRecursive(pUnconvertedParent, pManifest);
			if (!pParent)
			{
				// Some kind of failure in recursion
				return nullptr;
			}
		}
		
		SSceneNode *pResult = new SSceneNode;
		if (pParent)
		{
			// Link node into tree
			pResult->model.pParent = pParent;
			pParent->model.children.push_back(pResult);
		}
		else
		{
			// This is the root node, don't link
			pResult->model.pParent = nullptr;
		}

		// Set up node information
		pResult->model.name = QtUtil::ToQString(pNode->pName);
		if (pNode->meshIndex != SManifest::cInvalidIndex)
		{
			// Assign mesh information
			SManifest::SMesh *pMesh = pManifest->pMeshes + pNode->meshIndex;
			pResult->model.description = QObject::tr("Mesh: %1 triangles").arg(pMesh->numFaces);
			pResult->model.type = eNC_Geometry;
		}
		else
		{
			// Assign default information
			pResult->model.type = pNode->bUnsupportedAttribute ? eNC_Misc : eNC_None;
		}

		// Cache converted node to break recursion
		pNode->pConverted = pResult;
		return pResult;
	}

	// Destroy scene recursively
	static void DestroySceneRecursive(SSceneNode *pNode)
	{
		if (pNode)
		{
			for (QList<SSceneNode *>::const_iterator i = pNode->model.children.begin(); i != pNode->model.children.end(); ++i)
			{
				DestroySceneRecursive(*i);
			}
			delete pNode;
		}
	}
}

// Import FBX into scene nodes
CSceneGraph::CSceneGraph(const char *pFbxPath) : m_pRoot(nullptr), m_dUnitInCm(0.0)
{
	XmlNodeRef xml;
	SManifest *pManifest = LoadManifest(pFbxPath, xml);
	if (pManifest)
	{
		// Create root node for all gathered information
		m_pRoot = CreateRootNode(QtUtil::ToQString(pFbxPath));
		m_dUnitInCm = pManifest->dUnitSizeInCm;

		// Load the linear nodes in the manifest into a tree
		SManifest::SNode *pNode = pManifest->pNodes;
		for (size_t i = 0; i < pManifest->numNodes; ++i, ++pNode)
		{
			SSceneNode *pConverted = ConvertNodeToTreeRecursive(pNode, pManifest);
			if (pConverted->model.pParent == nullptr)
			{
				// Link root node of the scene to root of the result
				pConverted->model.name = QObject::tr("Scene");
				m_pRoot->model.children.push_back(pConverted);
				pConverted->model.pParent = m_pRoot;
			}
		}

#if 0 // Not currently supported in RC manifest, might eventually be included so let's leave the code
		bool bMaterials = IsImportSupported(eNC_Material);
		bool bAnimations = IsImportSupported(eNC_Animation);

		// Attach loaded materials to the root
		int numMaterials = pScene->GetMaterialCount();
		if (numMaterials && bMaterials)
		{
			SSceneNode *pMaterialRoot = CreateRootNode(QStringLiteral("Materials"));
			m_pRoot->model.children.push_back(pMaterialRoot);
			pMaterialRoot->model.pParent = m_pRoot;

			for (int i = 0; i < numMaterials; ++i)
			{
				// Add material node
				FbxSurfaceMaterial *pMaterial = pScene->GetMaterial(i);
				SSceneNode *pMaterialNode = CreateMaterialNode(pMaterial);
				pMaterialRoot->model.children.push_back(pMaterialNode);
				pMaterialNode->model.pParent = pMaterialRoot;
			}
		}

		// Attach loaded animations to the root
		int numAnimations = pScene->GetSrcObjectCount(FBX_TYPE(FbxAnimStack));
		if (numAnimations && bAnimations)
		{
			SSceneNode *pAnimationRoot = CreateRootNode(QStringLiteral("Animations"));
			m_pRoot->model.children.push_back(pAnimationRoot);
			pAnimationRoot->model.pParent = m_pRoot;

			for (int i = 0; i < numAnimations; ++i)
			{
				FbxAnimStack *pAnimation = static_cast<FbxAnimStack *>(pScene->GetSrcObject(FBX_TYPE(FbxAnimStack), i));
				SSceneNode *pAnimationNode = CreateAnimationNode(pAnimation);
				pAnimationRoot->model.children.push_back(pAnimationNode);
				pAnimationNode->model.pParent = pAnimationRoot;
			}
		}
#endif
	}
}

// Destroy scene (if any)
CSceneGraph::~CSceneGraph()
{
	DestroySceneRecursive(m_pRoot);
}