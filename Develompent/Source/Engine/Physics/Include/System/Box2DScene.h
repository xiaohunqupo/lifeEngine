/**
 * @file
 * @addtogroup Physics Physics
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef BOX2DSSCENE_H
#define BOX2DSSCENE_H

#if WITH_BOX2D
#include <box2d/box2d.h>
#include <vector>

#include "Math/Math.h"
#include "Misc/PhysicsTypes.h"
#include "Misc/Platform.h"

/**
 * @ingroup Physics
 * @brief Class of PhysX scene
 */
class CBox2DScene
{
public:
	/**
	 * @brief Constructor
	 */
	CBox2DScene();

	/**
	 * @brief Destructor
	 */
	~CBox2DScene();

	/**
	 * @brief Initialize scene
	 */
	void Init();

	/**
	 * @brief Tick scene
	 * 
	 * @param InDeltaTime The time since the last tick
	 */
	void Tick( float InDeltaTime );

	/**
	 * @brief Shutdown scene
	 */
	void Shutdown();

	/**
	 * @brief Add body on scene
	 * @param InBodyInstance Pointer to body instance
	 */
	void AddBody( class CPhysicsBodyInstance* InBodyInstance );

	/**
	 * @brief Remove body from scene
	 * @param InBodyInstance Pointer to body instance
	 */
	void RemoveBody( class CPhysicsBodyInstance* InBodyInstance );

	/**
	 * @brief Remove all bodies from scene
	 */
	FORCEINLINE void RemoveAllBodies()
	{
		bodies.clear();
		for ( uint32 index = 0; index < CC_Max; ++index )
		{
			fixturesMap[ index ].clear();
		}
	}

	/**
	 * Trace a ray against the world using a specific channel and return the first blocking hit
	 *
	 * @param OutHitResult Hit result
	 * @param InStart Start ray
	 * @param InEnd End ray
	 * @param InTraceChannel Trace channel
	 * @param InCollisionQueryParams Collision query params
	 * @return Return TRUE if a blocking hit is found, else false
	 */
	bool LineTraceSingleByChannel( HitResult& OutHitResult, const Vector& InStart, const Vector& InEnd, ECollisionChannel InTraceChannel, const CollisionQueryParams& InCollisionQueryParams = CollisionQueryParams::defaultQueryParam );

	/**
	 * @brief Get Box2D world
	 * @return Return Box2D world
	 */
	FORCEINLINE b2World* GetBox2DWorld() const
	{
		return bx2World;
	}

private:
	b2World*																		bx2World;						/**< Box2D world */
	std::vector< class CPhysicsBodyInstance* >										bodies;							/**< Array of bodies on scene */
	std::unordered_map< class CPhysicsBodyInstance*, std::vector< b2Fixture* > >	fixturesMap[ CC_Max ];			/**< Map of fixtures each collision channel */
};
#endif // WITH_BOX2D

#endif // !BOX2DSSCENE_H