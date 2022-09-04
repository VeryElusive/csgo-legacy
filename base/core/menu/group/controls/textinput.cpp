#include "../group.h"

void MenuGroup::TextInput( std::string& val ) {
	const auto Size = Vector2D( size.x - 30, std::min( 24.f, ( size.y + OldCursorPos.y ) - Menu::CursorPos.y ) );
	bool Opened = Menu::OpenedID == val;

	auto FillCol = Menu::ControlCol;

	const auto TextSize = Render::GetTextSize( val, Fonts::Menu ).x;
	const bool Hovered = Inputsys::hovered( Menu::CursorPos, Size );

	Render::FilledRectangle( Menu::CursorPos, Size, FillCol );
	Render::Rectangle( Menu::CursorPos, Size + 2, Color( 0, 0, 0 ) );
	Render::Rectangle( Menu::CursorPos + 1, Size, Menu::OutlineLight );

	Render::Text( Fonts::Menu, Menu::CursorPos + Vector2D( Size.y / 2, 5 ), Color( 255, 255, 255 ), 0, val.c_str( ) );

	if ( Opened ) {
		Menu::Typing = true;
		if ( Inputsys::pressed( VK_ESCAPE ) || Inputsys::pressed( VK_RETURN ) ) {
			Menu::OpenedID = "";
			Opened = false;
		}
		else if ( Inputsys::pressed( VK_BACK ) )
			val = val.substr( 0, val.size( ) - 1 );
		else {
			for ( int i = ( int )'A'; i <= ( int )'Z'; i++ ) {
				if ( isalpha( i ) ) {
					if ( Inputsys::pressed( i ) ) {
						auto final_i = i;
						if ( !GetKeyState( VK_CAPITAL ) && !Inputsys::down( VK_SHIFT ) )
							final_i += 32;

						val += final_i;
					}
				}
			}

			if ( Inputsys::pressed( VK_SPACE ) )
				val += int( ' ' );
		}
	}

	if ( Opened && !Hovered && Inputsys::pressed( VK_LBUTTON ) )
		Menu::OpenedID = "";
	else if ( Hovered && Inputsys::pressed( VK_LBUTTON ) && Menu::OpenedID == "" )
		Menu::OpenedID = val.c_str( );

	if ( Menu::OpenedID == "" )
		Menu::Typing = false;

	Menu::CursorPos += Vector2D( 0, 30 );
}