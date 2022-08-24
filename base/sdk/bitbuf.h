#pragma once
// @credits: https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/public/tier1/bitbuf.h

class bf_write {
public:
	unsigned char* m_pData;
	int m_nDataBytes;
	int m_nDataBits;
	int m_iCurBit;
	bool m_bOverflow;
	bool m_bAssertOnOverflow;
	const char* m_pDebugName;

	void StartWriting( void* pData, int nBytes, int iStartBit = 0, int nBits = -1 ) {
		// Make sure it's dword aligned and padded.
		// The writing code will overrun the end of the buffer if it isn't dword aligned, so truncate to force alignment
		nBytes &= ~3;

		m_pData = ( unsigned char* )pData;
		m_nDataBytes = nBytes;

		if ( nBits == -1 ) {
			m_nDataBits = nBytes << 3;
		}
		else {
			m_nDataBits = nBits;
		}

		m_iCurBit = iStartBit;
		m_bOverflow = false;
	}

	bf_write( ) {
		m_pData = NULL;
		m_nDataBytes = 0;
		m_nDataBits = -1; // set to -1 so we generate overflow on any operation
		m_iCurBit = 0;
		m_bOverflow = false;
		m_bAssertOnOverflow = true;
		m_pDebugName = NULL;
	}

	// nMaxBits can be used as the number of bits in the buffer.
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_write( void* pData, int nBytes, int nBits = -1 ) {
		m_bAssertOnOverflow = true;
		m_pDebugName = NULL;
		StartWriting( pData, nBytes, 0, nBits );
	}

	bf_write( const char* pDebugName, void* pData, int nBytes, int nBits = -1 ) {
		m_bAssertOnOverflow = true;
		m_pDebugName = pDebugName;
		StartWriting( pData, nBytes, 0, nBits );
	}
};

class bf_read
{
public:
	std::uintptr_t uBaseAddress;
	std::uintptr_t uCurrentOffset;

	bf_read(std::uintptr_t uAddress) : uBaseAddress(uAddress), uCurrentOffset(0U) {}

	void SetOffset(std::uintptr_t uOffset)
	{
		uCurrentOffset = uOffset;
	}

	void Skip(std::uintptr_t uLength)
	{
		uCurrentOffset += uLength;
	}

	int ReadByte()
	{
		char dValue = *reinterpret_cast<char*>(uBaseAddress + uCurrentOffset);
		++uCurrentOffset;
		return dValue;
	}

	bool ReadBool()
	{
		bool bValue = *reinterpret_cast<bool*>(uBaseAddress + uCurrentOffset);
		++uCurrentOffset;
		return bValue;
	}

	const char* ReadString()
	{
		char szBuffer[256];
		char chLength = *reinterpret_cast<char*>(uBaseAddress + uCurrentOffset);
		++uCurrentOffset;
		memcpy(szBuffer, reinterpret_cast<void*>(uBaseAddress + uCurrentOffset), chLength > 255 ? 255 : chLength);
		szBuffer[chLength > 255 ? 255 : chLength] = '\0';
		uCurrentOffset += chLength + 1;
		return szBuffer;
	}
};
