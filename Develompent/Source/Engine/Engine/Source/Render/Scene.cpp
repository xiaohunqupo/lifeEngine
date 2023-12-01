#include "Math/Math.h"
#include "Render/SceneRenderTargets.h"
#include "Render/Scene.h"
#include "System/ConVar.h"

#if WITH_EDITOR
/**
 * @ingroup Engine
 * @brief CVar enable/disable wireframe mode
 * @note This console variable is exist only with editor
 */
CConVar		CVarRWireframe( TEXT( "r.wireframe" ), TEXT( "0" ), CVT_Bool, TEXT( "Enable/Disable wireframe mode" ) );

/**
 * @ingroup Engine
 * @brief CVar enable/disable light pass
 * @note This console variable is exist only with editor
 */
CConVar		CVarRLight( TEXT( "r.light" ), TEXT( "1" ), CVT_Bool, TEXT( "Enable/Disable light pass" ) );

/**
 * @ingroup Engine
 * @brief CVar freeze rendering
 * @note This console variable is exist only with editor
 */
CConVar		CVarRFreezeRendering( TEXT( "r.freeze_rendering" ), TEXT( "0" ), CVT_Bool, TEXT( "Freeze rendering" ) );
#endif // WITH_EDITOR

/*
==================
CSceneView::CSceneView
==================
*/
CSceneView::CSceneView( const Vector& InPosition, const Matrix& InProjectionMatrix, const Matrix& InViewMatrix, float InSizeX, float InSizeY, const CColor& InBackgroundColor, ShowFlags_t InShowFlags )
	: viewMatrix( InViewMatrix )
	, projectionMatrix( InProjectionMatrix )
	, viewProjectionMatrix( InProjectionMatrix * InViewMatrix )
	, invViewProjectionMatrix( Math::matrixIdentity )
	, position( InPosition )
	, backgroundColor( InBackgroundColor )
	, showFlags( InShowFlags )
	, sizeX( InSizeX )
	, sizeY( InSizeY )
{
#if WITH_EDITOR
	if ( CVarRWireframe.GetValueBool() )
	{
		showFlags |= SHOW_Wireframe;
	}
	if ( !CVarRLight.GetValueBool() )
	{
		showFlags &= ~SHOW_Lights;
	}
#endif // WITH_EDITOR

	invViewProjectionMatrix = Math::InverseMatrix( InViewMatrix ) * Math::InverseMatrix( InProjectionMatrix );
	frustum.Update( viewProjectionMatrix );
}

/*
==================
CSceneView::ScreenToWorld
==================
*/
void CSceneView::ScreenToWorld( const Vector2D& InScreenPoint, Vector& OutWorldOrigin, Vector& OutWorldDirection ) const
{
	int32	x = Math::Trunc( InScreenPoint.x ),
			y = Math::Trunc( InScreenPoint.y );

	// Get the eye position and direction of the mouse cursor in two stages (inverse transform projection, then inverse transform view).
	// This avoids the numerical instability that occurs when a view matrix with large translation is composed with a projection matrix
	Matrix		inverseView			= Math::InverseMatrix( viewMatrix );
	Matrix		inverseProjection	= Math::InverseMatrix( projectionMatrix );

	// The start of the raytrace is defined to be at mousex,mousey,0 in projection space
	// The end of the raytrace is at mousex, mousey, 0.5 in projection space
	float		screenSpaceX				= ( x - sizeX / 2.f ) / ( sizeX / 2.f );
	float		screenSpaceY				= ( y - sizeY / 2.f ) / -( sizeY / 2.f );
	Vector4D	rayStartProjectionSpace		= Vector4D( screenSpaceX, screenSpaceY, 0, 1.0f );
	Vector4D	rayEndProjectionSpace		= Vector4D( screenSpaceX, screenSpaceY, 0.5f, 1.0f );

	// Projection (changing the W coordinate) is not handled by the Matrix transforms that work with vectors, so multiplications
	// by the projection matrix should use homogenous coordinates.
	Vector4D	hgRayStartViewSpace = inverseProjection * rayStartProjectionSpace;
	Vector4D	hgRayEndViewSpace	= inverseProjection * rayEndProjectionSpace;
	Vector		rayStartViewSpace( hgRayStartViewSpace.x, hgRayStartViewSpace.y, hgRayStartViewSpace.z );
	Vector		rayEndViewSpace( hgRayEndViewSpace.x, hgRayEndViewSpace.y, hgRayEndViewSpace.z );
	
	// Divide vectors by W to undo any projection and get the 3-space coordinate 
	if ( hgRayStartViewSpace.w != 0.0f )
	{
		rayStartViewSpace /= hgRayStartViewSpace.w;
	}
	if ( hgRayEndViewSpace.w != 0.0f )
	{
		rayEndViewSpace /= hgRayEndViewSpace.w;
	}

	Vector		rayDirViewSpace = rayEndViewSpace - rayStartViewSpace;
	rayDirViewSpace = Math::NormalizeVector( rayDirViewSpace );

	// The view transform does not have projection, so we can use the standard functions that deal with vectors and normals (normals
	// are vectors that do not use the translational part of a rotation/translation)
	Vector		rayStartWorldSpace	= inverseView * Vector4D( rayStartViewSpace, 1.f );
	Vector		rayDirWorldSpace	= inverseView * Vector4D( rayDirViewSpace, 0.f );

	// Finally, store the results in the hitcheck inputs.  The start position is the eye, and the end position
	// is the eye plus a long distance in the direction the mouse is pointing.
	OutWorldOrigin		= rayStartWorldSpace;
	OutWorldDirection	= Math::NormalizeVector( rayDirWorldSpace );
}


/*
==================
CScene::CScene
==================
*/
CScene::CScene()
	: exposure( 1.f )
{}

/*
==================
CScene::~CScene
==================
*/
CScene::~CScene()
{
	Clear();
}

/*
==================
CScene::AddPrimitive
==================
*/
void CScene::AddPrimitive( class CPrimitiveComponent* InPrimitive )
{
	Assert( InPrimitive );

	// If primitive already on scene
	if ( InPrimitive->scene == this )
	{
		return;
	}
	// Else if primitive on other scene - remove from old
	if ( InPrimitive->scene )
	{
		InPrimitive->scene->RemovePrimitive( InPrimitive );
	}

	InPrimitive->scene = this;
	InPrimitive->LinkDrawList();
	primitives.push_back( InPrimitive );
}

/*
==================
CScene::RemovePrimitive
==================
*/
void CScene::RemovePrimitive( class CPrimitiveComponent* InPrimitive )
{
	for ( auto it = primitives.begin(), itEnd = primitives.end(); it != itEnd; ++it )
	{
		if ( *it == InPrimitive )
		{
			InPrimitive->UnlinkDrawList();
			InPrimitive->scene = nullptr;
			primitives.erase( it );
			return;
		}
	}
}

/*
==================
CScene::AddLight
==================
*/
void CScene::AddLight( class CLightComponent* InLight )
{
	Assert( InLight );

	// If light already on scene
	if ( InLight->scene == this )
	{
		return;
	}
	// Else if light on other scene - remove from old
	if ( InLight->scene )
	{
		InLight->scene->RemoveLight( InLight );
	}

	InLight->scene = this;
	lights.push_back( InLight );
}

/*
==================
CScene::RemoveLight
==================
*/
void CScene::RemoveLight( class CLightComponent* InLight )
{
	for ( auto it = lights.begin(), itEnd = lights.end(); it != itEnd; ++it )
	{
		if ( *it == InLight )
		{
			InLight->scene = nullptr;
			lights.erase( it );
			return;
		}
	}
}

/*
==================
CScene::Clear
==================
*/
void CScene::Clear()
{
	for ( auto it = primitives.begin(), itEnd = primitives.end(); it != itEnd; ++it )
	{
		CPrimitiveComponent*		primitiveComponent = *it;
		primitiveComponent->UnlinkDrawList();
		primitiveComponent->scene = nullptr;
	}

	for ( auto it = lights.begin(), itEnd = lights.end(); it != itEnd; ++it )
	{
		CLightComponent*		lightComponent = *it;
		lightComponent->scene = nullptr;
	}

	primitives.clear();
	lights.clear();
	exposure = g_Engine ? g_Engine->GetExposure() : 1.f;
}

/*
==================
CScene::BuildView
==================
*/
void CScene::BuildView( const CSceneView& InSceneView )
{
	// We do nothing if r.freeze_rendering is true
#if WITH_EDITOR
	if ( CVarRFreezeRendering.GetValueBool() )
	{
		return;
	}
#endif // WITH_EDITOR

	// Add to SDGs visible primitives
	for ( auto it = primitives.begin(), itEnd = primitives.end(); it != itEnd; ++it )
	{
		CPrimitiveComponent*		primitiveComponent = *it;
		if ( primitiveComponent->IsVisibility() && InSceneView.GetFrustum().IsIn( primitiveComponent->GetBoundBox() ) )
		{
			primitiveComponent->AddToDrawList( InSceneView );

#if WITH_EDITOR
			if ( g_IsEditor )
			{
				primitiveComponent->DrawDebugComponent();
			}
#endif // WITH_EDITOR
		}
	}

	// Add to scene frame visible lights
	for ( auto it = lights.begin(), itEnd = lights.end(); it != itEnd; ++it )
	{
		CLightComponent*		lightComponent = *it;
		if ( lightComponent->IsEnabled() )
		{
			frame.visibleLights.push_back( lightComponent );

#if WITH_EDITOR
			if ( g_IsEditor )
			{
				lightComponent->DrawDebugComponent();
			}
#endif // WITH_EDITOR
		}
	}
}

/*
==================
CScene::ClearView
==================
*/
void CScene::ClearView()
{
	// We do nothing if r.freeze_rendering is true
#if WITH_EDITOR
	if ( CVarRFreezeRendering.GetValueBool() )
	{
		return;
	}
#endif // WITH_EDITOR

	// Clear all instances in scene depth groups
	for ( uint32 index = 0; index < SDG_Max; ++index )
	{
		frame.SDGs[ index ].Clear();
	}

	frame.visibleLights.clear();
}

/*
==================
CScene::SetExposure
==================
*/
void CScene::SetExposure( float InExposure )
{
	exposure = InExposure;
}

/*
==================
CScene::GetExposure
==================
*/
float CScene::GetExposure() const
{
	return exposure;
}