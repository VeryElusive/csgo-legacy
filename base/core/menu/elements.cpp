#include "menu.h"
#include "group/group.h"
#include "../../features/misc/logger.h"

#define APPEND_LEN Vector2D( 60, 140 )

std::vector<multi_item_t> removals = {
	{ "Scope", &Config::Get<bool>( Vars.RemovalScope ) },
	{ "Flash", &Config::Get<bool>( Vars.RemovalFlash ) },
	{ "Aim punch", &Config::Get<bool>( Vars.RemovalPunch ) },
	{ "Post processs", &Config::Get<bool>( Vars.RemovalPostProcess ) },
	{ "Zoom", &Config::Get<bool>( Vars.RemovalZoom ) },
	{ "Smoke", &Config::Get<bool>( Vars.RemovalSmoke ) }
};

std::vector<multi_item_t> buyother = {
	{ "Taser", &Config::Get<bool>( Vars.MiscBuyBotOtherTaser ) },
	{ "Armor", &Config::Get<bool>( Vars.MiscBuyBotOtherArmor ) },
	{ "Kit", &Config::Get<bool>( Vars.MiscBuyBotOtherKit ) },
	{ "Smoke", &Config::Get<bool>( Vars.MiscBuyBotOtherSmoke ) },
	{ "HE Grenade", &Config::Get<bool>( Vars.MiscBuyBotOtherNade ) },
	{ "Flashbang", &Config::Get<bool>( Vars.MiscBuyBotOtherFlashbang ) },
	{ "Molotov", &Config::Get<bool>( Vars.MiscBuyBotOtherMolotov ) }
};

std::vector<multi_item_t> WorldAdjustments = {
	{ "World Modulation", &Config::Get<bool>( Vars.WorldModulation ) },
	{ "Fullbright", &Config::Get<bool>( Vars.WorldFullbright ) }
};

// HAHAHHAAHHAHAH FUCKING CRY ABOUT IT I LOOOOOOOOVE ABUSING MACROS HAHAHAHHAHAHAHHA
#define Convert2PType( name, type ) Vars.##name##type
#define BoolConvert2PType( name, type ) Config::Get<bool>( Convert2PType( name, type ) )
#define IntConvert2PType( name, type ) Config::Get<int>( Convert2PType( name, type ) )
#define FloatConvert2PType( name, type ) Config::Get<float>( Convert2PType( name, type ) )
#define ColorConvert2PType( name, type ) Config::Get<Color>( Convert2PType( name, type ) )

#define PlayerCheckbox( group, name, varname, type ) switch ( type ) { \
case 0: group->Checkbox( name, BoolConvert2PType( varname, Local ) );break; \
case 1: group->Checkbox( name, BoolConvert2PType( varname, Team ) );break; \
case 2: group->Checkbox( name, BoolConvert2PType( varname, Enemy ) );break; }\

#define PlayerColorPicker( group, name, varname, type ) switch ( type ) { \
case 0: group->ColorPicker( name, ColorConvert2PType( varname, Local ) );break; \
case 1: group->ColorPicker( name, ColorConvert2PType( varname, Team ) );break; \
case 2: group->ColorPicker( name, ColorConvert2PType( varname, Enemy ) );break; }\

#define PlayerCombo( group, name, varname, type, items ) switch ( type ) { \
case 0: group->Combo( name, IntConvert2PType( varname, Local ), items );break; \
case 1: group->Combo( name, IntConvert2PType( varname, Team ), items );break; \
case 2: group->Combo( name, IntConvert2PType( varname, Enemy ), items );break; }\

#define PlayerMultiCombo( group, name, items, type ) switch ( type ) { \
case 0: group->MultiCombo( name, items##Local );break; \
case 1: group->MultiCombo( name, items##Team );break; \
case 2: group->MultiCombo( name, items##Enemy );break; }\

#define PlayerIntSlider( group, name, varname, type, min, max ) switch ( type ) { \
case 0: group->Slider( name, IntConvert2PType( varname, Local ), min, max );break; \
case 1: group->Slider( name, IntConvert2PType( varname, Team ), min, max );break; \
case 2: group->Slider( name, IntConvert2PType( varname, Enemy ), min, max );break; }\

#define PlayerFloatSlider( group, name, varname, type, min, max ) switch ( type ) { \
case 0: group->Slider( name, FloatConvert2PType( varname, Local ), min, max );break; \
case 1: group->Slider( name, FloatConvert2PType( varname, Team ), min, max );break; \
case 2: group->Slider( name, FloatConvert2PType( varname, Enemy ), min, max );break; }\

/* rage macros cuz fuck this cfg sys */
#define RAGECHECKBOX( group, name, varname, type ) switch ( type ) { \
case 0: group->Checkbox( name, BoolConvert2PType( varname, Pistol ) );break; \
case 1: group->Checkbox( name, BoolConvert2PType( varname, HeavyPistol ) );break; \
case 2: group->Checkbox( name, BoolConvert2PType( varname, SMG ) );break; \
case 3: group->Checkbox( name, BoolConvert2PType( varname, Rifle ) );break; \
case 4: group->Checkbox( name, BoolConvert2PType( varname, Shotgun ) );break;\
case 5: group->Checkbox( name, BoolConvert2PType( varname, AWP ) );break; \
case 6: group->Checkbox( name, BoolConvert2PType( varname, Scout ) );break; \
case 7: group->Checkbox( name, BoolConvert2PType( varname, Auto ) );break; \
case 8: group->Checkbox( name, BoolConvert2PType( varname, Machine ) );break; }\

#define RAGECOMBO( group, name, varname, type, items ) switch ( type ) { \
case 0: group->Combo( name, IntConvert2PType( varname, Pistol ), items );break; \
case 1: group->Combo( name, IntConvert2PType( varname, HeavyPistol ), items );break; \
case 2: group->Combo( name, IntConvert2PType( varname, SMG ), items );break; \
case 3: group->Combo( name, IntConvert2PType( varname, Rifle ), items );break; \
case 4: group->Combo( name, IntConvert2PType( varname, Shotgun ), items );break; \
case 5: group->Combo( name, IntConvert2PType( varname, AWP ), items );break; \
case 6: group->Combo( name, IntConvert2PType( varname, Scout ), items );break; \
case 7: group->Combo( name, IntConvert2PType( varname, Auto ), items );break; \
case 8: group->Combo( name, IntConvert2PType( varname, Machine ), items );break; }\

#define RAGEMULTICOMBO( group, name, items, type ) switch ( type ) { \
case 0: group->MultiCombo( name, items##Pistol );break; \
case 1: group->MultiCombo( name, items##HeavyPistol );break; \
case 2: group->MultiCombo( name, items##SMG );break; \
case 3: group->MultiCombo( name, items##Rifle );break; \
case 4: group->MultiCombo( name, items##Shotgun );break; \
case 5: group->MultiCombo( name, items##AWP );break; \
case 6: group->MultiCombo( name, items##Scout );break; \
case 7: group->MultiCombo( name, items##Auto );break; \
case 8: group->MultiCombo( name, items##Machine );break; }\

#define RAGEINTSLIDER( group, name, varname, type, min, max ) switch ( type ) { \
case 0: group->Slider( name, IntConvert2PType( varname, Pistol ), min, max );break; \
case 1: group->Slider( name, IntConvert2PType( varname, HeavyPistol ), min, max );break; \
case 2: group->Slider( name, IntConvert2PType( varname, SMG ), min, max );break; \
case 3: group->Slider( name, IntConvert2PType( varname, Rifle ), min, max );break; \
case 4: group->Slider( name, IntConvert2PType( varname, Shotgun ), min, max );break; \
case 5: group->Slider( name, IntConvert2PType( varname, AWP ), min, max );break; \
case 6: group->Slider( name, IntConvert2PType( varname, Scout ), min, max );break; \
case 7: group->Slider( name, IntConvert2PType( varname, Auto ), min, max );break; \
case 8: group->Slider( name, IntConvert2PType( varname, Machine ), min, max );break; }\

#define RAGEFLOATSLIDER( group, name, varname, type, min, max ) switch ( type ) { \
case 0: group->Slider( name, FloatConvert2PType( varname, Pistol ), min, max );break; \
case 1: group->Slider( name, FloatConvert2PType( varname, HeavyPistol ), min, max );break; \
case 2: group->Slider( name, FloatConvert2PType( varname, SMG ), min, max );break; \
case 3: group->Slider( name, FloatConvert2PType( varname, Rifle ), min, max );break; \
case 4: group->Slider( name, FloatConvert2PType( varname, Shotgun ), min, max );break; \
case 5: group->Slider( name, FloatConvert2PType( varname, AWP ), min, max );break; \
case 6: group->Slider( name, FloatConvert2PType( varname, Scout ), min, max );break; \
case 7: group->Slider( name, FloatConvert2PType( varname, Auto ), min, max );break; \
case 8: group->Slider( name, FloatConvert2PType( varname, Machine ), min, max );break; }\

#define CHECKRAGEBOOL( type, name, jmp )switch ( type ) { \
case 0: if ( !Config::Get<bool>( Vars.##name##Pistol ) ) goto jmp;break; \
case 1: if ( !Config::Get<bool>( Vars.##name##HeavyPistol ) ) goto jmp;break; \
case 2: if ( !Config::Get<bool>( Vars.##name##SMG ) ) goto jmp;break; \
case 3: if ( !Config::Get<bool>( Vars.##name##Rifle ) ) goto jmp;break; \
case 4: if ( !Config::Get<bool>( Vars.##name##Shotgun ) ) goto jmp;break; \
case 5: if ( !Config::Get<bool>( Vars.##name##AWP ) ) goto jmp;break; \
case 6: if ( !Config::Get<bool>( Vars.##name##Scout ) ) goto jmp;break; \
case 7: if ( !Config::Get<bool>( Vars.##name##Auto ) ) goto jmp;break; \
case 8: if ( !Config::Get<bool>( Vars.##name##Machine ) ) goto jmp;break; }\

void Menu::RenderSubtabs( std::vector<std::string> subTabs, int& activeSubtab ) {
	const Vector2D SubTabPos = Pos + Vector2D( 20, 55 );
	const Vector2D SubTabSize = Vector2D( ( Size.x - 40 ) / subTabs.size( ), 1 );
	int i{ };
	for ( const auto& SubTab : subTabs ) {
		const auto TextSize = Render::GetTextSize( SubTab, Fonts::Menu );

		const auto Hov = Inputsys::hovered( SubTabPos + Vector2D( SubTabSize.x * i, 0 ), Vector2D( SubTabSize.x, TextSize.y - 5 ) ) && Menu::OpenedID == "";
		if ( Hov && Inputsys::pressed( VK_LBUTTON ) )
			activeSubtab = i;

		Render::Text( Fonts::Menu, SubTabPos.x + SubTabSize.x * i + SubTabSize.x / 2, SubTabPos.y + 5, activeSubtab == i ? Color( 200, 200, 200 ) : Hov ? Color( 150, 150, 150 ) : Color( 80, 80, 80 ), FONT_CENTER, SubTab.c_str( ) );


		i++;
	}

	//Render::FilledRectangle( SubTabPos, Vector2D( Size.x - 40, 1 ), Color( 80, 80, 80 ) );
	//Render::FilledRectangle( SubTabPos + Vector2D( SubTabSize.x * activeSubtab, 0 ), SubTabSize, AccentCol );
}

void Menu::GetElements( ) {
	if ( MenuAlpha <= 0 )
		return;

	Menu::FrameAfterFocus = false;

	static int activeTab{ };
	{
		const char* tabs[ 4 ] = { _( "A" ), _( "C" ), _( "D" ), _( "E" ) };

		const auto TextSize = Render::GetTextSize( _( "A" ), Fonts::MenuTabs );
		Vector2D TabPos = Pos + Vector2D( Size.x / 2 - ( TextSize.x + 5 ) * 2, 8 );
		int i{ };
		for ( auto tab : tabs ) {
			const auto Hov = Inputsys::hovered( TabPos + Vector2D( i * ( TextSize.x + 10 ), 0 ), TextSize ) && Menu::OpenedID == "";
			if ( Hov && Inputsys::pressed( VK_LBUTTON ) )
				activeTab = i;

			Render::Text( Fonts::MenuTabs, TabPos.x + i * ( TextSize.x + 10 ), TabPos.y, activeTab == i ? Color( 200, 200, 200 ) : Hov ? Color( 150, 150, 150 ) : Color( 80, 80, 80 ), FONT_LEFT, tab );
			i++;
		}
	}

	switch ( activeTab ) {
	case 0:
		DrawRage( );
		break;
	case 1:
		DrawVisual( );
		break;
	case 2:
		DrawMisc( );
		break;
	case 3:
		DrawConfig( );
		break;
	}

	if ( Menu::MultiDropdown.has_value( ) ) {
		uint8_t i{ 1 };
		const auto drop = Menu::MultiDropdown.value( );
		for ( const auto o : drop.options ) {
			const bool Hov = Inputsys::hovered( drop.pos + Vector2D( 0, drop.size.y * i ), drop.size );

			Render::FilledRectangle( drop.pos + Vector2D( 0, drop.size.y * i ), drop.size, *o.value ? Color( 45, 45, 45 ) : Hov ? Color( 25, 25, 25 ) : Color( 20, 20, 20 ) );

			Render::Text( Fonts::Menu, drop.pos.x, drop.pos.y + drop.size.y * i, Color( 255, 255, 255 ), 0, o.name.c_str( ) );

			i++;
		}
	}

	if ( Menu::Dropdown.has_value( ) ) {
		uint8_t i{ 1 };
		const auto drop = Menu::Dropdown.value( );
		for ( const auto o : drop.options ) {
			const bool Hov = Inputsys::hovered( drop.pos + Vector2D( 0, drop.size.y * i ), drop.size );

			Render::FilledRectangle( drop.pos + Vector2D( 0, drop.size.y * i ), drop.size, i - 1 == drop.value ? Color( 45, 45, 45 ) : Hov ? Color( 25, 25, 25 ) : Color( 20, 20, 20 ) );

			Render::Text( Fonts::Menu, drop.pos.x, drop.pos.y + drop.size.y * i, Color( 255, 255, 255 ), 0, o.c_str( ) );

			i++;
		}
	}

	if ( Menu::ColorPicker.has_value( ) ) {
		const auto Size = Vector2D( 200, 200 );
		Render::FilledRectangle( Menu::ColorPicker.value( ).pos, Size, Menu::BackgroundCol );
		Render::Rectangle( Menu::ColorPicker.value( ).pos, Size, Menu::OutlineLight );

		const auto hue_draw_pos = Menu::ColorPicker.value( ).pos + Vector2D( 170, 9 );// -1 here shud be 10
		for ( int i = 0; i < 150; i++ ) {
			float hue = ( ( float )i / 150 ) * 360.f;
			Color hue_color = hsv2rgb( { hue, 1, 1 } );

			Render::Line( hue_draw_pos + Vector2D( 0, i + 1 ), hue_draw_pos + Vector2D( 20, i + 1 ), hue_color );
		}

		if ( Menu::ColorPicker.value( ).alpha ) {
			// checker pattern part two
			const auto alpha_draw_pos = Menu::ColorPicker.value( ).pos + Vector2D( 10, 170 );
			for ( int i = 0; i < 2; i++ ) {
				Vector2D thing_pos = alpha_draw_pos + Vector2D( 0, 10 * i );
				Vector2D thing_size = Vector2D( 10, 10 );

				for ( int n = 0; n < 150 / 10; ++n ) {
					Color thing_color = Color( 225, 225, 225 );
					if ( ( i + n ) % 2 == 0 ) {
						thing_color = Color( 255, 255, 255 );
					}

					Render::FilledRectangle( thing_pos + Vector2D( 10 * n, 0 ), thing_size, thing_color );
				}
			}

			Render::Gradient( alpha_draw_pos.x, alpha_draw_pos.y, 150, 20, Color( 0, 0, 0, 0 ), Menu::ColorPicker.value( ).Color.Set<COLOR_A>( 255 ), true );
		}

		// actual picker
		Render::Gradient( Menu::ColorPicker.value( ).pos.x + 10, Menu::ColorPicker.value( ).pos.y + 10, 150, 150, Color( 255, 255, 255 ), hsv2rgb( { Menu::ColorPicker.value( ).hue, 1.f, 1.f } ), true );
		Interfaces::Surface->DrawSetColor( Color( 0, 0, 0 ) );
		Interfaces::Surface->DrawFilledRectFade( Menu::ColorPicker.value( ).pos.x + 10, Menu::ColorPicker.value( ).pos.y + 10, Menu::ColorPicker.value( ).pos.x + 160, Menu::ColorPicker.value( ).pos.y + 160, 0, 255, false );

		Render::Gradient( Menu::ColorPicker.value( ).pos.x + 10, Menu::ColorPicker.value( ).pos.y + 10, 150, 150, Color( 0, 0, 0, 0 ), Color( 0, 0, 0 ), false );
	}

	if ( Menu::KeyBind.has_value( ) ) {
		std::string options[ 4 ]{ _( "Always on" ), _( "Toggle" ), _( "Hold" ), _( "Off hotkey" ) };

		const Vector2D size = { 100, 18 };
		int i{ 1 };
		for ( const auto o : options ) {
			const bool Hov = Inputsys::hovered( Menu::KeyBind->pos + Vector2D( 0, size.y * i ), size );

			Render::FilledRectangle( Menu::KeyBind->pos + Vector2D( 0, size.y * i ), size, i - 1 == Menu::KeyBind->key.mode ? Color( 45, 45, 45 ) : Hov ? Color( 25, 25, 25 ) : Color( 20, 20, 20 ) );

			Render::Text( Fonts::Menu, Menu::KeyBind->pos.x, Menu::KeyBind->pos.y + size.y * i, Color( 255, 255, 255 ), 0, o.c_str( ) );

			i++;
		}
	}

	if ( Inputsys::down( VK_LBUTTON ) )
		Render::Circle( Inputsys::MousePos.x, Inputsys::MousePos.y, 5, 100, Menu::OutlineLight );

	Dropdown.reset( );
	MultiDropdown.reset( );
	ColorPicker.reset( );
	KeyBind.reset( );
}

void Menu::DrawRage( ) {
	const Vector2D CompensatedLength = Vector2D( ( ( Size.x - APPEND_LEN.x ) / 2 ), Size.y - APPEND_LEN.y );

	static int ActiveSubTab{ };
	RenderSubtabs( { _( "Aimbot" ), _( "Anti-aim" ) }, ActiveSubTab );

	if ( ActiveSubTab == 0 ) {
		static int weapGroup{ };

		{
			static auto RagebotGroup{ std::make_unique< MenuGroup >( ) };
			RagebotGroup->Begin( _( "General" ), CompensatedLength );
			{
				RagebotGroup->Checkbox( _( "Enable" ), Config::Get<bool>( Vars.RagebotEnable ) );
				RagebotGroup->Combo( _( "Weapon group" ), weapGroup, { _( "Pistol" ), _( "Heavy pistol" ), _( "SMG" ), _( "Rifle" ), _( "Shotgun" ), _( "Awp" ), ( "Scout" ), ( "Auto" ), _( "Machine gun" ) } );
				RAGEINTSLIDER( RagebotGroup, _( "FOV" ), RagebotFOV, weapGroup, 1, 180 );
				RAGECHECKBOX( RagebotGroup, _( "Auto fire" ), RagebotAutoFire, weapGroup );
				RAGECHECKBOX( RagebotGroup, _( "Silent aim" ), RagebotSilentAim, weapGroup );
				RAGEINTSLIDER( RagebotGroup, _( "Hitchance" ), RagebotHitchance, weapGroup, 0, 100 );
				RAGECHECKBOX( RagebotGroup, _( "Ensure accuracy" ), RagebotHitchanceThorough, weapGroup );
				RAGEINTSLIDER( RagebotGroup, _( "Minimum damage" ), RagebotMinimumDamage, weapGroup, 0, 110 );
				RAGECHECKBOX( RagebotGroup, _( "Autowall" ), RagebotAutowall, weapGroup );

				CHECKRAGEBOOL( weapGroup, RagebotAutowall, NEXTLOL )
				RAGEINTSLIDER( RagebotGroup, _( "Penetration damage" ), RagebotPenetrationDamage, weapGroup, 0, 110 );
				NEXTLOL:

				RAGECHECKBOX( RagebotGroup, _( "Scale damage" ), RagebotScaleDamage, weapGroup );
				RagebotGroup->Checkbox( _( "Damage override" ), Config::Get<bool>( Vars.RagebotDamageOverride ) );
				RagebotGroup->Keybind( _( "Damage override key" ), Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ) );

				RAGEINTSLIDER( RagebotGroup, _( "Override damage" ), RagebotOverrideDamage, weapGroup, 0, 110 );
				RAGECHECKBOX( RagebotGroup, _( "Autostop" ), RagebotAutoStop, weapGroup );
				RAGECHECKBOX( RagebotGroup, _( "Move between shots" ), RagebotBetweenShots, weapGroup );
				RagebotGroup->Checkbox( _( "Zeusbot" ), Config::Get<bool>( Vars.RagebotZeusbot ) );
				RagebotGroup->Checkbox( _( "Doubletap" ), Config::Get<bool>( Vars.ExploitsDoubletap ) );
				RagebotGroup->Keybind( _( "Doubletap key" ), Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ) );

				RagebotGroup->Checkbox( _( "Lag peek" ), Config::Get<bool>( Vars.ExploitsDoubletapDefensive ) );
				RagebotGroup->Checkbox( _( "Hideshots" ), Config::Get<bool>( Vars.ExploitsHideshots ) );
				RagebotGroup->Keybind( _( "Hideshots key" ), Config::Get<keybind_t>( Vars.ExploitsHideshotsKey ) );
			}
			RagebotGroup->End( );
		}
		{
			static auto RagebotGroup{ std::make_unique< MenuGroup >( ) };
			RagebotGroup->Begin( _( "General" ), CompensatedLength );
			{
				RagebotGroup->Combo( _( "Target selection" ), Config::Get<int>( Vars.RagebotTargetSelection ), { _( "Highest damage" ), _( "FOV" ), _( "Distance" ), _( "Lowest health" ) } );

				std::vector<multi_item_t> ItemsPistol;
				std::vector<multi_item_t> ItemsHeavyPistol;
				std::vector<multi_item_t> ItemsSMG;
				std::vector<multi_item_t> ItemsRifle;
				std::vector<multi_item_t> ItemsShotgun;
				std::vector<multi_item_t> ItemsAWP;
				std::vector<multi_item_t> ItemsScout;
				std::vector<multi_item_t> ItemsAuto;
				std::vector<multi_item_t> ItemsMachine;

				ItemsPistol = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadPistol ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestPistol ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestPistol ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestPistol ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachPistol ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisPistol ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsPistol ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsPistol ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetPistol ) }, };
				ItemsHeavyPistol = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadHeavyPistol ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestHeavyPistol ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestHeavyPistol ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestHeavyPistol ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachHeavyPistol ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisHeavyPistol ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsHeavyPistol ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsHeavyPistol ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetHeavyPistol ) }, };
				ItemsSMG = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadSMG ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestSMG ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestSMG ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestSMG ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachSMG ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisSMG ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsSMG ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsSMG ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetSMG ) }, };
				ItemsRifle = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadRifle ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestRifle ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestRifle ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestRifle ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachRifle ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisRifle ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsRifle ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsRifle ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetRifle ) }, };
				ItemsShotgun = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadShotgun ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestShotgun ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestShotgun ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestShotgun ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachShotgun ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisShotgun ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsShotgun ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsShotgun ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetShotgun ) }, };
				ItemsAWP = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadAWP ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestAWP ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestAWP ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestAWP ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachAWP ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisAWP ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsAWP ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsAWP ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetAWP ) }, };
				ItemsScout = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadScout ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestScout ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestScout ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestScout ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachScout ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisScout ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsScout ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsScout ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetScout ) }, };
				ItemsAuto = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadAuto ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestAuto ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestAuto ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestAuto ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachAuto ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisAuto ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsAuto ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsAuto ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetAuto ) }, };
				ItemsMachine = { { "Head", &Config::Get<bool>( Vars.RagebotHBHeadMachine ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotHBUpperChestMachine ) }, { "Chest", &Config::Get<bool>( Vars.RagebotHBChestMachine ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotHBLowerChestMachine ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotHBStomachMachine ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotHBPelvisMachine ) }, { "Arms", &Config::Get<bool>( Vars.RagebotHBArmsMachine ) }, { "Legs", &Config::Get<bool>( Vars.RagebotHBLegsMachine ) }, { "Feet", &Config::Get<bool>( Vars.RagebotHBFeetMachine ) }, };
				RAGEMULTICOMBO( RagebotGroup, "Hitboxes", Items, weapGroup );

				ItemsPistol = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadPistol ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestPistol ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestPistol ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestPistol ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachPistol ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisPistol ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsPistol ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsPistol ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetPistol ) }, };
				ItemsHeavyPistol = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadHeavyPistol ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestHeavyPistol ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestHeavyPistol ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestHeavyPistol ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachHeavyPistol ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisHeavyPistol ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsHeavyPistol ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsHeavyPistol ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetHeavyPistol ) }, };
				ItemsSMG = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadSMG ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestSMG ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestSMG ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestSMG ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachSMG ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisSMG ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsSMG ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsSMG ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetSMG ) }, };
				ItemsRifle = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadRifle ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestRifle ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestRifle ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestRifle ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachRifle ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisRifle ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsRifle ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsRifle ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetRifle ) }, };
				ItemsShotgun = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadShotgun ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestShotgun ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestShotgun ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestShotgun ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachShotgun ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisShotgun ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsShotgun ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsShotgun ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetShotgun ) }, };
				ItemsAWP = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadAWP ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestAWP ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestAWP ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestAWP ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachAWP ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisAWP ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsAWP ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsAWP ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetAWP ) }, };
				ItemsScout = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadScout ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestScout ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestScout ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestScout ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachScout ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisScout ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsScout ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsScout ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetScout ) }, };
				ItemsAuto = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadAuto ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestAuto ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestAuto ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestAuto ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachAuto ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisAuto ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsAuto ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsAuto ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetAuto ) }, };
				ItemsMachine = { { "Head", &Config::Get<bool>( Vars.RagebotMPHeadMachine ) }, { "Upper chest", &Config::Get<bool>( Vars.RagebotMPUpperChestMachine ) }, { "Chest", &Config::Get<bool>( Vars.RagebotMPChestMachine ) }, { "Lower chest", &Config::Get<bool>( Vars.RagebotMPLowerChestMachine ) }, { "Stomach", &Config::Get<bool>( Vars.RagebotMPStomachMachine ) }, { "Pelvis", &Config::Get<bool>( Vars.RagebotMPPelvisMachine ) }, { "Arms", &Config::Get<bool>( Vars.RagebotMPArmsMachine ) }, { "Legs", &Config::Get<bool>( Vars.RagebotMPLegsMachine ) }, { "Feet", &Config::Get<bool>( Vars.RagebotMPFeetMachine ) }, };
				RAGEMULTICOMBO( RagebotGroup, "Multipoints", Items, weapGroup );

				RAGECHECKBOX( RagebotGroup, _( "Static pointscale" ), RagebotStaticPointscale, weapGroup );

				CHECKRAGEBOOL( weapGroup, RagebotStaticPointscale, NEXTSTATIC );
				RAGEINTSLIDER( RagebotGroup, _( "Head scale" ), RagebotHeadScale, weapGroup, 0, 100 );
				RAGEINTSLIDER( RagebotGroup, _( "Body scale" ), RagebotBodyScale, weapGroup, 0, 100 );
			NEXTSTATIC:

				RAGECHECKBOX( RagebotGroup, _( "Ignore limbs when moving" ), RagebotIgnoreLimbs, weapGroup );
				RagebotGroup->Checkbox( _( "Force baim after" ), Config::Get<bool>( Vars.RagebotForceBaimAfterX ) );
				RagebotGroup->Slider( _( "X shots" ), Config::Get<int>( Vars.RagebotForceBaimAfterXINT ), 1, 20 );


				ItemsPistol = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimPistol ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapPistol ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalPistol ) } };
				ItemsHeavyPistol = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimHeavyPistol ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapHeavyPistol ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalHeavyPistol ) } };
				ItemsSMG = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimSMG ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapSMG ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalSMG ) } };
				ItemsRifle = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimRifle ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapRifle ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalRifle ) } };
				ItemsShotgun = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimShotgun ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapShotgun ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalShotgun ) } };
				ItemsAWP = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimAWP ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapAWP ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalAWP ) } };
				ItemsScout = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimScout ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapScout ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalScout ) } };
				ItemsAuto = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimAuto ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapAuto ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalAuto ) } };
				ItemsMachine = { { "Always", &Config::Get<bool>( Vars.RagebotPreferBaimMachine ) }, { "Doubletap", &Config::Get<bool>( Vars.RagebotPreferBaimDoubletapMachine ) }, { "Lethal", &Config::Get<bool>( Vars.RagebotPreferBaimLethalMachine ) } };
				RAGEMULTICOMBO( RagebotGroup, "Prefer baim", Items, weapGroup );

				RagebotGroup->Label( _( "Force baim" ) );
				RagebotGroup->Keybind( _( "Force baim key" ), Config::Get<keybind_t>( Vars.RagebotForceBaimKey ) );
			}
			RagebotGroup->End( );
		}
	}
	else {
		{
			static auto AntiaimGroup{ std::make_unique< MenuGroup >( ) };
			AntiaimGroup->Begin( _( "General" ), CompensatedLength );
			{
				AntiaimGroup->Checkbox( _( "Enable" ), Config::Get<bool>( Vars.AntiaimEnable ) );
				AntiaimGroup->Combo( _( "Pitch" ), Config::Get<int>( Vars.AntiaimPitch ), { _( "Default" ), _( "Up" ), _( "Down" ), _( "Zero" ) } );
				AntiaimGroup->Combo( _( "Yaw" ), Config::Get<int>( Vars.AntiaimYaw ), { _( "Default" ), _( "Backward" ), _( "Rotate" ), _( "Jitter" ) } );
				if ( Config::Get<int>( Vars.AntiaimYaw ) == 2 || Config::Get<int>( Vars.AntiaimYaw ) == 3 )
					AntiaimGroup->Slider( _( "Yaw range" ), Config::Get<int>( Vars.AntiaimYawRange ), 2, 180 );

				if ( Config::Get<int>( Vars.AntiaimYaw ) == 2 )
					AntiaimGroup->Slider( _( "Yaw speed" ), Config::Get<int>( Vars.AntiaimYawSpeed ), 1, Config::Get<int>( Vars.AntiaimYawRange ) / 2 );

				AntiaimGroup->Combo( _( "At targets" ), Config::Get<int>( Vars.AntiaimAtTargets ), { _( "Off" ), _( "FOV" ), _( "Distance" ) } );

				AntiaimGroup->Checkbox( _( "Desync" ), Config::Get<bool>( Vars.AntiaimDesync ) );

				AntiaimGroup->Checkbox( _( "Anti backstab" ), Config::Get<bool>( Vars.AntiaimAntiBackStab ) );

				AntiaimGroup->Label( _( "Flip desync" ) );
				AntiaimGroup->Keybind( _( "Flip desync key" ), Config::Get<keybind_t>( Vars.AntiaimInvert ) );

				AntiaimGroup->Label( _( "Constant invert" ) );
				AntiaimGroup->Keybind( _( "Constant Invert key" ), Config::Get<keybind_t>( Vars.AntiaimInvertSpam ) );

				//AntiaimGroup->Combo( _( "Auto direction" ), Config::Get<int>( Vars.AntiaimFreestand ), { _( "Off" ), _( "Desync side" ), _( "Yaw" ) } );

				AntiaimGroup->Checkbox( _( "Manual direction" ), Config::Get<bool>( Vars.AntiAimManualDir ) );
				AntiaimGroup->ColorPicker( _( "Manual direction Color" ), Config::Get<Color>( Vars.AntiaimManualCol ) );
				AntiaimGroup->Label( _( "Left" ) );
				AntiaimGroup->Keybind( _( "Left key" ), Config::Get<keybind_t>( Vars.AntiaimLeft ) );
				Config::Get<keybind_t>( Vars.AntiaimLeft ).mode = EKeyMode::Toggle;
				AntiaimGroup->Label( _( "Right" ) );
				AntiaimGroup->Keybind( _( "Right key" ), Config::Get<keybind_t>( Vars.AntiaimRight ) );
				Config::Get<keybind_t>( Vars.AntiaimRight ).mode = EKeyMode::Toggle;
			}

			AntiaimGroup->End( );
		}

		//NewGroupRow( );

		{
			static auto AntiaimGroup{ std::make_unique< MenuGroup >( ) };
			AntiaimGroup->Begin( _( "Fakelag" ), Vector2D( CompensatedLength.x, ( Size.y - 119 ) / 2 - 10 ) );
			{
				AntiaimGroup->Slider( _( "Limit" ), Config::Get<int>( Vars.AntiaimFakeLagLimit ), 0, 15 );
				AntiaimGroup->Slider( _( "Randomization" ), Config::Get<int>( Vars.AntiaimFakeLagVariance ), 0, 100 );
				AntiaimGroup->Checkbox( _( "Break lagcompensation" ), Config::Get<bool>( Vars.AntiaimFakeLagBreakLC ) );
			}
			AntiaimGroup->End( );
		}


		//NewGroupRow( CompensatedLength.x );
	}
}

void Menu::DrawMisc( ) {
	const Vector2D CompensatedLength = Vector2D( ( ( Size.x - APPEND_LEN.x ) / 2 ), Size.y - APPEND_LEN.y );

	{
		static auto MiscGroup{ std::make_unique< MenuGroup >( ) };
		MiscGroup->Begin( _( "General" ), CompensatedLength );
		{
			/* propaganda section */
			MiscGroup->Checkbox( _( "Watermark" ), Config::Get<bool>( Vars.MiscWatermark ) );
			MiscGroup->Label( _( "Accent color" ) );
			MiscGroup->ColorPicker( _( "Accent Color" ), Menu::AccentCol, false );
			MiscGroup->Checkbox( _( "Clantag" ), Config::Get<bool>( Vars.MiscClantag ) );

			MiscGroup->Checkbox( _( "Keybinds list" ), Config::Get<bool>( Vars.MiscKeybindList ) );
			MiscGroup->Checkbox( _( "Aspect ratio" ), Config::Get<bool>( Vars.MiscAspectRatio ) );
			if ( Config::Get<bool>( Vars.MiscAspectRatio ) )
				MiscGroup->Slider( _( "Ratio" ), Config::Get<float>( Vars.MiscAspectRatioAmt ), 0.02f, 5.f );

			MiscGroup->Slider( _( "Field of view" ), Config::Get<int>( Vars.MiscFOV ), 0, 70 );

			/*MiscGroup->Combo( _( "Player model changer T" ), Config::Get<int>( Vars.MiscPlayerModelT ),
				{ "Default",
				"Enforcer",
				"Soldier",
				"Ground Rebel",
				"Maximus",
				"Osiris",
				"Slingshot",
				"Dragomir",
				"Blackwolf",
				"Prof. Shahmat",
				"Rezan The Ready",
				"Doctor Romanov",
				"Mr. Muhlik" } );

			MiscGroup->Combo( _( "Player model changer CT" ), Config::Get<int>( Vars.MiscPlayerModelCT ),
				{ "Default",
				"Seal Team 6",
				"3rd Commando",
				"Operator FBI",
				"Squadron Officer",
				"Markus Delrow",
				"Buckshot",
				"McCoy",
				"Commander Ricksaw",
				"Agent Ava" } );*/

			MiscGroup->Checkbox( _( "Preserve killfeed" ), Config::Get<bool>( Vars.MiscPreserveKillfeed ) );
			MiscGroup->Checkbox( _( "Watermark" ), Config::Get<bool>( Vars.MiscWatermark ) );
			MiscGroup->Checkbox( _( "Force crosshair" ), Config::Get<bool>( Vars.MiscForceCrosshair ) );
			MiscGroup->Slider( _( "Weapon volume" ), Config::Get<int>( Vars.MiscWeaponVolume ), 0, 100 );
			MiscGroup->Checkbox( _( "Hit marker" ), Config::Get<bool>( Vars.MiscHitmarker ) );
			MiscGroup->ColorPicker( _( "Hit marker Color" ), Config::Get<Color>( Vars.MiscHitmarkerCol ) );
			MiscGroup->Checkbox( _( "Damage marker" ), Config::Get<bool>( Vars.MiscDamageMarker ) );
			MiscGroup->ColorPicker( _( "Damage marker Color" ), Config::Get<Color>( Vars.MiscDamageMarkerCol ) );
			MiscGroup->Combo( _( "Hitsound" ), Config::Get<int>( Vars.MiscHitSound ), { _( "None" ), _( "Metallic" ), _( "Custom" ) } );

			if ( Config::Get<int>( Vars.MiscHitSound ) == 2 )
				MiscGroup->TextInput( Config::Get<std::string>( Vars.MiscCustomHitSound ) );

			MiscGroup->Checkbox( "Auto buy", Config::Get<bool>( Vars.MiscBuyBot ) );
			if ( Config::Get<bool>( Vars.MiscBuyBot ) ) {
				MiscGroup->Combo( "Primary weapons", Config::Get<int>( Vars.MiscBuyBotPrimary ), { "None", "Autosniper", "SSG-08", "awp", "negev", "ak-47/m4" } );
				MiscGroup->Combo( "Secondary weapons", Config::Get<int>( Vars.MiscBuyBotSecondary ), { "None", "Deagle/r8", "Dualies", "USP/Glock", "Tec9/Fiveseven" } );
				MiscGroup->MultiCombo( _( "Other" ), buyother );
			}

			if ( MiscGroup->Button( _( "Reset menu size" ) ) ) {
				Features::Logger.Log( _( "Menu size reset" ), true );
				Menu::Size = { 800, 550 };
			}
		}
		MiscGroup->End( );
	}

	{
		static auto MiscGroup{ std::make_unique< MenuGroup >( ) };
		MiscGroup->Begin( _( "Movement" ), CompensatedLength );
		{
			MiscGroup->Checkbox( _( "Bunnyhop" ), Config::Get<bool>( Vars.MiscBunnyhop ) );
			MiscGroup->Checkbox( _( "Autostrafer" ), Config::Get<bool>( Vars.MiscAutostrafe ) );
			//MiscGroup->Checkbox( _( "Crouch in air" ), Config::Get<bool>( Vars.MiscCrouchInAir ) );
			//MiscGroup->Checkbox( _( "Accurate walk" ), Config::Get<bool>( Vars.MiscAccurateWalk ) );
			MiscGroup->Checkbox( _( "Infinite Stamina" ), Config::Get<bool>( Vars.MiscInfiniteStamina ) );
			MiscGroup->Checkbox( _( "Quick stop" ), Config::Get<bool>( Vars.MiscQuickStop ) );
			MiscGroup->Checkbox( _( "Slide walk" ), Config::Get<bool>( Vars.MiscSlideWalk ) );
			MiscGroup->Checkbox( _( "Slow walk" ), Config::Get<bool>( Vars.MiscSlowWalk ) );
			MiscGroup->Keybind( _( "Slow walk key" ), Config::Get<keybind_t>( Vars.MiscSlowWalkKey ) );

			if ( Config::Get<bool>( Vars.MiscInfiniteStamina ) ) {
				MiscGroup->Checkbox( _( "Fake duck" ), Config::Get<bool>( Vars.MiscFakeDuck ) );
				MiscGroup->Keybind( _( "Fake duck key" ), Config::Get<keybind_t>( Vars.MiscFakeDuckKey ) );
			}

			MiscGroup->Checkbox( _( "Auto peek" ), Config::Get<bool>( Vars.MiscAutoPeek ) );
			MiscGroup->Keybind( _( "Auto peek key" ), Config::Get<keybind_t>( Vars.MiscAutoPeekKey ) );
			MiscGroup->Label( _( "Auto peek color" ) );
			MiscGroup->ColorPicker( _( "Auto peek color" ), Config::Get<Color>( Vars.MiscAutoPeekCol ) );
		}
		MiscGroup->End( );
	}
}

void Menu::DrawConfig( ) {

	{
		static auto CfgGroup = std::make_unique< MenuGroup >( );
		CfgGroup->Begin( _( "" ), Size - Vector2D( 40, 140 ) );

		Config::Refresh( );

		static int SelectedCfg{ };
		CfgGroup->ListBox( _( "Configs" ), Config::vecFileNames, SelectedCfg, Size - Vector2D( 70, Size.y * 0.75f ) );
		const auto Fig = Config::vecFileNames.empty( ) ? "" : Config::vecFileNames.at( SelectedCfg );

		static std::string Name;
		CfgGroup->TextInput( Name );

		if ( CfgGroup->Button( _( "Create" ) ) ) {
			Config::Save( Name );
			Features::Logger.Log( _( "Created config " ) + Name, true );
		}

		static float saveConfirm{ };
		static float loadConfirm{ };
		static float removeConfirm{ };

		if ( Interfaces::Globals->flRealTime - removeConfirm > 3.f ) {
			if ( CfgGroup->Button( _( "Remove" ) ) )
				removeConfirm = Interfaces::Globals->flRealTime;
		}
		else if ( CfgGroup->Button( _( "Are you sure?" ) ) ) {
			Config::Remove( SelectedCfg );
			Features::Logger.Log( _( "Removed config " ) + Fig, true );
			removeConfirm = 0;
		}

		if ( Interfaces::Globals->flRealTime - loadConfirm > 3.f ) {
			if ( CfgGroup->Button( _( "Load" ) ) )
				loadConfirm = Interfaces::Globals->flRealTime;
		}
		else if ( CfgGroup->Button( _( "Are you sure?" ) ) ) {
			Config::Load( Fig );
			Features::Logger.Log( _( "Loaded config " ) + Fig, true );
			loadConfirm = 0;
		}

		if ( Interfaces::Globals->flRealTime - saveConfirm > 3.f ) {
			if ( CfgGroup->Button( _( "Save" ) ) )
				saveConfirm = Interfaces::Globals->flRealTime;
		}
		else if ( CfgGroup->Button( _( "Are you sure?" ) ) ) {
			Config::Save( Fig );
			Features::Logger.Log( _( "Saved config " ) + Fig, true );
			saveConfirm = 0;
		}

		CfgGroup->End( );
	}
}

void Menu::DrawVisual( ) {
	static int ActiveSubTab{ };
	RenderSubtabs( { _( "Local" ), _( "Team" ), _( "Enemy" ), _( "Other" ) }, ActiveSubTab );

	const Vector2D CompensatedLength = Vector2D( ( ( Size.x - APPEND_LEN.x ) / 2 ), Size.y - APPEND_LEN.y );

	if ( ActiveSubTab < 3 ) {
		{
			static auto EspGroup{ std::make_unique< MenuGroup >( ) };
			EspGroup->Begin( _( "ESP" ), CompensatedLength );
			{
				PlayerCheckbox( EspGroup, _( "Enabled" ), VisEnable, ActiveSubTab );
				PlayerCheckbox( EspGroup, _( "Name" ), VisName, ActiveSubTab );
				PlayerColorPicker( EspGroup, _( "NameCol" ), VisNameCol, ActiveSubTab );
				PlayerCheckbox( EspGroup, _( "Box" ), VisBox, ActiveSubTab );
				PlayerColorPicker( EspGroup, _( "BoxCol" ), VisBoxCol, ActiveSubTab );
				PlayerCheckbox( EspGroup, _( "Health" ), VisHealth, ActiveSubTab );
				PlayerCheckbox( EspGroup, _( "Color override" ), VisHealthOverride, ActiveSubTab );
				PlayerColorPicker( EspGroup, _( "HealthCol" ), VisHealthCol, ActiveSubTab );
				PlayerCheckbox( EspGroup, _( "Ammo bar" ), VisAmmo, ActiveSubTab );
				PlayerColorPicker( EspGroup, _( "AmmoCol" ), VisAmmoCol, ActiveSubTab );
				PlayerCheckbox( EspGroup, _( "Skeleton" ), VisSkeleton, ActiveSubTab );
				PlayerColorPicker( EspGroup, _( "SkeletonCol" ), VisSkeletonCol, ActiveSubTab );
				PlayerCheckbox( EspGroup, _( "Glow" ), VisGlow, ActiveSubTab );
				PlayerColorPicker( EspGroup, _( "GlowCol" ), VisGlowCol, ActiveSubTab );
				if ( ActiveSubTab != 0 ) {
					PlayerCheckbox( EspGroup, _( "Out of FOV" ), VisOOF, ActiveSubTab );
					PlayerColorPicker( EspGroup, _( "Out of FOV Col" ), VisOOFCol, ActiveSubTab );
				}

				std::vector<multi_item_t> ItemsLocal;
				std::vector<multi_item_t> ItemsTeam;
				std::vector<multi_item_t> ItemsEnemy;

				ItemsLocal = { { "Weapon Text", &Config::Get<bool>( Vars.VisWeapIconLocal ) }, { "Weapon Icon", &Config::Get<bool>( Vars.VisWeapTextLocal ) } };
				ItemsTeam = { { "Weapon Text", &Config::Get<bool>( Vars.VisWeapIconTeam ) }, { "Weapon Icon", &Config::Get<bool>( Vars.VisWeapTextTeam ) } };
				ItemsEnemy = { { "Weapon Text", &Config::Get<bool>( Vars.VisWeapIconEnemy ) }, { "Weapon Icon", &Config::Get<bool>( Vars.VisWeapTextEnemy ) } };
				PlayerMultiCombo( EspGroup, "Weapon", Items, ActiveSubTab );

				const auto BackupCursorPos{ CursorPos };

				CursorPos -= Vector2D( 0, 18 );
				PlayerColorPicker( EspGroup, _( "WeaponCol" ), VisWeapCol, ActiveSubTab );
				CursorPos = BackupCursorPos;

				ItemsLocal = { { "Broke lagcompensation", &Config::Get<bool>( Vars.VisFlagBLCLocal ) }, { "C4", &Config::Get<bool>( Vars.VisFlagC4Local ) }, { "Armor", &Config::Get<bool>( Vars.VisFlagArmorLocal ) }, { "Flashed", &Config::Get<bool>( Vars.VisFlagFlashLocal ) }, { "Reloading", &Config::Get<bool>( Vars.VisFlagReloadLocal ) }, { "Scoped", &Config::Get<bool>( Vars.VisFlagScopedLocal ) }, { "Defusing", &Config::Get<bool>( Vars.VisFlagDefusingLocal ) }, };
				ItemsTeam = { { "Broke lagcompensation", &Config::Get<bool>( Vars.VisFlagBLCTeam ) }, { "C4", &Config::Get<bool>( Vars.VisFlagC4Team ) }, { "Armor", &Config::Get<bool>( Vars.VisFlagArmorTeam ) }, { "Flashed", &Config::Get<bool>( Vars.VisFlagFlashTeam ) }, { "Reloading", &Config::Get<bool>( Vars.VisFlagReloadTeam ) }, { "Scoped", &Config::Get<bool>( Vars.VisFlagScopedTeam ) }, { "Defusing", &Config::Get<bool>( Vars.VisFlagDefusingTeam ) }, };
				ItemsEnemy = { { "Broke lagcompensation", &Config::Get<bool>( Vars.VisFlagBLCEnemy ) }, { "Bomb", &Config::Get<bool>( Vars.VisFlagC4Enemy ) }, { "Armor", &Config::Get<bool>( Vars.VisFlagArmorEnemy ) }, { "Flashed", &Config::Get<bool>( Vars.VisFlagFlashEnemy ) }, { "Reloading", &Config::Get<bool>( Vars.VisFlagReloadEnemy ) }, { "Scoped", &Config::Get<bool>( Vars.VisFlagScopedEnemy ) }, { "Defusing", &Config::Get<bool>( Vars.VisFlagDefusingEnemy ) }, };

				PlayerMultiCombo( EspGroup, "Flags", Items, ActiveSubTab );
			}
			EspGroup->End( );
		}

		{
			static auto ChamGroup{ std::make_unique< MenuGroup >( ) };
			ChamGroup->Begin( _( "Chams" ), CompensatedLength );
			{
				PlayerCheckbox( ChamGroup, _( "Visible" ), ChamVis, ActiveSubTab );
				PlayerColorPicker( ChamGroup, _( "VisibleCol" ), ChamVisCol, ActiveSubTab );

				PlayerCheckbox( ChamGroup, _( "Hidden" ), ChamHid, ActiveSubTab );
				PlayerColorPicker( ChamGroup, _( "HiddenCol" ), ChamHidCol, ActiveSubTab );

				std::vector<std::string> items{ _( "Regular" ), _( "Flat" ), _( "Glow" ), _( "Metallic" ), _( "Galaxy" ) };
				PlayerCombo( ChamGroup, _( "Material" ), ChamMat, ActiveSubTab, items );

				PlayerCheckbox( ChamGroup, _( "Double layer" ), ChamDouble, ActiveSubTab );
				PlayerColorPicker( ChamGroup, _( "DoubleCol" ), ChamDoubleCol, ActiveSubTab );

				PlayerCombo( ChamGroup, _( "Double layer material" ), ChamDoubleMat, ActiveSubTab, items );

				if ( ActiveSubTab == 2 ) {
					ChamGroup->Checkbox( _( "Backtrack chams" ), Config::Get<bool>( Vars.ChamBacktrack ) );
					ChamGroup->ColorPicker( _( "Backtrack chams Col" ), Config::Get<Color>( Vars.ChamBacktrackCol ) );
					ChamGroup->Combo( _( "Backtrack chams material" ), Config::Get<int>( Vars.ChamBacktrackMat ), items );

					ChamGroup->Checkbox( _( "Onshot chams" ), Config::Get<bool>( Vars.MiscHitMatrix ) );
					ChamGroup->ColorPicker( _( "Onshot chams Color" ), Config::Get<Color>( Vars.MiscHitMatrixCol ) );
					ChamGroup->Combo( _( "Onshot chams material" ), Config::Get<int>( Vars.MiscHitMatrixMat ), items );
				}

				if ( ActiveSubTab == 0 ) {
					ChamGroup->Checkbox( _( "Desync chams" ), Config::Get<bool>( Vars.ChamDesync ) );
					ChamGroup->ColorPicker( _( "Desync chams Col" ), Config::Get<Color>( Vars.ChamDesyncCol ) );
					ChamGroup->Combo( _( "Desync chams material" ), Config::Get<int>( Vars.ChamDesyncMat ), items );
				}

				PlayerIntSlider( ChamGroup, _( "Glow strength" ), ChamGlowStrength, ActiveSubTab, 0, 100 );
			}
			ChamGroup->End( );
		}
	}
	// OTHER SUBTAB
	else {
		{
			static auto EspGroup{ std::make_unique< MenuGroup >( ) };
			EspGroup->Begin( _( "General" ), CompensatedLength );
			{
				EspGroup->MultiCombo( _( "Removals" ), removals );

				if ( Config::Get<bool>( Vars.RemovalZoom ) )
					EspGroup->Slider( _( "Second Scope Zoom" ), Config::Get<int>( Vars.SecondZoomAmt ), 0, 100 );

				EspGroup->Checkbox( _( "Penetration crosshair" ), Config::Get<bool>( Vars.VisPenetrationCrosshair ) );

				EspGroup->Checkbox( _( "Grenade prediction" ), Config::Get<bool>( Vars.VisGrenadePrediction ) );
				EspGroup->ColorPicker( _( "Grenade prediction Color" ), Config::Get<Color>( Vars.VisGrenadePredictionCol ) );

				EspGroup->Checkbox( _( "Thirdperson" ), Config::Get<bool>( Vars.VisThirdPerson ) );
				EspGroup->Keybind( _( "Thirdperson key" ), Config::Get<keybind_t>( Vars.VisThirdPersonKey ) );
				EspGroup->Slider( _( "Thirdperson distance" ), Config::Get<int>( Vars.VisThirdPersonDistance ), 30, 250 );

				EspGroup->Checkbox( _( "Dropped weapons" ), Config::Get<bool>( Vars.VisDroppedWeapon ) );
				EspGroup->ColorPicker( _( "Dropped weapons Color" ), Config::Get<Color>( Vars.VisDroppedWeaponCol ) );
				EspGroup->Checkbox( _( "Team grenades" ), Config::Get<bool>( Vars.VisGrenadesTeam ) );
				EspGroup->ColorPicker( _( "Team grenades Color" ), Config::Get<Color>( Vars.VisGrenadesTeamCol ) );
				EspGroup->Checkbox( _( "Enemy grenades" ), Config::Get<bool>( Vars.VisGrenadesEnemy ) );
				EspGroup->ColorPicker( _( "Enemy grenades Color" ), Config::Get<Color>( Vars.VisGrenadesEnemyCol ) );
				EspGroup->Checkbox( _( "Planted Bomb" ), Config::Get<bool>( Vars.VisBomb ) );
				EspGroup->Checkbox( _( "Local bullet impacts" ), Config::Get<bool>( Vars.VisLocalBulletImpacts ) );
				EspGroup->ColorPicker( _( "Local bullet impacts col" ), Config::Get<Color>( Vars.VisLocalBulletImpactsCol ) );
				EspGroup->Checkbox( _( "Other bullet impacts" ), Config::Get<bool>( Vars.VisServerBulletImpacts ) );
				EspGroup->ColorPicker( _( "Other bullet impacts col" ), Config::Get<Color>( Vars.VisServerBulletImpactsCol ) );

				/*EspGroup->Checkbox( _( "Local bullet tracers" ), Config::Get<bool>( Vars.VisLocalBulletTracers ) );
				EspGroup->ColorPicker( _( "Local bullet tracers col" ), Config::Get<Color>( Vars.VisLocalBulletTracersCol ) );
				
				EspGroup->Checkbox( _( "Other bullet tracers" ), Config::Get<bool>( Vars.VisOtherBulletTracers ) );
				EspGroup->ColorPicker( _( "Other bullet tracers col" ), Config::Get<Color>( Vars.VisOtherBulletTracersCol ) );

				EspGroup->Combo( _( "Bullet tracers type" ), Config::Get<int>( Vars.VisBulletTracersType ), { _( "Line" ), _( "Beam" ) } );*/
			}
			EspGroup->End( );
		}

		{
			static auto EspGroup{ std::make_unique< MenuGroup >( ) };
			EspGroup->Begin( _( "Modifications" ), CompensatedLength );
			{
				EspGroup->Checkbox( _( "Full bright" ), Config::Get<bool>( Vars.WorldFullbright ) );
				EspGroup->Checkbox( _( "World Modulation" ), Config::Get<bool>( Vars.WorldModulation ) );
				EspGroup->ColorPicker( _( "World Modulation Color" ), Config::Get<Color>( Vars.WorldModulationCol ), false );

				EspGroup->Checkbox( _( "Skybox Color Modulation" ), Config::Get<bool>( Vars.VisWorldSkyboxMod ) );
				EspGroup->ColorPicker( _( "Skybox Color" ), Config::Get<Color>( Vars.WorldSkyboxCol ), false );

				EspGroup->Checkbox( _( "Prop Color Modulation" ), Config::Get<bool>( Vars.VisWorldPropMod ) );
				EspGroup->ColorPicker( _( "Prop Color" ), Config::Get<Color>( Vars.VisWorldPropCol ) );

				EspGroup->Checkbox( _( "Ambient lighting" ), Config::Get<bool>( Vars.WorldAmbientLighting ) );
				EspGroup->ColorPicker( _( "Ambient lighting Color" ), Config::Get<Color>( Vars.WorldAmbientLightingCol ), false );

				EspGroup->Checkbox( _( "Bloom" ), Config::Get<bool>( Vars.VisWorldBloom ) );
				if ( Config::Get<bool>( Vars.VisWorldBloom ) ) {
					EspGroup->Slider( _( "Scale" ), Config::Get<int>( Vars.VisWorldBloomScale ), 0, 750 );
					EspGroup->Slider( _( "Ambience" ), Config::Get<int>( Vars.VisWorldBloomAmbience ), 0, 200 );
					EspGroup->Slider( _( "Exposure" ), Config::Get<int>( Vars.VisWorldBloomExposure ), 0, 200 );
				}

				EspGroup->Checkbox( _( "Fog" ), Config::Get<bool>( Vars.VisWorldFog ) );
				EspGroup->ColorPicker( _( "Fog color" ), Config::Get<Color>( Vars.VisWorldFogCol ) );
				if ( Config::Get<bool>( Vars.VisWorldFog ) ) {
					EspGroup->Slider( _( "Fog Distance" ), Config::Get<int>( Vars.VisWorldFogDistance ), 0, 3000 );
					EspGroup->Slider( _( "Fog Density" ), Config::Get<int>( Vars.VisWorldFogDensity ), 0, 100 );
					EspGroup->Slider( _( "Fog HDR" ), Config::Get<int>( Vars.VisWorldFogHDR ), 0, 100 );
				}

				EspGroup->Combo( _( "Skybox changer" ), Config::Get<int>( Vars.VisWorldSkybox ), {
					_( "Default" ),
					_( "cs_baggage_skybox_" ),
					_( "cs_tibet" ),
					_( "embassy" ),
					_( "italy" ),
					_( "jungle" ),
					_( "nukeblank" ),
					_( "office" ),
					_( "sky_csgo_cloudy01" ),
					_( "sky_csgo_night02" ),
					_( "sky_csgo_night02b" ),
					_( "sky_dust" ),
					_( "sky_venice" ),
					_( "vertigo" ),
					_( "vietnam" ),
					_( "sky_descent" )
					} );

			}
			EspGroup->End( );
		}
	}
}