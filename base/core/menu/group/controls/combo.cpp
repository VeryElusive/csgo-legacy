#include "../group.h"

void MenuGroup::Combo( const char* name, int& value, std::vector<std::string> options ) {
	const auto Size = Vector2D( size.x - 30, std::min( 18.f, ( size.y + OldCursorPos.y ) - Menu::CursorPos.y ) );
	const bool opened{ Menu::OpenedID == name };

	auto FillCol{ Menu::ControlCol };

	Render::Text( Fonts::Menu, Menu::CursorPos, Color( 255, 255, 255 ), 0, name );

	Menu::CursorPos += Vector2D( 0, 18 );

	Render::FilledRectangle( Menu::CursorPos, Size, FillCol );

	const bool Hovered{ Inputsys::hovered( Menu::CursorPos, Size ) };
	
	if ( opened ) {
		Menu::Dropdown = dropinfo( Menu::CursorPos + 1, Size, options, value );
		uint8_t i{ 1 };
		for ( const auto o : options ) {
			const bool Hov = Inputsys::hovered( Menu::CursorPos + Vector2D( 0, Size.y * i ), Size );

			if ( Inputsys::pressed( VK_LBUTTON ) && Hov ) {
				value = i - 1;
				break;
			}

			//Render::FilledRectangle( Menu::CursorPos + 1 + Vector2D( 0, Size.y * i - 1 ), Size, i == value ? Color( 45, 45, 45 ) : Hov ? Color( 25, 25, 25 ) : FillCol );
			//Render::Text( Menu::CursorPos + 1 + Vector2D( 0, Size.y * i - 1 ), o, Color( 255, 255, 255 ), fonts::Menu, 0 );

			i++;
		}
	}

	if ( opened && Inputsys::pressed( VK_LBUTTON ) ) {
		Menu::FrameAfterFocus = true;
		Menu::OpenedID = "";
	}
	else if ( Hovered && Inputsys::pressed( VK_LBUTTON ) && Menu::OpenedID == "" && !Menu::FrameAfterFocus )
		Menu::OpenedID = name;

	//const auto SizeEnd = opened ? Vector2D( Size.x, Size.y * options.size( ) ) : Size;

	Render::Rectangle( Menu::CursorPos, Size + 2, Color( 0, 0, 0 ) );
	Render::Rectangle( Menu::CursorPos + 1, Size, Menu::OutlineLight );

	Render::Text( Fonts::Menu, Menu::CursorPos + Vector2D( 6, 3 ), Color( 255, 255, 255 ), 0, options.at( value ).c_str( ) );

	Menu::CursorPos += Vector2D( 0, 24 );
}