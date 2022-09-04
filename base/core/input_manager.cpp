#include "input_manager.h"
#include "config.h"
#include "menu/menu.h"
#include "../features/rage/antiaim.h"

void Inputsys::update( ) {
	for ( auto& i : KeysToCheck ) {
		prev_state[ i ] = state[ i ];
		state[ i ] = GetAsyncKeyState( i );
	}

	static HWND window;
	if ( !window )
		window = FindWindowA( "Valve001", 0 );

	POINT mouse{ };
	GetCursorPos( &mouse );
	ScreenToClient( window, &mouse );

	MouseDelta = MousePos - Vector2D( mouse.x, mouse.y );

	MousePos = Vector2D( mouse.x, mouse.y );

	KeysToCheck.clear( );
}

#define DODAKEYBINDCUH( bindname ) KeysToCheck.emplace_back( Config::Get<keybind_t>( Vars.##bindname ).key ); Config::Get<keybind_t>( Vars.##bindname ).Resolve( );

void Inputsys::updateNeededKeys( ) {
	if ( !Interfaces::Engine->IsActiveApp( ) )
		return;

	if ( Menu::Opened ) {
		for ( int i = 0; i < 256; i++ )
			KeysToCheck.emplace_back( i );
	}
	else {
		if ( !Offsets::Cvars.cl_mouseenable->GetBool( ) )
			return;

		if ( Interfaces::Engine->IsConsoleVisible( ) )
			return;

		DODAKEYBINDCUH( VisThirdPersonKey );
		DODAKEYBINDCUH( MiscSlowWalkKey );
		DODAKEYBINDCUH( MiscFakeDuckKey );
		DODAKEYBINDCUH( MiscAutoPeekKey );
		DODAKEYBINDCUH( AntiaimInvert );
		DODAKEYBINDCUH( AntiaimInvertSpam );
		DODAKEYBINDCUH( RagebotDamageOverrideKey );
		DODAKEYBINDCUH( RagebotForceBaimKey );
		DODAKEYBINDCUH( ExploitsDoubletapKey );
		DODAKEYBINDCUH( ExploitsHideshotsKey );

		if ( !Config::Get<keybind_t>( Vars.AntiaimRight ).enabled )
			DODAKEYBINDCUH( AntiaimRight );

		if ( !Config::Get<keybind_t>( Vars.AntiaimLeft ).enabled )
			DODAKEYBINDCUH( AntiaimLeft );

		//if ( !Config::Get<keybind_t>( Vars.AntiaimLeft ).enabled && !Config::Get<keybind_t>( Vars.AntiaimRight ).enabled )
		//	Features::Antiaim.ManualSide = 0;

		if ( Config::Get<keybind_t>( Vars.AntiaimLeft ).enabled ) {
			if ( Features::Antiaim.ManualSide == 1 )
				Features::Antiaim.ManualSide = 0;
			else
				Features::Antiaim.ManualSide = 1;

			Config::Get<keybind_t>( Vars.AntiaimLeft ).enabled = false;
		}

		if ( Config::Get<keybind_t>( Vars.AntiaimRight ).enabled ) {
			if ( Features::Antiaim.ManualSide == 2 )
				Features::Antiaim.ManualSide = 0;
			else
				Features::Antiaim.ManualSide = 2;

			Config::Get<keybind_t>( Vars.AntiaimRight ).enabled = false;
		}

		if ( !Config::Get<keybind_t>( Vars.AntiaimRight ).key
			&& !Config::Get<keybind_t>( Vars.AntiaimLeft ).key
			&& Config::Get<keybind_t>( Vars.AntiaimRight ).mode != EKeyMode::AlwaysOn
			&& Config::Get<keybind_t>( Vars.AntiaimLeft ).mode != EKeyMode::AlwaysOn )
			Features::Antiaim.ManualSide = 0;

		ctx.m_bExploitsEnabled = ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled )
			|| ( Config::Get<bool>( Vars.ExploitsHideshots ) && Config::Get<keybind_t>( Vars.ExploitsHideshotsKey ).enabled );
	}
}

bool Inputsys::down( int key ) {
	return state[ key ] && prev_state[ key ];
}

bool Inputsys::pressed( int key ) {
	return state[ key ] && !prev_state[ key ];
}

bool Inputsys::released( int key ) {
	return !state[ key ] && prev_state[ key ];
}

bool Inputsys::hovered( const Vector2D& pos, const Vector2D& size ) {
	return ( MousePos > pos && MousePos < pos + size );
}
