#include "StdAfx.h"
#include "CoverSampler.h"

#include "DebugDrawContext.h"


CoverSampler::CoverSampler()
: m_externalState(None)
{
}

CoverSampler::~CoverSampler()
{
}

void CoverSampler::Release()
{
	delete this;
}

ICoverSampler::ESamplerState CoverSampler::StartSampling(const ICoverSampler::Params& params)
{
	m_state = SamplingState();
	m_state.internalState = SamplerStarting;

	m_params = params;
	m_externalState = InProgress;

	return m_externalState;
}

ICoverSampler::ESamplerState CoverSampler::Update(float timeLimit)
{
	CTimeValue now = gEnv->pTimer->GetAsyncTime();
	CTimeValue start = now;
	CTimeValue endTime = now + CTimeValue(timeLimit);
	
	do
	{
		switch(m_state.internalState)
		{
		case SamplerStarting:
			{
				m_tempSamples.clear();
				m_samples.clear();

				m_state.direction = m_params.direction;
				m_state.direction.z = 0.0f;
				m_state.direction.NormalizeSafe(Vec3Constants<float>::fVec3_OneY);

				m_state.right = m_state.direction.Cross(CoverUp);
				m_state.right.Normalize();

				Vec3 floor;

				if (SampleFloor(m_params.position, m_params.floorSearchHeight, m_params.floorSearchRadius, 
					&floor, 0))
				{
					floor += CoverUp * CoverOffsetUp;

					float depth = -1.0f;
					float height = SampleHeight(floor, m_params.widthSamplerInterval * 0.5f, m_params.heightSamplerInterval,
						m_params.limitHeight - CoverOffsetUp, &depth);
					
					if ((height > 0.0f) && (depth > 0.0f) && (height + CoverOffsetUp >= m_params.minHeight))
					{
						floor += CoverUp * -CoverOffsetUp;
						height += CoverOffsetUp;

						Vec3 surfaceOrigin = floor + m_state.direction * depth;

						m_tempSamples.push_back(Sample(surfaceOrigin, height));

						m_state.origin = floor + m_state.direction * (depth - m_params.maxClearance);

						m_state.currOffset = 0.0f;
						m_state.lastSurfaceOffset = 0.0f;
						m_state.edgeHeight = 0.0f;
						m_state.internalState = SamplerLeft;
						
						break;
					}
				}

				m_state.internalState = SamplerError;
			}
			break;
		case SamplerLeft:
			{
				if (!SampleCover(Left, m_params.widthSamplerInterval, m_params.widthGapTolerance, m_params.limitHeight))
				{
					// Stopped before limit was reached! Lets find the edge!
					if ((m_state.currOffset > -m_params.limitLeft) && (m_params.refineEdges))
					{
						m_state.internalState = SamplerLeftEdge;
						m_tempSamples.pop_front();

						m_state.lastSurfaceOffset += m_params.widthSamplerInterval;
						m_state.currOffset = m_state.lastSurfaceOffset;
						m_state.edgeHeight = m_state.lastSurfaceHeight;
					}
					else
					{
						m_state.internalState = SamplerRight;
						m_state.currOffset = 0.0f;
						m_state.lastSurfaceOffset = 0.0f;
					}
				}
			}
			break;
		case SamplerLeftEdge:
			{
				if (!SampleCover(Left, m_params.widthSamplerInterval * 0.25f, 0.001f,
					min(m_params.limitHeight, m_state.edgeHeight + m_params.heightSamplerInterval)))
				{
					m_state.internalState = SamplerRight;
					m_state.currOffset = 0.0f;
					m_state.lastSurfaceOffset = 0.0f;
				}
			}
			break;
		case SamplerRight:
			{
				if (!SampleCover(Right, m_params.widthSamplerInterval, m_params.widthGapTolerance, m_params.limitHeight))
				{
					if ((m_state.currOffset < m_params.limitRight) && (m_params.refineEdges))
					{
						m_state.internalState = SamplerRightEdge;
						m_tempSamples.pop_back();

						m_state.lastSurfaceOffset -= m_params.widthSamplerInterval;
						m_state.currOffset = m_state.lastSurfaceOffset;
						m_state.edgeHeight = m_state.lastSurfaceHeight;
					}
					else
					{
						m_state.internalState = SamplerSimplifySurface;
						m_state.currOffset = 0.0f;
						m_state.lastSurfaceOffset = 0.0f;
					}
				}
			}
			break;
		case SamplerRightEdge:
			{
				if (!SampleCover(Right, m_params.widthSamplerInterval * 0.25f, 0.001f, 
					min(m_params.limitHeight, m_state.edgeHeight + m_params.heightSamplerInterval)))
				{
					m_state.internalState = SamplerSimplifySurface;
					m_state.currOffset = 0.0f;
					m_state.lastSurfaceOffset = 0.0f;
				}
			}
			break;
		case SamplerSimplifySurface:
			{
				m_state.originalSurfaceSamples = m_tempSamples.size();

				Simplify();

				//m_state.internalState = SamplerSampleClearance;
				m_state.internalState = SamplerFinished;
			}
			break;
			/*
		case SamplerSampleClearance:
			{
				m_state.internalState = SamplerFinished;
			}
			break;
			*/
		case SamplerFinished:
			{
				m_samples.resize(m_tempSamples.size());

				std::copy(m_tempSamples.begin(), m_tempSamples.end(), m_samples.begin());

				m_state.internalState = SamplerReady;
				m_externalState = Finished;
			}
			break;
		case SamplerError:
			break;
		default:
			return m_externalState;
		}

		if (m_state.internalState == SamplerError)
		{
			m_state.internalState = SamplerReady;
			m_externalState = Error;

			break;
		}

		now = gEnv->pTimer->GetAsyncTime();

		if (m_externalState != InProgress)
			break;

	} while (now < endTime);

	++m_state.updateCount;
	m_state.totalTime += now - start;
		
	return m_externalState;
}

ICoverSampler::ESamplerState CoverSampler::GetState() const
{
	return m_externalState;
}

uint32 CoverSampler::GetSampleCount() const
{
	return m_samples.size();
}

const ICoverSampler::Sample* CoverSampler::GetSamples() const
{
	if (!m_samples.empty())
		return &m_samples[0];

	return 0;
}

void CoverSampler::DebugDraw() const
{
	CDebugDrawContext dc;

	dc->Draw3dLabel(m_state.origin + CoverUp * 0.5f, 1.35f, 
		"Time: ~%.2f/%.2fms\nUpdate Count: %d\nPWI Count: %d\nSimplify: %d/%d",	
		m_state.totalTime.GetMilliSeconds() / (float)m_state.updateCount, m_state.totalTime.GetMilliSeconds(),
		m_state.updateCount, m_state.pwiCount, m_samples.size(), m_state.originalSurfaceSamples);
}

bool CoverSampler::OverlapCylinder(const Vec3& center, const Vec3& dir, float radius,
	uint32 collisionFlags) const
{
	float halfHeight = dir.len();
	
  primitives::cylinder cylinder;
  cylinder.center = center;
  cylinder.axis = dir / halfHeight;
  cylinder.hh = halfHeight;
  cylinder.r = radius;
  
  IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
  
  float distance = pPhysics->PrimitiveWorldIntersection(cylinder.type, &cylinder,
  Vec3Constants<float>::fVec3_Zero, collisionFlags);

	++m_state.pwiCount;
    
  return distance != 0.0f;
}

bool CoverSampler::IntersectSweptSphere(const Vec3& center, const Vec3& dir, 
	float radius, uint32 collisionFlags, IntersectionResult* result) const
{
  primitives::sphere sphere;
  sphere.center = center;
  sphere.r = radius;

 	geom_contact *contact = 0;
  geom_contact **ppContact = result ? &contact : 0;
  
	IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
  
	float distance = pPhysics->PrimitiveWorldIntersection(sphere.type, &sphere, dir,
    m_params.collisionFlags, ppContact);

	++m_state.pwiCount;

  if (distance > 0.0f)
  {
  	if (result)
  	{
  		result->distance = distance;
  		result->position = contact->pt;
  		result->normal = contact->dir;
  	}
  	
    return true;
  }

	return false;
}

float CoverSampler::SampleHeight(const Vec3& position, float radius, float heightInterval, float maxHeight, 
																	float* depth)
{
	uint32 sampleCount = 1 + (uint32)cry_floorf(cry_fabsf(maxHeight / max(heightInterval, radius)));
	float deltaHeight = maxHeight / (float)sampleCount;
	
	Vec3 dir = m_state.direction * m_params.limitDepth;
		
	float currHeight = 0.0f;
	float currDepth = FLT_MAX;

	IntersectionResult result;

	for (uint32 i = 0 ; i < sampleCount; ++i)
	{
		float sampleHeight = (deltaHeight * (i + 1) - radius);
		sampleHeight = max(sampleHeight, (radius + 0.015f));

		Vec3 center = position + CoverUp * sampleHeight;
		
		if (IntersectSweptSphere(center, dir, radius, m_params.collisionFlags, &result))
		{
			currHeight = sampleHeight;
			if (currDepth > result.distance + radius)
				currDepth = result.distance + radius;
		}
		else
		{
			if ((sampleHeight - currHeight) > m_params.heightGapTolerance)
				break;
		}
	}

	if (depth)
		*depth = currDepth;
	
	return currHeight;
}

bool CoverSampler::SampleFloor(const Vec3& position, float searchHeight, float searchRadius,
																Vec3* floor, Vec3* normal)
{
	Vec3 center = position + Vec3(0.0f, 0.0f, searchHeight * 0.5f);
	Vec3 dir = CoverUp * -searchHeight;
	
	IntersectionResult result;
	
	if (IntersectSweptSphere(center, dir, searchRadius, m_params.collisionFlags | ent_terrain, &result))
	{
		if (floor)
			*floor = Vec3(position.x, position.y, position.z + ((searchHeight * 0.5f) - searchRadius) - result.distance);
			
		if (normal)
			*normal = result.normal;
		
		return true;
	}
	
	return false;
}

/*
float CCoverSampler::SampleClearance(const Vec3& position, const Vec3& direction)
{
	const float ClearanceStep = min(0.1f, (m_params.maxClearance - m_params.minClearance));

	Vec3 dir = CoverUp * (m_params.maxHeight * 0.5f);
	Vec3 center = position + dir;
	
	uint32 sampleCount = 1;
	
	if (ClearanceStep > 0.001f)
		sampleCount += (uint32)cry_floorf(cry_fabsf(m_params.maxClearance - m_params.minClearance) / ClearanceStep);

	float deltaRadius = cry_fabsf(m_params.maxClearance - m_params.minClearance) / (float)sampleCount;

	float currClearance = 0.0f;
	
	for (uint32 i = sampleCount; i >= 0; --i)
	{
		float sampleRadius = m_params.minClearance + (i * deltaRadius);

		if (!OverlapCylinder(center + direction * (sampleRadius + 0.01f), dir, sampleRadius, m_params.collisionFlags))
			return sampleRadius;
	}

	return 0.0f;
}*/

bool CoverSampler::SampleCover(EDirection direction, float widthInterval, float gapTolerance, float maxHeight)
{
	m_state.currOffset += (direction == Left) ? -widthInterval : widthInterval;

	Vec3 floor;

	if (SampleFloor(m_state.origin + m_state.right * m_state.currOffset, m_params.floorSearchHeight, 
		m_params.floorSearchRadius, &floor, 0))
	{
		floor += CoverUp * CoverOffsetUp;

		float depth = -1.0f;
		float height = SampleHeight(floor, widthInterval * 0.5f, m_params.heightSamplerInterval,
			maxHeight - CoverOffsetUp, &depth);

		if ((height > 0.0f) && (depth > 0.0f) && (height + CoverOffsetUp >= m_params.minHeight))
		{
			floor += CoverUp * -CoverOffsetUp;
			height += CoverOffsetUp;

			CLAMP(height, 0.0f, maxHeight);

			Vec3 sample = floor + (m_state.direction * depth);

			if (direction < 0.0f)
				m_tempSamples.insert(m_tempSamples.begin(), Sample(sample, height));
			else
				m_tempSamples.push_back(Sample(sample, height));

			m_state.lastSurfaceOffset = m_state.currOffset;
			m_state.lastSurfaceHeight = height;
		}
	}

	if (cry_fabsf(m_state.currOffset - m_state.lastSurfaceOffset) > gapTolerance)
		return false;

	if (direction == Left)
	{
		if (m_state.currOffset <= -m_params.limitLeft)
			return false;
	}
	else if (m_state.currOffset >= m_params.limitRight)
		return false;

	return true;
}

void CoverSampler::Simplify()
{
	uint32 coverSampleCount = m_tempSamples.size();

	if (coverSampleCount < 3)
		return;

	TempSamples::iterator rightIt = m_tempSamples.begin();
	TempSamples::iterator leftIt = rightIt++;
	TempSamples::iterator midIt = rightIt++;

	float minAngleCos = cry_cosf(m_params.simplifyMinAngle);

	while (rightIt != m_tempSamples.end())
	{
		float dot = 0.0f;
		const Sample& left = *leftIt;
		const Sample& mid = *midIt;
		const Sample& right = *rightIt;

		float leftTop = left.position.z + left.height;
		float midTop = mid.position.z + mid.height;
		float rightTop = right.position.z + right.height;

		Vec3 leftToMid = mid.position - left.position;
		Vec3 midToRight = right.position - mid.position;

		float leftToMidLenSq = leftToMid.len2();
		float midToRightLenSq = midToRight.len2();

		if ((cry_fabsf(leftTop - midTop) > m_params.heightSamplerInterval) ||
			(cry_fabsf(midTop - rightTop) > m_params.heightSamplerInterval))
			goto SkipToNext;

		if ((leftToMidLenSq > sqr(m_params.simplifyMaxDistance)) || (midToRightLenSq > sqr(m_params.simplifyMaxDistance)))
			goto SkipToNext;

		if (leftToMidLenSq <= sqr(0.2f))
			goto PerformErase;

		leftToMid.Normalize();
		midToRight.Normalize();

		dot = leftToMid.Dot(midToRight);

		if (dot >= minAngleCos)
		{
PerformErase:
			TempSamples::iterator erased = midIt;
			midIt = rightIt++;
			m_tempSamples.erase(erased);

			continue;
		}

SkipToNext:
		++leftIt;
		++midIt;
		++rightIt;
	}
}
