#include <thread>
#include "havoc.h"
#include "context.h"
#include "utils/math.h"
#include "utils/render.h"
#include "utils/crash_dumper.h"
#include "core/prop_manager.h"
#include "core/config.h"
#include "core/hooks.h"
#include "core/displacement.h"
#include "core/event_listener.h"
#include "features/misc/logger.h"
#include <tchar.h>

FILE* pStream;
void Entry( HMODULE hModule ) {
	SetUnhandledExceptionFilter( UnhandledExFilter );

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

	long long amongus = 0x69690004C201B0;
	for ( auto& sex : { CLIENT_DLL, ENGINE_DLL, STUDIORENDER_DLL, MATERIALSYSTEM_DLL } )
		WriteProcessMemory( GetCurrentProcess( ), ( LPVOID )MEM::FindPattern( sex, "55 8B EC 56 8B F1 33 C0 57 8B 7D 08" ), &amongus, 5, 0 );

	/*long long signature{ 0x69690004C201B0 };
	for ( auto mod : { _( "client.dll" ), _( "engine.dll" ), _( "server.dll" ), _( "studiorender.dll" ), _( "materialsystem.dll" ), _( "shaderapidx9.dll" ), _( "vstdlib.dll" ), _( "vguimatsurface.dll" ) } )
		WriteProcessMemory( GetCurrentProcess( ), 
			( LPVOID )MEM::FindPattern( mod, _( "55 8B EC 56 8B F1 33 C0 57 8B 7D 08" ) ), &signature, 
			7, 0 );*/

	const auto setupVelocityClamp{ MEM::FindPattern( CLIENT_DLL, _( "0F 2F 15 ? ? ? ? 0F 86 ? ? ? ? F3 0F 7E 4C 24" ) ) + 0x3 };
	auto p{ *reinterpret_cast< float** >( setupVelocityClamp ) };
	DWORD old{ };
	VirtualProtect( ( LPVOID )p, 1, PAGE_READWRITE, &old );
	*p = FLT_MAX;
	VirtualProtect( ( LPVOID )p, 1, old, &old );

	// paster!
	//_MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
	//_MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );

	SetPriorityClass( GetCurrentProcess( ), HIGH_PRIORITY_CLASS );

	// hook da funcs
	if ( !Hooks::Setup( ) )
		return;

	Features::Logger.Log( _( "Deployed Havoc." ), true );


	while ( !GetAsyncKeyState( VK_F11 ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

	// restore hooks
	Hooks::Restore( );

	EventListener.Destroy( );

	for ( int i{ 1 }; i <= 64; ++i ) {
		const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( player )
			player->m_bClientSideAnimation( ) = true;
	}

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