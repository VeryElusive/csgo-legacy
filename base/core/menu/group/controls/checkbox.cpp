#include "../group.h"

void MenuGroup::Checkbox( const char* name, bool& value ) {
	auto Size = Vector2D( 12, std::min( 12.f, ( size.y + OldCursorPos.y ) - Menu::CursorPos.y ) );

	const auto offScreen{ Menu::CursorPos.y + Size.y > OldCursorPos.y + size.y };

	auto FillCol = Menu::ControlCol;

	const auto TextSize = Render::GetTextSize( name, Fonts::Menu ).x;
	const bool Hovered = Inputsys::hovered( Menu::CursorPos, Size + Vector2D( TextSize + 12, 0 ) );

	if ( Hovered || value ) {
		if ( Hovered && Inputsys::pressed( VK_LBUTTON ) 
			&& Menu::OpenedID == ""
			&& !Menu::FrameAfterFocus )
			value = !value;

		if ( value )
			FillCol = Color( 40, 40, 40 );
		else
			FillCol = Color( 25, 25, 25 );
	}

	Render::FilledRectangle( Menu::CursorPos, Size, FillCol );
	Render::Rectangle( Menu::CursorPos, Size + 2, Color( 0, 0, 0 ) );
	Render::Rectangle( Menu::CursorPos + 1, Size, Menu::OutlineLight );

	Render::Text( Fonts::Menu, Menu::CursorPos + Vector2D( 24, 0 ), Color( 255, 255, 255 ), 0, name );

	Menu::CursorPos += Vector2D( 0, 18 );
}