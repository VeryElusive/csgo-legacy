#include <thread>
#include "havoc.h"
#include "context.h"
#include "utils/math.h"
#include "utils/render.h"
#include "utils/threading/threading.h"
#include "core/prop_manager.h"
#include "core/config.h"
#include "core/hooks.h"
#include "core/displacement.h"
#include "core/event_listener.h"
#include "features/misc/logger.h"
#include <tchar.h>

using ThreadIDFn = int( _cdecl* )( );
ThreadIDFn AllocateThreadID;
ThreadIDFn FreeThreadID;

static Semaphore dispatchSem;
static SharedMutex smtx;

int AllocateThreadIDWrapper( ) {
	return AllocateThreadID( );
}

int FreeThreadIDWrapper( ) {
	return FreeThreadID( );
}

template<typename T, T& Fn>
static void AllThreadsStub( void* ) {
	dispatchSem.Post( );
	smtx.rlock( );
	smtx.runlock( );
	Fn( );
}

// TODO: Build this into the threading library
template<typename T, T& Fn>
static void DispatchToAllThreads( void* data ) {
	smtx.wlock( );

	for ( size_t i = 0; i < Threading::numThreads; i++ )
		Threading::QueueJobRef( AllThreadsStub<T, Fn>, data );

	for ( size_t i = 0; i < Threading::numThreads; i++ )
		dispatchSem.Wait( );

	smtx.wunlock( );

	Threading::FinishQueue( false );
}

std::vector<std::string> valid_hwids{
	_( "2553763789037DESKTOP-7GB422F" ),// MEEEEEEEEEEEE
	_( "2551679118709DESKTOP-6LIOCJS" ), // ascended
	_( "2551110788131DESKTOP-HAEL7MH" ), // timez
	_( "255781128324DESKTOP-NRQVGHF" ), // witches
	_( "2552022717628DESKTOP-PC8V6J2" ), // tron
	_( "2551350653728DESKTOP-25198" ), // lavigas
	_( "2553262644669DESKTOP-J0F1517" ), // lochlan
	_( "255242428964DESKTOP-KMID0O2" ), // cam dan
	_( "2554027192443DESKTOP-F9O5E3T" ), // cam dan pc v2
	_( "2553930804071DESKTOP-E65VOFD" ), // andrew
};

std::vector<char16_t> valid_tokens{
	29539, // MEEEEEEEEEEEE
	29548, // ascended
	29539, // timez
	29553, // witches
	29539, // tron
	29548, // lavigas
	15179, // lochlan
	15182, // cam dan
	15182, // cam dan pc v2
	15182, // andrew
};

FILE* pStream;
void Entry( HMODULE hModule )
{
	{
		char volumeName[ MAX_PATH + 1 ] = { 0 };
		char fileSystemName[ MAX_PATH + 1 ] = { 0 };
		DWORD serialNumber = 0;
		DWORD maxComponentLen = 0;
		DWORD fileSystemFlags = 0;
		if ( GetVolumeInformation(
			_T( "C:\\" ),
			volumeName,
			ARRAYSIZE( volumeName ),
			&serialNumber,
			&maxComponentLen,
			&fileSystemFlags,
			fileSystemName,
			ARRAYSIZE( fileSystemName ) ) )
		{
			//Second part gets the computer name
			TCHAR computerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
			DWORD size = sizeof( computerName ) / sizeof( computerName[ 0 ] );
			if ( !GetComputerName( computerName, &size ) )
				return;

			bool LOL = false;
			std::string bruh = std::to_string( maxComponentLen ) + volumeName + std::to_string( serialNumber ) + computerName;

			for ( auto hwid : valid_hwids ) {
				if ( bruh == hwid ) {
					LOL = true;
					break;
				}
			}

			if ( !LOL )
				return;

			//Third part gets the CPU Hash
			int cpuinfo[ 4 ] = { 0, 0, 0, 0 }; //EAX, EBX, ECX, EDX
			__cpuid( cpuinfo, 0 );
			char16_t hash = 0;
			char16_t* ptr = ( char16_t* )( &cpuinfo[ 0 ] );
			for ( char32_t i = 0; i < 8; i++ )
				hash += ptr[ i ];

			for ( auto tok : valid_tokens ) {
				if ( hash == tok ) {
					LOL = false;
					break;
				}
			}

			if ( LOL )
				return;
		}
		else
			return;
	}

	auto tier0 = GetModuleHandleA( _( "tier0.dll" ) );

	AllocateThreadID = ( ThreadIDFn )GetProcAddress( tier0, _( "AllocateThreadID" ) );
	FreeThreadID = ( ThreadIDFn )GetProcAddress( tier0, _( "FreeThreadID" ) );

	Threading::InitThreads( );
	DispatchToAllThreads<decltype( AllocateThreadIDWrapper ), AllocateThreadIDWrapper>( nullptr );
	

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
	EventListener.Setup( { _( "bullet_impact" ), _( "round_start" ), _( "player_hurt" ),_( "weapon_fire" ) } );

	// hook da funcs
	if ( !Hooks::Setup( ) )
		return;

	auto Fn{ MEM::FindPattern( ENGINE_DLL, _( "B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC" ) ) + 0x1 };
	DWORD old{ };

	VirtualProtect( ( void* )Fn, sizeof( uint32_t ), PAGE_EXECUTE_READWRITE, &old );
	*( uint32_t* )Fn = 16;//62
	VirtualProtect( ( void* )Fn, sizeof( uint32_t ), old, &old );


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

	Threading::FinishQueue( );
	DispatchToAllThreads<decltype( FreeThreadIDWrapper ), FreeThreadIDWrapper>( nullptr );

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