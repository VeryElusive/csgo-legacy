#include <thread>
#include "havoc.h"
#include "context.h"
#include "utils/math.h"
#include "utils/render.h"
#include "core/prop_manager.h"
#include "core/config.h"
#include "core/hooks.h"
#include "core/displacement.h"
#include "core/event_listener.h"
#include "features/misc/logger.h"
#include <tchar.h>

FILE* pStream;
void Entry( HMODULE hModule ) {
	while ( !MEM::GetModuleBaseHandle( SERVERBROWSER_DLL ) )
		std::this_thread::sleep_for( 200ms );

	Config::Setup( );

	// interfaces
	if ( !Interfaces::Setup( ) )
		return;

	Interfaces::GameConsole->Clear( );

	// netvar
	if ( !PropManager::Get( ).Create( ) )
		return;

	Offsets::Init( );

	// math exports
	if ( !Math::Setup( ) )
		return;

	Render::CreateFonts( );
	EventListener.Setup( { _( "bullet_impact" ), _( "round_start" ), _( "player_hurt" ),_( "weapon_fire" ), _( "player_death" ) } );

	// hook da funcs
	if ( !Hooks::Setup( ) )
		return;

	// How many real new commands have queued up
	auto MAX_NEW_COMMANDS{ MEM::FindPattern( ENGINE_DLL, _( "B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC" ) ) + 0x1 };
	DWORD old{ };

	VirtualProtect( ( void* )MAX_NEW_COMMANDS, sizeof( uint32_t ), PAGE_EXECUTE_READWRITE, &old );
	*( uint32_t* )MAX_NEW_COMMANDS = 62;
	VirtualProtect( ( void* )MAX_NEW_COMMANDS, sizeof( uint32_t ), old, &old );

	Features::Logger.Log( _( "Deployed Havoc." ), true );


	while ( true ) {
		if ( GetAsyncKeyState( VK_F11 ) )
			break;

		// doesn't need to be ran very often
		// but catching CHLClient::Shutdown is important
		std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
	}

	// restore hooks
	Hooks::Restore( );

	EventListener.Destroy( );

	FreeLibraryAndExitThread( hModule, EXIT_SUCCESS );
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD dwReason, LPVOID lpReserved )
{
	if ( dwReason == DLL_PROCESS_ATTACH ) {
		DisableThreadLibraryCalls( hModule );

		// welcome to artiehack/timhack/ETHEREAL/havoc AKA BEST HVH CHEAT
		std::unique_ptr<void, decltype( &CloseHandle )> thread(
			CreateThread( nullptr, 0u, LPTHREAD_START_ROUTINE( Entry ), hModule, 0u, nullptr ),
			&CloseHandle
		);

		return TRUE;
	}

	return FALSE;
}