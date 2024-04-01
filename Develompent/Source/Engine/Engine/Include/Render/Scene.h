/**
 * @file
 * @addtogroup Engine Engine
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <set>
#include <list>

#include "Math/Math.h"
#include "Math/Color.h"
#include "Render/CameraTypes.h"
#include "Render/Material.h"
#include "Render/SceneRendering.h"
#include "Render/SceneHitProxyRendering.h"
#include "Render/DepthRendering.h"
#include "Render/Frustum.h"
#include "Render/HitProxies.h"
#include "Render/BatchedSimpleElements.h"
#include "Render/RenderingThread.h"
#include "Render/DynamicMeshBuilder.h"
#include "Components/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/LightComponent.h"
#include "RHI/BaseRHI.h"
#include "RHI/BaseBufferRHI.h"
#include "RHI/TypesRHI.h"

/**
 * @ingroup Engine
 * Typedef of show flags
 */
typedef uint64		ShowFlags_t;

/**
 * @ingroup Engine
 * Enumeration of show flag
 */
enum EShowFlag
{
	// General and game flags
	SHOW_None				= 0,			/**< Nothig show */
	SHOW_Sprite				= 1 << 0,		/**< Sprite show */
	SHOW_StaticMesh			= 1 << 1,		/**< Static mesh show */
	SHOW_Wireframe			= 1 << 2,		/**< Show all geometry in wireframe mode */
	SHOW_SimpleElements		= 1 << 3,		/**< Show simple elements (only for debug and WorldEd) */
	SHOW_DynamicElements	= 1 << 4,		/**< Show dynamic elements */
	SHOW_HitProxy			= 1 << 5,		/**< Show hit proxy elements (only for WorldEd) */
	SHOW_Gizmo				= 1 << 6,		/**< Show gizmo elements (only for WorldEd) */
	SHOW_Lights				= 1 << 7,		/**< Show lights */
	SHOW_PostProcess		= 1 << 8,		/**< Show post process */

	SHOW_DefaultGame		= SHOW_Sprite | SHOW_StaticMesh | SHOW_SimpleElements | SHOW_DynamicElements | SHOW_Lights | SHOW_PostProcess,								/**< Default show flags for game */
	SHOW_DefaultEditor		= SHOW_Sprite | SHOW_StaticMesh | SHOW_SimpleElements | SHOW_DynamicElements | SHOW_HitProxy | SHOW_Gizmo | SHOW_Lights | SHOW_PostProcess	/**< Default show flags for editor */
};

/**
 * @ingroup Engine
 * A projection from scene space into a 2D screen region
 */
class CSceneView
{
public:
	/**
	 * Constructor
	 * 
	 * @param InPosition				Camera position
	 * @param InProjectionMatrix		Projection matrix
	 * @param InViewMatrix				View matrix
	 * @param InSizeX					Size of viewport by X
	 * @param InSizeY					Size of viewport by Y
	 * @param InBackgroundColor			Background color
	 * @param InShowFlags				Show flags
	 */
	CSceneView( const Vector& InPosition, const Matrix& InProjectionMatrix, const Matrix& InViewMatrix, float InSizeX, float InSizeY, const CColor& InBackgroundColor, ShowFlags_t InShowFlags );

	/**
	 * Screen to world space
	 * 
	 * @param InScreenPoint		Screen point
	 * @param OutWorldOrigin	Output origin of ray in world space
	 * @param OutWorldDirection	Output direction of ray in world space
	 */
	void ScreenToWorld( const Vector2D& InScreenPoint, Vector& OutWorldOrigin, Vector& OutWorldDirection ) const;

	/**
	 * World space to screen
	 * 
	 * @param InWorldPoint		World point
	 * @return Return point in screen space
	 */
	FORCEINLINE Vector4D WorldToScreen( const Vector& InWorldPoint ) const
	{
		return viewProjectionMatrix * Vector4D( InWorldPoint, 1.f );
	}

	/**
	 * Get view matrix
	 * @return Return view matrix
	 */
	FORCEINLINE const Matrix& GetViewMatrix() const
	{
		return viewMatrix;
	}

	/**
	 * Get projection matrix
	 * @return Return projection matrix
	 */
	FORCEINLINE const Matrix& GetProjectionMatrix() const
	{
		return projectionMatrix;
	}

	/**
	 * Get View * Projection matrix
	 * @return Return View * Projection matrix
	 */
	FORCEINLINE const Matrix& GetViewProjectionMatrix() const
	{
		return viewProjectionMatrix;
	}

	/**
	 * Get inverse View * Projection matrix
	 * @return Return inverse View * Projection matrix
	 */
	FORCEINLINE const Matrix& GetInvViewProjectionMatrix() const
	{
		return invViewProjectionMatrix;
	}

	/**
	 * Get frustum
	 * @return Return frustum
	 */
	FORCEINLINE const CFrustum& GetFrustum() const
	{
		return frustum;
	}

	/**
	 * Get background color
	 * @return Return background color
	 */
	FORCEINLINE const CColor& GetBackgroundColor() const
	{
		return backgroundColor;
	}

	/**
	 * Get show flags
	 * @return Return show flags
	 */
	FORCEINLINE ShowFlags_t GetShowFlags() const
	{
		return showFlags;
	}

	/**
	 * Get size of viewport by X
	 * @return Return size of viewport by X
	 */
	FORCEINLINE float GetSizeX() const
	{
		return sizeX;
	}

	/**
	 * Get size of viewport by Y
	 * @return Return size of viewport by Y
	 */
	FORCEINLINE float GetSizeY() const
	{
		return sizeY;
	}

	/**
	 * Get camera position
	 * @return Return camera position
	 */
	FORCEINLINE const Vector& GetPosition() const
	{
		return position;
	}

private:
	Matrix			viewMatrix;							/**< View matrix */
	Matrix			projectionMatrix;					/**< Projection matrix */
	Matrix			viewProjectionMatrix;				/**< View * Projection matrix */
	Matrix			invViewProjectionMatrix;			/**< Inverse View * Projection matrix */
	Vector			position;							/**< Camera position */
	CFrustum		frustum;							/**< Frustum */
	CColor			backgroundColor;					/**< Background color */
	ShowFlags_t		showFlags;							/**< Show flags for the view */
	float			sizeX;								/**< Size X of viewport */
	float			sizeY;								/**< Size Y of viewport */
};

/**
 * @ingroup Engine
 * Mesh instance of batch
 */
struct MeshInstance
{
	Matrix			transformMatrix;	/**< Transform matrix */

#if ENABLE_HITPROXY
	CHitProxyId		hitProxyId;			/**< Hit proxy id */
#endif // ENABLE_HITPROXY

#if WITH_EDITOR
	bool			bSelected;			/**< Is selected instance */
#endif // WITH_EDITOR
};

/**
 * @ingroup Engine
 * A batch of mesh elements, all with the same material and vertex buffer
 */
struct MeshBatch
{
	/**
	 * @brief Functions to extract the mesh batch from MeshBatch as a key for std::set
	 */
	struct MeshBatchKeyFunc
	{
		/**
		 * @brief Compare MeshBatch
		 *
		 * @param InA First mesh batch
		 * @param InB Second mesh batch
		 * @return Return true if InA and InB equal, else returning false
		 */
		FORCEINLINE bool operator()( const MeshBatch& InA, const MeshBatch& InB ) const
		{
			return InA.GetTypeHash() < InB.GetTypeHash();
		}

		/**
		 * @brief Calculate hash of the MeshBatch
		 *
		 * @param InMeshBatch Mesh batch
		 * @return Return hash of this MeshBatch
		 */
		FORCEINLINE std::size_t operator()( const MeshBatch& InMeshBatch ) const
		{
			return InMeshBatch.GetTypeHash();
		}
	};

	/**
	 * Constructor
	 */
	FORCEINLINE MeshBatch()
		: baseVertexIndex( 0 ), firstIndex( 0 ), numPrimitives( 0 ), numInstances( 0 )
	{}

	/**
	 * Overrload operator ==
	 */
	FORCEINLINE friend bool operator==( const MeshBatch& InA, const MeshBatch& InB )
	{
		return	InA.indexBufferRHI == InB.indexBufferRHI		&& 
				InA.primitiveType == InB.primitiveType			&& 
				InA.baseVertexIndex == InB.baseVertexIndex		&& 
				InA.firstIndex == InB.firstIndex				&& 
				InA.numPrimitives == InB.numPrimitives;
	}

	/**
	 * Overrload operator !=
	 */
	FORCEINLINE friend bool operator!=( const MeshBatch& InA, const MeshBatch& InB )
	{
		return !( InA == InB );
	}

	/**
	 * Get type hash
	 * @return Return calculated hash
	 */
	FORCEINLINE uint64 GetTypeHash() const
	{
		uint64		hash = FastHash( indexBufferRHI );
		hash = FastHash( primitiveType, hash );
		hash = FastHash( baseVertexIndex, hash );
		hash = FastHash( firstIndex, hash );
		hash = FastHash( numPrimitives, hash );
		return hash;
	}

	IndexBufferRHIRef_t							indexBufferRHI;		/**< Index buffer */
	EPrimitiveType								primitiveType;		/**< Primitive type */
	uint32										baseVertexIndex;	/**< First index vertex in vertex buffer */
	uint32										firstIndex;			/**< First index */
	uint32										numPrimitives;		/**< Number primitives to render */
	mutable uint32								numInstances;		/**< Number instances of mesh */
	mutable std::vector< MeshInstance >		instances;			/**< Array of mesh instances */
};

/**
 * @brief Typedef set of mesh batches
 */
typedef std::unordered_set< MeshBatch, MeshBatch::MeshBatchKeyFunc >		MeshBatchList_t;

/**
 * @ingroup Engine
 * @brief Draw list of scene for mesh type
 */
template< typename TDrawingPolicyType, bool InAllowWireframe = true >
class CMeshDrawList
{
public:
	/**
	 * @brief A set of draw list elements with the same drawing policy
	 */
	struct DrawingPolicyLink : public CRefCounted
	{
		/**
		 * @brief Constructor
		 * 
		 * @param InWireframeColor		Wireframe color
		 */
		DrawingPolicyLink( const CColor& InWireframeColor = CColor::red )
#if WITH_EDITOR
			: wireframeColor( InWireframeColor )
#endif // WITH_EDITOR
		{}

		/**
		 * @brief Constructor
		 * 
		 * @param InDrawingPolicy		Drawing policy
		 * @param InWireframeColor		Wireframe color
		 */
		DrawingPolicyLink( const TDrawingPolicyType& InDrawingPolicy, const CColor& InWireframeColor = CColor::red )
			: drawingPolicy( InDrawingPolicy )
#if WITH_EDITOR
			, wireframeColor( InWireframeColor )
#endif // WITH_EDITOR
		{}

		/**
		 * Overrload operator ==
		 */
		FORCEINLINE friend bool operator==( const DrawingPolicyLink& InA, const DrawingPolicyLink& InB )
		{
			return InA.drawingPolicy.GetTypeHash() == InB.drawingPolicy.GetTypeHash();
		}

		/**
		 * Overrload operator !=
		 */
		FORCEINLINE friend bool operator!=( const DrawingPolicyLink& InA, const DrawingPolicyLink& InB )
		{
			return !( InA == InB );
		}

		/**
		 * Get type hash
		 * @return Return calculated hash
		 */
		FORCEINLINE uint64 GetTypeHash() const
		{
			return drawingPolicy.GetTypeHash();
		}

		mutable MeshBatchList_t					meshBatchList;			/**< Mesh batch list */
		mutable TDrawingPolicyType				drawingPolicy;			/**< Drawing policy */

#if WITH_EDITOR
		CColor									wireframeColor;			/**< Wireframe color */
#endif // WITH_EDITOR
	};

	/**
	 * @brief Typedef of reference to drawing policy link
	 */
	typedef TRefCountPtr< DrawingPolicyLink >		DrawingPolicyLinkRef_t;

	/**
	 * @brief Functions to extract the drawing policy from DrawingPolicyLink_t as a key for std::set
	 */
	struct DrawingPolicyKeyFunc
	{
		/**
		 * @brief Compare DrawingPolicyLink_t
		 * 
		 * @param InA First drawing policy
		 * @param InB Second drawing policy
		 * @return Return true if InA and InB equal, else returning false
		 */
		FORCEINLINE bool operator()( const DrawingPolicyLinkRef_t& InA, const DrawingPolicyLinkRef_t& InB ) const
		{
			return InA->GetTypeHash() < InB->GetTypeHash();
		}

		/**
		 * @brief Calculate hash of the DrawingPolicyLink_t
		 *
		 * @param InDrawingPolicyLink Drawing policy link
		 * @return Return hash of this DrawingPolicyLink_t
		 */
		FORCEINLINE std::size_t operator()( const DrawingPolicyLinkRef_t& InDrawingPolicyLink ) const
		{
			return InDrawingPolicyLink->GetTypeHash();
		}
	};

	/**
	 * @brief Functions for custom equal the drawing policy in std::set 
	 */
	struct DrawingPolicyEqualFunc
	{
		/**
		  * @brief Compare DrawingPolicyLinkRef_t
		  *
		  * @param InA First drawing policy
		  * @param InB Second drawing policy
		  * @return Return true if InA and InB equal, else returning false
		  */
		FORCEINLINE bool operator()( const DrawingPolicyLinkRef_t& InA, const DrawingPolicyLinkRef_t& InB ) const
		{
			return ( *InA ) == ( *InB );
		}
	};

	/**
	 * @brief Typedef map of draw data
	 */
	typedef std::unordered_set< DrawingPolicyLinkRef_t, DrawingPolicyKeyFunc, DrawingPolicyEqualFunc >		MapDrawData_t;

	/**
	 * @brief Add item
	 * 
	 * @param InDrawingPolicyLink Drawing policy link
	 * @return Return reference to drawing policy link in SDG
	 */
	DrawingPolicyLinkRef_t AddItem( const DrawingPolicyLinkRef_t& InDrawingPolicyLink )
	{
		Assert( InDrawingPolicyLink );

		// Get drawing policy link in std::set
		MapDrawData_t::iterator		it = meshes.find( InDrawingPolicyLink );

		// If drawing policy link is not exist - we insert
		if ( it == meshes.end() )
		{
			it = meshes.insert( InDrawingPolicyLink ).first;
		}

		// Return drawing policy link in SDG
		return *it;
	}

	/**
	 * @brief Remove item
	 *
	 * @param InDrawingPolicyLink Drawing policy link
	 */
	void RemoveItem( DrawingPolicyLinkRef_t& InDrawingPolicyLink )
	{
		Assert( InDrawingPolicyLink );
		if ( InDrawingPolicyLink->GetRefCount() <= 2 )
		{
			meshes.erase( InDrawingPolicyLink );
		}

		InDrawingPolicyLink = nullptr;
	}

	/**
	 * @brief Clear draw list
	 */
	FORCEINLINE void Clear()
	{
		for ( MapDrawData_t::const_iterator it = meshes.begin(), itEnd = meshes.end(); it != itEnd; ++it )
		{
			DrawingPolicyLinkRef_t		drawingPolicyLink = *it;
			for ( MeshBatchList_t::const_iterator itMeshBatch = drawingPolicyLink->meshBatchList.begin(), itMeshBatchEnd = drawingPolicyLink->meshBatchList.end(); itMeshBatch != itMeshBatchEnd; ++itMeshBatch )
			{
				itMeshBatch->numInstances = 0;
				itMeshBatch->instances.clear();
			}
		}
	}

	/**
	 * @brief Get number items in list
	 * @return Return number items in list
	 */
	FORCEINLINE uint32 GetNum() const
	{
		return meshes.size();
	}

	/**
	 * @brief Draw list
	 * 
	 * @param[in] InDeviceContext Device context
	 * @param InSceneView Current view of scene
	 */
	FORCEINLINE void Draw( class CBaseDeviceContextRHI* InDeviceContext, const CSceneView& InSceneView )
	{
		Assert( IsInRenderingThread() );

		// Wireframe drawing available only with editor
#if WITH_EDITOR
		TWireframeMeshDrawingPolicy< TDrawingPolicyType >		wireframeDrawingPolicy;		// Drawing policy for wireframe mode
		bool													bWireframe = InAllowWireframe && ( InSceneView.GetShowFlags() & SHOW_Wireframe );
#endif // WITH_EDITOR

		for ( MapDrawData_t::const_iterator it = meshes.begin(), itEnd = meshes.end(); it != itEnd; ++it )
		{
			bool						bIsInitedRenderState	= false;
			DrawingPolicyLinkRef_t		drawingPolicyLink		= *it;
			CMeshDrawingPolicy*			drawingPolicy			= nullptr;

#if WITH_EDITOR
			drawingPolicy				= !bWireframe ? &drawingPolicyLink->drawingPolicy : &wireframeDrawingPolicy;

			// If we use wireframe drawing policy - init him
			if ( bWireframe )
			{
				wireframeDrawingPolicy.Init( drawingPolicyLink->drawingPolicy.GetVertexFactory(), drawingPolicyLink->wireframeColor, drawingPolicyLink->drawingPolicy.GetDepthBias() );
			}
#else
			drawingPolicy				= &drawingPolicyLink->drawingPolicy;
#endif // WITH_EDITOR

			// If drawing policy is not valid - skip meshes
			if ( !drawingPolicy->IsValid() )
			{
				continue;
			}

			// Draw all mesh batches
			for ( MeshBatchList_t::const_iterator itMeshBatch = drawingPolicyLink->meshBatchList.begin(), itMeshBatchEnd = drawingPolicyLink->meshBatchList.end(); itMeshBatch != itMeshBatchEnd; ++itMeshBatch )
			{
				// If in mesh batch not exist instance - continue to next step
				if ( itMeshBatch->numInstances <= 0 )
				{
					continue;
				}

				// If we not initialized render state - do it!
				if ( !bIsInitedRenderState )
				{					
					drawingPolicy->SetRenderState( InDeviceContext );
					drawingPolicy->SetShaderParameters( InDeviceContext );
					bIsInitedRenderState = true;
				}

				// Draw mesh batch
				drawingPolicy->Draw( InDeviceContext, *itMeshBatch, InSceneView );
			}
		}
	}

private:
	MapDrawData_t		meshes;						/**< Map of meshes sorted by materials for draw */
};

/**
 * @ingroup Engine
 * @brief Make drawing policy link and add him to scene
 * 
 * @param InVertexFactory	Vertex factory
 * @param InMaterial		Material
 * @param InMeshBatch		Mesh batch
 * @param OutMeshBatchLink	Output pointer to mesh batch in drawing policy link
 * @param InMeshDrawList	Mesh draw list
 * @param InWireframeColor	Wireframe color
 */
template<typename TDrawingPolicyLink, typename TMeshDrawList>
FORCEINLINE TRefCountPtr<TDrawingPolicyLink> MakeDrawingPolicyLink( CVertexFactory* InVertexFactory, const TAssetHandle<CMaterial>& InMaterial, const MeshBatch& InMeshBatch, const MeshBatch*& OutMeshBatchLink, TMeshDrawList& InMeshDrawList, const CColor& InWireframeColor = CColor::red )
{
	// Init new drawing policy link
	Assert( InVertexFactory );
	TRefCountPtr<TDrawingPolicyLink>		tmpDrawPolicyLink = new TDrawingPolicyLink( InWireframeColor );
	tmpDrawPolicyLink->drawingPolicy.Init( InVertexFactory, InMaterial );
	tmpDrawPolicyLink->meshBatchList.insert( InMeshBatch );

	// Add to scene new drawing policy link
	TRefCountPtr<TDrawingPolicyLink>		drawingPolicyLink = InMeshDrawList.AddItem( tmpDrawPolicyLink );
	Assert( drawingPolicyLink );

	// Get link to mesh batch. If not founded insert new
	MeshBatchList_t::iterator        itMeshBatchLink = drawingPolicyLink->meshBatchList.find( InMeshBatch );
	if ( itMeshBatchLink != drawingPolicyLink->meshBatchList.end() )
	{
		OutMeshBatchLink = &( *itMeshBatchLink );
	}
	else
	{
		OutMeshBatchLink = &( *drawingPolicyLink->meshBatchList.insert( InMeshBatch ).first );
	}

	return drawingPolicyLink;
}

#if WITH_EDITOR
/**
 * @ingroup Engine
 * @brief Struct of dynamic mesh builder element
 */
struct DynamicMeshBuilderElement
{
	DynamicMeshBuilderRef_t			dynamicMeshBuilder;		/**< Dynamic mesh builder */
	Matrix							localToWorldMatrix;		/**< Matrix local to world */
	TAssetHandle<CMaterial>			material;				/**< Material */
};
#endif // WITH_EDITOR

#if ENABLE_HITPROXY
/**
 * @ingroup Engine
 * @brief Struct of hit proxy layer on scene
 */
struct HitProxyLayer
{
	/**
	 * @brief Clear
	 */
	FORCEINLINE void Clear()
	{
#if WITH_EDITOR
		simpleHitProxyElements.Clear();
		dynamicHitProxyMeshBuilders.clear();
#endif // WITH_EDITOR

		hitProxyDrawList.Clear();
	}

	/**
	 * @brief Is empty
	 * @return Return TRUE if layer is empty, else return FALSE
	 */
	FORCEINLINE bool IsEmpty() const
	{
		return hitProxyDrawList.GetNum() <= 0
#if WITH_EDITOR
			&& simpleHitProxyElements.IsEmpty() && dynamicHitProxyMeshBuilders.empty()
#endif // WITH_EDITOR
			;
	}

#if WITH_EDITOR
	CBatchedSimpleElements							simpleHitProxyElements;			/**< Batched simple hit proxy elements (lines, points, etc) */
	std::list<DynamicMeshBuilderElement>			dynamicHitProxyMeshBuilders;	/**< List of dynamic hit proxy mesh builders */
#endif // WITH_EDITOR

	CMeshDrawList<CHitProxyDrawingPolicy, false>	hitProxyDrawList;				/**< Draw list of hit proxy */
};
#endif // ENABLE_HITPROXY

/**
 * @ingroup Engine
 * @brief Enumeration of scene depth group
 */
enum ESceneDepthGroup
{
#if !SHIPPING_BUILD
	SDG_WorldEdBackground,	/**< Background of viewport in WorldEd */
#endif // !SHIPPING_BUILD
	
	SDG_World,				/**< World */

#if !SHIPPING_BUILD
	SDG_WorldEdForeground,	/**< Foreground of viewport in WorldEd */
#endif // !SHIPPING_BUILD

	SDG_Foreground,			/**< Foreground of viewport for rendering UI */
	SDG_Max					/**< Num depth groups */
};

#if !SHIPPING_BUILD
/**
 * @ingroup Engine
 * @brief Get scene SDG name
 * @return Return name of scene SDG
 */
FORCEINLINE const tchar* GetSceneSDGName( ESceneDepthGroup SDG )
{
	switch ( SDG )
	{
	case SDG_WorldEdBackground:		return TEXT( "WorldEd Background" );
	case SDG_World:					return TEXT( "World" );
	case SDG_WorldEdForeground:		return TEXT( "WorldEd Foreground" );
	case SDG_Foreground:			return TEXT( "Foreground" );
	default:						return TEXT( "Unknown" );
	}
}
#endif // SHIPPING_BUILD

/**
 * @ingroup Engine
 * @brief Scene depth group
 */
struct SceneDepthGroup
{
	/**
	 * @brief Clear
	 */
	FORCEINLINE void Clear()
	{
#if WITH_EDITOR
		simpleElements.Clear();
		dynamicMeshBuilders.clear();
		gizmoDrawList.Clear();
#endif // WITH_EDITOR

		dynamicMeshElements.Clear();
		staticMeshDrawList.Clear();
		spriteDrawList.Clear();
		depthDrawList.Clear();

#if ENABLE_HITPROXY
		for ( uint32 index = 0; index < HPL_Num; ++index )
		{
			hitProxyLayers[ index ].Clear();
		}
#endif // ENABLE_HITPROXY
	}

	/**
	 * @brief Is empty
	 * @return Return TRUE if SDG is empty, else return FALSE
	 */
	FORCEINLINE bool IsEmpty() const
	{
#if ENABLE_HITPROXY
		bool		bEmptyHitProxyLayers = true;
		for ( uint32 index = 0; index < HPL_Num && bEmptyHitProxyLayers; ++index )
		{
			bEmptyHitProxyLayers = hitProxyLayers[ index ].IsEmpty();
		}
#endif // ENABLE_HITPROXY

		return staticMeshDrawList.GetNum() <= 0 && spriteDrawList.GetNum() <= 0 && dynamicMeshElements.GetNum() <= 0 && depthDrawList.GetNum() <= 0
#if WITH_EDITOR
			&& simpleElements.IsEmpty() && dynamicMeshBuilders.empty() && gizmoDrawList.GetNum() <= 0
#endif // WITH_EDITOR

#if ENABLE_HITPROXY
			&& bEmptyHitProxyLayers
#endif // ENABLE_HITPROXY
			;
	}

	// Simple elements use only for debug and WorldEd
#if WITH_EDITOR
	CBatchedSimpleElements									simpleElements;				/**< Batched simple elements (lines, points, etc) */
	std::list<DynamicMeshBuilderElement>					dynamicMeshBuilders;		/**< List of dynamic mesh builders */
	CMeshDrawList<CMeshDrawingPolicy, false>				gizmoDrawList;				/**< Draw list of gizmos */
#endif // WITH_EDITOR

	CMeshDrawList<CMeshDrawingPolicy>						dynamicMeshElements;		/**< Draw list of dynamic meshes */
	CMeshDrawList<CMeshDrawingPolicy>						staticMeshDrawList;			/**< Draw list of static meshes */
	CMeshDrawList<CMeshDrawingPolicy>						spriteDrawList;				/**< Draw list of sprites */
	CMeshDrawList<CDepthDrawingPolicy>						depthDrawList;				/**< Draw list all of geometry for PrePass */

#if ENABLE_HITPROXY
	HitProxyLayer											hitProxyLayers[ HPL_Num ];	/**< Hit proxy layers */
#endif // ENABLE_HITPROXY
};

/**
 * @ingroup Engine
 * @brief Base implementation of the scene manager
 */
class CBaseScene
{
public:
	/**
	 * @brief Destructor
	 */
	virtual ~CBaseScene() {}

	/**
	 * @brief Add new primitive component to scene
	 *
	 * @param InPrimitive Primitive component to add
	 */
	virtual void AddPrimitive( class CPrimitiveComponent* InPrimitive ) {}

	/**
	 * @brief Remove primitive component from scene
	 *
	 * @param InPrimitive Primitive component to remove
	 */
	virtual void RemovePrimitive( class CPrimitiveComponent* InPrimitive ) {}

	/**
	 * @brief Add new light component to scene
	 *
	 * @param InLight Light component to add
	 */
	virtual void AddLight( class CLightComponent* InLight ) {}

	/**
	 * @brief Remove light component from scene
	 *
	 * @param InLight Light component to remove
	 */
	virtual void RemoveLight( class CLightComponent* InLight ) {}

	/**
	 * @brief Clear scene
	 */
	virtual void Clear() {}

	/**
	 * @brief Build view for render scene from current view
	 * 
	 * @param InSceneView Current view of scene
	 */
	virtual void BuildView( const CSceneView& InSceneView ) {}

	/**
	 * @brief Clear all instances from scene frame
	 */
	virtual void ClearView() {}

	/**
	 * @brief Set current exposure of the scene
	 * @paran InExposure		New exposure
	 */
	virtual void SetExposure( float InExposure ) {};

	/**
	 * @brief Get current exposure of the scene
	 * @return Return exposure
	 */
	virtual float GetExposure() const { return g_Engine->GetExposure(); };
};

/**
 * @ingroup Engine
 * @brief Main of scene manager containing all primitive components
 */
class CScene : public CBaseScene
{
public:
	/**
	 * @brief Constructor
	 */
	CScene();

	/**
	 * @brief Destructor
	 */
	virtual ~CScene();

	/**
	 * @brief Add new primitive component to scene
	 *
	 * @param InPrimitive Primitive component to add
	 */
	virtual void AddPrimitive( class CPrimitiveComponent* InPrimitive ) override;

	/**
	 * @brief Remove primitive component from scene
	 *
	 * @param InPrimitive Primitive component to remove
	 */
	virtual void RemovePrimitive( class CPrimitiveComponent* InPrimitive ) override;

	/**
	 * @brief Add new light component to scene
	 *
	 * @param InLight Light component to add
	 */
	virtual void AddLight( class CLightComponent* InLight ) override;

	/**
	 * @brief Remove light component from scene
	 *
	 * @param InLight Light component to remove
	 */
	virtual void RemoveLight( class CLightComponent* InLight ) override;

	/**
	 * @brief Clear scene
	 */
	virtual void Clear() override;

	/**
	 * @brief Build view for render scene from current view
	 *
	 * @param InSceneView Current view of scene
	 */
	virtual void BuildView( const CSceneView& InSceneView ) override;

	/**
	 * @brief Clear all instances from scene frame
	 */
	virtual void ClearView() override;

	/**
	 * @brief Get depth group
	 * 
	 * @param InSDGType SDG type
	 * @return Return reference to depth group
	 */
	FORCEINLINE SceneDepthGroup& GetSDG( ESceneDepthGroup InSDGType )
	{
		Assert( InSDGType < SDG_Max );
		return frame.SDGs[ InSDGType ];
	}

	/**
	 * @brief Get depth group
	 *
	 * @param InSDGType SDG type
	 * @return Return reference to depth group
	 */
	FORCEINLINE const SceneDepthGroup& GetSDG( ESceneDepthGroup InSDGType ) const
	{
		Assert( InSDGType < SDG_Max );
		return frame.SDGs[ InSDGType ];
	}

	/**
	 * @brief Get list of visible lights on the current frame
	 * @return Return list of visible lights
	 */
	FORCEINLINE const std::list<CLightComponent*>& GetVisibleLights() const
	{
		return frame.visibleLights;
	}

	/**
	 * @brief Set current exposure of the scene
	 * @paran InExposure		New exposure
	 */
	virtual void SetExposure( float InExposure ) override;

	/**
	 * @brief Get current exposure of the scene
	 * @return Return exposure
	 */
	virtual float GetExposure() const override;

private:
	/**
	 * @brief One frame of the scene
	 */
	struct SceneFrame
	{
		SceneDepthGroup						SDGs[SDG_Max];		/**< Scene depth groups */
		std::list<CLightComponent*>			visibleLights;		/**< List of visible lights */
	};
	
	float									exposure;			/**< Current exposure of the scene */
	SceneFrame								frame;				/**< Scene frame */
	std::list<CPrimitiveComponent*>			primitives;			/**< List of primitives on scene */
	std::list<CLightComponent*>				lights;				/**< List of lights on scene */
};

//
// Serialization
//

FORCEINLINE CArchive& operator<<( CArchive& InArchive, ESceneDepthGroup& InValue )
{
	InArchive.Serialize( &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE CArchive& operator<<( CArchive& InArchive, const ESceneDepthGroup& InValue )
{
	Assert( InArchive.IsSaving() );
	InArchive.Serialize( ( void* )&InValue, sizeof( InValue ) );
	return InArchive;
}

#endif // !SCENE_H
