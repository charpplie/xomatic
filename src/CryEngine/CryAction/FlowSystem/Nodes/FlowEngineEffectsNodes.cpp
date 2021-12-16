#include "StdAfx.h"
#include "FlowBaseNode.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Color correction
////////////////////////////////////////////////////////////////////////////////////////////////////

struct SColorCorrection
{  
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("Global_User_ColorC", 0.0f, 0, "Cyan"),
      InputPortConfig<float>("Global_User_ColorM", 0.0f, 0, "Magenta"),
      InputPortConfig<float>("Global_User_ColorY", 0.0f, 0, "Yellow"),
      InputPortConfig<float>("Global_User_ColorK", 0.0f, 0, "Luminance"),
      InputPortConfig<float>("Global_User_Brightness", 1.0f, 0, "Brightness"),
      InputPortConfig<float>("Global_User_Contrast", 1.0f, 0, "Contrast"),
      InputPortConfig<float>("Global_User_Saturation", 1.0f, 0, "Saturation"),
      InputPortConfig<float>("Global_User_ColorHue", 0.0f, "Change Hue", "Hue"),      
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Sets color correction parameters");
  }
  
  static void Disable()
  {
    SFlowNodeConfig config;
    GetConfiguration(config);
    I3DEngine* pEngine = gEnv->p3DEngine;

    for ( int i=2; config.pInputPorts[i].name ; ++i )
    {        
      if (config.pInputPorts[i].defaultData.GetType()==eFDT_Float)
      {          
        float fVal;
        bool ok = config.pInputPorts[i].defaultData.GetValueWithConversion( fVal );
        if (ok)
          pEngine->SetPostEffectParam( config.pInputPorts[i].name, fVal );
      }
    }
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Glow post process settings
////////////////////////////////////////////////////////////////////////////////////////////////////

struct SGlow
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {		
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("Glow_Scale", 0.5f, "Sets overall glow multiplier", "Final glow multiplier"),
      InputPortConfig<float>("Glow_ScreenThreshold", 0.5f, "Sets fullscreen glow threshold", "Screen glow threshold"),
      InputPortConfig<float>("Glow_ScreenMul", 0.2f, "Sets fullscreen glow multiplier", "Screen glow multiplier"),
      InputPortConfig<Vec3>("clr_Glow_StreaksColor", Vec3(0.8f, 0.7f, 1.5f), 0, "Streaks color"),
      InputPortConfig<float>("Glow_StreaksMul", 3.0f, "Sets glow streaks color multiplier", "Streaks multiplier"),
      InputPortConfig<float>("Glow_StreaksStretch", 1.0f, "Sets glow streaks stretch amount", "Streaks stretch"),
      InputPortConfig<float>("Glow_FlaresMul", 0.25f, "Sets glow lens-flares (ghost-flares) multiplier", "Lens-flares multiplier"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Misc post process setup");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "Glow_Scale", 0.5f );
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Filters
////////////////////////////////////////////////////////////////////////////////////////////////////

struct SFilterBlur
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<int>("FilterBlurring_Type", 0, "Set blur type, 0 is Gaussian", "Type"),
      InputPortConfig<float>("FilterBlurring_Amount", 0.0f, "Amount of blurring", "Amount"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Sets blur filter");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "FilterBlurring_Amount", 0 );
  }
};

struct SFilterRadialBlur
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {      
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("FilterRadialBlurring_Amount", 0.0f, "Amount of blurring", "Amount"),
      InputPortConfig<float>("FilterRadialBlurring_ScreenPosX", 0.5f, "Horizontal screen position", "ScreenPosX"),
      InputPortConfig<float>("FilterRadialBlurring_ScreenPosY", 0.5f, "Vertical screen position", "ScreenPosY"),
      InputPortConfig<float>("FilterRadialBlurring_Radius", 1.0f, "Blurring radius", "BlurringRadius"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Sets radial blur filter");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "FilterRadialBlurring_Amount", 0 );
  }
};

struct SFilterSharpen
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {      
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<int>("FilterSharpening_Type", 0, "Sets sharpening filter, 0 is Unsharp Mask", "Type"),
      InputPortConfig<float>("FilterSharpening_Amount", 1.0f, "Amount of sharpening", "Amount"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Sets sharpen filter");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "FilterSharpening_Amount", 1.0f );
  }
};

struct SFilterChromaShift
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {            
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("FilterChromaShift_User_Amount", 0.0f, "Amount of chroma shift", "Amount"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Sets chroma shift filter");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "FilterChromaShift_User_Amount", 0 );
  }
};

struct SFilterGrain
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {            
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("FilterGrain_Amount", 0.0f, "Amount of grain", "Amount"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Sets grain filter");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "FilterGrain_Amount", 0 );
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Effects 
////////////////////////////////////////////////////////////////////////////////////////////////////

struct SEffectFrost
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {      
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("ScreenFrost_Amount", 0.0f, "Amount of frost", "Amount"),
      InputPortConfig<float>("ScreenFrost_CenterAmount", 1.0f, "Amount of frost at center of screen", "CenterAmount"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Freezing effect");
  }
    
  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "ScreenFrost_Amount", 0 );
  }
};

struct SEffectCondensation
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {      
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("ScreenCondensation_Amount", 0.0f, "Amount of condensation ", "Amount"),
      InputPortConfig<float>("ScreenCondensation_CenterAmount", 1.0f, "Amount of condensation at center of screen", "CenterAmount"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY | EFLN_OBSOLETE;	// obsolete - to be removed
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Condensation effect");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "ScreenCondensation_Amount", 0 );
  }
};

struct SEffectWaterDroplets
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {      
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("WaterDroplets_Amount", 0.0f, "Amount of water", "Amount"),      
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Water droplets effect - Use when camera goes out of water or similar extreme condition");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "WaterDroplets_Amount", 0 );
  }
};

struct SEffectWaterFlow
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {      
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("WaterFlow_Amount", 0.0f, "Amount of water", "Amount"),      
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Water flow effect - Use when camera receiving splashes of water or similar");
  }
    
  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "WaterFlow_Amount", 0 );
  }
};


struct SEffectBloodSplats
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {			
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<int>  ("BloodSplats_Type", 0, "Blood type: human or alien", "Type"),
      InputPortConfig<float>("BloodSplats_Amount", 0.0f, "Amount of visible blood", "Amount"),
      InputPortConfig<bool> ("BloodSplats_Spawn", false, "Spawn more blood particles", "Spawn"),			
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Blood splats effect");
  }
    
  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "BloodSplats_Amount", 0 );
  }
};

struct SEffectDistantRain
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {			
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),      
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("DistantRain_Amount", 0.0f, "Amount of visible distant rain", "Amount"),
      InputPortConfig<float>("DistantRain_Speed", 1.0f, "Distant rain speed multiplier", "Speed"),
      InputPortConfig<float>("DistantRain_DistanceScale", 1.0f, "Distant rain distance scale", "DistanceScale"),
      InputPortConfig<Vec3>("clr_DistantRain_Color", Vec3(1.0f, 1.0f, 1.0f), "Sets distance rain color", "Color"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Distant rain effect");
  }
    
  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "DistantRain_Amount", 0 );
  }
};

struct SEffectDof
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {			
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<bool>  ("Dof_User_Active", false, "Enables dof effect", "EnableDof"),
      InputPortConfig<float>("Dof_User_FocusDistance", 3.5f, "Set focus distance", "FocusDistance"),
      InputPortConfig<float> ("Dof_User_FocusRange", 5.0f, "Set focus range", "FocusRange"),			
      InputPortConfig<float> ("Dof_User_BlurAmount", 1.0f, "Set blurring amount", "BlurAmount"),			
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Depth of field effect");
  }

  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "Dof_User_BlurAmount", 1 );
  }
};

struct SEffectDirectionalBlur
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {			
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<Vec3>("Global_DirectionalBlur_Vec", Vec3(0.0f, 0.0f, 0.0f), "Sets directional blur direction", "Direction"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Directional Blur effect");
  }
    
  static void Disable()
  {
    Vec4 vec( 0, 0, 0, 1);
    gEnv->p3DEngine->SetPostEffectParamVec4( "Global_DirectionalBlur_Vec", vec );
  }
};

struct SEffectAlienInterference
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {			
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", false, "Disables node", "Disabled"),
      InputPortConfig<float>("AlienInterference_Amount", 0.0f, "Sets alien interference amount", "Amount"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Alien interference effect");
  }
    
  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "AlienInterference_Amount", 0 );
  }
};

struct SEffectRainDrops
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {			
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("RainDrops_Amount", 0.0f, "Sets rain drops visibility", "Amount"),
      InputPortConfig<float>("RainDrops_SpawnTimeDistance", 0.35f, "Sets rain drops spawn time distance", "Spawn Time Distance"),
      InputPortConfig<float>("RainDrops_Size", 5.0f, "Sets rain drops size", "Size"),
      InputPortConfig<float>("RainDrops_SizeVariation", 2.5f, "Sets rain drops size variation", "Size Variation"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Rain drops effect");
  }
    
  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "RainDrops_Amount", 0 );
  }
};

struct SEffectVolumetricScattering
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = 
    {			
      InputPortConfig<bool> ("Enabled", false, "Enables node", "Enabled"),
      InputPortConfig<bool> ("Disabled", true, "Disables node", "Disabled"),
      InputPortConfig<float>("VolumetricScattering_Amount", 0.0f, "Sets volumetric scattering amount", "Amount"),
      InputPortConfig<float>("VolumetricScattering_Tilling", 1.0f, "Sets volumetric scattering tilling", "Tilling"),
      InputPortConfig<float>("VolumetricScattering_Speed", 1.0f, "Sets volumetric scattering animation speed", "Speed"),
      InputPortConfig<Vec3>("clr_VolumetricScattering_Color", Vec3(0.6f, 0.75f, 1.0f), "Sets volumetric scattering amount", "Color"),
      InputPortConfig<int>("VolumetricScattering_Type", 0, "Set volumetric scattering type", "Type"),
      {0}
    };

    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Volumetric scattering effect");
  }
    
  static void Disable()
  {
    gEnv->p3DEngine->SetPostEffectParam( "VolumetricScattering_Amount", 0 );
  }
};

template<class T>
class CFlowImageNode : public CFlowBaseNode
{
public:
  CFlowImageNode( SActivationInfo * pActInfo )
  {
  }

  ~CFlowImageNode()
  {
  }

	/* It's now a singleton node, so clone MUST not be implemented
  IFlowNodePtr Clone( SActivationInfo * pActInfo )
  {
    return this;
  }
	*/

	enum EInputPorts
	{
		EIP_Enabled = 0,
		EIP_Disabled
	};

  virtual void GetConfiguration(SFlowNodeConfig& config)
  {
    T::GetConfiguration(config);
		if( !(config.nFlags & EFLN_OBSOLETE) )
			config.SetCategory( EFLN_ADVANCED );
  }
  
  virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
  {
    if ( event != eFE_Activate )
    {
      return;
    }

		if (!InputEntityIsLocalPlayer( pActInfo )) 
		  return;
    
    bool bIsEnabled = IsPortActive(pActInfo, EIP_Enabled) && GetPortBool( pActInfo, EIP_Enabled );
    const bool bIsDisabled = IsPortActive(pActInfo, EIP_Disabled) && GetPortBool( pActInfo, EIP_Disabled );
    
    if (!bIsDisabled && GetPortBool( pActInfo, EIP_Enabled ))  // this is to support when effects parameters are changed after the effect was activated. would be easier to just make sure that the bool value of both inputs is always complementary, but that conflicts on how triggers work, and other nodes already rely on that way of working.
      bIsEnabled = true;
      
    if (!bIsEnabled)
    {
      T::Disable();
      return;
    }

    SFlowNodeConfig config;
    T::GetConfiguration(config);
    I3DEngine* pEngine = gEnv->p3DEngine;

    for ( int i( 2 ); config.pInputPorts[i].name ; ++i )
    {        
      const TFlowInputData& anyVal = GetPortAny(pActInfo, i);

      switch( anyVal.GetType() )
      {          

        case eFDT_Vec3:
          {                        
            Vec3 pVal( GetPortVec3( pActInfo, i) );      
            if (!bIsEnabled)
              config.pInputPorts[i].defaultData.GetValueWithConversion( pVal );
            pEngine->SetPostEffectParamVec4(config.pInputPorts[i].name, Vec4(pVal, 1));
            
            break;
          }

        case eFDT_String:
          {            
            const string &pszString = GetPortString( pActInfo, i );
            pEngine->SetPostEffectParamString(config.pInputPorts[i].name, pszString.c_str());

            break;
          }

        default:
          {
            float fVal;
            bool ok( anyVal.GetValueWithConversion(fVal) );

            if (!bIsEnabled)
              ok = config.pInputPorts[i].defaultData.GetValueWithConversion( fVal );

            if( ok )
            {                
              pEngine->SetPostEffectParam(config.pInputPorts[i].name, fVal);
            }

            break;
          }
      }
    }
  }

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

// macro similar to REGISTER_FLOW_NODE, but allows a different name for registration
#define REGISTER_FLOW_NODE_SINGLETON_EX( FlowNodeClassName,FlowNodeClass,RegName ) \
  CAutoRegFlowNodeSingleton<FlowNodeClass> g_AutoReg##RegName ( FlowNodeClassName );

// these macros expose the nodes in the FlowSystem, otherwise they're not available in the system
REGISTER_FLOW_NODE_SINGLETON_EX("Image:ColorCorrection", CFlowImageNode<SColorCorrection>, SColorCorrection);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:FilterBlur", CFlowImageNode<SFilterBlur>, SFilterBlur);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:FilterRadialBlur", CFlowImageNode<SFilterRadialBlur>, SFilterRadialBlur);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:FilterSharpen", CFlowImageNode<SFilterSharpen>, SFilterSharpen);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:ChromaShift", CFlowImageNode<SFilterChromaShift>, SFilterChromaShift);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:EffectFrost", CFlowImageNode<SEffectFrost>, SEffectFrost);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:FilterGrain", CFlowImageNode<SFilterGrain>, SFilterGrain);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:EffectCondensation", CFlowImageNode<SEffectCondensation>, SEffectCondensation);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:EffectWaterDroplets", CFlowImageNode<SEffectWaterDroplets>, SEffectWaterDroplets);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:EffectWaterFlow", CFlowImageNode<SEffectWaterFlow>, SEffectWaterFlow);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:EffectBloodSplats", CFlowImageNode<SEffectBloodSplats>, SEffectBloodSplats);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:DepthOfField", CFlowImageNode<SEffectDof>, SEffectDof);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:VolumetricScattering", CFlowImageNode<SEffectVolumetricScattering>, SEffectVolumetricScattering);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:DirectionalBlur", CFlowImageNode<SEffectDirectionalBlur>, SEffectDirectionalBlur);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:AlienInterference", CFlowImageNode<SEffectAlienInterference>, SEffectAlienInterference);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:RainDrops", CFlowImageNode<SEffectRainDrops>, SEffectRainDrops);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:DistantRain", CFlowImageNode<SEffectDistantRain>, SEffectDistantRain);
REGISTER_FLOW_NODE_SINGLETON_EX("Image:Glow", CFlowImageNode<SGlow>, SGlow);
