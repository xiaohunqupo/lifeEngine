#include "Misc/WorldEdGlobals.h"
#include "ImGUI/ImGUIEngine.h"
#include "Render/RenderingThread.h"
#include "Windows/WindowsManager.h"
#include "Core.h"
#include "WorldEd.h"

// Widgets
#include "Widgets/BaseWidget.h"
#include "Widgets/MainMenuBar.h"

// Windows
#include "Windows/WindowImportTexture.h"

FWorldEd::FWorldEd()
{}

FWorldEd::~FWorldEd()
{}

void FWorldEd::Init()
{
	GImGUIEngine->Init();
	InitUI();
}

void FWorldEd::InitUI()
{
	mainMenuBar.Init();

	// Add menu 'File'
	{
		WMainMenuBar::FMenu*		menu = new WMainMenuBar::FMenu( TEXT( "File" ) );
		menu->AddItem( new WMainMenuBar::FMenuItem( TEXT( "Exit" ), []() { GIsRequestingExit = true; } ) );
		mainMenuBar.AddMenu( menu );
	}

	// Add menu 'Tools'
	{
		WMainMenuBar::FMenu*		menu = new WMainMenuBar::FMenu( TEXT( "Tools" ) );
		menu->AddItem( new WMainMenuBar::FMenuItem( TEXT( "Import texture" ), []() 
					   {
						   WWindowImportTexture*		importTexture = new WWindowImportTexture();
						   importTexture->Init();

						   importTexture->OnWindowOpened().Add( []( class WBaseWindow* InWindow )
																 {
																	 check( InWindow );
																	 GWindowsManager->Add( InWindow );
																 } );
						   importTexture->OnWindowClosed().Add( []( class WBaseWindow* InWindow )
																 {
																	 check( InWindow );
																	 GWindowsManager->Remove( InWindow );
																 } );
						   importTexture->Open();
					   } ) );
		mainMenuBar.AddMenu( menu );
	}
}

void FWorldEd::ProcessEvent( struct SWindowEvent& InWindowEvent )
{
	GImGUIEngine->ProcessEvent( InWindowEvent );
}

void FWorldEd::Tick()
{
	GImGUIEngine->BeginDraw();
	mainMenuBar.Tick();

	GWindowsManager->Tick();
	GImGUIEngine->EndDraw();
}

void FWorldEd::Shutdown()
{
	GImGUIEngine->Shutdown();
}