#include "../group.h"

const char* keys[ ] = { "[-]", "[M1]", "[M2]", "[BRK]", "[M3]", "[M4]", "[M5]",
"[_]", "[BSPC]", "[TAB]", "[_]", "[_]", "[_]", "[ENTER]", "[_]", "[_]", "[SHFT]",
"[CTRL]", "[ALT]", "[PAUSE]", "[CAPS]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[ESC]", "[_]", "[_]", "[_]", "[_]", "[SPACE]", "[PGUP]", "[PGDOWN]", "[END]", "[HOME]", "[LEFT]",
"[UP]", "[RIGHT]", "[DOWN]", "[_]", "[PRNT]", "[_]", "[PRTSCR]", "[INS]", "[DEL]", "[_]", "[0]", "[1]",
"[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[A]", "[B]", "[C]", "[D]", "[E]", "[F]", "[G]", "[H]", "[I]", "[J]", "[K]", "[L]", "[M]", "[N]", "[O]", "[P]", "[Q]", "[R]", "[S]", "[T]", "[U]",
"[V]", "[W]", "[X]", "[Y]", "[Z]", "[LFTWIN]", "[RGHTWIN]", "[_]", "[_]", "[_]", "[NUM0]", "[NUM1]",
"[NUM2]", "[NUM3]", "[NUM4]", "[NUM5]", "[NUM6]", "[NUM7]", "[NUM8]", "[NUM9]", "[*]", "[+]", "[_]", "[-]", "[.]", "[/]", "[F1]", "[F2]", "[F3]",
"[F4]", "[F5]", "[F6]", "[F7]", "[F8]", "[F9]", "[F10]", "[F11]", "[F12]", "[F13]", "[F14]", "[F15]", "[F16]", "[F17]", "[F18]", "[F19]", "[F20]", "[F21]",
"[F22]", "[F23]", "[F24]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[NUMLCK]", "[SCRLLCK]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[LSHFT]", "[RSHFT]", "[LCTRL]",
"[RCTRL]", "[LALT]", "[RALT]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[NTRK]", "[PTRK]", "[STOP]", "[PLAY]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[_]", "[;]", "[+]", "[,]", "[-]", "[.]", "[?]", "[~]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]", "[_]",
"[_]", "[_]" };

void MenuGroup::Keybind( const char* name, keybind_t& value ) {
	const bool opened{ Menu::OpenedID == name };
	const char* rendaName{ keys[ value.key ] };
	const auto Size{ Render::GetTextSize( rendaName, Fonts::Menu ) };
	const auto pos{ Menu::CursorPos + Vector2D( size.x - 30 - Size.x, -18 ) };

	const bool Hovered{ Inputsys::hovered( pos, { Size.x, 12 } ) };

	const auto col = opened ? Color( 200, 200, 200 ) : Hovered ? Color( 150, 150, 150 ) : Color( 80, 80, 80 );

	if ( opened ) {
		Menu::KeyBind = { pos, value };
		for ( int i = 3; i < 255; i++ ) {
			if ( Inputsys::pressed( i ) ) {
				if ( i == VK_ESCAPE ) {
					value.key = 0;
					Menu::OpenedID = "";
					break;
				}

				value.key = i;
				Menu::OpenedID = "";
				break;
			}
		}

		const Vector2D sz = { 100, 18 };
		for ( int i{ 1 }; i <= 4; i++ ) {
			const bool Hov = Inputsys::hovered( Menu::KeyBind->pos + Vector2D( 0, sz.y * i ), sz );

			if ( Hov && Inputsys::pressed( VK_LBUTTON ) )
				value.mode = i - 1;
		}
	}

	if ( opened && ( Inputsys::pressed( VK_LBUTTON ) || Inputsys::pressed( VK_ESCAPE ) ) ) {
		Menu::FrameAfterFocus = true;
		Menu::OpenedID = "";
	}
	else if ( Hovered && Inputsys::pressed( VK_LBUTTON ) && Menu::OpenedID == "" && !Menu::FrameAfterFocus )
		Menu::OpenedID = name;

	Render::Text( Fonts::Menu, pos, Color( col ), 0, rendaName );
}