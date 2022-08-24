#include "visuals.h"

void CVisuals::ManageHitmarkers( ) {
    if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) ) {
        hits.clear( );
        return;
    }

    hits.erase(
        std::remove_if(
            hits.begin( ), hits.end( ),
            [ & ]( const std::shared_ptr<hitmarker_t>& hit ) -> bool {
                return hit->time + 2.f < Interfaces::Globals->flCurTime || hit->alpha <= 0.f;
            }
        ),
        hits.end( )
                );

    for ( const auto& hit : hits ) {
        Math::WorldToScreen( hit->position, hit->screen );

        if ( hit->screen.IsZero( ) )
            continue;

        const auto step = Interfaces::Globals->flFrameTime * 2;

        if ( hit->time + 1.5f <= Interfaces::Globals->flCurTime )
            hit->alpha -= step;

        hit->step += step;

        WorldHitMarker( hit );
    }
}

void CVisuals::WorldHitMarker( const std::shared_ptr<hitmarker_t>& hit ) {
    if ( hit->alpha > 0 ) {
        if ( Config::Get<bool>( Vars.MiscDamageMarker ) ) {
            const auto col = Config::Get<Color>( Vars.MiscDamageMarkerCol ).Set<COLOR_A>( Config::Get<Color>( Vars.MiscDamageMarkerCol ).Get<COLOR_A>( ) * hit->alpha );
            Render::Text( Fonts::DamageMarker, Vector2D( hit->screen.x, hit->screen.y - 7 - hit->step * 5 ), col, FONT_CENTER, std::to_string( hit->damage ).c_str( ) );
        }

        if ( Config::Get<bool>(Vars.MiscHitmarker ) ) {
            const auto col = Config::Get<Color>( Vars.MiscHitmarkerCol ).Set<COLOR_A>( Config::Get<Color>( Vars.MiscHitmarkerCol ).Get<COLOR_A>( ) * hit->alpha );
            Render::Line( Vector2D( hit->screen.x - 3, hit->screen.y ), Vector2D( hit->screen.x + 4, hit->screen.y ), col );
            Render::Line( Vector2D( hit->screen.x, hit->screen.y - 3 ), Vector2D( hit->screen.x, hit->screen.y + 4 ), col );
        }
    }
}