/*
 *  GSPlayer - The audio player for WindowsCE
 *  Copyright (C) 2003  Y.Nagamidori
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef __LIBOVD_H__
#define __LIBOVD_H__

#define OVD_FATAL_ERR			-2
#define OVD_ERR					-1
#define OVD_OK					0
#define OVD_NEED_MORE_OUTPUT	1
#define OVD_EOF					2
#define OVD_PCMBUF_LEN			4096
#define OVD_COMMENT_LEN			64

typedef struct {
	int version;
	int channels;
	long rate;

	long bitrate_upper;
	long bitrate_nominal;
	long bitrate_lower;
	long bitrate_window;
} ovd_info;

typedef struct {
	TCHAR szDate[OVD_COMMENT_LEN];
	TCHAR szTrackNum[OVD_COMMENT_LEN];
	TCHAR szTitle[OVD_COMMENT_LEN];
	TCHAR szArtist[OVD_COMMENT_LEN];
	TCHAR szAlbum[OVD_COMMENT_LEN];
	TCHAR szGenre[OVD_COMMENT_LEN];
	TCHAR szComment[OVD_COMMENT_LEN];
} ovd_comment;


typedef struct {
	char* buf;
	unsigned long len;
	HANDLE handle;
} ovd_stream_buf;


#ifdef LIBOVD_EXPORTS
#define LIBOVD_EXPORT __declspec(dllexport)
#else
#define LIBOVD_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBOVD_EXPORT BOOL ovd_test_file(LPCTSTR pszFile);
LIBOVD_EXPORT HANDLE ovd_open_file(LPCTSTR pszFile);
LIBOVD_EXPORT void ovd_close(HANDLE hOv);
LIBOVD_EXPORT int ovd_read(HANDLE hov, LPBYTE pcmbuf, int pcmbuf_len, int* pcmout);
LIBOVD_EXPORT BOOL ovd_seek(HANDLE hov, LONGLONG samples);
LIBOVD_EXPORT LONGLONG ovd_get_current(HANDLE hov);
LIBOVD_EXPORT LONGLONG ovd_get_duration(HANDLE hov);
LIBOVD_EXPORT BOOL ovd_get_info(HANDLE hov, ovd_info* info);
LIBOVD_EXPORT BOOL ovd_get_comment(HANDLE hov, ovd_comment* comment);
LIBOVD_EXPORT BOOL ovd_get_comment_from_file(LPCTSTR pszFile, ovd_comment* comment);

#define OVD_STREAM_BUF_LEN	4096

LIBOVD_EXPORT ovd_stream_buf* ovd_init_stream();
LIBOVD_EXPORT BOOL ovd_parse_stream(ovd_stream_buf* buf);
LIBOVD_EXPORT BOOL ovd_get_stream_info(ovd_stream_buf* buf, ovd_info* info);
LIBOVD_EXPORT BOOL ovd_get_stream_comment(ovd_stream_buf* buf, ovd_comment* comment);
LIBOVD_EXPORT int ovd_decode_stream(ovd_stream_buf* buf, LPBYTE pcmbuf, int pcmbuf_len, int* pcmout);
LIBOVD_EXPORT void ovd_uninit_stream(ovd_stream_buf* buf);

#ifdef __cplusplus
}
#endif

#endif // __LIBOVD_H__