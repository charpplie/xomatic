#include "StdAfx.h"
#include "CoverSystem.h"
#include "CoverSampler.h"
#include "CoverSurface.h"


const uint32 BAI_COVER_FILE_VERSION_READ = 1;


CCoverSystem::CCoverSystem()
{
}

ICoverSampler* CCoverSystem::CreateCoverSampler(const char* samplerName)
{
	if (!stricmp(samplerName, "default"))
		return new CoverSampler();

	return 0;
}

void CCoverSystem::Reset()
{
	m_surfaces.resize(m_staticCount);
	FreeIDs().swap(m_freeIDs);
}

void CCoverSystem::Clear()
{
	Surfaces().swap(m_surfaces);
	FreeIDs().swap(m_freeIDs);
}


bool CCoverSystem::ReadSurfacesFromFile(const char* fileName)
{
	assert(m_surfaces.empty());

	CCryFile file;

	if(!file.Open(fileName, "rb"))
	{
		AIWarning("Could not read AI Cover Surfaces. [%s]", fileName);

		return false;
	}

	uint32 fileVersion = 0;

	file.ReadType(&fileVersion);

	if (fileVersion < BAI_COVER_FILE_VERSION_READ)
	{
		AIWarning("Wrong BAI file version '%d'. Please regenerate AI Cover Surfaces in the Editor.", fileVersion);

		return false;
	}

	CoverSurfaceID surfaceID = CoverSurfaceID(0);

	uint32 surfaceCount = 0;
	file.ReadType(&surfaceCount);

	if (!surfaceCount)
		return true;

	m_surfaces.reserve(surfaceCount + PreallocatedDynamicCount);
	m_surfaces.resize(surfaceCount);
	m_staticCount = surfaceCount;

	std::vector<ICoverSampler::Sample> samples;

	for (uint32 i = 0; i < surfaceCount; ++i)
	{
		ICoverSystem::SurfaceInfo surfaceInfo;

		file.ReadType(&surfaceInfo.sampleCount);

		if (!surfaceInfo.sampleCount)
			continue;

		samples.resize(surfaceInfo.sampleCount);

		for (uint s = 0; s < surfaceInfo.sampleCount; ++s)
		{
			ICoverSampler::Sample& sample = samples[s];

			file.ReadType(&sample.position);
			file.ReadType(&sample.height);
			file.ReadType(&sample.flags);
		}

		surfaceInfo.samples = &samples.front();

		UpdateSurface(++surfaceID, surfaceInfo);
	}

	return true;
}

CoverSurfaceID CCoverSystem::AddSurface(const SurfaceInfo& surfaceInfo)
{
	CoverSurfaceID surfaceID = m_surfaces.size() + 1;
	if (m_surfaces.size() < m_surfaces.capacity())
		m_surfaces.resize(m_surfaces.size() + 1);
	else
	{
		if (!m_freeIDs.empty())
		{
			surfaceID = m_freeIDs.front();
			std::swap(m_freeIDs.front(), m_freeIDs.back());
			m_freeIDs.pop_back();
		}
		else
		{
			m_surfaces.reserve(m_surfaces.capacity() + PreallocatedDynamicCount);
			m_surfaces.resize(m_surfaces.size() + 1);
		}
	}

	CoverSurface(surfaceInfo).Swap(m_surfaces[surfaceID - 1]);

	return surfaceID;
}

void CCoverSystem::RemoveSurface(const CoverSurfaceID& surfaceID)
{
	if ((surfaceID > 0) && (surfaceID <= m_surfaces.size()))
	{
		CoverSurface().Swap(m_surfaces[surfaceID - 1]);
		assert(std::find(m_freeIDs.begin(), m_freeIDs.end(), surfaceID) == m_freeIDs.end());
		m_freeIDs.push_back(surfaceID);
	}
}

void CCoverSystem::UpdateSurface(const CoverSurfaceID& surfaceID, const SurfaceInfo& surfaceInfo)
{
	if ((surfaceID > 0) && (surfaceID <= m_surfaces.size()))
		CoverSurface(surfaceInfo).Swap(m_surfaces[surfaceID - 1]);
}

uint32 CCoverSystem::GetSurfaceCount() const
{
	return m_surfaces.size() - m_freeIDs.size();
}

bool CCoverSystem::GetSurfaceInfo(const CoverSurfaceID& surfaceID, SurfaceInfo* surfaceInfo) const
{
	if ((surfaceID > 0) && (surfaceID <= m_surfaces.size()))
	{
		const CoverSurface& surface = m_surfaces[surfaceID - 1];
		return surface.GetSurfaceInfo(surfaceInfo);
	}

	return false;
}

void CCoverSystem::DrawSurface(const CoverSurfaceID& surfaceID)
{
	if ((surfaceID > 0) && (surfaceID <= m_surfaces.size()))
	{
		CoverSurface& surface = m_surfaces[surfaceID - 1];
		surface.DebugDraw();
	}
}

void CCoverSystem::DebugDraw()
{
	if (gAIEnv.CVars.DebugDrawCover != 2)
		return;

	Surfaces::iterator it = m_surfaces.begin();
	Surfaces::iterator end = m_surfaces.end();

	const Vec3& pos = gEnv->pSystem->GetViewCamera().GetPosition();

	for ( ; it != end; ++it)
	{
		CoverSurface& surface = *it;
		if ((surface.GetAABB().GetCenter() - pos).len2() <= sqr(50.0f))
			surface.DebugDraw();
	}
}