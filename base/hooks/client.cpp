#include "../core/hooks.h"
#include "../utils/threading/threading.h"
#include "../core/config.h"
#include "../context.h"
#include "../features/visuals/visuals.h"
#include "../features/misc/engine_prediction.h"
#include "../features/misc/shot_info.h"
#include "../features/animations/animation.h"
#include "../features/rage/exploits.h"


void draw_server_hitboxes( int index ) {
	//if ( g_Vars.globals.m_iServerType != 8 )
	//	return;

	auto get_player_by_index = [ ]( int index ) -> CBasePlayer* {
		typedef CBasePlayer* ( __fastcall* player_by_index )( int );
		static auto player_index = reinterpret_cast< player_by_index >( MEM::FindPattern( _( "server.dll" ), _( "85 C9 ? ? A1 ? ? ? ? 3B 48 18 7F 28" ) ) );

		if ( !player_index )
			return false;

		return player_index( index );
	};

	static auto fn = MEM::FindPattern( _( "server.dll" ), _( "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" ) );
	auto duration = -1.f;
	PVOID entity = nullptr;

	entity = get_player_by_index( index );

	if ( !entity )
		return;

	__asm {
		pushad
		movss xmm1, duration
		push 0 // 0 - colored, 1 - blue
		mov ecx, entity
		call fn
		popad
	}
}

const char* skynames[ 16 ] = {
	"Default",
	"cs_baggage_skybox_",
	"cs_tibet",
	"embassy",
	"italy",
	"jungle",
	"nukeblank",
	"office",
	"sky_csgo_cloudy01",
	"sky_csgo_night02",
	"sky_csgo_night02b",
	"sky_dust",
	"sky_venice",
	"vertigo",
	"vietnam",
	"sky_descent"
};

void FASTCALL Hooks::hkFrameStageNotify( IBaseClientDll* thisptr, int edx, EClientFrameStage stage ) {
	static auto oFrameStageNotify = DTR::FrameStageNotify.GetOriginal<decltype( &hkFrameStageNotify )>( );

	ctx.m_iLastFSNStage = stage;
	ctx.GetLocal( );

	static int backupsmokeCount{ };

	const auto backupInterpAmt{ Interfaces::Globals->flInterpolationAmount };

	if ( stage == FRAME_RENDER_START ) {
		if ( ctx.m_pLocal ) {
			if ( Features::Exploits.m_iRechargeCmd == Interfaces::ClientState->iLastOutgoingCommand )
				Interfaces::Globals->flInterpolationAmount = 0;

			/*for ( int i{ 1 }; i <= 64; i++ ) {
				const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
				if ( !player || player == ctx.m_pLocal )
					continue;

				//draw_server_hitboxes( i );
			}*/

			if ( Config::Get<bool>( Vars.RemovalFlash ) )
				ctx.m_pLocal->m_flFlashDuration( ) = 0.f;

			if ( Config::Get<bool>( Vars.MiscForceCrosshair ) && !ctx.m_pLocal->m_bIsScoped( ) ) {
				if ( Offsets::Cvars.weapon_debug_spread_show->GetInt( ) != 3 )
					Offsets::Cvars.weapon_debug_spread_show->SetValue( 3 );
			}
			else {
				if ( Offsets::Cvars.weapon_debug_spread_show->GetInt( ) )
					Offsets::Cvars.weapon_debug_spread_show->SetValue( 0 );
			}

			if ( Offsets::Cvars.cl_csm_shadows->GetBool( ) )
				Offsets::Cvars.cl_csm_shadows->SetValue( 0 );
		}

		{

			static int oldSky = 0;
			static auto fnLoadNamedSkys = ( void( __fastcall* )( const char* ) )Offsets::Sigs.LoadNamedSkys;
			static auto defaultSkyname = Interfaces::ConVar->FindVar( _( "sv_skyname" ) );
			if ( oldSky != Config::Get<int>( Vars.VisWorldSkybox ) ) {
				const char* name = Config::Get<int>( Vars.VisWorldSkybox ) ? skynames[ Config::Get<int>( Vars.VisWorldSkybox ) ] : defaultSkyname->GetString( );
				fnLoadNamedSkys( name );
				oldSky = Config::Get<int>( Vars.VisWorldSkybox );
			}
			else if ( !ctx.m_pLocal )
				oldSky = 0;

			if ( Offsets::Cvars.r_modelAmbientMin->GetFloat( ) != ( Config::Get<bool>( Vars.VisWorldBloom ) ? Config::Get<int>( Vars.VisWorldBloomAmbience ) / 10.0f : 0.f ) )
				Offsets::Cvars.r_modelAmbientMin->SetValue( Config::Get<bool>( Vars.VisWorldBloom ) ? Config::Get<int>( Vars.VisWorldBloomAmbience ) / 10.0f : 0.f );

			static bool reset = false;
			if ( Config::Get<bool>( Vars.WorldAmbientLighting ) ) {
				reset = false;
				if ( Offsets::Cvars.mat_ambient_light_r->GetFloat( ) != Config::Get<Color>( Vars.WorldAmbientLightingCol ).Get<COLOR_R>( ) / 255.f )
					Offsets::Cvars.mat_ambient_light_r->SetValue( Config::Get<Color>( Vars.WorldAmbientLightingCol ).Get<COLOR_R>( ) / 255.f );

				if ( Offsets::Cvars.mat_ambient_light_g->GetFloat( ) != Config::Get<Color>( Vars.WorldAmbientLightingCol ).Get<COLOR_G>( ) / 255.f )
					Offsets::Cvars.mat_ambient_light_g->SetValue( Config::Get<Color>( Vars.WorldAmbientLightingCol ).Get<COLOR_G>( ) / 255.f );

				if ( Offsets::Cvars.mat_ambient_light_b->GetFloat( ) != Config::Get<Color>( Vars.WorldAmbientLightingCol ).Get<COLOR_B>( ) / 255.f )
					Offsets::Cvars.mat_ambient_light_b->SetValue( Config::Get<Color>( Vars.WorldAmbientLightingCol ).Get<COLOR_B>( ) / 255.f );
			}
			else {
				if ( !reset ) {
					Offsets::Cvars.mat_ambient_light_r->SetValue( 0.f );
					Offsets::Cvars.mat_ambient_light_g->SetValue( 0.f );
					Offsets::Cvars.mat_ambient_light_b->SetValue( 0.f );
					reset = true;
				}
			}

			auto& smokeCount = **reinterpret_cast< int** >( Offsets::Sigs.SmokeCount + 0x1 );
			backupsmokeCount = smokeCount;

			if ( Config::Get<bool>( Vars.RemovalSmoke ) )
				smokeCount = 0;


			if ( Offsets::Cvars.r_drawspecificstaticprop->GetBool( ) )
				Offsets::Cvars.r_drawspecificstaticprop->SetValue( FALSE );

			if ( Offsets::Cvars.cl_foot_contact_shadows->GetBool( ) )
				Offsets::Cvars.cl_foot_contact_shadows->SetValue( FALSE );

			//if ( Offsets::Cvars.sv_showimpacts->GetInt( ) != 2 )
			//	Offsets::Cvars.sv_showimpacts->SetValue( 2 );

			static bool* post_process{ *reinterpret_cast< bool** >( Offsets::Sigs.PostProcess + 0x2 ) };
			*post_process = ( ctx.m_pLocal && !Config::Get<bool>( Vars.VisWorldBloom ) && Config::Get<bool>( Vars.RemovalPostProcess ) );
		}
	}
	else if ( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END ) {
		for ( int i{ 1 }; i <= 64; i++ ) {
			const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
			if ( !player || player->IsDead( ) || !player->IsPlayer( ) || player == ctx.m_pLocal )
				continue;

			const auto varMapping{ *reinterpret_cast< std::uintptr_t* >( ( reinterpret_cast< std::uintptr_t >( player ) + 36u ) ) };
			if ( !varMapping )
				continue;

			*( WORD* )( varMapping + 14u ) = 0u;
			*( DWORD* )( *( DWORD* )( varMapping + 20u ) + 36u ) = 0;

			*( WORD* )( varMapping + 26u ) = 0u;
			*( DWORD* )( *( DWORD* )( varMapping + 32u ) + 36u ) = 0;

			*( WORD* )( varMapping + 38u ) = 0u;
			*( DWORD* )( *( DWORD* )( varMapping + 44u ) + 36u ) = 0;

			*( WORD* )( varMapping + 50u ) = 0u;
			*( DWORD* )( *( DWORD* )( varMapping + 56u ) + 36u ) = 0;

			*( DWORD* )( *( DWORD* )( varMapping + 8u ) + 36u ) = 0;
			*( DWORD* )( *( DWORD* )( varMapping + 68u ) + 36u ) = Interfaces::Globals->flIntervalPerTick * 2;
		}

		if ( Config::Get<bool>( Vars.RemovalSmoke ) ) {
			static const std::array< IMaterial*, 4u > smokeMaterials{
				Interfaces::MaterialSystem->FindMaterial( _( "particle/vistasmokev1/vistasmokev1_fire" ), nullptr ),
				Interfaces::MaterialSystem->FindMaterial( _( "particle/vistasmokev1/vistasmokev1_smokegrenade" ), nullptr ),
				Interfaces::MaterialSystem->FindMaterial( _( "particle/vistasmokev1/vistasmokev1_emods" ), nullptr ),
				Interfaces::MaterialSystem->FindMaterial( _( "particle/vistasmokev1/vistasmokev1_emods_impactdust" ), nullptr )
			};

			for ( auto& material : smokeMaterials )
				if ( material )
					material->SetMaterialVarFlag( MATERIAL_VAR_NO_DRAW, true );
		}
	}


	oFrameStageNotify( thisptr, edx, stage );

	switch ( stage ) {
	case FRAME_RENDER_START: {
		Interfaces::Globals->flInterpolationAmount = backupInterpAmt;
		Features::AnimSys.SetupLocalMatrix( );
		Features::AnimSys.UpdatePlayerMatrixes( );
	}break;
	case FRAME_RENDER_END: {
		**reinterpret_cast< int** >( Offsets::Sigs.SmokeCount + 0x1 ) = backupsmokeCount;

		//Features::Visuals.BulletTracers.Draw( );
	}break;
	case FRAME_NET_UPDATE_END: {
		if ( ctx.m_pLocal ) {
			const auto correctionTicks{ ctx.CalcCorrectionTicks( ) };
			if ( correctionTicks == -1 )
				Features::Exploits.m_iCorrectionAmount = 0;
			else {
				if ( ctx.m_pLocal->m_flSimulationTime( ) > ctx.m_pLocal->m_flOldSimulationTime( ) ) {
					const auto delta{ TIME_TO_TICKS( ctx.m_pLocal->m_flSimulationTime( ) ) - Interfaces::ClientState->iServerTick };
					if ( std::abs( delta ) <= correctionTicks )
						Features::Exploits.m_iCorrectionAmount = delta;
				}
			}

			if ( ctx.m_pLocal && !ctx.m_pLocal->IsDead( ) ) {
				for ( auto i{ Interfaces::ClientState->pEvents }; i != nullptr; i = Interfaces::ClientState->pEvents->next ) {
					if ( i->iClassID == 0 )
						continue;

					i->flFireDelay = 0.f;
				}
			}


			Interfaces::Engine->FireEvents( );
			Features::Shots.OnNetUpdate( );

			static bool did{ };
			if ( !did )
				did = Features::EnginePrediction.AddToDataMap( );

			//Features::AnimSys.UpdateCommands( );
		}

		Features::AnimSys.RunAnimationSystem( );

		{
			static DWORD* KillFeedTime = nullptr;
			if ( ctx.m_pLocal && !ctx.m_pLocal->IsDead( ) ) {
				if ( !KillFeedTime )
					KillFeedTime = MEM::FindHudElement<DWORD>( ( "CCSGO_HudDeathNotice" ) );

				if ( KillFeedTime ) {
					auto LocalDeathNotice = ( float* )( ( uintptr_t )KillFeedTime + 0x50 );

					if ( LocalDeathNotice )
						*LocalDeathNotice = Config::Get<bool>( Vars.MiscPreserveKillfeed ) ? FLT_MAX : 1.5f;

					if ( ctx.m_bClearKillfeed ) {
						using Fn = void( __thiscall* )( uintptr_t );
						static auto clearNotices = ( Fn )Offsets::Sigs.ClearNotices;

						clearNotices( ( uintptr_t )KillFeedTime - 0x14 );

						ctx.m_bClearKillfeed = false;
					}
				}
			}
			else
				KillFeedTime = 0;
		}
	}break;
	default: break;

	}

	if ( ctx.m_pLocal ) {
		if ( !Interfaces::GameRules )
			Interfaces::GameRules = ( ( **reinterpret_cast< CCSGameRules*** >( MEM::FindPattern( CLIENT_DLL, _( "A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 80 B8 ? ? ? ? ? 74 7A" ) ) + 0x1 ) ) );

		if ( ctx.m_pLocal->m_hViewModel( ) ) {
			if ( const auto viewModel{ static_cast< CBaseViewModel* >( Interfaces::ClientEntityList->GetClientEntityFromHandle( ctx.m_pLocal->m_hViewModel( ) ) ) }; viewModel ) {
				static float cycle{ }, animTime{ };

				if ( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START ) {
					viewModel->m_flCycle( ) = cycle;
					viewModel->m_flAnimTime( ) = animTime;
				}

				cycle = viewModel->m_flCycle( );
				animTime = viewModel->m_flAnimTime( );
			}
		}
	}
	else
		Interfaces::GameRules = nullptr;
}

bool InPeek( ) {
	matrix3x4_t backup_matrix[ 256 ];
	memcpy( backup_matrix, ctx.m_pLocal->m_CachedBoneData( ).Base( ), ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	const auto delta = ctx.m_pLocal->m_vecVelocity( ) * ( TICKS_TO_TIME( 7 ) + ctx.m_flOutLatency );

	// credits: diamondhack!
	for ( std::size_t i{ }; i < ctx.m_pLocal->m_CachedBoneData( ).Count( ); ++i ) {
		auto& bone = ctx.m_pLocal->m_CachedBoneData( ).Base( )[ i ];

		bone[ 0 ][ 3 ] += delta.x;
		bone[ 1 ][ 3 ] += delta.y;
		bone[ 2 ][ 3 ] += delta.z;
	}

	const auto backup_origin = ctx.m_pLocal->m_vecOrigin( );

	ctx.m_pLocal->m_vecOrigin( ) += delta;
	ctx.m_pLocal->SetAbsOrigin( ctx.m_pLocal->m_vecOrigin( ) );

	const auto hitboxSet = ctx.m_pLocal->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( ctx.m_pLocal->m_nHitboxSet( ) );
	if ( !hitboxSet )
		return false;

	const auto hitbox = hitboxSet->GetHitbox( HITBOX_PELVIS );
	if ( !hitbox )
		return false;

	Vector center = ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f;
	center = Math::VectorTransform( center, ctx.m_pLocal->m_CachedBoneData( ).Base( )[ hitbox->iBone ] );

	if ( ctx.m_iTicksAllowed ) {
		for ( int i = 1; i <= 64; i++ ) {
			auto ent = static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) );
			if ( !ent || ent->IsDormant( ) || ent == ctx.m_pLocal || ent->IsDead( ) || ent->m_bGunGameImmunity( ) || ent->IsTeammate( ) )
				continue;

			const auto data{ Features::Autowall.FireEmulated( ent, ctx.m_pLocal,
				ent->m_vecOrigin( ) + ent->m_vecViewOffset( ), center ) };

			if ( data.dmg > 1 ) {
				ctx.m_pLocal->m_vecOrigin( ) = backup_origin;
				ctx.m_pLocal->SetAbsOrigin( backup_origin );
				memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), backup_matrix, ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
				return true;
			}
		}
	}

	return false;
}

bool FASTCALL Hooks::hkWriteUserCmdDeltaToBuffer( void* ecx, void* edx, int slot, bf_write* buf, int from, int to, bool is_new_command ) {
	static auto oWriteUserCmdDeltaToBuffer = DTR::WriteUserCmdDeltaToBuffer.GetOriginal<decltype( &hkWriteUserCmdDeltaToBuffer )>( );
	if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) 
		|| ctx.m_pLocal->m_fFlags( ) & FL_FROZEN )
		return oWriteUserCmdDeltaToBuffer( ecx, edx, slot, buf, from, to, is_new_command );

	const auto moveMsg{ reinterpret_cast< MoveMsg_t* >( *reinterpret_cast< std::uintptr_t* >(
			reinterpret_cast< std::uintptr_t >( _AddressOfReturnAddress( ) ) - sizeof( std::uintptr_t ) ) - 0x58u ) };

	const auto nextCmdNumber{ Interfaces::ClientState->nChokedCommands + Interfaces::ClientState->iLastOutgoingCommand + 1 };

	bool stoplol{ };
	static int timer{ };


	if ( Interfaces::ClientState->iLastOutgoingCommand == Features::Exploits.m_iRechargeCmd
		|| Features::Exploits.m_iShiftAmount ) {
		if ( from == -1 ) {
			if ( Interfaces::ClientState->iLastOutgoingCommand == Features::Exploits.m_iRechargeCmd ) {
				moveMsg->m_iNewCmds = 1;
				moveMsg->m_iBackupCmds = 0;

				for ( to = nextCmdNumber - moveMsg->m_iNewCmds + 1; to <= nextCmdNumber; ++to ) {
					if ( !oWriteUserCmdDeltaToBuffer( ecx, edx, slot, buf, from, to, true ) ) {
						stoplol = true;
						break;
					}

					from = to;
				}
			}
			else
				stoplol = Features::Exploits.Shift( ecx, edx, slot, buf, from, to, moveMsg );
		}

		return !stoplol;
	}
	else if ( ctx.m_iTicksAllowed >= 14
		&& Config::Get<bool>( Vars.ExploitsDoubletapDefensive ) ) {
		static bool stoppedRunning{ };

		if ( from == -1 ) {
			if ( timer++ >= 14 )
				timer = 0;
			stoppedRunning = timer;// InPeek( );
			if ( !stoppedRunning )
				stoplol = Features::Exploits.BreakLC( ecx, edx, slot, buf, from, to, moveMsg, timer >= 14 );
		}

		if ( !stoppedRunning )
			return !stoplol;
	}
	else
		timer = 0;

	/*else if ( ctx.m_iTicksAllowed >= 14
		&& Config::Get<bool>( Vars.ExploitsDoubletapDefensive ) ) {
		static bool stoppedRunning{ };
		static bool finalTick{ };

		if ( from == -1 ) {
			const bool b4{ finalTick };
			finalTick = InPeek( );

			stoppedRunning = b4;
			if ( !stoppedRunning )
				stoplol = Features::Exploits.BreakLC( ecx, edx, slot, buf, from, to, moveMsg, finalTick );
		}

		if ( !stoppedRunning )
			return !stoplol;
	}
	else
		timer = 0;*/

	if ( from == -1
		&& ctx.m_iTicksAllowed > 0 ) {
		const auto extraTicks{ std::max( **( int** )Offsets::Sigs.numticks - 1, 0 ) };

		const auto newCmds{ std::min( moveMsg->m_iNewCmds + extraTicks + ctx.m_iTicksAllowed, 16 ) };

		auto maxShiftedCmds{ newCmds - moveMsg->m_iNewCmds - extraTicks };

		ctx.m_iTicksAllowed = std::max( maxShiftedCmds, 0 );
	}

	return oWriteUserCmdDeltaToBuffer( ecx, edx, slot, buf, from, to, is_new_command );
}