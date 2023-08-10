#include "framework.h"
#define PAD 8.f

bool MenuFramework::CWindow::Create( const char* title ) {
	//if ( Inputsys::pressed( VK_INSERT ) )
	//	m_bOpen = !m_bOpen;

	Animate( m_bOpen, 10.f, m_flAnimation );

	static auto movePress{ false };
	if ( Inputsys::hovered( m_vecPosition, { m_vecSize.x - 30 - ( 30 * m_vecTabs.size( ) ), 25 } ) && Inputsys::down( 0x01 ) && !m_iActive )
		movePress = true;

	if ( movePress ) {
		m_vecPosition -= Inputsys::MouseDelta;
		movePress = Inputsys::down( 0x01 ) && !m_iActive;
	}

	static auto menuPress{ false };
	if ( Inputsys::hovered( m_vecPosition + m_vecSize - Vector2D{ 8, 8 }, { 8, 8 } ) && Inputsys::down( 0x01 ) && !m_iActive )
		menuPress = true;

	if ( menuPress ) {
		m_vecSize -= Inputsys::MouseDelta;
		menuPress = Inputsys::down( 0x01 ) && !m_iActive; m_vecSize.x = std::clamp( m_vecSize.x, 650.f, 1000.f ); m_vecSize.y = std::clamp( m_vecSize.y, 550.f, 1100.f );
	}

	Render::FilledRoundedBox( m_vecPosition + Vector2D{ 1, 1 }, m_vecSize, 5, 5, Accent::Accent.Set<COLOR_A>( MenuAlpha( ) ) );
	Render::FilledRoundedBox( m_vecPosition - Vector2D{ 1, 1 }, m_vecSize, 5, 5, Accent::Accent2.Set<COLOR_A>( MenuAlpha( ) ) );

	Render::FilledRoundedBox( m_vecPosition, m_vecSize, 5, 5, Accent::Background.Lerp( Color( 0.f, 0.f, 0.f ), .1f ).Set<COLOR_A>( MenuAlpha( ) ) );
	Render::FilledRoundedBox( m_vecPosition + Vector2D{ 1, 1 }, m_vecSize - Vector2D{ 2, 2 }, 3, 3, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( ) ) );
	Render::FilledRoundedBox( m_vecPosition + Vector2D{ 2, 2 }, m_vecSize - Vector2D{ 4, 4 }, 5, 5, Accent::Background.Lerp( Color( 0.f, 0.f, 0.f ), .1f ).Set<COLOR_A>( MenuAlpha( ) ) );

	const auto& barSize{ 75 };
	Render::Gradient( m_vecPosition.x + ( barSize * .75f ), m_vecPosition.y, ( m_vecSize.x - ( barSize * .75f ) ) * .2f, m_vecSize.y, Color( 0.f, 0.f, 0.f, MenuAlpha( 1.f ) ), Color( 0.f, 0.f, 0.f, 0.f ), true );

	Render::FilledRoundedBox( m_vecPosition, { barSize, m_vecSize.y }, 5, 5, Accent::Background.Set<COLOR_A>( MenuAlpha( ) ) );
	Render::FilledRoundedBox( m_vecPosition + Vector2D{ 2, 2 }, { barSize - 4, m_vecSize.y - 4 }, 5, 5, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( ) ) );
	Render::FilledRoundedBox( m_vecPosition + Vector2D{ 3, 3 }, { barSize - 6, m_vecSize.y - 6 }, 5, 5, Accent::Background.Set<COLOR_A>( MenuAlpha( ) ) );

	Vector2D tabArea{ m_vecPosition + Vector2D{ 3 + ( barSize * .5f ), 24 } };
	const auto gap{ ( m_vecSize.y - 50 ) / static_cast<float>( m_vecTabs.size( ) ) };

	auto idx{ 0 };
	for ( auto& cur : m_vecTabs ) {
		auto sz{ Render::GetTextSize( cur.m_strTitle, Fonts::MenuTabs ) };
		if ( Inputsys::hovered( tabArea - Vector2D{ barSize * .5f, 5 }, { barSize, sz.y + 10 } ) && Inputsys::pressed( 0x01 ) && !m_iActive )
			m_iTab = idx;

		AnimateLerp( m_iTab == idx, 8.f, cur.m_flAnim );

		Render::Text( Fonts::MenuTabs, tabArea + Vector2D{ -1, idx == 0 ? 0 : ( -14.f * cur.m_flAnim ) }, Color( .75f, .75f, .75f ).Lerp( ( idx % 2 == 0 ? Accent::Accent : Accent::Accent2 ).Lerp( Color( 1.f, 1.f, 1.f ), .25f ), cur.m_flAnim ).Set<COLOR_A>( MenuAlpha( 100.f + ( 155.f * cur.m_flAnim ) ) ), FONT_CENTER, cur.m_strTitle.c_str( ) );
		tabArea.y += gap;

		cur.m_vecRenderArea = m_vecPosition + Vector2D{
			barSize + PAD, PAD
		};

		cur.m_vecRenderSize = m_vecSize - Vector2D{
			barSize + ( PAD * 2 ), ( PAD * 2 )
		};

		idx++;
	}

	return ( m_flAnimation >= .1f || m_bOpen );
}

bool MenuFramework::CGroupbox::Open( const std::string& title, const Vector2D& position, const Vector2D& size ) {
	m_vecPos = std::move( position ); m_vecSz = std::move( size ); m_vecDraw = m_vecPos + Vector2D{ PAD * 2, PAD * 2.5f };

	AnimateLerp( !m_iActive, 10.f, m_flDimAnim );

	Render::FilledRoundedBox( m_vecPos, m_vecSz, 5, 5, Accent::Group.Lerp( Color( 0.f, 0.f, 0.f ), .1f ).Set<COLOR_A>( MenuAlpha( 100.f + ( 155.f * m_flDimAnim ) ) ) );
	Render::FilledRoundedBox( m_vecPos + Vector2D{ 1, 1 }, m_vecSz - Vector2D{ 2, 2 }, 3, 3, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( 255.f * m_flDimAnim ) ) );
	Render::FilledRoundedBox( m_vecPos + Vector2D{ 2, 2 }, m_vecSz - Vector2D{ 4, 4 }, 5, 5, Accent::Group.Lerp( Color( 0.f, 0.f, 0.f ), 0.f /*.1f*/ ).Set<COLOR_A>( MenuAlpha( 100.f + ( 155.f * m_flDimAnim ) ) ) );

	auto textX{ Render::GetTextSize( title, Fonts::Menu ).x + ( PAD * 2 ) };
	Render::FilledRectangle( m_vecPos + Vector2D{ PAD, 0.f }, { textX, 4.f, }, Accent::Group.Lerp( Color( 0.f, 0.f, 0.f ), 0.f /*.1f*/ ).Set<COLOR_A>( MenuAlpha( 100.f + ( 155.f * m_flDimAnim ) ) ) );
	Render::Text( Fonts::Menu, m_vecPos + Vector2D{ ( PAD * 2 ), -4.f }, Color( 1.f, 1.f, 1.f, MenuAlpha( .25f + ( .75f * m_flDimAnim ) ) ), FONT_LEFT, title.c_str( ) );
	
	return !m_bInit;
}

void MenuFramework::CGroupbox::Exit( const std::function<bool( )>& visible ) {
	m_bInit = true;
	if ( m_vecElements.empty( ) )
		return;

	if ( !visible( ) )
		return;

	Vector2D activeArea{ };
	std::optional<std::shared_ptr<CElement>> activeItem;

	auto refPoint = m_vecDraw;
	for ( auto& item : m_vecElements ) {
		item->m_vecContainerSize = m_vecSz;
		auto increment = item->Render( m_vecDraw );
		if ( m_iActive == item->m_iVarID ) { activeItem = item; activeArea = m_vecDraw; }
		m_vecDraw.y += increment;
	} if ( activeItem.has_value( ) )
		activeItem->get( )->Overlay( activeArea );
}

#define ITEMHEIGHT PAD * 1.25f

int MenuFramework::CCheckbox::Render( Vector2D& area ) {
	if ( Inputsys::hovered( area, { PAD * 2 + Render::GetTextSize( m_strTitle, Fonts::Menu ).x, PAD } ) && Inputsys::pressed( 0x01 ) && !m_iActive )
		Config::Get<bool>( m_iVarID ) = !Config::Get<bool>( m_iVarID );

	AnimateLerp( Config::Get<bool>( m_iVarID ), 10.f, m_flAnim[0] );
	AnimateLerp( !m_iActive, 10.f, m_flAnim[1] );

	Render::FilledRectangle( area, { ITEMHEIGHT, ITEMHEIGHT }, Accent::Control.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[1] ) ) ) );
	Render::FilledRectangle( area, { ITEMHEIGHT, ITEMHEIGHT }, Accent::Accent.Set<COLOR_A>( MenuAlpha( ( 55.f + ( 200.f * m_flAnim[1] ) ) * m_flAnim[0] ) ) );

	Render::Rectangle( area.x, area.y, ITEMHEIGHT, ITEMHEIGHT, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[1] ) ) ) );

	Render::Text( Fonts::Menu, area + Vector2D( ITEMHEIGHT * 2, -2 ), Color( 1.f, 1.f, 1.f, MenuAlpha( .25f + ( .5f * m_flAnim[1] ) + ( .25f * m_flAnim[0] * m_flAnim[1] ) ) ), FONT_LEFT, m_strTitle.c_str( ) );

	return ITEMHEIGHT + PAD;
}

inline float map_number( float input, float input_min, float input_max, float output_min, float output_max ) {
	return ( input - input_min ) / ( input_max - input_min ) * ( output_max - output_min ) + output_min;
}

int MenuFramework::CSlider::Render( Vector2D& area ) {
	auto width{ m_vecContainerSize.x - ( PAD * 4 ) };
	if ( m_iActive == m_iVarID ) {
		if ( Inputsys::down( VK_LBUTTON ) ) {
			const int offset = std::clamp( Vector2D( Inputsys::MousePos - area ).x, 0.f, width );
			Config::Get<int>( m_iVarID ) = (int)map_number( offset, 0, width, m_flMin, m_flMax + 1.f );
		}

		if ( Inputsys::released( VK_LBUTTON ) )
			m_iActive = 0;
	}

	if ( !m_iActive && Inputsys::hovered( area + Vector2D{ 0, 15 }, { width, ITEMHEIGHT } ) ) {
		if ( Inputsys::pressed( VK_LBUTTON ) )
			m_iActive = m_iVarID;
	}

	if ( m_flVisual != Config::Get<int>( m_iVarID ) )
		m_flVisual = Math::Interpolate( m_flVisual, Config::Get<int>( m_iVarID ), 10.f * Interfaces::Globals->flFrameTime );

	AnimateLerp( !m_iActive || m_iActive == m_iVarID, 10.f, m_flAnim[0] );
	AnimateLerp( m_iActive == m_iVarID, 10.f, m_flAnim[1] );

	auto percent{ ( m_flVisual - m_flMin ) / m_flMax };
	auto add{ std::ceil( -5.f * m_flAnim[1] ) };
	auto _r{ std::min( std::abs( m_flVisual ), 3.f ) };

	Render::Text( Fonts::Menu, area + Vector2D{ 0.f, add }, Color( 1.f, 1.f, 1.f, MenuAlpha( .25f + ( .75f * m_flAnim[0] ) ) ), FONT_LEFT, m_strTitle.c_str( ) );
	Render::Text( Fonts::Menu, area + Vector2D{ width, add }, Color( 1.f, 1.f, 1.f, MenuAlpha( .25f + ( .75f * m_flAnim[0] ) ) ), FONT_RIGHT, std::to_string( Config::Get<int>( m_iVarID ) ).c_str( ) );

	Render::FilledRoundedBox( area + Vector2D{ -1, 16 }, { width, ITEMHEIGHT }, 3, 3, Color( 0.f, 0.f, 0.f, MenuAlpha( .25f * m_flAnim[1] ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 0, 15 + add }, { width, ITEMHEIGHT }, 3, 3, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 1, 16 + add }, { width - 2, ITEMHEIGHT - 2 }, 3, 3, Accent::Control.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 2, 17 + add }, { ( width - 4 ) * percent, ITEMHEIGHT - 4 }, _r, _r, Accent::Accent.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );

	return ITEMHEIGHT + ( PAD + 15.f );
}

int MenuFramework::CCombo::Render( Vector2D& area ) {
	const auto height{ ITEMHEIGHT * 2.f };

	// detail: poo floats.
	AnimateLerp( !m_iActive || m_iActive == m_iVarID, 10.f, m_flAnim[0] );
	AnimateLerp( m_iActive == m_iVarID, 10.f, m_flAnim[1] );

	auto add{ std::ceil( -5.f * m_flAnim[1] ) };
	auto width{ m_vecContainerSize.x - ( PAD * 4 ) };

	if ( ( !m_iActive || m_iActive == m_iVarID ) && Inputsys::hovered( area + Vector2D{ 0, 15 + add }, { width, height } ) ) {
		if ( Inputsys::pressed( VK_LBUTTON ) )
			m_iActive = m_iActive == m_iVarID ? 0 : m_iVarID;
	}

	Render::Text( Fonts::Menu, area + Vector2D{ 0.f, add }, Color( 1.f, 1.f, 1.f, MenuAlpha( .25f + ( .75f * m_flAnim[0] ) ) ), FONT_LEFT, m_strTitle.c_str( ) );

	Render::FilledRoundedBox( area + Vector2D{ -1, 16 }, { width, height }, 3, 3, Color( 0.f, 0.f, 0.f, MenuAlpha( .25f * m_flAnim[1] ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 0, 15 + add }, { width, height }, 3, 3, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 1, 16 + add }, { width - 2, height - 2 }, 3, 3, Accent::Control.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );

	Render::Text( Fonts::Menu, area + Vector2D{ PAD, 17 + add }, Color( 1.f, 1.f, 1.f, MenuAlpha( .25f + ( .75f * m_flAnim[0] ) ) ), FONT_LEFT, m_vecItems[Config::Get<int>( m_iVarID )].m_strName );

	return height + ( PAD + 15.f );
}

void MenuFramework::CCombo::Overlay( Vector2D& area ) {
	auto height{ ITEMHEIGHT * 2.f };

	auto add{ std::ceil( -5.f * m_flAnim[1] ) };
	auto width{ m_vecContainerSize.x - ( PAD * 4 ) };

	area.y += height + PAD;
	height = ( ITEMHEIGHT * 2.f ) * m_vecItems.size( );

	Render::FilledRoundedBox( area + Vector2D{ -1, 16 }, { width, height }, 3, 3, Color( 0.f, 0.f, 0.f, MenuAlpha( .25f * m_flAnim[1] ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 0, 15 + add }, { width, height }, 3, 3, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( 255.f * m_flAnim[1] ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 1, 16 + add }, { width - 2, height - 2 }, 3, 3, Accent::Control.Set<COLOR_A>( MenuAlpha( 255.f * m_flAnim[1] ) ) );

	auto i{ 0 };
	for ( auto& item : m_vecItems ) {
		auto y{ 15 + add + ( i * ( ITEMHEIGHT * 2.f ) ) };

		if ( i % 2 == 1 )
			Render::FilledRoundedBox( area + Vector2D{ 1, y }, { width - 2, ITEMHEIGHT * 2.f }, 3, 3, Color( 0.f, 0.f, 0.f, MenuAlpha( .1f * m_flAnim[1] ) ) );

		AnimateLerp( Config::Get<int>( m_iVarID ) == i, 10.f, item.m_flAnim );

		auto xAdd{ 5.f * item.m_flAnim * m_flAnim[1] };
		Render::Text( Fonts::Menu, area + Vector2D{ PAD, y + ( PAD / 4.f ) }, Color( 0.f, 0.f, 0.f, MenuAlpha( .25f * m_flAnim[1] * item.m_flAnim ) ), FONT_LEFT, item.m_strName );
		Render::Text( Fonts::Menu, area + Vector2D{ PAD + xAdd, y + ( PAD / 4.f ) }, Color( 1.f, 1.f, 1.f ).Lerp( ( i % 2 == 0 ? Accent::Accent : Accent::Accent2 ), item.m_flAnim ).Set<COLOR_A>( MenuAlpha( 255.f * m_flAnim[1] ) ), FONT_LEFT, item.m_strName );

		if ( Inputsys::pressed( 0x01 ) && Inputsys::hovered( area + Vector2D{ 0, y }, { width, ITEMHEIGHT * 2.f } ) ) {
			Config::Get<int>( m_iVarID ) = i;
			m_iActive = 0;
		}

		i++;
	}
}

int MenuFramework::CMulti::Render( Vector2D& area ) {
	const auto height{ ITEMHEIGHT * 2.f };

	// detail: poo floats.
	AnimateLerp( !m_iActive || m_iActive == m_iVarID, 10.f, m_flAnim[0] );
	AnimateLerp( m_iActive == m_iVarID, 10.f, m_flAnim[1] );

	auto add{ std::ceil( -5.f * m_flAnim[1] ) };
	auto width{ m_vecContainerSize.x - ( PAD * 4 ) };

	if ( ( !m_iActive || m_iActive == m_iVarID ) && Inputsys::hovered( area + Vector2D{ 0, 15 + add }, { width, height } ) ) {
		if ( Inputsys::pressed( VK_LBUTTON ) )
			m_iActive = m_iActive == m_iVarID ? 0 : m_iVarID;
	}

	Render::Text( Fonts::Menu, area + Vector2D{ 0.f, add }, Color( 1.f, 1.f, 1.f, MenuAlpha( .25f + ( .75f * m_flAnim[0] ) ) ), FONT_LEFT, m_strTitle.c_str( ) );

	Render::FilledRoundedBox( area + Vector2D{ -1, 16 }, { width, height }, 3, 3, Color( 0.f, 0.f, 0.f, MenuAlpha( .25f * m_flAnim[1] ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 0, 15 + add }, { width, height }, 3, 3, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 1, 16 + add }, { width - 2, height - 2 }, 3, 3, Accent::Control.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );

	auto selected{ 0 };
	for ( auto i : m_vecItems )
		if ( Config::Get<bool>( i.m_iVarID ) )
			selected++;

	Render::Text( Fonts::Menu, area + Vector2D{ PAD, 17 + add }, Color( 1.f, 1.f, 1.f, MenuAlpha( .25f + ( .75f * m_flAnim[0] ) ) ), FONT_LEFT, ( std::to_string( selected ) + " selected.." ).c_str( ) );

	return height + ( PAD + 15.f );
}

void MenuFramework::CMulti::Overlay( Vector2D& area ) {
	auto height{ ITEMHEIGHT * 2.f };

	auto add{ std::ceil( -5.f * m_flAnim[1] ) };
	auto width{ m_vecContainerSize.x - ( PAD * 4 ) };

	area.y += height + PAD;
	height = ( ITEMHEIGHT * 2.f ) * m_vecItems.size( );

	Render::FilledRoundedBox( area + Vector2D{ -1, 16 }, { width, height }, 3, 3, Color( 0.f, 0.f, 0.f, MenuAlpha( .25f * m_flAnim[1] ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 0, 15 + add }, { width, height }, 3, 3, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( 255.f * m_flAnim[1] ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 1, 16 + add }, { width - 2, height - 2 }, 3, 3, Accent::Control.Set<COLOR_A>( MenuAlpha( 255.f * m_flAnim[1] ) ) );

	auto i{ 0 };
	for ( auto& item : m_vecItems ) {
		auto y{ 15 + add + ( i * ( ITEMHEIGHT * 2.f ) ) };

		if ( i % 2 == 1 )
			Render::FilledRoundedBox( area + Vector2D{ 1, y }, { width - 2, ITEMHEIGHT * 2.f }, 3, 3, Color( 0.f, 0.f, 0.f, MenuAlpha( .1f * m_flAnim[1] ) ) );

		AnimateLerp( Config::Get<bool>( item.m_iVarID ), 10.f, item.m_flAnim );

		auto xAdd{ 5.f * item.m_flAnim * m_flAnim[1] };
		Render::Text( Fonts::Menu, area + Vector2D{ PAD, y + ( PAD / 4.f ) }, Color( 0.f, 0.f, 0.f, MenuAlpha( .25f * m_flAnim[1] * item.m_flAnim ) ), FONT_LEFT, item.m_strName );
		Render::Text( Fonts::Menu, area + Vector2D{ PAD + xAdd, y + ( PAD / 4.f ) }, Color( 1.f, 1.f, 1.f ).Lerp( ( i % 2 == 0 ? Accent::Accent : Accent::Accent2 ), item.m_flAnim ).Set<COLOR_A>( MenuAlpha( 255.f * m_flAnim[1] ) ), FONT_LEFT, item.m_strName );

		if ( Inputsys::pressed( 0x01 ) && Inputsys::hovered( area + Vector2D{ 0, y }, { width, ITEMHEIGHT * 2.f } ) )
			Config::Get<bool>( item.m_iVarID ) = !Config::Get<bool>( item.m_iVarID );

		i++;
	}
}

int MenuFramework::CButton::Render( Vector2D& area ) {
	auto width{ m_vecContainerSize.x - ( PAD * 4 ) };
	const auto height{ ITEMHEIGHT * 2.f };

	AnimateLerp( !m_iActive, 10.f, m_flAnim[0] );

	if ( !m_iActive && Inputsys::hovered( area, { width, height } ) && Inputsys::pressed( 0x01 ) ) {
		m_fnCallback( ); m_flAnim[0] = 0.f;
	}

	Render::FilledRoundedBox( area, { width, height }, 3, 3, Accent::OutlineLight.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );
	Render::FilledRoundedBox( area + Vector2D{ 1, 1 }, { width - 2, height - 2 }, 3, 3, Accent::Control.Set<COLOR_A>( MenuAlpha( 55.f + ( 200.f * m_flAnim[0] ) ) ) );

	Render::Text( Fonts::Menu, area + Vector2D{ width / 2.f, 2 }, Color( 1.f, 1.f, 1.f ).Lerp( Accent::Accent2, 1.f - m_flAnim[0] ).Set<COLOR_A>( MenuAlpha( ) ), FONT_CENTER, m_strTitle.c_str( ) );

	return height + PAD;
}

inline void Aimbot( MenuFramework::CWindow& window ) {
	auto& inst{ window.GetTabs( )[MenuFramework::m_iTab] };
	const auto SELECT_Y{ 75.f };

	static auto wpnGroup{ 0 };
	static auto selection{ MenuFramework::CGroupbox( ) };
	if ( selection.Open( "Selection", inst.m_vecRenderArea, { inst.m_vecRenderSize.x - PAD, SELECT_Y } ) ) {
		selection.Add( std::make_shared< MenuFramework::CCombo >( _( "Weapon" ), std::vector{ _( "Pistol" ), _( "Heavy pistol" ), _( "SMG" ), _( "Rifle" ), _( "Shotgun" ), _( "Awp" ), ( "Scout" ), ( "Auto" ), _( "Machine gun" ) }, Vars.RagebotWeaponGroup ) );
	}

	inst.m_vecRenderSize.y -= SELECT_Y + ( PAD * 2 );
	inst.m_vecRenderArea.y += SELECT_Y + PAD;

	const Vector2D stdSize{ inst.m_vecRenderSize.x / 2 - PAD, inst.m_vecRenderSize.y };

	static auto general{ MenuFramework::CGroupbox( ) };
	if ( general.Open( "General", inst.m_vecRenderArea, stdSize ) ) {
		general.Add( std::make_shared< MenuFramework::CCheckbox >( _( "Enable" ), Vars.RagebotEnable ) );
		general.Add( std::make_shared< MenuFramework::CSlider >( _( "Field-of-view" ), 1.f, 180.f, Vars.RagebotFOVAuto ) );
		general.Add( std::make_shared< MenuFramework::CCombo >( _( "Combobox" ), std::vector{ _( "Default" ), _( "Up" ), _( "Down" ), _( "Zero" ), _( "Random" ) }, Vars.AntiaimPitch ) );
		general.Add( std::make_shared< MenuFramework::CMulti >( _( "Multiselect" ), std::vector<MenuFramework::Item_t>{
			{ "Head", Vars.RagebotHBHeadPistol },
			{ "Upper chest", Vars.RagebotHBUpperChestPistol },
			{ "Chest", Vars.RagebotHBChestPistol },
			{ "Lower chest", Vars.RagebotHBLowerChestPistol },
			{ "Stomach", Vars.RagebotHBStomachPistol },
			{ "Pelvis", Vars.RagebotHBPelvisPistol },
			{ "Arms", Vars.RagebotHBArmsPistol },
			{ "Legs", Vars.RagebotHBLegsPistol },
			{ "Feet", Vars.RagebotHBFeetPistol }
		} ) );
		general.Add( std::make_shared< MenuFramework::CButton >( _( "Button" ), [&]( ) { /* do nothing. */ } ) );
	}
	general.Exit( [&] { return true; } );

	static auto other{ MenuFramework::CGroupbox( ) };
	if ( other.Open( "Other", inst.m_vecRenderArea + Vector2D{ stdSize.x + PAD, 0.f }, { stdSize.x, ( stdSize.y - PAD ) * .75F } ) ) {
		// ...
	}
	other.Exit( [&] { return true; } );

	static auto exploits{ MenuFramework::CGroupbox( ) };
	if ( exploits.Open( "Exploits", inst.m_vecRenderArea + Vector2D{ stdSize.x + PAD, ( stdSize.y ) * .75F }, { stdSize.x, ( stdSize.y - PAD ) * .25F } ) ) {
		exploits.Add( std::make_shared< MenuFramework::CCheckbox >( _( "Doubletap" ), Vars.ExploitsDoubletap ) );
		exploits.Add( std::make_shared< MenuFramework::CMulti >( _( "Doubletap options" ), std::vector<MenuFramework::Item_t>{
			{ "Lag peek", Vars.ExploitsDoubletapDefensive },
			{ "Delay teleport", Vars.ExploitsDoubletapExtended },
			{ "Continuous in-air", Vars.ExploitsDefensiveInAir },
		} ) );

		exploits.Add( std::make_shared< MenuFramework::CCheckbox >( _( "Hideshots" ), Vars.ExploitsHideshots ) );
	}
	exploits.Exit( [&] { return true; } );

	selection.Exit( [&] { return true; } );
}

void MenuFramework::Main( ) {
	static auto window{ CWindow( { 
		{ _( "A" ) },
		{ _( "B" ) },
		{ _( "F" ) },
		{ _( "C" ) },
		{ _( "D" ) },
		{ _( "E" ) }
	} ) };

	if ( window.Create( _( "Havoc" ) ) ) { 
		
		switch ( m_iTab ) {
			case 0: {
					Aimbot( window );
				} break;

			default:
				break;
		}

	}
}