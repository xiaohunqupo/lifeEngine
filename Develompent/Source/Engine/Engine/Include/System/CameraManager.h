/**
 * @file
 * @addtogroup Engine Engine
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include "Misc/RefCountPtr.h"
#include "Components/CameraComponent.h"

/**
 * @ingroup Engine
 * @brief Manager cameras in engine
 */
class FCameraManager
{
public:
    /**
     * @brief Process event
     *
     * @param InWindowEvent Window event
     */
    void ProcessEvent( struct SWindowEvent& InWindowEvent );

    /**
     * @brief Set active camera for render frame
     * @param InCameraComponent Camera component
     */
    FORCEINLINE void SetActiveCamera( LCameraComponent* InCameraComponent )
    {
        if ( activeCamera )
        {
            activeCamera->bIsActive = false;
        }

        activeCamera = InCameraComponent;

        if ( activeCamera )
        {
            activeCamera->bIsActive = true;
        }
    }

    /**
     * @brief Get active camera
     * @return Return active camera. If not seted returing nullptr
     */
    FORCEINLINE TRefCountPtr< LCameraComponent > GetActiveCamera() const
    {
        return activeCamera;
    }

private:
    TRefCountPtr< LCameraComponent >        activeCamera;       /**< Current active camera */
};

#endif // !CAMERAMANAGER_H
