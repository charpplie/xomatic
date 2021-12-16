/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.

-------------------------------------------------------------------------
History:
- 15:05:2009   Created by Federico Rebora

*************************************************************************/

#include "StdAfx.h"

#include "ColorGradientManager.h"

namespace Graphics
{
	CColorGradientManager::CColorGradientManager()
	{
	}

	void CColorGradientManager::TriggerFadingColorGradient(const string& filePath, const float fadeInTimeInSeconds)
	{
		const unsigned int numGradients = (int) m_currentGradients.size();
		for (unsigned int currentGradientIndex = 0; currentGradientIndex < numGradients; ++currentGradientIndex)
		{
			m_currentGradients[currentGradientIndex].FreezeMaximumBlendAmount();
		}

		m_colorGradientsToLoad.push_back(LoadingColorGradient(filePath, fadeInTimeInSeconds));
	}

	void CColorGradientManager::UpdateForThisFrame(const float frameTimeInSeconds)
	{
		RemoveZeroWeightedLayers();
		
		LoadGradients();

		FadeInLastLayer(frameTimeInSeconds);
		FadeOutCurrentLayers();

		SetLayersForThisFrame();
	}

	void CColorGradientManager::FadeInLastLayer(const float frameTimeInSeconds)
	{
		if (m_currentGradients.empty())
		{
			return;
		}

		m_currentGradients.back().FadeIn(frameTimeInSeconds);
	}

	void CColorGradientManager::FadeOutCurrentLayers()
	{
		if (m_currentGradients.size() <= 1u)
		{
			return;
		}

		const unsigned int numberofFadingOutGradients = (int) m_currentGradients.size() - 1;
		const float fadingInLayerBlendAmount = m_currentGradients[numberofFadingOutGradients].m_layer.m_blendAmount;
		for (unsigned int index = 0; index < numberofFadingOutGradients; ++index)
		{
			m_currentGradients[index].FadeOut(fadingInLayerBlendAmount);
		}
	}

	void CColorGradientManager::RemoveZeroWeightedLayers()
	{
		std::vector<LoadedColorGradient>::iterator currentGradient = m_currentGradients.begin();

		while (currentGradient != m_currentGradients.end())
		{
			if (currentGradient->m_layer.m_blendAmount == 0.0f)
			{
				GetColorGradingController().UnloadColorChart(currentGradient->m_layer.m_texID);

				currentGradient = m_currentGradients.erase(currentGradient);
			}
      else
      {
        ++currentGradient;
      }
		}
	}

	void CColorGradientManager::SetLayersForThisFrame()
	{
		std::vector<SColorChartLayer> thisFrameLayers;

		const unsigned int numberOfFadingInGradients = (int) m_currentGradients.size();
		thisFrameLayers.reserve(numberOfFadingInGradients + thisFrameLayers.size());
		for (unsigned int index = 0; index < numberOfFadingInGradients; ++index)
		{
			thisFrameLayers.push_back(m_currentGradients[index].m_layer);
		}

		if (!thisFrameLayers.empty())
		{
 			GetColorGradingController().SetLayers(&thisFrameLayers.front(), (int) thisFrameLayers.size());
		}
	}

	void CColorGradientManager::LoadGradients()
	{
		const unsigned int numGradientsToLoad = (int) m_colorGradientsToLoad.size();
		m_currentGradients.reserve(numGradientsToLoad +  m_currentGradients.size());
		for (unsigned int index = 0; index < numGradientsToLoad; ++index)
		{
			LoadedColorGradient loadedGradient = m_colorGradientsToLoad[index].Load(GetColorGradingController());
			
			m_currentGradients.push_back(loadedGradient);
		}

		m_colorGradientsToLoad.clear();
	}

	IColorGradingController& CColorGradientManager::GetColorGradingController()
	{
		return *gEnv->pRenderer->GetIColorGradingController();
 }

	CColorGradientManager::LoadedColorGradient::LoadedColorGradient(const SColorChartLayer& layer, const float fadeInTimeInSeconds)
	: m_layer(layer)
	, m_fadeInTimeInSeconds(fadeInTimeInSeconds)
	, m_elapsedTime(0.0f)
	, m_maximumBlendAmount(1.0f)
	{

	}

	void CColorGradientManager::LoadedColorGradient::FadeIn(const float frameTimeInSeconds)
	{
		if (m_fadeInTimeInSeconds == 0.0f)
		{
			m_layer.m_blendAmount = 1.0f;

			return;
		}

		m_elapsedTime += frameTimeInSeconds;

		const float blendAmount = m_elapsedTime / m_fadeInTimeInSeconds;

		m_layer.m_blendAmount = min(blendAmount, 1.0f);
	}

	void CColorGradientManager::LoadedColorGradient::FadeOut( const float blendAmountOfFadingInGradient )
	{
		m_layer.m_blendAmount = m_maximumBlendAmount * (1.0f - blendAmountOfFadingInGradient);
	}

	void CColorGradientManager::LoadedColorGradient::FreezeMaximumBlendAmount()
	{
		m_maximumBlendAmount = m_layer.m_blendAmount;
	}

	CColorGradientManager::LoadingColorGradient::LoadingColorGradient(const string& filePath, const float fadeInTimeInSeconds)
	: m_filePath(filePath)
	, m_fadeInTimeInSeconds(fadeInTimeInSeconds)
	{

	}

	Graphics::CColorGradientManager::LoadedColorGradient CColorGradientManager::LoadingColorGradient::Load(IColorGradingController& colorGradingController) const
	{
		const int textureID = colorGradingController.LoadColorChart(m_filePath);

		return LoadedColorGradient(SColorChartLayer(textureID, 1.0f), m_fadeInTimeInSeconds);
	}
}
