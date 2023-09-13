#pragma once
#include "../../../features/animations/animation.h"
#include "../../prop_manager.h"

namespace Wrappers::Entity {
	class CPlayer {
	public:
		// so, we need this in order to be able to use the entity inside this class..
		// the entity will be constructed upon calling GetClientEntity from entitylist
		CPlayer( CBasePlayer* player ) {
			m_pPlayer = player;
		}

		void SetInt( int offset, int input ) {
			if ( !m_pPlayer )
				return;

			*reinterpret_cast< int* >( uintptr_t( m_pPlayer ) + offset ) = input;
		}

		void SetBool( int offset, bool input ) {
			if ( !m_pPlayer )
				return;

			*reinterpret_cast< bool* >( uintptr_t( m_pPlayer ) + offset ) = input;
		}

		void SetFloat( int offset, float input ) {
			if ( !m_pPlayer )
				return;

			*reinterpret_cast< float* >( uintptr_t( m_pPlayer ) + offset ) = input;
		}

		void SetVector( int offset, Vector input ) {
			if ( !m_pPlayer )
				return;

			*reinterpret_cast< Vector* >( uintptr_t( m_pPlayer ) + offset ) = input;
		}

		int GetInt( int offset ) {
			if ( !m_pPlayer )
				return 0;

			return *reinterpret_cast< int* >( uintptr_t( m_pPlayer ) + offset );
		}

		bool GetBool( int offset ) {
			if ( !m_pPlayer )
				return false;

			return *reinterpret_cast< bool* >( uintptr_t( m_pPlayer ) + offset );
		}

		float GetFloat( int offset ) {
			if ( !m_pPlayer )
				return 0.f;

			return *reinterpret_cast< float* >( uintptr_t( m_pPlayer ) + offset );
		}

		Vector GetVector( int offset ) {
			if ( !m_pPlayer )
				return { 0,0,0 };

			return *reinterpret_cast< Vector* >( uintptr_t( m_pPlayer ) + offset );
		}

		std::string GetString( int offset ) {
			if ( !m_pPlayer )
				return { 0,0,0 };

			return static_cast< std::string >( *reinterpret_cast< const char** >( uintptr_t( m_pPlayer ) + offset ) );
		}

		bool Valid( ) {
			return m_pPlayer != nullptr;
		}

		bool IsAlive( ) {
			if ( !m_pPlayer )
				return false;

			return !m_pPlayer->IsDead( );
		}

		bool IsPlayer( ) {
			if ( !m_pPlayer )
				return false;

			return !m_pPlayer->IsPlayer( );
		}

		bool IsDormant( ) {
			if ( !m_pPlayer )
				return true;

			return m_pPlayer->Dormant( );
		}

		int GetLayerSequenceActivity( int layer ) {
			if ( !m_pPlayer )
				return 0;

			return m_pPlayer->GetSequenceActivity( m_pPlayer->m_AnimationLayers( )[ layer ].nSequence );
		}

		void AnimatePlayer( ) {
			if ( !m_pPlayer )
				return;

			const auto backupCurtime{ Interfaces::Globals->flCurTime };
			const auto backupFrametime{ Interfaces::Globals->flFrameTime };
			const auto backupHLTV{ Interfaces::ClientState->bIsHLTV };

			Interfaces::ClientState->bIsHLTV = true;
			Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;

			m_pPlayer->m_pAnimState( )->iLastUpdateFrame = Interfaces::Globals->iFrameCount - 1;

			m_pPlayer->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = true;
			m_pPlayer->UpdateClientsideAnimations( );
			m_pPlayer->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = false;

			Interfaces::ClientState->bIsHLTV = backupHLTV;
			Interfaces::Globals->flCurTime = backupCurtime;
			Interfaces::Globals->flFrameTime = backupFrametime;
		}

		void SetupBones( ) {
			if ( !m_pPlayer )
				return;

			Features::AnimSys.SetupBonesRebuilt( m_pPlayer, nullptr,
				BONE_USED_BY_SERVER, m_pPlayer->m_flSimulationTime( ), false );
		}

		Vector GetHitboxPosition( int index ) {
			if ( !m_pPlayer )
				return { 0,0,0 };

			const auto hitboxSet{ m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( m_pPlayer->m_nHitboxSet( ) ) };
			if ( !hitboxSet )
				return { 0,0,0 };

			const auto hitbox{ hitboxSet->GetHitbox( index ) };
			if ( !hitbox )
				return { 0,0,0 };

			return Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, m_pPlayer->m_CachedBoneData( ).Base( )[ hitbox->iBone ] );
		}

		operator CBasePlayer* ( ) {
			return m_pPlayer;
		}

	//private:
		CBasePlayer* m_pPlayer;
	};

	int GetOffset( const char* table, const char* prop ) {
		return PropManager::Get( ).GetOffset( table, prop );
	}
}