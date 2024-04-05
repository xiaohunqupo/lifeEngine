#include "Misc/CoreGlobals.h"
#include "Misc/PhysicsGlobals.h"
#include "Misc/CommandLine.h"
#include "Logger/LoggerMacros.h"
#include "Reflection/Class.h"
#include "Reflection/ObjectGC.h"
#include "Reflection/ObjectPackage.h"
#include "Reflection/ObjectGlobals.h"
#include "Reflection/ObjectIterator.h"
#include "System/Config.h"
#include "System/World.h"
#include "System/Package.h"
#include "System/BaseFileSystem.h"
#include "System/PhysicsEngine.h"
#include "Misc/EngineGlobals.h"
#include "System/BaseEngine.h"
#include "System/InputSystem.h"
#include "Misc/UIGlobals.h"
#include "UIEngine.h"
#include "Render/Shaders/BasePassShader.h"
#include "Render/Shaders/WireframeShader.h"
#include "Render/RenderingThread.h"
#include "System/CameraManager.h"

IMPLEMENT_CLASS( CBaseEngine )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CBaseEngine )

/*
==================
CBaseEngine::CBaseEngine
==================
*/
CBaseEngine::CBaseEngine()
	: bPrePass( true )
	, bAutoExposure( true )
	, exposure( 1.f )
	, exposureMin( 0.2f )
	, exposureMax( 2.f )
	, gamma( 2.2f )
{}

/*
==================
CBaseEngine::Init
==================
*/
void CBaseEngine::Init()
{
	// Load default texture
	{	
		// Loading default texture from packages only when we in game
		if ( !g_IsCooker && !g_IsCommandlet )
		{
			CConfigValue		configDefaultTexture = g_Config.GetValue( CT_Engine, TEXT( "Engine.Engine" ), TEXT( "DefaultTexture" ) );
			if ( configDefaultTexture.IsValid() )
			{
				std::wstring			pathAsset = configDefaultTexture.GetString();
				TAssetHandle<CAsset>	asset = g_PackageManager->FindAsset( pathAsset );
				if ( asset.IsAssetValid() )
				{
					defaultTexture = asset;
				}
				else
				{
					Warnf( TEXT( "Default texture '%s' not loaded\n" ), pathAsset.c_str() );
				}
			}
			else
			{
				Warnf( TEXT( "Need set in config 'Engine' default texture in section 'Engine.Engine:DefaultTexture'\n" ) );
			}
		}

		// If default texture not loaded we create in virtual package
		if ( !defaultTexture.IsAssetValid() )
		{
			std::vector< byte >			data		= { 0, 0, 0, 0 };
			PackageRef_t				package		= g_PackageManager->LoadPackage( TEXT( "" ), true );
			TSharedPtr<CTexture2D>		texture2D	= MakeSharedPtr<CTexture2D>();
			texture2D->SetAssetName( TEXT( "DefaultTexture" ) );
			texture2D->SetData( PF_A8R8G8B8, 1, 1, data );

			defaultTexture		= texture2D->GetAssetHandle();
			package->Add( defaultTexture );
		}
	}

	// Load default material
	{
		// Loading default material from packages only when we in game
		if ( !g_IsCooker && !g_IsCommandlet )
		{
			CConfigValue		configDefaultMaterial = g_Config.GetValue( CT_Engine, TEXT( "Engine.Engine" ), TEXT( "DefaultMaterial" ) );
			if ( configDefaultMaterial.IsValid() )
			{
				std::wstring			pathAsset = configDefaultMaterial.GetString();
				TAssetHandle<CAsset>	asset = g_PackageManager->FindAsset( pathAsset );
				if ( asset.IsAssetValid() )
				{
					defaultMaterial = asset;
				}
				else
				{
					Warnf( TEXT( "Default material '%s' not loaded\n" ), pathAsset.c_str() );
				}
			}
			else
			{
				Warnf( TEXT( "Need set in config 'Engine' default material in section 'Engine.Engine:DefaultMaterial'\n" ) );
			}
		}

		// If default material not loaded we create in virtual package
		if ( !defaultMaterial.IsAssetValid() )
		{
			PackageRef_t			package = g_PackageManager->LoadPackage( TEXT( "" ), true );
			TSharedPtr<CMaterial>	material = MakeSharedPtr<CMaterial>();
			material->SetAssetName( TEXT( "DefaultMaterial" ) );

			defaultMaterial		= material->GetAssetHandle();
			package->Add( defaultMaterial );
		}
	}

	// Load default wireframe material (only for build with editor)
#if WITH_EDITOR
	{
		// Loading default wireframe material from packages only when we in game
		if ( !g_IsCooker && !g_IsCommandlet )
		{
			CConfigValue		configDefaultWireframeMaterial = g_Config.GetValue( CT_Engine, TEXT( "Editor.Editor" ), TEXT( "DefaultWireframeMaterial" ) );
			if ( configDefaultWireframeMaterial.IsValid() )
			{
				std::wstring			pathAsset	= configDefaultWireframeMaterial.GetString();
				TAssetHandle<CAsset>	asset		= g_PackageManager->FindAsset( pathAsset );
				if ( asset.IsAssetValid() )
				{
					defaultWireframeMaterial = asset;
				}
				else
				{
					Warnf( TEXT( "Default wireframe material '%s' not loaded\n" ), pathAsset.c_str() );
				}
			}
			else
			{
				Warnf( TEXT( "Need set in config 'Editor' default wireframe material in section 'Editor.Editor:DefaultWireframeMaterial'\n" ) );
			}
		}

		// If default wireframe material not loaded we create in virtual package
		if ( !defaultWireframeMaterial.IsAssetValid() )
		{
			PackageRef_t			package = g_PackageManager->LoadPackage( TEXT( "" ), true );
			TSharedPtr<CMaterial>	material = MakeSharedPtr<CMaterial>();
			material->SetAssetName( TEXT( "DefaultWireframeMaterial" ) );
			material->SetWireframe( true );

			defaultWireframeMaterial	= material->GetAssetHandle();
			package->Add( defaultWireframeMaterial );
		}
	}
#endif // WITH_EDITOR

	// Get from configs PrePass, HDRExposure, etc values
	{
		CConfigValue	configPrePass = g_Config.GetValue( CT_Engine, TEXT( "Engine.SystemSettings" ), TEXT( "PrePass" ) );
		if ( configPrePass.IsValid() )
		{
			bPrePass = configPrePass.GetBool();
		}

		CConfigValue	configExposure = g_Config.GetValue( CT_Engine, TEXT( "Engine.SystemSettings" ), TEXT( "Exposure" ) );
		if ( configExposure.IsValid() )
		{
			exposure = configExposure.GetNumber();
		}

		CConfigValue	configAutoExposure = g_Config.GetValue( CT_Engine, TEXT( "Engine.SystemSettings" ), TEXT( "AutoExposure" ) );
		if ( configAutoExposure.IsValid() )
		{
			bAutoExposure = configAutoExposure.GetBool();
		}

		CConfigValue	configExposureMin = g_Config.GetValue( CT_Engine, TEXT( "Engine.SystemSettings" ), TEXT( "ExposureMin" ) );
		if ( configExposureMin.IsValid() )
		{
			exposureMin = configExposureMin.GetNumber();
		}

		CConfigValue	configExposureMax = g_Config.GetValue( CT_Engine, TEXT( "Engine.SystemSettings" ), TEXT( "ExposureMax" ) );
		if ( configExposureMax.IsValid() )
		{
			exposureMax = configExposureMax.GetNumber();
		}

		CConfigValue	configGamma = g_Config.GetValue( CT_Engine, TEXT( "Engine.SystemSettings" ), TEXT( "Gamma" ) );
		if ( configGamma.IsValid() )
		{
			gamma = configGamma.GetNumber();
		}
	}

	// Register default assets
	g_AssetFactory.SetDefault( defaultTexture, AT_Texture2D );
	g_AssetFactory.SetDefault( defaultMaterial, AT_Material );

	g_InputSystem->Init();
	g_UIEngine->Init();
	g_PhysicsEngine.Init();
}

/*
==================
CBaseEngine::Shutdown
==================
*/
void CBaseEngine::Shutdown()
{
	// Wait while render thread is rendering of the frame
	FlushRenderingCommands();

	// End play in the world and shutdown other systems
	if ( g_World )
	{
		g_World->EndPlay();
		g_World->RemoveFromRoot();
		g_World = nullptr;
	}

	g_UIEngine->Shutdown();
	g_PhysicsEngine.Shutdown();
}

/*
==================
CBaseEngine::Tick
==================
*/
void CBaseEngine::Tick( float InDeltaSeconds )
{
	if ( g_World )
	{
		g_World->Tick( InDeltaSeconds );
	}
	g_UIEngine->Tick( InDeltaSeconds );
	g_PhysicsEngine.Tick( InDeltaSeconds );
}

/*
==================
CBaseEngine::ProcessEvent
==================
*/
void CBaseEngine::ProcessEvent( struct WindowEvent& InWindowEvent )
{
	g_InputSystem->ProcessEvent( InWindowEvent );
	g_UIEngine->ProcessEvent( InWindowEvent );
	g_CameraManager->ProcessEvent( InWindowEvent );
}

/*
==================
CBaseEngine::GetMaxTickRate
==================
*/
float CBaseEngine::GetMaxTickRate() const
{
	CConfigValue		configMaxTickRate = g_Config.GetValue( CT_Engine, TEXT( "Engine.Engine" ), TEXT( "MaxTickRate" ) );
	if ( configMaxTickRate.IsValid() )
	{
		return configMaxTickRate.GetNumber();
	}

	return 0.f;
}

/*
==================
CBaseEngine::LoadMap
==================
*/
bool CBaseEngine::LoadMap( const std::wstring& InMap, std::wstring& OutError )
{
	Logf( TEXT( "Load map: %s\n" ), InMap.c_str() );
	CObjectPackage*		mapPackage = CObjectPackage::LoadPackage( nullptr, InMap.c_str(), LOAD_None );
	if ( !mapPackage )
	{
		OutError = TEXT( "Failed to load map" );
		return false;
	}

	// Clean up and serialize world
	if ( g_World )
	{
		g_World->EndPlay();
		g_World->RemoveFromRoot();
		g_World = nullptr;
	}
	
	g_World = FindObjectFast<CWorld>( mapPackage, TEXT( "TheWorld" ), true );
	Assert( g_World );
	g_World->AddToRoot();

	// Call garbage collector of unused packages and assets for free used memory
	CObjectGC::Get().CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
	g_PackageManager->GarbageCollector();
	return true;
}

/*
==================
CBaseEngine::OverrideConfigurationFromCommandLine
==================
*/
void CBaseEngine::OverrideConfigurationFromCommandLine( uint32& InOutWindowWidth, uint32& InOutWindowHeight, bool& InOutFullscreen )
{
	// Check for windowed mode command line override
	if ( g_CommandLine.HasParam( TEXT( "windowed" ) ) || g_CommandLine.HasParam( TEXT( "window" ) ) )
	{
		InOutFullscreen = false;
	}
	// Check for fullscreen override
	else if ( g_CommandLine.HasParam( TEXT( "full" ) ) || g_CommandLine.HasParam( TEXT( "fullscreen" ) ) )
	{
		InOutFullscreen = true;
	}

	// Get width
	const tchar*	widthParam = nullptr;
	if ( g_CommandLine.HasParam( TEXT( "width" ) ) )
	{
		widthParam = TEXT( "width" );
	}
	else if ( g_CommandLine.HasParam( TEXT( "w" ) ) )
	{
		widthParam = TEXT( "w" );
	}

	// Override width
	if ( widthParam )
	{
		InOutWindowWidth = L_Atoi( g_CommandLine.GetFirstValue( widthParam ).c_str() );
	}

	// Get height
	const tchar*	heightParam = nullptr;
	if ( g_CommandLine.HasParam( TEXT( "height" ) ) )
	{
		heightParam = TEXT( "height" );
	}
	else if ( g_CommandLine.HasParam( TEXT( "h" ) ) )
	{
		heightParam = TEXT( "h" );
	}

	// Override height
	if ( heightParam )
	{
		InOutWindowHeight = L_Atoi( g_CommandLine.GetFirstValue( heightParam ).c_str() );
	}
}