#ifndef __MAHELPER_H__
#define __MAHELPER_H__

extern const int tabbitrate[3][3][16];
extern const int tabsamplingrate[3][4];
extern const LPTSTR genre_strings[];

typedef struct
{
	int			nVersion;
	int			nLayer;
	int			nChannels;
	int			nSamplingRate;
	int			nBitRate;
	int			nFrameSize;
	int			nSamplesPerFrame;
}MPEG_AUDIO_INFO;

#pragma pack(1)
typedef struct tID3Tagv1 {
	BYTE	tag[3];
	BYTE	trackName[30];
	BYTE	artistName[30];
	BYTE	albumName[30];
	BYTE	year[4];
	BYTE	comment[30];
	BYTE	genre;
}ID3_TAG_V1;
#pragma pack()

inline BOOL CheckAudioHeader(DWORD dwHeader)
{
	if ( (dwHeader & 0xffe00000) != 0xffe00000) // sync word
		return FALSE;
	if (!((dwHeader >> 17) & 3)) // layer
		return FALSE;
	if ( ((dwHeader >> 12) & 0xf) == 0xf) // bitrate
		return FALSE;
	if ( ((dwHeader >> 10) & 0x3) == 0x3) // samplingrate
		return FALSE;

#ifdef _WIN32_WCE
	// free format check 
	if ( ((dwHeader >> 12) & 0xf) == 0x0)
		return FALSE;
#endif

	return TRUE;
}

inline BOOL CheckAudioHeader(LPBYTE pbBuff /* need 4bytes */) 
{
	if ( (pbBuff[0] & 0xFF) != 0xFF || (pbBuff[1] & 0xE0) != 0xE0) // sync word
		return FALSE;
	if (!((pbBuff[1])&0x6)) // layer
		return FALSE;
	if ( ((pbBuff[2])&0xF0) == 0xF0) // bitrate
		return FALSE;
	if ( ((pbBuff[2])&0xC) == 0xC) // samplingrate
		return FALSE;

#ifdef _WIN32_WCE
	// free format check 
	if ( ((pbBuff[2])&0xF0) == 0x0)
		return FALSE;
#endif


	return TRUE;
}

BOOL IsValidFile(LPCTSTR pszFile);
BOOL ParseHeader(DWORD dwHeader, MPEG_AUDIO_INFO* pinfo);
BOOL ParseHeader(LPBYTE pbHeader, MPEG_AUDIO_INFO* pinfo);
int GetFrameSize(DWORD dwHeader);
int GetFrameSize(LPBYTE pbHeader);
BOOL GetId3Tag(LPCTSTR pszFile, ID3TAGV1* pTag);
BOOL SetId3Tag(LPCTSTR pszFile, ID3TAGV1* pTag);
void ConvertFromTagStr(BYTE buff[30], LPTSTR pszBuff, int nLen = 30);
BOOL ParseId3TagV2(LPBYTE buf, int buflen, ID3TAGV1* pTag);

#define ID3TAG_HEADER_LEN	10
#define ID3TAG23_FRAME_LEN	10
#define ID3TAG20_FRAME_LEN	6


#endif // __MAHELPER_H__