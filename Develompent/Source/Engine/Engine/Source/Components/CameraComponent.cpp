#include "Misc/EngineGlobals.h"
#include "System/InputSystem.h"
#include "Components/CameraComponent.h"
#include "EngineDefines.h"

IMPLEMENT_CLASS( LCameraComponent )

LCameraComponent::LCameraComponent()
    : bIsActive( false )
    , projectionMode( CPM_Perspective )
    , fieldOfView( 90.f )
    , orthoWidth( 512.f )
    , orthoHeight( 512.f )
    , nearClipPlane( 0.01f )
    , farClipPlane( WORLD_MAX )
    , aspectRatio( 1.777778f )
{}

void LCameraComponent::RotateComponentByMouse( bool InConstrainYaw /* = true */ )
{	
	if ( bIsIgnoreRotateByMouse )
	{
		return;
	}

	FVector2D		mouseOffset = GInputSystem->GetMouseOffset();
	if ( mouseOffset.x == 0.f && mouseOffset.y == 0.f )
	{
		return;
	}

	float			sensitivity = GInputSystem->GetMouseSensitivity();
	FRotator		rotator = GetRelativeRotation();
	
	// Update Yaw axis
	if ( mouseOffset.x != 0.f )
	{
		rotator.yaw += mouseOffset.x * sensitivity;
		if ( rotator.yaw < -360.f || rotator.yaw > 360.f )
		{
			rotator.yaw = 0.f;
		}
	}

	// Update Pitch axis
	if ( mouseOffset.y != 0.f )
	{
		rotator.pitch -= mouseOffset.y * sensitivity;
		if ( InConstrainYaw )
		{
			if ( rotator.pitch > 90.f )
			{
				rotator.pitch = 90.f;
			}
			else if ( rotator.pitch < -90.f )
			{
				rotator.pitch = -90.f;
			}
		}
		else if ( rotator.pitch < -360.f || rotator.pitch > 360.f )
		{
			rotator.pitch = 0.f;
		}
	}

	SetRelativeRotation( rotator );
}

void LCameraComponent::Serialize( class FArchive& InArchive )
{
	Super::Serialize( InArchive );

	InArchive << bIsActive;
	InArchive << bIsIgnoreRotateByMouse;
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

void LCameraComponent::GetCameraView( FCameraView& OutDesiredView )
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
