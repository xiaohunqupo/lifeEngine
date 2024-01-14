#include "Render/Sphere.h"
#include "Render/SceneUtils.h"
#include "Components/SphereComponent.h"

IMPLEMENT_CLASS( CSphereComponent )

/*
==================
CSphereComponent::CSphereComponent
==================
*/
CSphereComponent::CSphereComponent()
	: radius( 0.f )
	, SDGLevel( SDG_World )
	, pendingSDGLevel( SDG_World )
{}

/*
==================
CSphereComponent::Serialize
==================
*/
void CSphereComponent::Serialize( class CArchive& InArchive )
{
	Super::Serialize( InArchive );
	InArchive << radius;
	InArchive << pendingSDGLevel;
	InArchive << material;
}

/*
==================
CSphereComponent::StaticInitializeClass
==================
*/
void CSphereComponent::StaticInitializeClass()
{
	new( staticClass, TEXT( "Radius" ), OBJECT_Public )		CFloatProperty( CPP_PROPERTY( ThisClass, radius ), TEXT( "Primitive" ), TEXT( "Radius of sphere" ), CPF_Edit );
	new( staticClass, TEXT( "Material" ), OBJECT_Public )	CAssetProperty( CPP_PROPERTY( ThisClass, material ), TEXT( "Display" ), TEXT( "Material" ), CPF_Edit, AT_Material );
}

#if WITH_EDITOR
/*
==================
CStaticMeshComponent::PostEditChangeProperty
==================
*/
void CSphereComponent::PostEditChangeProperty( const PropertyChangedEvenet& InPropertyChangedEvenet )
{
	CProperty*	changedProperty = InPropertyChangedEvenet.property;
	if ( changedProperty->GetCName() == TEXT( "Material" ) )
	{
		SetMaterial( material );
	}
	Super::PostEditChangeProperty( InPropertyChangedEvenet );
}
#endif // WITH_EDITOR

/*
==================
CSphereComponent::UpdateBodySetup
==================
*/
void CSphereComponent::UpdateBodySetup()
{}

/*
==================
CSphereComponent::AddToDrawList
==================
*/
void CSphereComponent::AddToDrawList( const class CSceneView& InSceneView )
{
	// If primitive is empty - exit from method
	if ( !bIsDirtyDrawingPolicyLink && meshBatchLinks.empty() )
	{
		return;
	}

	// If drawing policy link is dirty - we update it
	if ( bIsDirtyDrawingPolicyLink )
	{
		bIsDirtyDrawingPolicyLink = false;
		LinkDrawList();
	}

	// Add to mesh batch new instance
	CTransform				transform = GetComponentTransform();
	transform.SetScale( Vector( radius, radius, radius ) );

	for ( uint32 index = 0, count = meshBatchLinks.size(); index < count; ++index )
	{
		const MeshBatch*	meshBatchLink = meshBatchLinks[index];
		++meshBatchLink->numInstances;
		meshBatchLink->instances.push_back( MeshInstance{ transform.ToMatrix() } );
	}
}

/*
==================
CSphereComponent::LinkDrawList
==================
*/
void CSphereComponent::LinkDrawList()
{
	Assert( scene );

	// If the primitive already added to scene - remove all draw policy links
	if ( drawingPolicyLink )
	{
		UnlinkDrawList();
	}

	SDGLevel = pendingSDGLevel;
	SceneDepthGroup&				SDG = scene->GetSDG( SDGLevel );

	// Generate mesh batch of sprite
	MeshBatch			            meshBatch;
	meshBatch.baseVertexIndex		= 0;
	meshBatch.firstIndex			= 0;
	meshBatch.numPrimitives			= g_SphereMesh.GetNumPrimitives();
	meshBatch.indexBufferRHI		= g_SphereMesh.GetIndexBufferRHI();
	meshBatch.primitiveType			= PT_TriangleList;
	
	// Make and add to scene new draw policy link	
	const MeshBatch*				meshBatchLink = nullptr;
	drawingPolicyLink				= ::MakeDrawingPolicyLink<DrawingPolicyLink_t>( g_SphereMesh.GetVertexFactory(), material, meshBatch, meshBatchLink, SDG.dynamicMeshElements, DEC_DYNAMICELEMENTS );
	meshBatchLinks.push_back( meshBatchLink );

	// Make and add to scene new depth draw policy link
	depthDrawingPolicyLink			= ::MakeDrawingPolicyLink<DepthDrawingPolicyLink_t>( g_SphereMesh.GetVertexFactory(), material, meshBatch, meshBatchLink, SDG.depthDrawList, DEC_DYNAMICELEMENTS );
	meshBatchLinks.push_back( meshBatchLink );
}

/*
==================
CSphereComponent::UnlinkDrawList
==================
*/
void CSphereComponent::UnlinkDrawList()
{
	Assert( scene );
	SceneDepthGroup&	SDG = scene->GetSDG( SDGLevel );

	// If the primitive already added to scene - remove all draw policy links
	if ( drawingPolicyLink  )
	{
		SDG.dynamicMeshElements.RemoveItem( drawingPolicyLink );		
	}

	if ( depthDrawingPolicyLink )
	{
		SDG.depthDrawList.RemoveItem( depthDrawingPolicyLink );
	}

	meshBatchLinks.clear();
}