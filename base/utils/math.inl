#include "math.h"

FORCEINLINE void Math::VectorAngles( const Vector& vecForward, QAngle& angView )
{
	float flPitch, flYaw;

	if ( vecForward.x == 0.f && vecForward.y == 0.f )
	{
		flPitch = ( vecForward.z > 0.f ) ? 270.f : 90.f;
		flYaw = 0.f;
	}
	else
	{
		flPitch = std::atan2f( -vecForward.z, vecForward.Length2D( ) ) * 180.f / M_PI;

		if ( flPitch < 0.f )
			flPitch += 360.f;

		flYaw = std::atan2f( vecForward.y, vecForward.x ) * 180.f / M_PI;

		if ( flYaw < 0.f )
			flYaw += 360.f;
	}

	angView.x = flPitch;
	angView.y = flYaw;
	angView.z = 0.f;
}

FORCEINLINE void Math::AngleVectors( const QAngle& angView, Vector* pForward, Vector* pRight, Vector* pUp )
{
	float sp, sy, sr, cp, cy, cr;

	DirectX::XMScalarSinCos( &sp, &cp, DEG2RAD( angView.x ) );
	DirectX::XMScalarSinCos( &sy, &cy, DEG2RAD( angView.y ) );
	DirectX::XMScalarSinCos( &sr, &cr, DEG2RAD( angView.z ) );

	if ( pForward != nullptr )
	{
		pForward->x = cp * cy;
		pForward->y = cp * sy;
		pForward->z = -sp;
	}

	if ( pRight != nullptr )
	{
		pRight->x = -1 * sr * sp * cy + -1 * cr * -sy;
		pRight->y = -1 * sr * sp * sy + -1 * cr * cy;
		pRight->z = -1 * sr * cp;
	}

	if ( pUp != nullptr )
	{
		pUp->x = cr * sp * cy + -sr * -sy;
		pUp->y = cr * sp * sy + -sr * cy;
		pUp->z = cr * cp;
	}
}

FORCEINLINE void Math::AngleMatrix( const QAngle& angView, matrix3x4_t& matOutput, const Vector& vecOrigin )
{
	float sp, sy, sr, cp, cy, cr;

	DirectX::XMScalarSinCos( &sp, &cp, DEG2RAD( angView.x ) );
	DirectX::XMScalarSinCos( &sy, &cy, DEG2RAD( angView.y ) );
	DirectX::XMScalarSinCos( &sr, &cr, DEG2RAD( angView.z ) );

	matOutput.SetForward( Vector( cp * cy, cp * sy, -sp ) );

	const float crcy = cr * cy;
	const float crsy = cr * sy;
	const float srcy = sr * cy;
	const float srsy = sr * sy;

	matOutput.SetLeft( Vector( sp * srcy - crsy, sp * srsy + crcy, sr * cp ) );
	matOutput.SetUp( Vector( sp * crcy + srsy, sp * crsy - srcy, cr * cp ) );
	matOutput.SetOrigin( vecOrigin );
}

FORCEINLINE Vector2D Math::AnglePixels( const float flSensitivity, const float flPitch, const float flYaw, const QAngle& angBegin, const QAngle& angEnd )
{
	QAngle angDelta = angBegin - angEnd;
	angDelta.Normalize( );

	const float flPixelMovePitch = ( -angDelta.x ) / ( flYaw * flSensitivity );
	const float flPixelMoveYaw = ( angDelta.y ) / ( flPitch * flSensitivity );

	return Vector2D( flPixelMoveYaw, flPixelMovePitch );
}

FORCEINLINE QAngle Math::PixelsAngle( const float flSensitivity, const float flPitch, const float flYaw, const Vector2D& vecPixels )
{
	const float flAngleMovePitch = ( -vecPixels.x ) * ( flYaw * flSensitivity );
	const float flAngleMoveYaw = ( vecPixels.y ) * ( flPitch * flSensitivity );

	return QAngle( flAngleMoveYaw, flAngleMovePitch, 0.f );
}

FORCEINLINE float Math::GetFov( const QAngle& viewAngle, const QAngle& aimAngle ) {
	QAngle delta = aimAngle - viewAngle;
	delta.Normalize( );

	return std::min( sqrtf( powf( delta.x, 2.0f ) + powf( delta.y, 2.0f ) ), 180.0f );
}

FORCEINLINE QAngle Math::CalcAngle( const Vector& vecStart, const Vector& vecEnd )
{
	QAngle angView;
	const Vector vecDelta = vecEnd - vecStart;
	VectorAngles( vecDelta, angView );
	angView.Normalize( );

	return angView;
}

FORCEINLINE Vector Math::VectorTransform( const Vector& vecTransform, const matrix3x4_t& matrix )
{
	return Vector( vecTransform.DotProduct( matrix[ 0 ] ) + matrix[ 0 ][ 3 ],
		vecTransform.DotProduct( matrix[ 1 ] ) + matrix[ 1 ][ 3 ],
		vecTransform.DotProduct( matrix[ 2 ] ) + matrix[ 2 ][ 3 ] );
}

FORCEINLINE  Vector CrossProduct( const Vector& a, const Vector& b )
{
	return Vector( a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x );
}


FORCEINLINE void Math::VectorVectors( const Vector& forward, Vector& right, Vector& up )
{
	Vector tmp;

	if ( fabs( forward[ 0 ] ) < 1e-6 && fabs( forward[ 1 ] ) < 1e-6 )
	{
		// pitch 90 degrees up/down from identity
		right[ 0 ] = 0;
		right[ 1 ] = -1;
		right[ 2 ] = 0;
		up[ 0 ] = -forward[ 2 ];
		up[ 1 ] = 0;
		up[ 2 ] = 0;
	}
	else
	{
		tmp[ 0 ] = 0; tmp[ 1 ] = 0; tmp[ 2 ] = 1.0;
		right = CrossProduct( forward, tmp );
		right = right.Normalized( );
		up = CrossProduct( right, forward );
		up = up.Normalized( );
	}
}

FORCEINLINE void MatrixSetColumn( const Vector& in, int column, matrix3x4_t& out )
{
	out[ 0 ][ column ] = in.x;
	out[ 1 ][ column ] = in.y;
	out[ 2 ][ column ] = in.z;
}

FORCEINLINE  matrix3x4_t Math::VectorMatrix( const Vector& forward )
{
	matrix3x4_t matrix;
	Vector right, up;
	VectorVectors( forward, right, up );

	MatrixSetColumn( forward, 0, matrix );
	MatrixSetColumn( right * -1.f, 1, matrix );
	MatrixSetColumn( up, 2, matrix );

	return matrix;
}

FORCEINLINE void Math::VectorIRotate( const Vector& in1, const matrix3x4_t& in2, Vector& out )
{
	out.x = in1.x * in2[ 0 ][ 0 ] + in1.y * in2[ 1 ][ 0 ] + in1.z * in2[ 2 ][ 0 ];
	out.y = in1.x * in2[ 0 ][ 1 ] + in1.y * in2[ 1 ][ 1 ] + in1.z * in2[ 2 ][ 1 ];
	out.z = in1.x * in2[ 0 ][ 2 ] + in1.y * in2[ 1 ][ 2 ] + in1.z * in2[ 2 ][ 2 ];
}

FORCEINLINE void Math::VectorITransform( const Vector& in1, const matrix3x4_t& in2, Vector& out )
{
	out.x = ( in1.x - in2[ 0 ][ 3 ] ) * in2[ 0 ][ 0 ] + ( in1.y - in2[ 1 ][ 3 ] ) * in2[ 1 ][ 0 ] + ( in1.z - in2[ 2 ][ 3 ] ) * in2[ 2 ][ 0 ];
	out.y = ( in1.x - in2[ 0 ][ 3 ] ) * in2[ 0 ][ 1 ] + ( in1.y - in2[ 1 ][ 3 ] ) * in2[ 1 ][ 1 ] + ( in1.z - in2[ 2 ][ 3 ] ) * in2[ 2 ][ 1 ];
	out.z = ( in1.x - in2[ 0 ][ 3 ] ) * in2[ 0 ][ 2 ] + ( in1.y - in2[ 1 ][ 3 ] ) * in2[ 1 ][ 2 ] + ( in1.z - in2[ 2 ][ 3 ] ) * in2[ 2 ][ 2 ];
}

FORCEINLINE Vector Math::ExtrapolateTick( const Vector& p0, const Vector& v0 )
{
	// position formula: p0 + v0t
	return p0 + ( v0 * Interfaces::Globals->flIntervalPerTick );
}

typedef __declspec( align( 16 ) ) union
{
	float f[ 4 ];
	__m128 v;
} m128;

FORCEINLINE  __m128 sqrt_ps( const __m128 squared )
{
	return _mm_sqrt_ps( squared );
}

FORCEINLINE bool Math::WorldToScreen( const Vector& origin, Vector& screen )
{
	if ( !ScreenTransform( origin, screen ) )
	{
		float x = ctx.m_ve2ScreenSize.x / 2;
		float y = ctx.m_ve2ScreenSize.y / 2;
		x += 0.5 * screen.x * ctx.m_ve2ScreenSize.x + 0.5f;
		y -= 0.5 * screen.y * ctx.m_ve2ScreenSize.y + 0.5f;
		screen.x = x;
		screen.y = y;
		return true;
	}

	return false;
}

FORCEINLINE bool Math::WorldToScreen( const Vector& in, Vector2D& out ) {
	if ( ScreenTransform( in, out ) ) {
		out.x = ( ctx.m_ve2ScreenSize.x * 0.5f ) + ( out.x * ctx.m_ve2ScreenSize.x ) * 0.5f;
		out.y = ( ctx.m_ve2ScreenSize.y * 0.5f ) - ( out.y * ctx.m_ve2ScreenSize.y ) * 0.5f;

		return true;
	}
	return false;
}

FORCEINLINE float Math::SegmentToSegment( const Vector s1, const Vector s2, const Vector k1, const Vector k2 )
{
	static auto constexpr epsilon = 0.00000011920929f;

	const auto u = s2 - s1;
	const auto v = k2 - k1;
	const auto w = s1 - k1;

	const auto a = u.DotProduct( u );
	const auto b = u.DotProduct( v );
	const auto c = v.DotProduct( v );
	const auto d = u.DotProduct( w );
	const auto e = v.DotProduct( w );
	const auto D = a * c - b * b;
	float sn, sd = D;
	float tn, td = D;

	if ( D < epsilon ) {
		sn = 0.0f;
		sd = 1.0f;
		tn = e;
		td = c;
	}
	else {
		sn = b * e - c * d;
		tn = a * e - b * d;

		if ( sn < 0.0f ) {
			sn = 0.0f;
			tn = e;
			td = c;
		}
		else if ( sn > sd ) {
			sn = sd;
			tn = e + b;
			td = c;
		}
	}

	if ( tn < 0.0f ) {
		tn = 0.0f;

		if ( -d < 0.0f )
			sn = 0.0f;
		else if ( -d > a )
			sn = sd;
		else {
			sn = -d;
			sd = a;
		}
	}
	else if ( tn > td ) {
		tn = td;

		if ( -d + b < 0.0f )
			sn = 0.f;
		else if ( -d + b > a )
			sn = sd;
		else {
			sn = -d + b;
			sd = a;
		}
	}

	const float sc = abs( sn ) < epsilon ? 0.0f : sn / sd;
	const float tc = abs( tn ) < epsilon ? 0.0f : tn / td;

	const auto dp = w + u * sc - v * tc;
	return dp.Length( );
}

FORCEINLINE bool Math::IntersectionBoundingBox( const Vector& src, const Vector& dir, const Vector& min, const Vector& max, Vector* hit_point ) {
	/*
	Fast Ray-Box Intersection
	by Andrew Woo
	from "Graphics Gems", Academic Press, 1990
*/

	constexpr auto NUMDIM = 3;
	constexpr auto RIGHT = 0;
	constexpr auto LEFT = 1;
	constexpr auto MIDDLE = 2;

	bool inside = true;
	char quadrant[ NUMDIM ];
	int i;

	// Rind candidate planes; this loop can be avoided if
	// rays cast all from the eye(assume perpsective view)
	Vector candidatePlane;
	for ( i = 0; i < NUMDIM; i++ ) {
		if ( src[ i ] < min[ i ] ) {
			quadrant[ i ] = LEFT;
			candidatePlane[ i ] = min[ i ];
			inside = false;
		}
		else if ( src[ i ] > max[ i ] ) {
			quadrant[ i ] = RIGHT;
			candidatePlane[ i ] = max[ i ];
			inside = false;
		}
		else {
			quadrant[ i ] = MIDDLE;
		}
	}

	// Ray origin inside bounding box
	if ( inside ) {
		if ( hit_point )
			*hit_point = src;
		return true;
	}

	// Calculate T distances to candidate planes
	Vector maxT;
	for ( i = 0; i < NUMDIM; i++ ) {
		if ( quadrant[ i ] != MIDDLE && dir[ i ] != 0.f )
			maxT[ i ] = ( candidatePlane[ i ] - src[ i ] ) / dir[ i ];
		else
			maxT[ i ] = -1.f;
	}

	// Get largest of the maxT's for final choice of intersection
	int whichPlane = 0;
	for ( i = 1; i < NUMDIM; i++ ) {
		if ( maxT[ whichPlane ] < maxT[ i ] )
			whichPlane = i;
	}

	// Check final candidate actually inside box
	if ( maxT[ whichPlane ] < 0.f )
		return false;

	for ( i = 0; i < NUMDIM; i++ ) {
		if ( whichPlane != i ) {
			float temp = src[ i ] + maxT[ whichPlane ] * dir[ i ];
			if ( temp < min[ i ] || temp > max[ i ] ) {
				return false;
			}
			else if ( hit_point ) {
				( *hit_point )[ i ] = temp;
			}
		}
		else if ( hit_point ) {
			( *hit_point )[ i ] = candidatePlane[ i ];
		}
	}

	// ray hits box
	return true;
}

FORCEINLINE float Math::ApproachAngle( float target, float value, float speed )
{
	target = anglemod( target );
	value = anglemod( value );

	float delta = target - value;

	// Speed is assumed to be positive
	if ( speed < 0 )
		speed = -speed;

	if ( delta < -180 )
		delta += 360;
	else if ( delta > 180 )
		delta -= 360;

	if ( delta > speed )
		value += speed;
	else if ( delta < -speed )
		value -= speed;
	else
		value = target;

	return value;
}
