#include "Misc/EngineGlobals.h"
#include "System/InputSystem.h"
#include "Components/CameraComponent.h"
#include "EngineDefines.h"

IMPLEMENT_CLASS( CCameraComponent )

/*
==================
CCameraComponent::CCameraComponent
==================
*/
CCameraComponent::CCameraComponent()
    : bIsActive( false )
	, bAutoViewData( false )
    , projectionMode( CPM_Perspective )
    , fieldOfView( 90.f )
    , orthoWidth( 512.f )
    , orthoHeight( 512.f )
    , nearClipPlane( 0.01f )
    , farClipPlane( HALF_WORLD_MAX )
    , aspectRatio( 1.777778f )
{}

/*
==================
CCameraComponent::StaticInitializeClass
==================
*/
void CCameraComponent::StaticInitializeClass()
{
	new CBoolProperty( staticClass, TEXT( "bIsActive" ), TEXT( "Camera" ), TEXT( "Is active camera. This field setted from CCameraManager::SetActiveCamera" ), CPP_PROPERTY( bIsActive ), 0 );
	new CBoolProperty( staticClass, TEXT( "bAutoViewData" ), TEXT( "Camera" ), TEXT( "Auto view data. If this flag is true, CCameraManager auto set aspect ratio (for CPM_Perspective) or set ortho width/height (for CPM_Orthographic)" ), CPP_PROPERTY( bAutoViewData ), 0 );	
	// TODO BS yehor.pohuliaka - Need implemente CEnumProperty for enums
	//new CByteProperty( staticClass, TEXT( "projectionMode" ), TEXT( "Camera" ), TEXT( "Projection mode" ), CPP_PROPERTY( projectionMode ), 0 );
	new CFloatProperty( staticClass, TEXT( "fieldOfView" ), TEXT( "Camera" ), TEXT( "The horizontal field of view (in degrees) in perspective mode (ignored in Orthographic mode)" ), CPP_PROPERTY( fieldOfView ), 0 );
	new CFloatProperty( staticClass, TEXT( "orthoWidth" ), TEXT( "Camera" ), TEXT( "The desired width (in world units) of the orthographic view (ignored in Perspective mode)" ), CPP_PROPERTY( orthoWidth ), 0 );
	new CFloatProperty( staticClass, TEXT( "orthoHeight" ), TEXT( "Camera" ), TEXT( "The desired height (in world units) of the orthographic view (ignored in Perspective mode)" ), CPP_PROPERTY( orthoHeight ), 0 );
	new CFloatProperty( staticClass, TEXT( "nearClipPlane" ), TEXT( "Camera" ), TEXT( "The near plane distance (in world units)" ), CPP_PROPERTY( nearClipPlane ), 0 );
	new CFloatProperty( staticClass, TEXT( "farClipPlane" ), TEXT( "Camera" ), TEXT( "The far plane distance (in world units)" ), CPP_PROPERTY( farClipPlane ), 0 );
	new CFloatProperty( staticClass, TEXT( "aspectRatio" ), TEXT( "Camera" ), TEXT( "Aspect Ratio (Width/Height) (ignored in Orthographic mode)" ), CPP_PROPERTY( aspectRatio ), 0 );
}

/*
==================
CCameraComponent::Serialize
==================
*/
void CCameraComponent::Serialize( class CArchive& InArchive )
{
	Super::Serialize( InArchive );

	InArchive << bIsActive;
	InArchive << bAutoViewData;
	InArchive << projectionMode;
	InArchive << fieldOfView;
	InArchive << orthoWidth;
	InArchive << orthoHeight;
	InArchive << nearClipPlane;
	InArchive << farClipPlane;
	InArchive << aspectRatio;

	if ( nearClipPlane <= 0.f )
	{
		nearClipPlane = 0.01f;
	}
}

/*
==================
CCameraComponent::GetCameraView
==================
*/
void CCameraComponent::GetCameraView( SCameraView& OutDesiredView )
{
	OutDesiredView.location			= GetComponentLocation();
	OutDesiredView.rotation			= GetComponentRotation();
	OutDesiredView.aspectRatio		= aspectRatio;
	OutDesiredView.farClipPlane		= farClipPlane;
	OutDesiredView.fov				= fieldOfView;
	OutDesiredView.nearClipPlane	= nearClipPlane;
	OutDesiredView.orthoHeight		= orthoHeight;
	OutDesiredView.orthoWidth		= orthoWidth;
	OutDesiredView.projectionMode	= projectionMode;
}
