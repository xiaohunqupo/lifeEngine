#include "Misc/StringConv.h"
#include "Misc/EngineGlobals.h"
#include "Misc/UIGlobals.h"
#include "System/MemoryArchive.h"
#include "System/World.h"
#include "System/InputSystem.h"
#include "System/ObjectExporter.h"
#include "Windows/ExplorerLevelWindow.h"
#include "Windows/InputTextDialog.h"
#include "Windows/DialogWindow.h"
#include "ImGUI/ImGUIEngine.h"

/**
  * @ingroup WorldEd
  * @brief Runnable object for rename actor on level
  */
class CRenameActorRunnable : public CRunnable
{
public:
	/**
	 * @brief Constructor
	 *
	 * @param InOwner	Owner
	 * @param InActor	Actor to rename
	 */
	CRenameActorRunnable( CExplorerLevelWindow* InOwner, AActor* InActor )
		: owner( InOwner )
		, actor( InActor )
	{}

	/**
	 * @brief Initialize
	 *
	 * Allows per runnable object initialization. NOTE: This is called in the
	 * context of the thread object that aggregates this, not the thread that
	 * passes this runnable to a new thread.
	 *
	 * @return True if initialization was successful, false otherwise
	 */
	virtual bool Init() override
	{
		Assert( owner && actor );
		return true;
	}

	/**
	 * @brief Run
	 *
	 * This is where all per object thread work is done. This is only called
	 * if the initialization was successful.
	 *
	 * @return The exit code of the runnable object
	 */
	virtual uint32 Run() override
	{
		// Get new actor name and Assert on exist another actor with this name
		bool			bIsOk = false;
		std::wstring	newActorName;
		while ( !bIsOk )
		{
			// Get new actor name. If we not press 'ok' nothing apply and exit from method
			{
				TSharedPtr<CInputTextDialog>	popup = owner->OpenPopup<CInputTextDialog>( TEXT( "Enter" ), TEXT( "New Actor Name" ), actor->GetName() );
				popup->OnTextEntered().Add( [&]( const std::string& InText )
											{
												bIsOk = true;
												newActorName = ANSI_TO_TCHAR( InText.c_str() );
												eventResponse.Trigger();
											} );

				popup->OnCenceled().Add( [&]()
										 {
											 bIsOk = false;
											 eventResponse.Trigger();
										 } );
				eventResponse.Wait();
				if ( !bIsOk )
				{
					return 0;
				}
			}

			// If actor with new name already exist - try enter another name 
			if ( !actor->Rename( newActorName.c_str(), nullptr, REN_Test ) )
			{
				TSharedPtr<CDialogWindow>	popup = owner->OpenPopup<CDialogWindow>( TEXT( "Error" ), L_Sprintf( TEXT( "Name '%s' already exist in the world" ), newActorName.c_str() ), CDialogWindow::BT_Ok );
				popup->OnButtonPressed().Add( [&]( CDialogWindow::EButtonType InButtonType )
											  {
												  eventResponse.Trigger();
											  } );
				bIsOk = false;
				eventResponse.Wait();
			}
		}

		// Add change actor name
		actor->Rename( newActorName.c_str() );

		// Mark the world's package as dirty
		g_World->MarkPackageDirty();
		return 0;
	}

	/**
	 * @brief Stop
	 *
	 * This is called if a thread is requested to terminate early
	 */
	virtual void Stop() override
	{}

	/**
	 * @brief Exit
	 *
	 * Called in the context of the aggregating thread to perform any cleanup.
	 */
	virtual void Exit() override
	{}

private:
	CExplorerLevelWindow*	owner;			/**< Owner */
	CEvent					eventResponse;	/**< Event used when opened popup */
	AActor*					actor;			/**< Actor to rename */
};

/*
==================
CExplorerLevelWindow::CExplorerLevelWindow
==================
*/
CExplorerLevelWindow::CExplorerLevelWindow( const std::wstring& InName )
	: CImGUILayer( InName )
{}

/*
==================
CExplorerLevelWindow::OnTick
==================
*/
void CExplorerLevelWindow::OnTick()
{
	// Draw table of actors
	if ( ImGui::BeginTable( "##ExploerLevelTable", 3, ImGuiTableFlags_Resizable ) )
	{
		// Init header table
		ImGui::TableSetupColumn( "Visibility" );
		ImGui::TableSetupColumn( "Label" );
		ImGui::TableSetupColumn( "Class" );
		ImGui::TableHeadersRow();

		// Init style
		ImGui::PushStyleVar( ImGuiStyleVar_FramePadding,	ImVec2( ImGui::GetStyle().FramePadding.x, 2.f ) );
		ImGui::PushStyleColor( ImGuiCol_Header,				g_ImGUIEngine->GetStyleColor( IGC_Selection ) );

		// Print info about each actor on level
		for ( uint32 index = 0; index < g_World->GetNumActors(); ++index )
		{
			AActor*			actor = g_World->GetActor( index );
			Assert( actor );
			bool			bNewVisibility = actor->IsVisibility();

			ImGui::TableNextRow();
			ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, !( index % 2 ) ? ImGui::ColorConvertFloat4ToU32( g_ImGUIEngine->GetStyleColor( IGC_TableBgColor0 ) ) : ImGui::ColorConvertFloat4ToU32( g_ImGUIEngine->GetStyleColor( IGC_TableBgColor1 ) ) );
			
			// Visibility flag
			ImGui::TableNextColumn();
			if ( ImGui::Checkbox( TCHAR_TO_ANSI( L_Sprintf( TEXT( "##Visibility_ID_%i" ), index ).c_str() ), &bNewVisibility ) )
			{
				actor->SetVisibility( bNewVisibility );
				g_World->MarkPackageDirty();
			}
			
			// Label
			ImGui::TableNextColumn();
			ImGui::Text( TCHAR_TO_ANSI( actor->GetName().c_str() ) );		

			// Class name
			ImGui::TableNextColumn();
			ImGui::Text( TCHAR_TO_ANSI( actor->GetClass()->GetName().c_str() ) );

			// Process events
			ProcessItemEvents( index, actor );
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::EndTable();
	}

	// Draw popup menu
	DrawPopupMenu();

	// Reset selection if clicked not on item
	if ( ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseDown( ImGuiMouseButton_Right ) ) )
	{
		g_World->UnselectAllActors();
	}
}

/*
==================
CExplorerLevelWindow::ProcessItemEvents
==================
*/
void CExplorerLevelWindow::ProcessItemEvents( uint32 InIndex, AActor* InActor )
{
	ImGui::SameLine();
	{
		const bool	bCtrlDown = g_InputSystem->IsKeyDown( BC_KeyLControl ) || g_InputSystem->IsKeyDown( BC_KeyRControl );
		const bool	bRMBDown = ImGui::IsMouseDown( ImGuiMouseButton_Right );
		bool		bNeedPopStyleColor = false;
		bool		bSelected = InActor->IsSelected();

		if ( bSelected )
		{
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, g_ImGUIEngine->GetStyleColor( IGC_Selection ) );
			bNeedPopStyleColor = true;
		}
		if ( ImGui::Selectable( TCHAR_TO_ANSI( L_Sprintf( TEXT( "##Selectable_ID_%i" ), InIndex ).c_str() ), &bSelected, ImGuiSelectableFlags_SpanAllColumns ) ||
			 ImGui::IsItemHovered() && bRMBDown )
		{
			bool	bUnselectedAllActors = false;
			if ( ( !bCtrlDown && !bRMBDown ) || ( bRMBDown && !bSelected ) )
			{
				g_World->UnselectAllActors();
				bUnselectedAllActors = true;
			}

			if ( bSelected || ( bUnselectedAllActors && !bCtrlDown ) || bRMBDown )
			{
				g_World->SelectActor( InActor );
			}
			else
			{
				g_World->UnselectActor( InActor );
			}
		}
		if ( bNeedPopStyleColor )
		{
			ImGui::PopStyleColor();
		}
	}
}

/*
==================
CExplorerLevelWindow::DrawPopupMenu
==================
*/
void CExplorerLevelWindow::DrawPopupMenu()
{
	if ( ImGui::BeginPopupContextWindow( "", ImGuiMouseButton_Right ) )
	{	
		bool	bSelectedActors			= g_World->GetNumSelectedActors() > 0;
		bool	bSelectedOnlyOneActor	= g_World->GetNumSelectedActors() == 1;

		// Copy
		if ( ImGui::MenuItem( "Copy", "", nullptr, bSelectedActors ) )
		{
			std::wstring		buffer;
			CObjectExporter		objectExporter;
			objectExporter.Export( ( const std::vector<CObject*>& )g_World->GetSelectedActors(), buffer );
			Sys_SetClipboardText( buffer );
		}

		// Paste
		if ( ImGui::MenuItem( "Paste", "", nullptr, bSelectedActors && !Sys_GetClipboardText().empty() ) )
		{
		}

		// Duplicate
		if ( ImGui::MenuItem( "Duplicate", "", nullptr, bSelectedActors ) )
		{		
			// Copy actor
			std::wstring		buffer;
			CObjectExporter		objectExporter;
			objectExporter.Export( ( const std::vector<CObject*>& )g_World->GetSelectedActors(), buffer );
		}

		// Delete
		if ( ImGui::MenuItem( "Delete", "", nullptr, bSelectedActors ) )
		{
			std::vector<AActor*>		selectedActors = g_World->GetSelectedActors();
			for ( uint32 index = 0, count = selectedActors.size(); index < count; ++index )
			{
				g_World->DestroyActor( selectedActors[index] );
			}
		}

		// Rename
		if ( ImGui::MenuItem( "Rename", "", nullptr, bSelectedOnlyOneActor ) )
		{
			const std::vector<AActor*>&		selectedActors = g_World->GetSelectedActors();
			CRunnableThread::Create( new CRenameActorRunnable( this, selectedActors[0] ), L_Sprintf( TEXT( "RenameActor_%s" ), selectedActors[0]->GetName() ).c_str(), true, true );
		}

		ImGui::EndPopup();
	}
}