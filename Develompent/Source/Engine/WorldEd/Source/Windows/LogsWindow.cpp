#include "Misc/StringConv.h"
#include "Misc/EngineGlobals.h"
#include "Misc/UIGlobals.h"
#include "ImGUI/ImGUIEngine.h"
#include "System/Cvar.h"
#include "Windows/LogsWindow.h"
#include "ImGUI/imgui_stdlib.h"

/*
==================
CLogsWindow::CLogsWindow
==================
*/
CLogsWindow::CLogsWindow( const std::wstring& InName )
	: CImGUILayer( InName )
{}

/*
==================
CLogsWindow::OnTick
==================
*/
void CLogsWindow::OnTick()
{
	// Draw log history
	{
		// Reserve enough left-over height for 1 separator + 1 input text
		const float		footerHeightToReserve	= ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild( "##ScrollingRegion", ImVec2( 0, -footerHeightToReserve ), false, ImGuiWindowFlags_HorizontalScrollbar );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4.f, 1.f ) );			// Tighten spacing
		
		for ( uint32 index = 0, count = history.size(); index < count; ++index )
		{
			bool				bHasColor = false;
			const LogInfo&		logInfo = history[index];
			switch ( logInfo.type )
			{
			case LT_Warning:
				ImGui::PushStyleColor( ImGuiCol_Text, g_ImGUIEngine->GetStyleColor( IGC_WarningColor ) );
				bHasColor = true;
				break;

			case LT_Error:
				ImGui::PushStyleColor( ImGuiCol_Text, g_ImGUIEngine->GetStyleColor( IGC_ErrorColor ) );
				bHasColor = true;
				break;
			}

			ImGui::TextUnformatted( logInfo.message.c_str() );
			
			if ( bHasColor )
			{
				ImGui::PopStyleColor();
			}
		}

		if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
		{
			ImGui::SetScrollHereY( 1.0f );
		}

		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		// Command-line
		bool	bReclaimFocus		= false;
		float	panelWidth			= ImGui::GetContentRegionAvail().x;
		float	inputCommandWidth	= panelWidth / 1.15f;
		ImGui::PushItemWidth( inputCommandWidth );
		if ( ImGui::InputTextWithHint( "##InputCommand", "Console Command", &commandBuffer, ImGuiInputTextFlags_EnterReturnsTrue ) )
		{
			if ( !commandBuffer.empty() )
			{
				ExecCommand( commandBuffer );
				commandBuffer.clear();
			}
			bReclaimFocus = true;
		}
		ImGui::PopItemWidth();

		// Auto-focus on window apparition
		ImGui::SetItemDefaultFocus();
		if ( bReclaimFocus )
		{
			ImGui::SetKeyboardFocusHere( -1 );		// Auto focus previous widget
		}

		ImGui::SameLine();
		if ( ImGui::Button( "Enter", ImVec2( panelWidth - inputCommandWidth, 0.f ) ) )
		{
			ExecCommand( commandBuffer );
			commandBuffer.clear();
		}
	}
}

/*
==================
CLogsWindow::ExecCommand
==================
*/
void CLogsWindow::ExecCommand( const std::string& InCommand )
{
	if ( !InCommand.empty() )
	{
		std::wstring	command = ANSI_TO_TCHAR( InCommand.c_str() );
		Logf( TEXT( "Cmd: %s\n" ), command.c_str() );
		g_Cvar.Exec( command.c_str() );
	}
}