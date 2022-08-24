#include "logger.h"

void CLogger::PrintToScreen( ) {
	int iterations = 1;
	m_cOnscreenText.erase(
		std::remove_if(
			m_cOnscreenText.begin( ), m_cOnscreenText.end( ),
			[ & ]( const OnsCreen_t& onscreen ) -> bool {
				return onscreen.m_flTimeOnScreen <= 0.f;
			}
		),
		m_cOnscreenText.end( )
				);

	// I'M HIGH ON CODEINE RN
	for ( auto& screenText : m_cOnscreenText ) {
		screenText.m_flTimeOnScreen -= Interfaces::Globals->flFrameTime;

		if ( screenText.m_flTimeOnScreen < 0.5f )
			screenText.m_fAlpha = std::max( screenText.m_fAlpha - ( Interfaces::Globals->flFrameTime * 2.f ), 0.f );
		else if ( screenText.m_flTimeOnScreen > 4.5f )
			screenText.m_fAlpha = std::min( screenText.m_fAlpha + ( Interfaces::Globals->flFrameTime * 2.f ), 1.f );

		const auto textSize = Render::GetTextSize( screenText.m_strText.c_str( ), Fonts::Logs );

		Render::FilledRectangle( 5, 22 * iterations - 3, textSize.x + 10, 18, Menu::BackgroundCol.Set<COLOR_A>( Menu::BackgroundCol.Get<COLOR_A>( ) / 2  * screenText.m_fAlpha ) );
		Render::Rectangle( 5, 22 * iterations - 4, textSize.x + 11, 20, Menu::AccentCol.Set<COLOR_A>( Menu::AccentCol.Get<COLOR_A>( ) / 2 * screenText.m_fAlpha ) );
		Render::Text( Fonts::Logs, Vector2D( 10, 22 * iterations ), Color( 255, 255, 255, static_cast<int>( 255 * screenText.m_fAlpha ) ), 0, screenText.m_strText.c_str( ) );

		iterations++;
	}
}