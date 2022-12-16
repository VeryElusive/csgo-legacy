#pragma once
#include "../../context.h"

struct CompressionVars_t {
	QAngle m_aimPunchAngle{ };
	Vector m_aimPunchAngleVel{ };
	float m_vecViewOffsetZ{ };

	int m_iCommandNumber{ };
};

class CEnginePrediction {
public:
	void PreStart( );
	void RunCommand( CUserCmd& cmd );
	void Finish( );

	void RestoreNetvars( int slot );
	void StoreNetvars( int slot );

	bool AddToDataMap( );

	float Spread{ };
	float Inaccuracy{ };
	std::array< CompressionVars_t, 150> m_cCompressionVars{ };
private:
	float CurTime{ };
	float FrameTime{ };
	//void* m_pOldWeapon{ };

	//float VelocityModifier{ };
	float RecoilIndex{ };
	float AccuracyPenalty{ };

	bool FirstTimePrediction{ };
	bool InPrediction{ };


	CMoveData MoveData{ };
};

namespace Features { inline CEnginePrediction EnginePrediction; };