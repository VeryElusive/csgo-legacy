#pragma once

using matrix3x3_t = float[3][3];

struct matrix3x4_t
{
	matrix3x4_t() = default;

	constexpr matrix3x4_t(
		const float m00, const float m01, const float m02, const float m03,
		const float m10, const float m11, const float m12, const float m13,
		const float m20, const float m21, const float m22, const float m23)
	{
		arrData[0][0] = m00; arrData[0][1] = m01; arrData[0][2] = m02; arrData[0][3] = m03;
		arrData[1][0] = m10; arrData[1][1] = m11; arrData[1][2] = m12; arrData[1][3] = m13;
		arrData[2][0] = m20; arrData[2][1] = m21; arrData[2][2] = m22; arrData[2][3] = m23;
	}

	constexpr matrix3x4_t(const Vector& xAxis, const Vector& yAxis, const Vector& zAxis, const Vector& vecOrigin)
	{
		Init(xAxis, yAxis, zAxis, vecOrigin);
	}

	matrix3x4_t ConcatTransforms( matrix3x4_t in ) const {
		auto& m = arrData;
		matrix3x4_t out;
		out[ 0 ][ 0 ] = m[ 0 ][ 0 ] * in[ 0 ][ 0 ] + m[ 0 ][ 1 ] * in[ 1 ][ 0 ] + m[ 0 ][ 2 ] * in[ 2 ][ 0 ];
		out[ 0 ][ 1 ] = m[ 0 ][ 0 ] * in[ 0 ][ 1 ] + m[ 0 ][ 1 ] * in[ 1 ][ 1 ] + m[ 0 ][ 2 ] * in[ 2 ][ 1 ];
		out[ 0 ][ 2 ] = m[ 0 ][ 0 ] * in[ 0 ][ 2 ] + m[ 0 ][ 1 ] * in[ 1 ][ 2 ] + m[ 0 ][ 2 ] * in[ 2 ][ 2 ];
		out[ 0 ][ 3 ] = m[ 0 ][ 0 ] * in[ 0 ][ 3 ] + m[ 0 ][ 1 ] * in[ 1 ][ 3 ] + m[ 0 ][ 2 ] * in[ 2 ][ 3 ] + m[ 0 ][ 3 ];
		out[ 1 ][ 0 ] = m[ 1 ][ 0 ] * in[ 0 ][ 0 ] + m[ 1 ][ 1 ] * in[ 1 ][ 0 ] + m[ 1 ][ 2 ] * in[ 2 ][ 0 ];
		out[ 1 ][ 1 ] = m[ 1 ][ 0 ] * in[ 0 ][ 1 ] + m[ 1 ][ 1 ] * in[ 1 ][ 1 ] + m[ 1 ][ 2 ] * in[ 2 ][ 1 ];
		out[ 1 ][ 2 ] = m[ 1 ][ 0 ] * in[ 0 ][ 2 ] + m[ 1 ][ 1 ] * in[ 1 ][ 2 ] + m[ 1 ][ 2 ] * in[ 2 ][ 2 ];
		out[ 1 ][ 3 ] = m[ 1 ][ 0 ] * in[ 0 ][ 3 ] + m[ 1 ][ 1 ] * in[ 1 ][ 3 ] + m[ 1 ][ 2 ] * in[ 2 ][ 3 ] + m[ 1 ][ 3 ];
		out[ 2 ][ 0 ] = m[ 2 ][ 0 ] * in[ 0 ][ 0 ] + m[ 2 ][ 1 ] * in[ 1 ][ 0 ] + m[ 2 ][ 2 ] * in[ 2 ][ 0 ];
		out[ 2 ][ 1 ] = m[ 2 ][ 0 ] * in[ 0 ][ 1 ] + m[ 2 ][ 1 ] * in[ 1 ][ 1 ] + m[ 2 ][ 2 ] * in[ 2 ][ 1 ];
		out[ 2 ][ 2 ] = m[ 2 ][ 0 ] * in[ 0 ][ 2 ] + m[ 2 ][ 1 ] * in[ 1 ][ 2 ] + m[ 2 ][ 2 ] * in[ 2 ][ 2 ];
		out[ 2 ][ 3 ] = m[ 2 ][ 0 ] * in[ 0 ][ 3 ] + m[ 2 ][ 1 ] * in[ 1 ][ 3 ] + m[ 2 ][ 2 ] * in[ 2 ][ 3 ] + m[ 2 ][ 3 ];
		return out;
	}

	constexpr void Init(const Vector& vecForward, const Vector& vecLeft, const Vector& vecUp, const Vector& vecOrigin)
	{
		SetForward(vecForward);
		SetLeft(vecLeft);
		SetUp(vecUp);
		SetOrigin(vecOrigin);
	}

	constexpr void SetForward(const Vector& vecForward)
	{
		this->arrData[0][0] = vecForward.x;
		this->arrData[1][0] = vecForward.y;
		this->arrData[2][0] = vecForward.z;
	}

	constexpr void SetLeft(const Vector& vecLeft)
	{
		this->arrData[0][1] = vecLeft.x;
		this->arrData[1][1] = vecLeft.y;
		this->arrData[2][1] = vecLeft.z;
	}

	constexpr void SetUp(const Vector& vecUp)
	{
		this->arrData[0][2] = vecUp.x;
		this->arrData[1][2] = vecUp.y;
		this->arrData[2][2] = vecUp.z;
	}

	constexpr void SetOrigin(const Vector& vecOrigin)
	{
		this->arrData[0][3] = vecOrigin.x;
		this->arrData[1][3] = vecOrigin.y;
		this->arrData[2][3] = vecOrigin.z;
	}	

	void SetAngles( const float yaw, const float pitch = 0, const float roll = 0 ) {
		constexpr auto ONE_DEGREE_IN_RADIANS = static_cast< float >( 3.14159265358979323846 ) / 180.f;
		auto degs2sincos = [ ]( float degs, float& sine, float& cosine ) { sine = std::sin( degs * ONE_DEGREE_IN_RADIANS ), cosine = std::cos( degs * ONE_DEGREE_IN_RADIANS ); };

		auto sp = 0.f, cp = 0.f, sy = 0.f, cy = 0.f, sz = 0.f, cz = 0.f;
		degs2sincos( pitch, sp, cp ), degs2sincos( yaw, sy, cy ), degs2sincos( roll, sz, cz );

		this->arrData[ 0 ][ 0 ] = cp * cy; this->arrData[ 1 ][ 0 ] = cp * sy; this->arrData[ 2 ][ 0 ] = -sp;

		auto szsy = sz * sy, szcy = sz * cy, czsy = cz * sy, czcy = cz * cy;

		this->arrData[ 0 ][ 1 ] = sp * szcy - czsy; this->arrData[ 1 ][ 1 ] = sp * szsy + czcy; this->arrData[ 2 ][ 1 ] = sz * cp;
		this->arrData[ 0 ][ 2 ] = sp * czcy + szsy; this->arrData[ 1 ][ 2 ] = sp * czsy - szcy; this->arrData[ 2 ][ 2 ] = cz * cp;
	}

	constexpr Vector GetOrigin( )
	{
		return { this->arrData[ 0 ][ 3 ],
			this->arrData[ 1 ][ 3 ],
			this->arrData[ 2 ][ 3 ] };
	}

	constexpr void Invalidate()
	{
		for (auto& arrSubData : arrData)
		{
			for (auto& flData : arrSubData)
				flData = std::numeric_limits<float>::infinity();
		}
	}

	float* operator[](const int nIndex)
	{
		return arrData[nIndex];
	}

	const float* operator[](const int nIndex) const
	{
		return arrData[nIndex];
	}

	matrix3x4_t operator*( const matrix3x4_t& vm ) const {
		return ConcatTransforms( vm );
	}

	matrix3x4_t operator*( const float& other ) const {
		matrix3x4_t ret;
		auto& m = arrData;
		for ( int i = 0; i < 12; i++ ) {
			( ( float* )ret.arrData )[ i ] = ( ( float* )m )[ i ] * other;
		}
		return ret;
	}

	matrix3x4_t operator+( const matrix3x4_t& other ) const {
		matrix3x4_t ret;
		auto& m = arrData;
		for ( int i = 0; i < 12; i++ ) {
			( ( float* )ret.arrData )[ i ] = ( ( float* )m )[ i ] + ( ( float* )other.arrData )[ i ];
		}
		return ret;
	}

	Vector operator*( const Vector& vVec ) const {
		auto& m = arrData;
		Vector vRet;
		vRet.x = m[ 0 ][ 0 ] * vVec.x + m[ 0 ][ 1 ] * vVec.y + m[ 0 ][ 2 ] * vVec.z + m[ 0 ][ 3 ];
		vRet.y = m[ 1 ][ 0 ] * vVec.x + m[ 1 ][ 1 ] * vVec.y + m[ 1 ][ 2 ] * vVec.z + m[ 1 ][ 3 ];
		vRet.z = m[ 2 ][ 0 ] * vVec.x + m[ 2 ][ 1 ] * vVec.y + m[ 2 ][ 2 ] * vVec.z + m[ 2 ][ 3 ];

		return vRet;
	}

	[[nodiscard]] constexpr Vector at(const int nIndex) const
	{
		return Vector(arrData[0][nIndex], arrData[1][nIndex], arrData[2][nIndex]);
	}

	float* Base()
	{
		return &arrData[0][0];
	}

	[[nodiscard]] const float* Base() const
	{
		return &arrData[0][0];
	}

	float arrData[3][4] = { };
};

struct alignas( 16 ) matrix3x4a_t : public matrix3x4_t {};
struct ViewMatrix_t
{
	ViewMatrix_t() = default;

	constexpr ViewMatrix_t(
		const float m00, const float m01, const float m02, const float m03,
		const float m10, const float m11, const float m12, const float m13,
		const float m20, const float m21, const float m22, const float m23,
		const float m30, const float m31, const float m32, const float m33)
	{
		arrData[0][0] = m00; arrData[0][1] = m01; arrData[0][2] = m02; arrData[0][3] = m03;
		arrData[1][0] = m10; arrData[1][1] = m11; arrData[1][2] = m12; arrData[1][3] = m13;
		arrData[2][0] = m20; arrData[2][1] = m21; arrData[2][2] = m22; arrData[2][3] = m23;
		arrData[3][0] = m30; arrData[3][1] = m31; arrData[3][2] = m32; arrData[3][3] = m33;
	}

	constexpr ViewMatrix_t(const matrix3x4_t& matFrom, const Vector4D& vecAdditionalColumn = { })
	{
		arrData[0][0] = matFrom.arrData[0][0]; arrData[0][1] = matFrom.arrData[0][1]; arrData[0][2] = matFrom.arrData[0][2]; arrData[0][3] = matFrom.arrData[0][3];
		arrData[1][0] = matFrom.arrData[1][0]; arrData[1][1] = matFrom.arrData[1][1]; arrData[1][2] = matFrom.arrData[1][2]; arrData[1][3] = matFrom.arrData[1][3];
		arrData[2][0] = matFrom.arrData[2][0]; arrData[2][1] = matFrom.arrData[2][1]; arrData[2][2] = matFrom.arrData[2][2]; arrData[2][3] = matFrom.arrData[2][3];
		arrData[3][0] = vecAdditionalColumn.x; arrData[3][1] = vecAdditionalColumn.y; arrData[3][2] = vecAdditionalColumn.z; arrData[3][3] = vecAdditionalColumn.w;
	}

	float* operator[](const int nIndex)
	{
		return arrData[nIndex];
	}

	const float* operator[](const int nIndex) const
	{
		return arrData[nIndex];
	}

	[[nodiscard]] constexpr Vector4D at(const int nIndex) const
	{
		return Vector4D(arrData[0][nIndex], arrData[1][nIndex], arrData[2][nIndex], arrData[3][nIndex]);
	}

	constexpr ViewMatrix_t& operator+=(const ViewMatrix_t& matAdd)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int n = 0; n < 4; n++)
				this->arrData[i][n] += matAdd[i][n];
		}

		return *this;
	}

	constexpr ViewMatrix_t& operator-=(const ViewMatrix_t& matSubtract)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int n = 0; n < 4; n++)
				this->arrData[i][n] -= matSubtract[i][n];
		}

		return *this;
	}

	ViewMatrix_t operator*(const ViewMatrix_t& matMultiply) const
	{
		return ViewMatrix_t(
			arrData[0][0] * matMultiply.arrData[0][0] + arrData[0][1] * matMultiply.arrData[1][0] + arrData[0][2] * matMultiply.arrData[2][0] + arrData[0][3] * matMultiply.arrData[3][0],
			arrData[0][0] * matMultiply.arrData[0][1] + arrData[0][1] * matMultiply.arrData[1][1] + arrData[0][2] * matMultiply.arrData[2][1] + arrData[0][3] * matMultiply.arrData[3][1],
			arrData[0][0] * matMultiply.arrData[0][2] + arrData[0][1] * matMultiply.arrData[1][2] + arrData[0][2] * matMultiply.arrData[2][2] + arrData[0][3] * matMultiply.arrData[3][2],
			arrData[0][0] * matMultiply.arrData[0][3] + arrData[0][1] * matMultiply.arrData[1][3] + arrData[0][2] * matMultiply.arrData[2][3] + arrData[0][3] * matMultiply.arrData[3][3],

			arrData[1][0] * matMultiply.arrData[0][0] + arrData[1][1] * matMultiply.arrData[1][0] + arrData[1][2] * matMultiply.arrData[2][0] + arrData[1][3] * matMultiply.arrData[3][0],
			arrData[1][0] * matMultiply.arrData[0][1] + arrData[1][1] * matMultiply.arrData[1][1] + arrData[1][2] * matMultiply.arrData[2][1] + arrData[1][3] * matMultiply.arrData[3][1],
			arrData[1][0] * matMultiply.arrData[0][2] + arrData[1][1] * matMultiply.arrData[1][2] + arrData[1][2] * matMultiply.arrData[2][2] + arrData[1][3] * matMultiply.arrData[3][2],
			arrData[1][0] * matMultiply.arrData[0][3] + arrData[1][1] * matMultiply.arrData[1][3] + arrData[1][2] * matMultiply.arrData[2][3] + arrData[1][3] * matMultiply.arrData[3][3],

			arrData[2][0] * matMultiply.arrData[0][0] + arrData[2][1] * matMultiply.arrData[1][0] + arrData[2][2] * matMultiply.arrData[2][0] + arrData[2][3] * matMultiply.arrData[3][0],
			arrData[2][0] * matMultiply.arrData[0][1] + arrData[2][1] * matMultiply.arrData[1][1] + arrData[2][2] * matMultiply.arrData[2][1] + arrData[2][3] * matMultiply.arrData[3][1],
			arrData[2][0] * matMultiply.arrData[0][2] + arrData[2][1] * matMultiply.arrData[1][2] + arrData[2][2] * matMultiply.arrData[2][2] + arrData[2][3] * matMultiply.arrData[3][2],
			arrData[2][0] * matMultiply.arrData[0][3] + arrData[2][1] * matMultiply.arrData[1][3] + arrData[2][2] * matMultiply.arrData[2][3] + arrData[2][3] * matMultiply.arrData[3][3],

			arrData[3][0] * matMultiply.arrData[0][0] + arrData[3][1] * matMultiply.arrData[1][0] + arrData[3][2] * matMultiply.arrData[2][0] + arrData[3][3] * matMultiply.arrData[3][0],
			arrData[3][0] * matMultiply.arrData[0][1] + arrData[3][1] * matMultiply.arrData[1][1] + arrData[3][2] * matMultiply.arrData[2][1] + arrData[3][3] * matMultiply.arrData[3][1],
			arrData[3][0] * matMultiply.arrData[0][2] + arrData[3][1] * matMultiply.arrData[1][2] + arrData[3][2] * matMultiply.arrData[2][2] + arrData[3][3] * matMultiply.arrData[3][2],
			arrData[3][0] * matMultiply.arrData[0][3] + arrData[3][1] * matMultiply.arrData[1][3] + arrData[3][2] * matMultiply.arrData[2][3] + arrData[3][3] * matMultiply.arrData[3][3]);
	}

	constexpr void Identity()
	{
		for (int i = 0; i < 4; i++)
		{
			for (int n = 0; n < 4; n++)
				this->arrData[i][n] = i == n ? 1.0f : 0.0f;
		}
	}

	const matrix3x4_t& As3x4() const
	{
		return *reinterpret_cast<const matrix3x4_t*>(this);
	}

	matrix3x4_t& As3x4()
	{
		return *reinterpret_cast<matrix3x4_t*>(this);
	}

	float arrData[4][4] = { };
};

