#include "StdAfx.h"
#include "SkinningHelpers.h"
#include "MeshHelpers.h"
#include "ExportNodeHelpers.h"
#include <xsi_envelope.h>
#include <xsi_application.h>
#include <xsi_model.h>
#include <xsi_time.h>

namespace
{
	void RecurseAndListBones(std::map<string, XSI::CRef>& bones, XSI::X3DObject& bone)
	{
		std::pair<string, XSI::CRef> value = std::make_pair(string(bone.GetRef().GetAsText().GetAsciiString()), bone.GetRef());
		if (bones.insert(value).second)
		{
			// Recurse to the node's children.
			for (int childIndex = 0, childCount = bone.GetChildren().GetCount(); childIndex < childCount; ++childIndex)
				RecurseAndListBones(bones, XSI::X3DObject(bone.GetChildren().GetItem(childIndex)));
		}
	}
}

std::set<XSI::CRef> SkinningHelpers::GetSkeletonRoots(XSI::CRef model)
{
	std::set<XSI::CRef> skeletonRoots;

	// Create an object to help us access the mesh.
	MeshHelpers::MeshAccessor meshAccessor = XSI::X3DObject(model);

	// Loop through all the envelopes associated with this model.
	XSI::CRefArray clusters;
	meshAccessor.mesh.GetClusters().Filter(XSI::siVertexCluster, XSI::CStringArray(), L"", clusters);
	for (int clusterIndex = 0, clusterCount = clusters.GetCount(); clusterIndex < clusterCount; ++clusterIndex)
	{
		XSI::Cluster cluster = clusters[clusterIndex];
		XSI::CRefArray envelopes = cluster.GetEnvelopes();
		std::map<string, XSI::CRef> weightedBones;
		for (int envelopeIndex = 0, envelopeCount = envelopes.GetCount(); envelopeIndex < envelopeCount; ++envelopeIndex)
		{
			XSI::Envelope envelope = envelopes[envelopeIndex];

			// Loop through all the bones associated with this envelope.
			XSI::CRefArray deformers = envelope.GetDeformers();
			for (int deformerIndex = 0, deformerCount = deformers.GetCount(); deformerIndex < deformerCount; ++deformerIndex)
			{
				XSI::X3DObject deformer = deformers[deformerIndex];

				weightedBones.insert(std::make_pair(deformer.GetRef().GetAsText().GetAsciiString(), deformer.GetRef()));
			}
		}

		// Recurse to create a full list of all bones.
		std::map<string, XSI::CRef> bones;
		for (std::map<string, XSI::CRef>::iterator bonePos = weightedBones.begin(), boneEnd = weightedBones.end(); bonePos != boneEnd; ++bonePos)
			RecurseAndListBones(bones, XSI::X3DObject((*bonePos).second));

		// Now just find the parents.
		std::map<string, XSI::CRef> rootMap;
		for (std::map<string, XSI::CRef>::iterator bonePos = bones.begin(), boneEnd = bones.end(); bonePos != boneEnd; ++bonePos)
		{
			// Find this bone's root.
			XSI::X3DObject root = (*bonePos).second;
			std::string testName = root.GetName().GetAsciiString();
			std::string testParentName;
			for (;;)
			{
				XSI::X3DObject ancestor = XSI::X3DObject(root.GetParent());
				testParentName = ancestor.GetName().GetAsciiString();
				if (bones.find(ancestor.GetName().GetAsciiString()) == bones.end())
					break;
				root = ancestor;
				testName = root.GetName().GetAsciiString();
			}

			rootMap.insert(std::make_pair(root.GetRef().GetAsText().GetAsciiString(), root.GetRef()));
		}

		for (std::map<string, XSI::CRef>::iterator bonePos = rootMap.begin(), boneEnd = rootMap.end(); bonePos != boneEnd; ++bonePos)
			skeletonRoots.insert((*bonePos).second);
	}

	return skeletonRoots;
}

std::map<int, std::map<int, double> > SkinningHelpers::GetVertexWeights(XSI::CRef model, const std::map<string, int>& boneNameIDMap)
{
	std::map<int, std::map<int, double> > vertexWeights;

	// Create an object to help us access the mesh.
	MeshHelpers::MeshAccessor meshAccessor = XSI::X3DObject(model);

	// Loop through all the envelopes associated with this skeleton.
	XSI::CRefArray clusters;
	meshAccessor.mesh.GetClusters().Filter(XSI::siVertexCluster, XSI::CStringArray(), L"", clusters);
	for (int clusterIndex = 0, clusterCount = clusters.GetCount(); clusterIndex < clusterCount; ++clusterIndex)
	{
		XSI::Cluster cluster = clusters[clusterIndex];
		XSI::CRefArray envelopes = cluster.GetEnvelopes();
		for (int envelopeIndex = 0, envelopeCount = envelopes.GetCount(); envelopeIndex < envelopeCount; ++envelopeIndex)
		{
			XSI::Envelope envelope = envelopes[envelopeIndex];

			// Get mapping from cluster element index to geometry position index
			XSI::CLongArray clusterToMeshMap = envelope.GetElements(XSI::CTime().GetTime()).GetArray();

			// Loop through all the bones associated with this envelope.
			XSI::CRefArray deformers = envelope.GetDeformers();
			for (int deformerIndex = 0, deformerCount = deformers.GetCount(); deformerIndex < deformerCount; ++deformerIndex)
			{
				XSI::X3DObject deformer = deformers[deformerIndex];

				XSI::CDoubleArray weights = envelope.GetDeformerWeights(deformer, XSI::CTime().GetTime());

				int boneID = (*boneNameIDMap.find(deformer.GetRef().GetAsText().GetAsciiString())).second;
				for (int clusterVertexIndex = 0, clusterVertexCount = weights.GetCount(); clusterVertexIndex < clusterVertexCount; ++clusterVertexIndex)
				{
					if (weights[clusterVertexIndex] > 0.0)
					{
						int vertexIndex = clusterToMeshMap[clusterVertexIndex];
						std::map<int, double>& weightsForVertex = vertexWeights[vertexIndex];
						double& weight = weightsForVertex[boneID];
						weight += weights[clusterVertexIndex] * 0.01; // Seems like theres a factor of 100 in here for some reason.
					}
				}
			}
		}
	}

	return vertexWeights;
}
