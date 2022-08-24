#pragma once
#include "../../context.h"
#include "../../utils/math.h"
#include "../animations/animation.h"
#include "logger.h"

// pasteed from nemeseis
struct shot_t {
    __forceinline constexpr shot_t( ) = default;

    __forceinline shot_t(
        const Vector& src,
        std::shared_ptr<LagRecord_t> lag_record,
        int cmd_number,
        CBasePlayer* target,
        int shot_hitgroup,
        Vector shotpos
    ) : m_src{ src }, m_lag_record{ lag_record }, m_shot_hitgroup{ shot_hitgroup }, m_cmd_number{ cmd_number }, m_target{ target }, m_shot_pos{ shotpos } {}

    Vector                            m_src{ };

    std::shared_ptr<LagRecord_t> m_lag_record;

    CBasePlayer* m_target;

    bool                            m_processed{ };
    int  m_dmg{ },
        m_shot_hitgroup{ },
        m_cmd_number{ -1 }, m_process_tick{ };

    Vector    m_shot_pos{ };
    struct {
        Vector    m_impact_pos{ };
        int        m_fire_tick{ }, m_hurt_tick{ }, m_hitgroup{ }, m_damage{ };
    }                                m_server_info{ };
};

class CShots {
public:
	std::deque< shot_t > m_elements{ };

    void OnNetUpdate( );

    __forceinline void add(
        const Vector& src,
        std::shared_ptr<LagRecord_t> lag_record,
        int cmd_number,
        CBasePlayer* target,
        int hitgroup,
        Vector shotpos
    ) {
        m_elements.emplace_back( src, lag_record, cmd_number, target, hitgroup, shotpos );

        if ( m_elements.size( ) < 128 )
            return;

        m_elements.pop_front( );
    }

    __forceinline shot_t* LastUnprocessed( ) {
        if ( m_elements.empty( ) )
            return nullptr;

        const auto shot = std::find_if(
            m_elements.rbegin( ), m_elements.rend( ),
            [ ]( const shot_t& shot ) {
                return !shot.m_processed
                    && shot.m_server_info.m_fire_tick
                    && shot.m_server_info.m_fire_tick == Interfaces::ClientState->iServerTick;
            }
        );

        return shot == m_elements.rend( ) ? nullptr : &*shot;
    }
};

namespace Features { inline CShots Shots; };