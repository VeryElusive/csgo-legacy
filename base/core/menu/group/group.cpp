#include "group.h"

void MenuGroup::Begin( const char* name, Vector2D Size ) {
	OldCursorPos = Menu::CursorPos;

	size = Size;
	size.floor( );

	Render::FilledRectangle( OldCursorPos, size, Menu::GroupCol );
	Render::Rectangle( OldCursorPos, size + 2, Color( 0, 0, 0 ) );
	Render::Rectangle( OldCursorPos + 1, size, Menu::OutlineLight );

	Interfaces::Surface->SetClipRect( OldCursorPos.x - 1, OldCursorPos.y - 1, size.x, size.y );

	Render::Text( Fonts::Menu, OldCursorPos + 5 + Vector2D( size.x / 2, 0 ), Color( 255, 255, 255 ), FONT_CENTER, name );

	Menu::CursorPos += Vector2D( 15, 30 );
}

void MenuGroup::End( ) {

	Menu::CursorPos = OldCursorPos + Vector2D( size.x + 20, 0 );

	//Render::Gradient( OldCursorPos.x + 2, OldCursorPos.y + size.y * 0.5f - 1, size.x - 2, size.y / 2, Menu::BackgroundCol.Set<COLOR_A>( 0 ), Menu::BackgroundCol, false );

	Interfaces::Surface->SetClipRect( 0,0, ctx.m_ve2ScreenSize.x, ctx.m_ve2ScreenSize.y );

	Menu::CachedGroupSize = size.y;
}