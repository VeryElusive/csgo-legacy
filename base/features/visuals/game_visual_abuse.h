#pragma once
#include "../../core/interfaces.h"
#include "../../core/config.h"
#include "../../context.h"
#include "../../utils/math.h"
#include "../../utils/render.h"

/*
class CBulletTracer {
public:
	struct BulletImpactInfo
	{
		float m_flExpTime;
		Vector m_vecStartPos;
		Vector m_vecHitPos;
		Color m_cColor;
		int m_nIndex;
		int m_nTickbase;
		bool ignore[ 64 ];
	};

	void Draw( );
	FORCEINLINE void AddBeamInfo( BulletImpactInfo beamInfo ) {
		m_vecBulletImpactInfo.emplace_back( beamInfo );
	}

private:
	std::vector<BulletImpactInfo> m_vecBulletImpactInfo;
};
*/