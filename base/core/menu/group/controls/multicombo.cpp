#include "../group.h"

void MenuGroup::MultiCombo( const char* name, std::vector<multi_item_t> options ) {
	const auto Size = Vector2D( size.x - 30, 18 );
	const bool opened{ Menu::OpenedID == name };

	if ( Size.y + Menu::CursorPos.y > OldCursorPos.y + size.y + 18 )
		return;

	auto FillCol{ Menu::ControlCol };

	Render::Text( Fonts::Menu, Menu::CursorPos, Color( 255, 255, 255 ), 0, name );

	Menu::CursorPos += Vector2D( 0, 18 );

	Render::FilledRectangle( Menu::CursorPos, Size, FillCol );

	bool Hovered{ };

	const bool preHov{ Inputsys::hovered( Menu::CursorPos, Size ) };

	if ( opened ) {
		Menu::MultiDropdown = multidropinfo( Menu::CursorPos + 1, Size, options );
		uint8_t i{ 1 };
		for ( const auto o : options ) {
			const bool Hov = Inputsys::hovered( Menu::CursorPos + Vector2D( 0, Size.y * i ), Size );

			if ( Hov )
				Hovered = true;

			if ( Inputsys::pressed( VK_LBUTTON ) && Hov )
				*o.value = !*o.value;

			//Render::FilledRectangle( Menu::CursorPos + 1 + Vector2D( 0, Size.y * i ), Size, *o.value ? Color( 45, 45, 45 ) : Hov ? Color( 25, 25, 25 ) : FillCol );
			//Render::Text( Menu::CursorPos + 1 + Vector2D( 0, Size.y * i ), o.name, Color( 255, 255, 255 ), fonts::Menu, 0 );

			i++;
		}
	}

	if ( opened && !Hovered && Inputsys::pressed( VK_LBUTTON ) ) {
		Menu::FrameAfterFocus = true;
		Menu::OpenedID = "";
	}
	else if ( preHov && Inputsys::pressed( VK_LBUTTON ) && Menu::OpenedID == "" && !Menu::FrameAfterFocus )
		Menu::OpenedID = name;

	//const auto SizeEnd = opened ? Vector2D( Size.x, Size.y * ( options.size( ) + 1 ) ) : Size;
	Render::Rectangle( Menu::CursorPos, Size + 2, Color( 0, 0, 0 ) );
	Render::Rectangle( Menu::CursorPos + 1, Size, Menu::OutlineLight );

	Menu::CursorPos += Vector2D( 0, 24 );
}