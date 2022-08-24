#include "../core/hooks.h"
#include "../utils/render.h"
#include "../core/menu/menu.h"
#include "../context.h"
#include <intrin.h>

LRESULT CALLBACK Hooks::hkWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( uMsg == WM_KEYDOWN && wParam == VK_INSERT )
		Menu::Opened = !Menu::Opened;

	// disable game input when menu is opened
	Interfaces::InputSystem->EnableInput( !Menu::Opened );

	if ( uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEHWHEEL )
		Inputsys::scroll += ( float )GET_WHEEL_DELTA_WPARAM( wParam ) / ( float )WHEEL_DELTA;

	ctx.GetLocal( );

	//if ( Menu::Opened && ( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_MOUSEMOVE ) )
	//	return false;

	// return input controls to the game
	return CallWindowProc( pOldWndProc, hWnd, uMsg, wParam, lParam );
}