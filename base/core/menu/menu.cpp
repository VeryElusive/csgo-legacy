#include "menu.h"

void Menu::render( ) {
	if ( MenuAlpha <= 0 )
		return;

	const bool TopBarHovered = Inputsys::hovered( Pos, Vector2D( Size.x, 50 ) );
	const bool BottomBarHovered = Inputsys::hovered( Pos + Size - Vector2D( 20, 20 ), Vector2D( 20, 20 ) );


	static bool is_dragging = false;
	static Vector2D bot_right;
	if ( BottomBarHovered || is_dragging ) {
		if ( Inputsys::down( VK_LBUTTON ) ) {
			is_dragging = true;
			Size += Vector2D( Inputsys::MousePos - ( Pos + Size ) );
		}
		else
			is_dragging = false;
	}

	Size.x = std::clamp( static_cast< int >( Size.x ), 550, 1000 );
	Size.y = std::clamp( static_cast< int >( Size.y ), 420, 1000 );

	if ( !DraggingMouse && Inputsys::pressed( VK_LBUTTON ) && TopBarHovered )
		DraggingMouse = true;
	
	else if ( DraggingMouse && Inputsys::down( VK_LBUTTON ) )
		Pos -= Inputsys::MouseDelta;
	else if ( DraggingMouse && !Inputsys::down( VK_LBUTTON ) )
		DraggingMouse = false;

	Render::FilledRoundedBox( Pos - Vector2D( 1, 1 ), Size + 2, 5, 5, Color( 10, 10, 10 ) );
	Render::FilledRoundedBox( Pos, Size, 90, 5, OutlineLight );
	Render::FilledRoundedBox( Pos + 1, Size - Vector2D( 2, 2 ), 5, 5, Color( 10, 10, 10 ) );
	Render::FilledRoundedBox( Pos + 2, Size - Vector2D( 4, 4 ), 5, 5, BackgroundCol );

	Render::Line( Pos + Vector2D( 1, 51 ), Pos + Vector2D( Size.x - 2, 51 ), Color( 10, 10, 10 ) );
	Render::Line( Pos + Vector2D( 0, 50 ), Pos + Vector2D( Size.x, 50 ), OutlineLight );
	Render::Line( Pos + Vector2D( 1, 49 ), Pos + Vector2D( Size.x - 2, 49 ), Color( 10, 10, 10 ) );

	for ( int i{ }; i < 12; ++i ) {
		Render::Line( Pos + Vector2D( Size.x - 55 - i, 2 ), Pos + Vector2D( Size.x - 9 - i, 48 ), Menu::AccentCol );
		Render::Line( Pos + Vector2D( Size.x - 32 - i, 2 ), Pos + Vector2D( Size.x - 3, 31 + i ), Menu::Accent2Col );

		Render::Line( Pos + Vector2D( 32 + i, 2 ), Pos + Vector2D( 2, 32 + i ), Menu::Accent2Col );
		Render::Line( Pos + Vector2D( 56 + i, 2 ), Pos + Vector2D( 10 + i, 48 ), Menu::AccentCol );
	}

	Render::Line( Pos + Vector2D( 1, 79 ), Pos + Vector2D( Size.x - 2, 79 ), Color( 10, 10, 10 ) );
	Render::Line( Pos + Vector2D( 0, 80 ), Pos + Vector2D( Size.x, 80 ), OutlineLight );
	Render::Line( Pos + Vector2D( 1, 81 ), Pos + Vector2D( Size.x - 2, 81 ), Color( 10, 10, 10 ) );

	Render::Gradient( Pos.x + 40, Pos.y + 80, Size.x / 2 + 40, 1, OutlineLight, AccentCol, true );
	Render::Gradient( Pos.x + Size.x / 2 + 40, Pos.y + 80, Size.x / 2 - 40, 1, AccentCol, OutlineLight, true );


	//Render::Gradient( Pos.x, Pos.y + 51, Size.x, ( Size.y / 2 ) - 51, Color( 20, 20, 20 ), BackgroundCol, false );


	Render::Text( Fonts::Menu, Pos + Vector2D( Size.x / 2, 85), Color( 100, 100, 100 ), FONT_CENTER, _( "This menu is TEMPORARY! I have only coded barebone menu framework and will create cool design soon. (ish) (eventually) (maybe)" ) );

	Render::Line( Pos + Vector2D( 2, Size.y - 29 ), Pos + Vector2D( Size.x - 2, Size.y - 29 ), Color( 10, 10, 10 ) );
	Render::Line( Pos + Vector2D( 1, Size.y - 30 ), Pos + Vector2D( Size.x - 1, Size.y - 30 ), OutlineLight );
	Render::Line( Pos + Vector2D( 2, Size.y - 31 ), Pos + Vector2D( Size.x - 2, Size.y - 31 ), Color( 10, 10, 10 ) );

	Render::Text( Fonts::Menu, Pos + Vector2D( 20, Size.y - 22 ), AccentCol, 0, _( "Havoc" ) );
	Render::Text( Fonts::Menu, Pos + Vector2D( Size.x - 125, Size.y - 22 ), Color( 50, 50, 50 ), 0, _( "Developed by" ) );
	Render::Text( Fonts::Menu, Pos + Vector2D( Size.x - 56, Size.y - 22 ), AccentCol, 0, _( "Artie" ) );

	Menu::CursorPos = Pos + Vector2D( 20, 100 );
}

void Menu::NewGroupRow( float append_x ) {
	CursorPos.y += CachedGroupSize + 20;
	CursorPos.x = Menu::Pos.x + 20 + append_x;
}

Color Menu::hsv2rgb( hsv hsv ) {
	float red, grn, blu;
	float i, f, p, q, t;
	Color result( 0, 0, 0 );

	if ( hsv.val == 0 ) {
		red = 0;
		grn = 0;
		blu = 0;
	}
	else {
		hsv.hue /= 60;
		i = floor( hsv.hue );
		f = hsv.hue - i;
		p = hsv.val * ( 1 - hsv.sat );
		q = hsv.val * ( 1 - ( hsv.sat * f ) );
		t = hsv.val * ( 1 - ( hsv.sat * ( 1 - f ) ) );
		if ( i == 0 ) { red = hsv.val; grn = t; blu = p; }
		else if ( i == 1 ) { red = q; grn = hsv.val; blu = p; }
		else if ( i == 2 ) { red = p; grn = hsv.val; blu = t; }
		else if ( i == 3 ) { red = p; grn = q; blu = hsv.val; }
		else if ( i == 4 ) { red = t; grn = p; blu = hsv.val; }
		else if ( i == 5 ) { red = hsv.val; grn = p; blu = q; }
	}
	result[ 0 ] = red * 255;
	result[ 1 ] = grn * 255;
	result[ 2 ] = blu * 255;
	result[ 3 ] = 255;
	return result;
}

hsv Menu::rgb2hsv( Color a ) {
	float red, grn, blu;
	red = ( float )a.Get<COLOR_R>( ) / 255.f;
	grn = ( float )a.Get<COLOR_G>( ) / 255.f;
	blu = ( float )a.Get<COLOR_B>( ) / 255.f;
	float hue, sat, val;
	float x, f, i;
	hsv result{ };

	x = std::min( std::min( red, grn ), blu );
	val = std::max( std::max( red, grn ), blu );
	if ( x == val ) {
		hue = 0;
		sat = 0;
	}
	else {
		f = ( red == x ) ? grn - blu : ( ( grn == x ) ? blu - red : red - grn );
		i = ( red == x ) ? 3 : ( ( grn == x ) ? 5 : 1 );
		hue = fmod( ( i - f / ( val - x ) ) * 60, 360 );
		sat = ( ( val - x ) / val );
	}
	result.hue = hue;
	result.sat = sat;
	result.val = val;
	return result;
}
