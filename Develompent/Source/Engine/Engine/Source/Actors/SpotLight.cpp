#include "Actors/SpotLight.h"

IMPLEMENT_CLASS( ASpotLight )

/*
==================
ASpotLight::ASpotLight
==================
*/
ASpotLight::ASpotLight()
{
	spotLightComponent	= CreateComponent<CSpotLightComponent>( TEXT( "SpotLightComponent0" ) );

#if WITH_EDITOR
	gizmoComponent		= CreateComponent<CSpriteComponent>( TEXT( "GizmoComponent0" ), true );
	gizmoComponent->SetGizmo( true );
	gizmoComponent->SetType( ST_Rotating );
	gizmoComponent->SetSpriteSize( Vector2D( 64.f, 64.f ) );
	gizmoComponent->SetMaterial( g_PackageManager->FindAsset( TEXT( "Material'EditorMaterials:ASpotLight_Gizmo_Mat" ), AT_Material ) );

	arrowComponent		= CreateComponent<CArrowComponent>( TEXT( "ArrowComponent0" ), true );
#endif // WITH_EDITOR
}

/*
==================
ASpotLight::StaticInitializeClass
==================
*/
void ASpotLight::StaticInitializeClass()
{
	// Native properties
	new( staticClass, TEXT( "Spot Light Component" ) ) CObjectProperty( CPP_PROPERTY( ThisClass, spotLightComponent ), TEXT( "Light" ), TEXT( "Spot light component" ), CPF_Edit, CSpotLightComponent::StaticClass() );
	new( staticClass, NAME_None ) CObjectProperty( CPP_PROPERTY( ThisClass, gizmoComponent ), NAME_None, TEXT( "" ), CPF_None, CSpriteComponent::StaticClass() );
	new( staticClass, NAME_None ) CObjectProperty( CPP_PROPERTY( ThisClass, arrowComponent ), NAME_None, TEXT( "" ), CPF_None, CArrowComponent::StaticClass() );
}

#if WITH_EDITOR
/*
==================
ASpotLight::GetActorIcon
==================
*/
std::wstring ASpotLight::GetActorIcon() const
{
	return TEXT( "Engine/Editor/Icons/Actor_SpotLight.png" );
}
#endif // WITH_EDITOR