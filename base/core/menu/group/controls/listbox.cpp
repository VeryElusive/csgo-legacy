#include "../group.h"

void MenuGroup::ListBox( const char* name, std::deque<std::string> options, int& Opt, Vector2D Size ) {
	Render::FilledRectangle( Menu::CursorPos, Size, Menu::ControlCol );
	Render::Rectangle( Menu::CursorPos, Size + 2, Color( 0, 0, 0 ) );
	Render::Rectangle( Menu::CursorPos + 1, Size, Menu::OutlineLight );

	Render::Text( Fonts::Menu, Menu::CursorPos + 5 + Vector2D( Size.x / 2, 0 ), Color( 255, 255, 255 ), FONT_CENTER, name );

	const bool Hovered = Inputsys::hovered( Menu::CursorPos, Size );

	const int outside{ static_cast< int >( std::max( 0.f, options.size( ) * 15 - Size.y + 15 ) ) };

	// static cuz im only using it once
	static int scroll{ };
	if ( Hovered ) {
		scroll += Inputsys::scroll * 15;
		scroll = std::min( outside, scroll );
		scroll = std::max( 0, scroll );
	}

	Interfaces::Surface->SetClipRect( Menu::CursorPos.x, Menu::CursorPos.y + 15, Size.x, Size.y - 15 );

	int i{ };
	for ( auto o : options ) {
		if ( Inputsys::hovered( Menu::CursorPos + Vector2D( 10, 15 + 15 * i - scroll ), Vector2D( Size.x, 15 ) ) ) {
			if ( Inputsys::down( VK_LBUTTON ) )
				Opt = i;
		}

		Render::Text( Fonts::Menu, Menu::CursorPos + Vector2D( 10, 15 + 15 * i - scroll ), Opt == i ? Color( 255, 255, 255 ) : Color( 100, 100, 100 ), 0, o.c_str( ) );

		i++;
	}

	Interfaces::Surface->SetClipRect( OldCursorPos.x - 1, OldCursorPos.y - 1, size.x, size.y );

	Menu::CursorPos += Vector2D( 0, Size.y + 20 );
}