#include "engine_prediction.h"

void CEnginePrediction::RunCommand( CUserCmd& cmd ) {
	if ( !ctx.m_pWeapon )
		return;

	m_flCurtime = Interfaces::Globals->flCurTime;
	m_flFrametime = Interfaces::Globals->flFrameTime;

	Interfaces::Globals->flCurTime = TICKS_TO_TIME( ctx.m_pLocal->m_nTickBase( ) );
	Interfaces::Globals->flFrameTime = Interfaces::Prediction->bEnginePaused ? 0.f : Interfaces::Globals->flIntervalPerTick;

	const auto backupInPrediction{ Interfaces::Prediction->bInPrediction };
	const auto backupFirstTimePrediction{ Interfaces::Prediction->Split->bIsFirstTimePredicted };

	Interfaces::Prediction->bInPrediction = true;
	Interfaces::Prediction->Split->bIsFirstTimePredicted = false;

	// run boost fix 
	Interfaces::Prediction->CheckMovingGround( ctx.m_pLocal, Interfaces::Globals->flFrameTime );

	Interfaces::MoveHelper->SetHost( ctx.m_pLocal );
	Interfaces::Prediction->SetupMove( ctx.m_pLocal, &cmd, Interfaces::MoveHelper, &MoveData );
	Interfaces::GameMovement->ProcessMovement( ctx.m_pLocal, &MoveData );
	Interfaces::Prediction->FinishMove( ctx.m_pLocal, &cmd, &MoveData );

	if ( ctx.m_pLocal->m_fFlags( ) & FL_DUCKING ) {
		//const auto backupMaxs{ ctx.m_pLocal->m_vecMaxs( ).z };
	
		ctx.m_pLocal->SetCollisionBounds( { -16.f, -16.f, 0.f }, { 16.f, 16.f, 54.f } );
		ctx.m_pLocal->m_vecViewOffset( ).z = 46.f;

		/*if ( backupMaxs != ctx.m_pLocal->m_vecMaxs( ).z ) {
			// rebuild: server.dll/client.dll @ 55 8B EC 8B 45 10 F3 0F 10 81
			ctx.m_pLocal->m_flNewBoundsMaxs( ) = ctx.m_pLocal->m_flUnknownVar( ) + backupMaxs;
			ctx.m_pLocal->m_flNewBoundsTime( ) = ctx.m_flFixedCurtime;
		}*/

	}
	else {
		//const auto backupMaxs{ ctx.m_pLocal->m_vecMaxs( ).z };

		ctx.m_pLocal->SetCollisionBounds( { -16.f, -16.f, 0.f }, { 16.f, 16.f, 72.f } );
		ctx.m_pLocal->m_vecViewOffset( ).z = 64.f;

		/*if ( backupMaxs != ctx.m_pLocal->m_vecMaxs( ).z ) {
			// rebuild: server.dll/client.dll @ 55 8B EC 8B 45 10 F3 0F 10 81
			ctx.m_pLocal->m_flNewBoundsMaxs( ) = ctx.m_pLocal->m_flUnknownVar( ) + backupMaxs;
			ctx.m_pLocal->m_flNewBoundsTime( ) = ctx.m_flFixedCurtime;
		}*/
	}

	ctx.m_pWeapon->UpdateAccuracyPenalty( );

	Spread = ctx.m_pWeapon->GetSpread( );
	Inaccuracy = ctx.m_pWeapon->GetInaccuracy( );

	Interfaces::Prediction->bInPrediction = backupInPrediction;
	Interfaces::Prediction->Split->bIsFirstTimePredicted = backupFirstTimePrediction;

}

void CEnginePrediction::Finish( ) {
	if ( !ctx.m_pWeapon )
		return;

	Interfaces::Globals->flCurTime = m_flCurtime;
	Interfaces::Globals->flFrameTime = m_flFrametime;

	Interfaces::MoveHelper->SetHost( nullptr );
}

void CEnginePrediction::RestoreNetvars( int slot ) {
	const auto& local{ ctx.m_pLocal };
	if ( !local || local->IsDead( ) )
		return;

	const auto& data{ m_cCompressionVars.at( slot % 150 ) };
	if ( data.m_iCommandNumber != slot )
		return;

	const auto aimPunchAngleVelDiff{ local->m_aimPunchAngleVel( ) - data.m_aimPunchAngleVel };
	const auto aimPunchAngleDiff{ local->m_aimPunchAngle( ) - data.m_aimPunchAngle };
	const auto viewOffsetDiff{ local->m_vecViewOffset( ).z - data.m_vecViewOffsetZ };

	if ( std::abs( viewOffsetDiff ) <= 0.03125f )
		local->m_vecViewOffset( ).z = data.m_vecViewOffsetZ;

	if ( std::abs( aimPunchAngleDiff.x ) <= 0.03125f 
		&& std::abs( aimPunchAngleDiff.y ) <= 0.03125f 
		&& std::abs( aimPunchAngleDiff.z ) <= 0.03125f )
		local->m_aimPunchAngle( ) = data.m_aimPunchAngle;

	if ( std::abs( aimPunchAngleVelDiff.x ) <= 0.03125f 
		&& std::abs( aimPunchAngleVelDiff.y ) <= 0.03125f 
		&& std::abs( aimPunchAngleVelDiff.z ) <= 0.03125f )
		local->m_aimPunchAngleVel( ) = data.m_aimPunchAngleVel;

	auto& viewOffset{ local->m_vecViewOffset( ) };
	if ( viewOffset.z > 46.f && viewOffset.z < 47.f )
		viewOffset.z = 46.f;
	else if ( viewOffset.z > 64.f )
		viewOffset.z = 64.f;
}

void CEnginePrediction::StoreNetvars( int slot ) {
	const auto& local{ ctx.m_pLocal };
	if ( !local || local->IsDead( ) )
		return;

	auto& data = m_cCompressionVars.at( slot % 150 );

	data.m_aimPunchAngleVel = local->m_aimPunchAngleVel( );
	data.m_aimPunchAngle = local->m_aimPunchAngle( );
	data.m_vecViewOffsetZ = local->m_vecViewOffset( ).z;
}

bool CEnginePrediction::AddToDataMap( ) {
	return true;
	/*
	auto map{ ctx.m_pLocal->GetPredDescMap( ) };

	bool ret{ };

	while ( map ) {
		// BEGIN_PREDICTION_DATA( C_CSPlayer )
		if ( FNV1A::Hash( map->szDataClassName ) == FNV1A::HashConst( _( "C_CSPlayer" ) ) ) {
			static std::unique_ptr<TypeDescription_t[ ]> data( new TypeDescription_t[ map->nDataFields + 1 ] );
			std::memcpy( data.get( ), map->pDataDesc, map->nDataFields * sizeof TypeDescription_t );

			// extend datamap
			map->pDataDesc = data.get( );
			map->nDataFields += 1;
			map->iPackedSize += sizeof TypeDescription_t;

			// add m_flVelocityModifier
			TypeDescription_t velocityModifier{ };
			velocityModifier.szFieldName = _( "m_flVelocityModifier" );
			velocityModifier.fFlags = 0x100;
			velocityModifier.fieldTolerance = 0.01f;
			velocityModifier.iFieldOffset = Offsets::m_flVelocityModifier;
			velocityModifier.uFieldSize = 0x1;
			velocityModifier.fieldSizeInBytes = 0x4;
			velocityModifier.iFieldType = 0x1;
			velocityModifier.flatOffset[ TD_OFFSET_NORMAL ] = Offsets::m_flVelocityModifier;
			map->pDataDesc[ map->nDataFields - 1 ] = velocityModifier;

			map->pOptimizedDataMap = nullptr;

			ret = true;
		}

		map = map->pBaseMap;
	}

	if ( ret ) {
		Interfaces::Prediction->ShutDownPredictables( );
		Interfaces::Prediction->ReinitPredictables( );
	}

	return ret;*/
}