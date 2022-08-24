#include "../group.h"

inline float map_number( float input, float input_min, float input_max, float output_min, float output_max ) {
	return ( input - input_min ) / ( input_max - input_min ) * ( output_max - output_min ) + output_min;
}

void MenuGroup::Slider( const char* name, int& value, int min, int max ) {
	const auto Size = Vector2D( size.x - 30, std::min( 12.f, ( size.y + OldCursorPos.y ) - Menu::CursorPos.y ) );

	value = std::clamp<int>( value, min, max );

	//auto FillCol = Color( 20, 20, 20 );

	Render::Text( Fonts::Menu, Menu::CursorPos, Color( 255, 255, 255 ), 0, name );

	Menu::CursorPos += Vector2D( 0, 18 );

	const bool hovered = Inputsys::hovered( Menu::CursorPos - Vector2D( 12, 0 ), Size + Vector2D( 24, 0 ) );

	if ( Menu::OpenedID == name ) {
		if ( Inputsys::down( VK_LBUTTON ) && !Menu::DraggingMouse ) {
			const int offset = std::clamp( Vector2D( Inputsys::MousePos - Menu::CursorPos ).x, 0.f, Size.x );
			value = ( int )map_number( offset, 0, Size.x, min, max );
		}

		if ( Inputsys::released( VK_LBUTTON ) )
			Menu::OpenedID = "";
	}

	if ( Menu::OpenedID == "" && hovered && !Menu::FrameAfterFocus ) {
		if ( Inputsys::pressed( VK_LBUTTON ) )
			Menu::OpenedID = name;
	}

	const float slider_ratio = map_number( ( float )value, ( float )min, ( float )max, 0.f, 1.f );

	//Render::FilledRectangle( Menu::CursorPos, Size, FillCol );
	Render::Rectangle( Menu::CursorPos, Size + 2, Color( 0, 0, 0 ) );
	Render::FilledRectangle( Menu::CursorPos + 1, Vector2D( Size.x * slider_ratio, Size.y ), Menu::AccentCol );

	Render::Text( Fonts::Menu, Menu::CursorPos + Vector2D( Size.x / 2, 0 ), Color( 255, 255, 255 ), FONT_CENTER, std::to_string( value ).c_str( ) );

	Menu::CursorPos += Vector2D( 0, 18 );
}

void MenuGroup::Slider( const char* name, float& value, float min, float max ) {
	const auto Size = Vector2D( size.x - 30, 12 );

	value = std::clamp<float>( value, min, max );

	//auto FillCol = Color( 20, 20, 20 );

	Render::Text( Fonts::Menu, Menu::CursorPos, Color( 255, 255, 255 ), 0, name );

	Menu::CursorPos += Vector2D( 0, 18 );

	const bool Hovered = Inputsys::hovered( Menu::CursorPos - Vector2D( 12, 0 ), Size + Vector2D( 12, 0 ) );

	if ( Hovered ) {
		if ( Hovered && Inputsys::down( VK_LBUTTON ) && !Menu::DraggingMouse ) {
			float offset = std::clamp( Vector2D( Inputsys::MousePos - Menu::CursorPos ).x, 0.f, Size.x );
			value = map_number( offset, 0, Size.x, min, max );
		}
	}

	const float slider_ratio = map_number( ( float )value, ( float )min, ( float )max, 0.f, 1.f );

	//Render::FilledRectangle( Menu::CursorPos, Size, FillCol );
	Render::Rectangle( Menu::CursorPos, Size + 2, Color( 0, 0, 0 ) );
	Render::FilledRectangle( Menu::CursorPos + 1, Vector2D( Size.x * slider_ratio, Size.y ), Menu::AccentCol );

	Render::Text( Fonts::Menu, Menu::CursorPos + Vector2D( Size.x / 2, 0 ), Color( 255, 255, 255 ), FONT_CENTER, std::to_string( value ).c_str( ) );

	Menu::CursorPos += Vector2D( 0, 18 );
}