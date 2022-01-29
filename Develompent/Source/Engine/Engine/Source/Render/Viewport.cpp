#include "Misc/EngineGlobals.h"
#include "RHI/BaseRHI.h"
#include "RHI/BaseDeviceContextRHI.h"
#include "RHI/BaseViewportRHI.h"
#include "Render/RenderingThread.h"
#include "Render/Viewport.h"
#include "Render/SceneRenderTargets.h"
#include "Render/SceneRendering.h"


FViewport::FViewport() :
	windowHandle( nullptr ),
	sizeX( 0 ),
	sizeY( 0 )
{}

FViewport::~FViewport()
{}

// BS yehor.pohuliaka - This code for test, need delete it
#include "Render/Shaders/BasePassShader.h"
#include "Render/Texture.h"
#include "System/Archive.h"
#include "System/BaseFileSystem.h"
#include "Misc/CoreGlobals.h"
#include "System/World.h"
#include "Misc/Misc.h"
#include "Components/CameraComponent.h"
#include "Render/Scene.h"
#include "System/InputSystem.h"
#include "Render/Material.h"
#include "Render/SceneRendering.h"
#include "System/Package.h"
#include "Render/StaticMesh.h"
#include "Render/SceneUtils.h"
#include "Containers/String.h"

extern LCameraComponent*				cameraComponent;

void FViewport::InitRHI()
{
	// If viewport already created - we need resize
	if ( viewportRHI )
	{
		viewportRHI->Resize( sizeX, sizeY );
	}
	// Else we need create it
	else
	{
		viewportRHI = GRHI->CreateViewport( windowHandle, sizeX, sizeY );
	}

	GSceneRenderTargets.Allocate( sizeX, sizeY );

	/*FPackage		pak;
	pak.Open( TEXT( "Content/Tiger.lpak" ), true );

	{
		FTexture2DRef		texture2D = GPackageManager->FindAsset( TEXT( "Content/Tiger.lpak" ), appCalcHash( TEXT( "TankBody_C" ) ) );
		FMaterialRef		material = new FMaterial();
		material->SetShader( FBasePassVertexShader::staticType );
		material->SetShader( FBasePassPixelShader::staticType );
		material->UsageOnStaticMesh( true );
		material->SetTextureParameterValue( TEXT( "diffuse" ), texture2D );
		material->SetAssetName( TEXT( "TankBody_Mat" ) );
		material->SetAssetHash( appCalcHash( TEXT( "TankBody_Mat" ) ) );
		staticMesh->SetMaterial( 1, material );
		pak.Add( texture2D );
		pak.Add( material );
	}

	{
		FTexture2DRef		texture2D = GPackageManager->FindAsset( TEXT( "Content/Tiger.lpak" ), appCalcHash( TEXT( "TigerMG_C" ) ) );
		FMaterialRef		material = new FMaterial();
		material->SetShader( FBasePassVertexShader::staticType );
		material->SetShader( FBasePassPixelShader::staticType );
		material->UsageOnStaticMesh( true );
		material->SetTextureParameterValue( TEXT( "diffuse" ), texture2D );
		material->SetAssetName( TEXT( "TigerMG_Mat" ) );
		material->SetAssetHash( appCalcHash( TEXT( "TigerMG_Mat" ) ) );
		staticMesh->SetMaterial( 0, material );
		pak.Add( texture2D );
		pak.Add( material );
	}

	{
		FTexture2DRef		texture2D = GPackageManager->FindAsset( TEXT( "Content/Tiger.lpak" ), appCalcHash( TEXT( "TigerTrack_C" ) ) );
		FMaterialRef		material = new FMaterial();
		material->SetShader( FBasePassVertexShader::staticType );
		material->SetShader( FBasePassPixelShader::staticType );
		material->UsageOnStaticMesh( true );
		material->SetTextureParameterValue( TEXT( "diffuse" ), texture2D );
		material->SetAssetName( TEXT( "TigerTrack_Mat" ) );
		material->SetAssetHash( appCalcHash( TEXT( "TigerTrack_Mat" ) ) );
		staticMesh->SetMaterial( 2, material );
		pak.Add( texture2D );
		pak.Add( material );
	}

	pak.Add( staticMesh );
	pak.Serialize();*/
}

void FViewport::ReleaseRHI()
{
	viewportRHI.SafeRelease();
}

void FViewport::Update( bool InIsDestroyed, uint32 InNewSizeX, uint32 InNewSizeY, void* InNewWindowHandle )
{
	bool	isNewWindowHandle = windowHandle != InNewWindowHandle;

	// Update the viewport attributes
	windowHandle = InNewWindowHandle;
	sizeX = InNewSizeX;
	sizeY = InNewSizeY;

#if DO_CHECK
	if ( !InIsDestroyed )
	{
		check( windowHandle );
	}
#endif // DO_CHECK

	// Don't reinitialize the viewport RHI if the viewport has been destroyed
	if ( InIsDestroyed || isNewWindowHandle )
	{	
		BeginReleaseResource( this );

		if ( InIsDestroyed )
		{
			return;
		}
	}

	// Initialize the viewport's resources if viewport RHI not created
	if ( !viewportRHI )
	{
		BeginInitResource( this );
	}
	// Else update resource
	else
	{
		BeginUpdateResource( this );
	}
}

void FViewport::Draw( bool InIsShouldPresent /* = true */ )
{
	if ( !IsInitialized() )
	{
		return;
	}

	FSceneView*		sceneView = new FSceneView();
	if ( cameraComponent )
	{
		sceneView->SetCameraView( cameraComponent );
	}

	struct Helper
	{
		static void Execute( FViewportRHIRef viewportRHI, FSceneView* sceneView, bool isShouldPresent )
		{
			FBaseDeviceContextRHI*		immediateContext = GRHI->GetImmediateContext();
			FSceneRenderer				sceneRenderer( sceneView );
			
			GRHI->BeginDrawingViewport( immediateContext, viewportRHI );
			sceneRenderer.Render( viewportRHI );
			GRHI->EndDrawingViewport( immediateContext, viewportRHI, isShouldPresent, false );
			
			delete sceneView;
		}
	};

	UNIQUE_RENDER_COMMAND_THREEPARAMETER( FQRenderCommand,
										  FViewportRHIRef, viewportRHI, viewportRHI,
										  FSceneView*, sceneView, sceneView,
										  bool, isShouldPresent, InIsShouldPresent,
										  {
											  Helper::Execute( viewportRHI, sceneView, isShouldPresent );
										  } );
}