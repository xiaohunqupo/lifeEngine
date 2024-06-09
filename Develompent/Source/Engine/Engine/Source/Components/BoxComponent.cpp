#include "Components/BoxComponent.h"
#include "Render/Scene.h"
#include "Render/SceneUtils.h"
#include "System/World.h"
#include "Misc/EngineGlobals.h"

IMPLEMENT_CLASS( CBoxComponent )

/*
==================
CBoxComponent::CBoxComponent
==================
*/
CBoxComponent::CBoxComponent()
	: size( 1.f, 1.f, 1.f )
{
	if ( g_IsEditor )
	{
		SetVisibility( true );
	}
	bodyInstance.SetSimulatePhysics( true );
}

/*
==================
CBoxComponent::StaticInitializeClass
==================
*/
void CBoxComponent::StaticInitializeClass()
{
	// Native properties
	new( staticClass, TEXT( "Size" ), OBJECT_Public ) CVectorProperty( CPP_PROPERTY( ThisClass, size ), TEXT( "Primitive" ), TEXT( "Set size of box" ), CPF_Edit, 1 );
}

#if WITH_EDITOR
/*
==================
CBoxComponent::DrawDebugComponent
==================
*/
void CBoxComponent::DrawDebugComponent()
{
	boundbox = CBox::BuildAABB( GetComponentLocation() + size / 2.f, size / 2.f );
	DrawWireframeBox( ( ( CScene* )g_World->GetScene() )->GetSDG( SDG_WorldEdForeground ), boundbox, DEC_COLLISION );
}
#endif // WITH_EDITOR

/*
==================
CBoxComponent::UpdateBodySetup
==================
*/
void CBoxComponent::UpdateBodySetup()
{
	bodySetup = new CPhysicsBodySetup();

	PhysicsBoxGeometry				boxGeometry( size.x, size.y, size.z );
	boxGeometry.collisionProfile	= collisionProfile;
	boxGeometry.material			= physicsMaterial;
	bodySetup->AddBoxGeometry( boxGeometry );
}