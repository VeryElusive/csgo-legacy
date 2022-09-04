#include "../group.h"

void MenuGroup::ColorPicker( const char* name, Color& value, bool doalpha ) {
	const auto Size = Vector2D( 24, std::min( 12.f, ( size.y + OldCursorPos.y ) - Menu::CursorPos.y ) );
	const bool opened = Menu::OpenedID == name;

	auto pos = Menu::CursorPos + Vector2D( size.x - 57, - 18 );

	const auto TextSize = Render::GetTextSize( name, Fonts::Menu ).x;
	bool Hovered = Inputsys::hovered( pos, Size );

	hsv NewHue{ Menu::rgb2hsv( value ) };

	Render::FilledRectangle( pos, Size, value );
	Render::Rectangle( pos, Size, Menu::OutlineLight );

	pos += Vector2D( 0, 12 );

	if ( opened ) {
		Hovered = Inputsys::hovered( pos, Vector2D( 200, 200 ) );
		Vector2D CursorDelta = { NewHue.sat * 150.f, ( 1.f - NewHue.val ) * 150.f };

		Menu::ColorPicker = colorpickerinfo( pos, NewHue.hue, value, doalpha );

		if ( Inputsys::hovered( pos + Vector2D( 170, 10 ), Vector2D( 20, 150 ) ) ) {
			const auto HueDrawPos = pos + Vector2D( 170, 10 );

			if ( Inputsys::down( VK_LBUTTON ) ) {
				NewHue.hue = ( ( Inputsys::MousePos.y - HueDrawPos.y ) / 150.f ) * 360.f;

				NewHue.hue = std::clamp<float>( NewHue.hue, 0.f, 359.f );
			}
		}
		else if ( Inputsys::hovered( pos + Vector2D( 8, 8 ), Vector2D( 154, 154 ) ) ) {
			if ( Inputsys::down( VK_LBUTTON ) ) {
				CursorDelta = Inputsys::MousePos - pos - Vector2D( 11, 11 );

				NewHue.sat = std::clamp<float>( CursorDelta.x / 150.f, 0.f, 1.f );
				NewHue.val = std::clamp<float>( 1.f - CursorDelta.y / 150.f, 0.f, 1.f );
			}
		}

		const auto alpha = value.Get<COLOR_A>( );

		value = Menu::hsv2rgb( NewHue ).Set<COLOR_A>( alpha );

		if ( doalpha ) {
			if ( Inputsys::hovered( pos + Vector2D( 8, 168 ), Vector2D( 154, 20 ) ) ) {
				if ( Inputsys::down( VK_LBUTTON ) ) {
					CursorDelta = Inputsys::MousePos - pos - Vector2D( 11, 11 );

					value = value.Set<COLOR_A>( 255 * std::clamp<float>( CursorDelta.x / 150.f, 0.f, 1.f ) );
				}
			}
		}
	}

	if ( opened && !Hovered && Inputsys::pressed( VK_LBUTTON ) ) {
		Menu::FrameAfterFocus = true;
		Menu::OpenedID = "";
	}
	else if ( !opened && Hovered && Inputsys::pressed( VK_LBUTTON ) && Menu::OpenedID == "" && !Menu::FrameAfterFocus )
		Menu::OpenedID = name;
}