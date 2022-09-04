#include "../group.h"

bool MenuGroup::Button( const char* name ) {
	auto Size = Vector2D( size.x - 30, std::min( 24.f, ( size.y + OldCursorPos.y ) - Menu::CursorPos.y ) );

	bool ret{ };

	auto FillCol = Menu::ControlCol;

	const auto TextSize = Render::GetTextSize( name, Fonts::Menu ).x;
	const bool Hovered = Inputsys::hovered( Menu::CursorPos, Size );

	if ( Hovered && Menu::OpenedID == "" && !Menu::FrameAfterFocus ) {
		FillCol = Color( 25, 25, 25 );
		if ( Inputsys::pressed( VK_LBUTTON ) )
			ret = true;
	}

	Render::FilledRectangle( Menu::CursorPos, Size, FillCol );
	Render::Rectangle( Menu::CursorPos, Size + 2, Color( 0, 0, 0 ) );
	Render::Rectangle( Menu::CursorPos + 1, Size, Menu::OutlineLight );

	Render::Text( Fonts::Menu, Menu::CursorPos.x + Size.x / 2, Menu::CursorPos.y + 5, Color(255,255,255), FONT_CENTER, name );

	Menu::CursorPos += Vector2D( 0, 30 );

	return ret;
}