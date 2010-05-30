#include <windows.h>
#include <tchar.h>
#include "libovd.h"

#include ".\tremor\ivorbiscodec.h"
#include ".\tremor\ivorbisfile.h"
#include ".\tremor\codebook.h"
#include ".\tremor\misc.h"

typedef struct
{
	FILE*			fp;
	OggVorbis_File	vf;
}LIBOV_STRUCT;

extern "C" void *calloc2( size_t num, size_t size )
{
	void* p = malloc(num * size);
	if (!p) return p;
	memset(p, 0, num * size);
	return p;
}

int UTF8toUCS2(char* pszSrc, WCHAR* pszDst, DWORD dwDstLen)
{
	DWORD dwLen = 0;
	while (*pszSrc) {
		if (++dwLen == dwDstLen)
			break;
		if ((*pszSrc & 0x80) == 0x0) {
			// 1 byte
			*pszDst++ = *pszSrc++;
		}
		else if ((*pszSrc & 0xE0) == 0xC0) {
			// 2 bytes
			*pszDst++ = (((WORD)*pszSrc & 0x1F) << 6) | ((WORD)*(pszSrc + 1) & 0x3F);
			pszSrc += 2;
		}
		else if ((*pszSrc & 0xE0) == 0xE0) {
			// 3 bytes
			*pszDst++ = (((WORD)*pszSrc & 0x0F) << 12) | (((WORD)*(pszSrc + 1) & 0x3F) << 6) | ((WORD)*(pszSrc + 2) & 0x3F);
			pszSrc += 3;
		}
	}

	*pszDst = NULL;
	return dwLen;
}

BOOL ovd_test_file(LPCTSTR pszFile)
{
	if (!_tcslen(pszFile))
		return FALSE;

	FILE* fp = _tfopen(pszFile, _T("rb"));
	if (!fp)
		return NULL;

	OggVorbis_File vf;
	int ret = ov_test(fp, &vf, NULL, 0);
	fclose(fp);
	ov_clear(&vf);

	return ret == 0;
}

HANDLE ovd_open_file(LPCTSTR pszFile)
{
	if (!_tcslen(pszFile))
		return NULL;

	FILE* fp = _tfopen(pszFile, _T("rb"));
	if (!fp)
		return NULL;

	LIBOV_STRUCT* lov = new LIBOV_STRUCT;
	if (ov_open(fp, &lov->vf, NULL, 0) < 0)
	{
		delete lov;
		fclose(fp);
		return NULL;
	}

	lov->fp = fp;
	return (HANDLE)lov;
}

void ovd_close(HANDLE hov)
{
	LIBOV_STRUCT* lov = (LIBOV_STRUCT*)hov;
	if (!lov) return;

	ov_clear(&lov->vf);
	if (lov->fp) 
		fclose(lov->fp);
	delete lov;
}

int ovd_read(HANDLE hov, LPBYTE pcmbuf, int pcmbuf_len, int* pcmout)
{
	*pcmout = 0;
	LIBOV_STRUCT* lov = (LIBOV_STRUCT*)hov;
	if (!lov) return OVD_FATAL_ERR;

	if (pcmbuf_len < OVD_PCMBUF_LEN)
		return OVD_NEED_MORE_OUTPUT;

	int ret = ov_read(&lov->vf, (char*)pcmbuf, pcmbuf_len, NULL);
	if (ret == 0)		// eof
		return OVD_EOF;
	else if (ret < 0)	// error
		return OVD_FATAL_ERR;

	// succeeded
	*pcmout = ret;
	return OVD_OK;
}

BOOL ovd_seek(HANDLE hov, LONGLONG samples)
{
	LIBOV_STRUCT* lov = (LIBOV_STRUCT*)hov;
	if (!lov) return FALSE;

	return ov_pcm_seek_page(&lov->vf, samples) == 0;
}

LONGLONG ovd_get_current(HANDLE hov)
{
	LIBOV_STRUCT* lov = (LIBOV_STRUCT*)hov;
	if (!lov) return FALSE;

	return ov_pcm_tell(&lov->vf);
}

LONGLONG ovd_get_duration(HANDLE hov)
{
	LIBOV_STRUCT* lov = (LIBOV_STRUCT*)hov;
	if (!lov) return FALSE;

	return ov_pcm_total(&lov->vf, -1);
}

BOOL ovd_get_info(HANDLE hov, ovd_info* info)
{
	LIBOV_STRUCT* lov = (LIBOV_STRUCT*)hov;
	if (!lov) return FALSE;

	vorbis_info* pvi = ov_info(&lov->vf, -1);
	if (!pvi)
		return FALSE;

	info->version = pvi->version;
	info->rate = pvi->rate;
	info->channels = pvi->channels;
	info->bitrate_lower = pvi->bitrate_lower;
	info->bitrate_nominal = pvi->bitrate_nominal;
	info->bitrate_upper = pvi->bitrate_upper;
	info->bitrate_window = pvi->bitrate_window;
	return TRUE;
}

void ovd_strupr(char* str)
{
	for (int i = 0; str[i] != NULL; i++) {
		if (str[i] >= 'a' && str[i] <= 'z')
			str[i] -= 'a' - 'A';
	}
}

#define OVD_COMMENT_TITLE		"TITLE"
#define OVD_COMMENT_ARTIST		"ARTIST"
#define OVD_COMMENT_ALBUM		"ALBUM"
#define OVD_COMMENT_GENRE		"GENRE"
#define OVD_COMMENT_COMMENT		"COMMENT"
#define OVD_COMMENT_DATE		"DATE"
#define OVD_COMMENT_TRACKNUM	"TRACKNUMBER"
BOOL ovd_get_comment(HANDLE hov, ovd_comment* comment)
{
	LIBOV_STRUCT* lov = (LIBOV_STRUCT*)hov;
	if (!lov) return FALSE;

	memset(comment, 0, sizeof(ovd_comment));
	vorbis_comment* p = ov_comment(&lov->vf, -1);
	for (int i = 0; i < p->comments; i++)
	{
		LPTSTR psz = NULL;
		char* s = strchr(p->user_comments[i], '=');
		if (s)
		{
			*s = NULL;
			ovd_strupr(p->user_comments[i]);
			if (strcmp(p->user_comments[i], OVD_COMMENT_DATE) == 0)
				psz = comment->szDate;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_TRACKNUM) == 0)
				psz = comment->szTrackNum;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_TITLE) == 0)
				psz = comment->szTitle;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_ARTIST) == 0)
				psz = comment->szArtist;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_ALBUM) == 0)
				psz = comment->szAlbum;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_GENRE) == 0)
				psz = comment->szGenre;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_COMMENT) == 0)
				psz = comment->szComment;

			if (psz)
			{
#ifdef _UNICODE
				UTF8toUCS2(s + 1, psz, OVD_COMMENT_LEN);
#else
				WCHAR sz[OVD_COMMENT_LEN];
				UTF8toUCS2(s + 1, sz, OVD_COMMENT_LEN);
				WideCharToMultiByte(CP_ACP, 0, sz, -1, psz, OVD_COMMENT_LEN, 0, 0);
#endif	
			}
		}
	}
	return TRUE;
}

BOOL ovd_get_comment_from_file(LPCTSTR pszFile, ovd_comment* comment)
{
	if (!ovd_test_file(pszFile))
		return FALSE;

	FILE* fp = _tfopen(pszFile, _T("rb"));
	if (!fp)
		return FALSE;

	OggVorbis_File	vf;
	if (ov_open(fp, &vf, NULL, 0) < 0)
	{
		fclose(fp);
		return FALSE;
	}

	memset(comment, 0, sizeof(ovd_comment));
	vorbis_comment* p = ov_comment(&vf, -1);
	for (int i = 0; i < p->comments; i++)
	{
		LPTSTR psz = NULL;
		char* s = strchr(p->user_comments[i], '=');
		if (s)
		{
			*s = NULL;
			ovd_strupr(p->user_comments[i]);
			if (strcmp(p->user_comments[i], OVD_COMMENT_DATE) == 0)
				psz = comment->szDate;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_TRACKNUM) == 0)
				psz = comment->szTrackNum;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_TITLE) == 0)
				psz = comment->szTitle;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_ARTIST) == 0)
				psz = comment->szArtist;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_ALBUM) == 0)
				psz = comment->szAlbum;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_GENRE) == 0)
				psz = comment->szGenre;
			else if (strcmp(p->user_comments[i], OVD_COMMENT_COMMENT) == 0)
				psz = comment->szComment;

			if (psz)
			{
#ifdef _UNICODE
				UTF8toUCS2(s + 1, psz, OVD_COMMENT_LEN);
#else
				WCHAR sz[OVD_COMMENT_LEN];
				UTF8toUCS2(s + 1, sz, OVD_COMMENT_LEN);
				WideCharToMultiByte(CP_ACP, 0, sz, -1, psz, OVD_COMMENT_LEN, 0, 0);
#endif	
			}
		}
	}
	fclose(fp);
	return TRUE;
}

// for streaming
typedef struct {
	int init;
	ogg_sync_state oy;
	ogg_stream_state os;
	ogg_page og;
	ogg_packet op;
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;

	int samples;
	int eof;
	ogg_int32_t **pcm;
} ovd_handle;

ovd_stream_buf* ovd_init_stream()
{
	ovd_handle* handle = new ovd_handle;
	memset(handle, 0, sizeof(ovd_handle));

	ogg_sync_init(&handle->oy);

	ovd_stream_buf* ret = new ovd_stream_buf;
	ret->handle = (HANDLE)handle;
	ret->buf = ogg_sync_buffer(&handle->oy, OVD_STREAM_BUF_LEN);
	ret->len = OVD_STREAM_BUF_LEN;

	return ret;
}

int ovd_header_init(ovd_handle* handle)
{
	if (handle->init < 2) {
		if (handle->init < 0)
			return -1;

		while (handle->init < 2) {
			int result = ogg_sync_pageout(&handle->oy, &handle->og);
			if (result == 0) 
				return FALSE; // need more data
			if (result == 1) {
				ogg_stream_pagein(&handle->os, &handle->og);
				while(handle->init < 2){
					result = ogg_stream_packetout(&handle->os, &handle->op);
					if (result == 0)
						return 0; // need more data
					if (result < 0) {
						handle->init = -1; // fatal error
						return -1;
					}

					vorbis_synthesis_headerin(&handle->vi, &handle->vc, &handle->op);
					handle->init++;
				}
			}
			else if (result = -1) return -1;
		}
		
		vorbis_synthesis_init(&handle->vd, &handle->vi);
		vorbis_block_init(&handle->vd, &handle->vb);
		return 1;
	}
	return 1;
}

BOOL ovd_parse_stream(ovd_stream_buf* buf)
{
	ovd_handle* handle = (ovd_handle*)buf->handle;

	ogg_sync_wrote(&handle->oy, buf->len);
	if (ogg_sync_pageout(&handle->oy, &handle->og) != 1)
		goto fail;

	ogg_stream_init(&handle->os, ogg_page_serialno(&handle->og));
	vorbis_info_init(&handle->vi);
    vorbis_comment_init(&handle->vc);
	if (ogg_stream_pagein(&handle->os, &handle->og) < 0)
		goto fail;

	if (ogg_stream_packetout(&handle->os,&handle->op) != 1)
		goto fail;

	if (vorbis_synthesis_headerin(&handle->vi, &handle->vc, &handle->op) < 0)
		goto fail;

	handle->init = 0;
	handle->eof = 0;
	ovd_header_init(handle);

	buf->buf = ogg_sync_buffer(&handle->oy, OVD_STREAM_BUF_LEN);
	buf->len = OVD_STREAM_BUF_LEN;
	return TRUE;
fail:
	ogg_sync_init(&handle->oy);
	buf->buf = ogg_sync_buffer(&handle->oy, OVD_STREAM_BUF_LEN);
	buf->len = OVD_STREAM_BUF_LEN;
	return FALSE;
}

BOOL ovd_get_stream_info(ovd_stream_buf* buf, ovd_info* info)
{
	ovd_handle* handle = (ovd_handle*)buf->handle;

	info->version = handle->vi.version;
	info->rate = handle->vi.rate;
	info->channels = handle->vi.channels;
	info->bitrate_lower = handle->vi.bitrate_lower;
	info->bitrate_nominal = handle->vi.bitrate_nominal;
	info->bitrate_upper = handle->vi.bitrate_upper;
	info->bitrate_window = handle->vi.bitrate_window;

	return TRUE;
}

BOOL ovd_get_stream_comment(ovd_stream_buf* buf, ovd_comment* comment)
{
	ovd_handle* handle = (ovd_handle*)buf->handle;
	
	memset(comment, 0, sizeof(ovd_comment));
	for (int i = 0; i < handle->vc.comments; i++)
	{
		LPTSTR psz = NULL;
		char* s = strchr(handle->vc.user_comments[i], '=');
		if (s)
		{
			*s = NULL;
			ovd_strupr(handle->vc.user_comments[i]);
			if (strcmp(handle->vc.user_comments[i], OVD_COMMENT_DATE) == 0)
				psz = comment->szDate;
			else if (strcmp(handle->vc.user_comments[i], OVD_COMMENT_TRACKNUM) == 0)
				psz = comment->szTrackNum;
			else if (strcmp(handle->vc.user_comments[i], OVD_COMMENT_TITLE) == 0)
				psz = comment->szTitle;
			else if (strcmp(handle->vc.user_comments[i], OVD_COMMENT_ARTIST) == 0)
				psz = comment->szArtist;
			else if (strcmp(handle->vc.user_comments[i], OVD_COMMENT_ALBUM) == 0)
				psz = comment->szAlbum;
			else if (strcmp(handle->vc.user_comments[i], OVD_COMMENT_GENRE) == 0)
				psz = comment->szGenre;
			else if (strcmp(handle->vc.user_comments[i], OVD_COMMENT_COMMENT) == 0)
				psz = comment->szComment;

			if (psz)
			{
#ifdef _UNICODE
				UTF8toUCS2(s + 1, psz, OVD_COMMENT_LEN);
#else
				WCHAR sz[OVD_COMMENT_LEN];
				UTF8toUCS2(s + 1, sz, OVD_COMMENT_LEN);
				WideCharToMultiByte(CP_ACP, 0, sz, -1, psz, OVD_COMMENT_LEN, 0, 0);
#endif	
			}
		}
	}
	return TRUE;
}

BOOL ovd_reparse_stream(ovd_stream_buf* buf)
{
	ovd_handle* handle = (ovd_handle*)buf->handle;

	if (buf->len) {
		ogg_sync_wrote(&handle->oy, buf->len);
		int result = ogg_sync_pageout(&handle->oy, &handle->og);
		if (result == 0) {
			buf->len = OVD_STREAM_BUF_LEN;
			return TRUE;
		}
		else if (result < 0)
			return FALSE;
	}

	ogg_stream_init(&handle->os, ogg_page_serialno(&handle->og));
	vorbis_info_init(&handle->vi);
    vorbis_comment_init(&handle->vc);
	if (ogg_stream_pagein(&handle->os, &handle->og) < 0)
		return FALSE;

	if (ogg_stream_packetout(&handle->os,&handle->op) != 1)
		return FALSE;

	if (vorbis_synthesis_headerin(&handle->vi, &handle->vc, &handle->op) < 0)
		return FALSE;

	handle->init = 0;
	handle->eof = 0;
	ovd_header_init(handle);

	buf->buf = ogg_sync_buffer(&handle->oy, OVD_STREAM_BUF_LEN);
	buf->len = OVD_STREAM_BUF_LEN;
	return TRUE;
}

int ovd_decode_stream(ovd_stream_buf* buf, LPBYTE pcmbuf, int pcmbuf_len, int* pcmout)
{
	int result, samples, ret = OVD_OK;
	ovd_handle* handle = (ovd_handle*)buf->handle;
	*pcmout = 0;

	if (handle->samples) {
		if (pcmbuf_len < handle->samples * handle->vi.channels * 2) {
			buf->len = 0;
			return OVD_NEED_MORE_OUTPUT;
		}

		for (int i = 0; i < handle->vi.channels; i++) {
			ogg_int32_t* src = handle->pcm[i];
			short* dest = ((short *)(pcmbuf + *pcmout)) + i;
			for (int j = 0; j < handle->samples; j++) {
				*dest = CLIP_TO_15(src[j] >> 9);
				dest += handle->vi.channels;
			}
		}

		vorbis_synthesis_read(&handle->vd, handle->samples);
		*pcmout += handle->samples * handle->vi.channels * 2;
		pcmbuf_len -= handle->samples * handle->vi.channels * 2;
		handle->samples = 0;
	}

	if (handle->eof) {
		if (!ovd_reparse_stream(buf))
			return OVD_FATAL_ERR;
		goto done;
	}

	if (buf->len) {
		ogg_sync_wrote(&handle->oy, buf->len);
		buf->len = OVD_STREAM_BUF_LEN;

		result = ovd_header_init(handle);
		if (result != 1) {
			if (result < 0)
				ret = OVD_FATAL_ERR;
			goto done;
		}

		result = ogg_sync_pageout(&handle->oy, &handle->og);
		if (result == 0)
			goto done; // need more
		if (result < 0) {
			ret = OVD_FATAL_ERR;
			handle->init = -1;
			goto done;
		}

		ogg_stream_pagein(&handle->os, &handle->og);

		while (1) {
			result = ogg_stream_packetout(&handle->os, &handle->op);

			if (result == 0)
				break;
			if (result > 0) {
				if (vorbis_synthesis(&handle->vb, &handle->op) == 0)
					vorbis_synthesis_blockin(&handle->vd, &handle->vb);

				vorbis_synthesis_headerin(&handle->vi, &handle->vc, &handle->op);

				while ((samples = vorbis_synthesis_pcmout(&handle->vd, &handle->pcm)) > 0) {
					if (pcmbuf_len < samples * handle->vi.channels * 2) {
						handle->samples = samples;
						buf->len = 0;
						return OVD_NEED_MORE_OUTPUT;
					}

					for (int i = 0; i < handle->vi.channels; i++) {
						ogg_int32_t* src = handle->pcm[i];
						short* dest = ((short *)(pcmbuf + *pcmout)) + i;
						for (int j = 0; j < samples; j++) {
							*dest = CLIP_TO_15(src[j] >> 9);
							dest += handle->vi.channels;
						}
					}

					vorbis_synthesis_read(&handle->vd, samples);
					*pcmout += samples * handle->vi.channels * 2;
					pcmbuf_len -= samples * handle->vi.channels * 2;
					handle->samples = 0;
				}
			}
		}
	}
	else {
		result = 1;
		buf->len = OVD_STREAM_BUF_LEN;
		while (1) {
			if (result > 0) {
				while ((samples = vorbis_synthesis_pcmout(&handle->vd, &handle->pcm)) > 0) {
					if (pcmbuf_len < samples * handle->vi.channels * 2) {
						handle->samples = samples;
						buf->len = 0;
						return OVD_NEED_MORE_OUTPUT;
					}

					for (int i = 0; i < handle->vi.channels; i++) {
						ogg_int32_t* src = handle->pcm[i];
						short* dest = ((short *)(pcmbuf + *pcmout)) + i;
						for (int j = 0; j < samples; j++) {
							*dest = CLIP_TO_15(src[j] >> 9);
							dest += handle->vi.channels;
						}
					}

					vorbis_synthesis_read(&handle->vd, samples);
					*pcmout += samples * handle->vi.channels * 2;
					pcmbuf_len -= samples * handle->vi.channels * 2;
					handle->samples = 0;
				}
			}

			result = ogg_stream_packetout(&handle->os, &handle->op);
			if (result == 0)
				break;
			if (result > 0) {
				if (vorbis_synthesis(&handle->vb, &handle->op) == 0)
					vorbis_synthesis_blockin(&handle->vd, &handle->vb);

				vorbis_synthesis_headerin(&handle->vi, &handle->vc, &handle->op);
			}
		}
	}

	if (ogg_page_eos(&handle->og)) {
		handle->samples = 0;
		handle->init = 0;
		ogg_stream_clear(&handle->os);
		vorbis_block_clear(&handle->vb);
		vorbis_dsp_clear(&handle->vd);
		vorbis_comment_clear(&handle->vc);
		vorbis_info_clear(&handle->vi);  /* must be called last */

		handle->eof = 1;
		buf->len = OVD_STREAM_BUF_LEN;
	}

done:
	if (buf->len) buf->buf = ogg_sync_buffer(&handle->oy, OVD_STREAM_BUF_LEN);
	return ret;
}

void ovd_uninit_stream(ovd_stream_buf* buf)
{
	ovd_handle* handle = (ovd_handle*)buf->handle;	

	vorbis_block_clear(&handle->vb);
    vorbis_dsp_clear(&handle->vd);
	vorbis_comment_clear(&handle->vc);
    vorbis_info_clear(&handle->vi);
	ogg_sync_clear(&handle->oy);
	delete handle;
	delete buf;
}
