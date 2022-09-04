#include "group.h"

void MenuGroup::Begin( const char* name, Vector2D Size ) {
	OldCursorPos = Menu::CursorPos;

	size = Size;
	size.floor( );

	Render::FilledRectangle( OldCursorPos, size, Menu::GroupCol );
	Render::Rectangle( OldCursorPos, size + 2, Color( 0, 0, 0 ) );
	Render::Rectangle( OldCursorPos + 1, size, Menu::OutlineLight );

	Render::Text( Fonts::Menu, OldCursorPos + 5 + Vector2D( size.x / 2, 0 ), Color( 255, 255, 255 ), FONT_CENTER, name );

	Interfaces::Surface->SetClipRect( OldCursorPos.x - 1, OldCursorPos.y + 29, size.x, size.y - 30 );

	Menu::CursorPos += Vector2D( 15, 30 );

	Menu::CursorPos.y -= scroll;
}

void MenuGroup::End( ) {
	const bool Hovered{ Inputsys::hovered( OldCursorPos, size ) };

	const float outside{ std::max( 0.f, Menu::CursorPos.y - OldCursorPos.y - size.y + scroll ) };

	if ( Hovered ) {
		scroll -= Inputsys::scroll * 12;
		scroll = std::min( outside, scroll );
		scroll = std::max( 0.f, scroll );
	}

	if ( outside ) {
		const float percent{ scroll / outside };
		const float barLength{ ( size.y - 30 ) * ( 1.f - ( outside / size.y ) ) };
		Render::FilledRectangle( OldCursorPos + Vector2D( size.x - 3, 30 + percent * ( size.y - 30 - barLength ) ),
			{ 2, barLength }, Menu::AccentCol );
	}

	Interfaces::Surface->SetClipRect( 0, 0, ctx.m_ve2ScreenSize.x, ctx.m_ve2ScreenSize.y );

	Menu::CursorPos = OldCursorPos + Vector2D( size.x + 20, 0 );

	//Render::Gradient( OldCursorPos.x + 2, OldCursorPos.y + size.y * 0.5f - 1, size.x - 2, size.y / 2, Menu::BackgroundCol.Set<COLOR_A>( 0 ), Menu::BackgroundCol, false );

	Menu::CachedGroupSize = size.y;
}