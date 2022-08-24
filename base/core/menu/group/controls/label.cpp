#include "../group.h"

void MenuGroup::Label( const char* name ) {
	Render::Text( Fonts::Menu, Menu::CursorPos, Color( 255, 255, 255 ), 0, name );

	Menu::CursorPos += Vector2D( 0, 18 );
}