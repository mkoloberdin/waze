/* tts_defs.h - The tts common definitions
 *
 *
 *
 * LICENSE:
 *
 *   Copyright 2011, Waze Ltd
 *                   Alex Agranovich   (AGA)
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef INCLUDE__TTS_DEFS__H
#define INCLUDE__TTS_DEFS__H
#ifdef __cplusplus
extern "C" {
#endif

//#define TTS_DEBUG

#ifndef MIN
#define MIN(a, b) (a) < (b) ? (a) : (b)
#endif

/*
 * ********************************** TTS common *******************************************
 */
#define TTS_LOG_STR( str ) ( "TTS Engine. " str )
#define TTS_DATA_INITIALIZER           { NULL, 0 }
#define TTS_PATH_INITIALIZER           { {0} }

/*
 * AGA TODO::
 * Distinguish between provider flags and client flags.
 * Client flags should be used inside the tts engine. Provider flags are sent to the provider through sync request
 */
#define TTS_FLAG_NONE                               0x00000000
#define TTS_FLAG_RETRY                              0x00000001             // Indicates if apply retry
#define TTS_FLAG_RETRY_CALLBACK                     0x00000002             // Indicates if callbacks should be called upon retry requests

// TODO:: Separate statuses for provider and user.
#define TTS_RES_STATUS_ERROR                        0x00000001
#define TTS_RES_STATUS_PARTIAL_SUCCESS              0x00000002
#define TTS_RES_STATUS_SUCCESS                      0x00000004
#define TTS_RES_STATUS_NO_RESULTS                   0x00000008
#define TTS_RES_STATUS_NO_INPUT_STRINGS             0x00000010
#define TTS_RES_STATUS_NULL_TEXT                    0x00000020
#define TTS_RES_STATUS_RETRY_ON                     0x00000040            // Indicates that the text will be requested again
#define TTS_RES_STATUS_RETRY_EXHAUSTED              0x00000080            // Indicates that all the retries exhausted and text will not be requested again

// TTS text types
#define TTS_TEXT_TYPE_MAXLEN                        64
#define TTS_TEXT_TYPE_STR_DEFAULT                   "default"
#define TTS_TEXT_TYPE_STR_STREET                    "street"

#define TTS_GENERIC_LIST_INITIALIZER                {NULL}
#define TTS_INTEGER_LIST_INITIALIZER                {0}

/*
 * ********************************** TTS section *******************************************
 */
#define TTS_CFG_CATEGORY                     ("TTS")
#define TTS_CFG_FEATURE_ENABLED              ("Feature Enabled")
#define TTS_CFG_FEATURE_ENABLED_DEFAULT      ("yes")
#define TTS_CFG_CACHE_ENABLED                ("Cache Enabled")
#define TTS_CFG_CACHE_ENABLED_DEFAULT        ("yes")
#define TTS_CFG_DB_TYPE                      ("Database Type")
#define TTS_CFG_DB_TYPE_DEFAULT              (__tts_db_data_storage__file)
#define TTS_CFG_DB_VERSION                   ("DB Version")
#define TTS_CFG_VOICE_ID                     ("Voice ID")
#define TTS_CFG_DEFAULT_VOICE_ID             ("Default Voice ID")
#define TTS_CFG_VOICE_ID_NOT_DEFINED         ("NOT DEFINED")
#define TTS_CFG_VOICE_ID_DEFAULT             ("local_nuance_8_mp3_Samantha_Female")

#define TTS_TEXT_MAX_LENGTH                       (4096)  // Maximum length of TTS text buffer
#define TTS_RESULT_FILE_NAME_MAXLEN               (TTS_PATH_MAXLEN)
#define TTS_PROVIDERS_MAX_NUM                     (16)
#define TTS_BATCH_REQUESTS_LIMIT                  (16)    // Maximum number of tts request per one commit
#define TTS_ACTIVE_REQUESTS_LIMIT                 (128)    // Maximum number of active tts requests

/*
 * ********************************** TTS Voices section *******************************************
 */

#define TTS_PATH_MAXLEN                            512

#define TTS_VOICE_MAXLEN                          (128)

#define TTS_RESULT_FILE_NAME_EXT                  "tts"

/*
 * ********************************** TTS COMMON TYPES *******************************************
 */

typedef enum
{
   __tts_text_type_default = 0x0,
   __tts_text_type_street
} TtsTextType;

/*
 *
 */
typedef struct
{
   char path[TTS_PATH_MAXLEN];
} TtsPath;

typedef struct {
   void* data;
   size_t data_size;
} TtsData;

typedef const TtsPath* TtsPathList[TTS_BATCH_REQUESTS_LIMIT];

typedef const char* TtsTextList[TTS_BATCH_REQUESTS_LIMIT];

typedef TtsTextType TtsTextTypeList[TTS_BATCH_REQUESTS_LIMIT];

typedef enum
{
   __tts_db_data_storage__none = 0x0,
   __tts_db_data_storage__blob_record = 0x1,
   __tts_db_data_storage__file = 0x2,
   __tts_db_data_storage__both = 0x3
} TtsDbDataStorageType;

typedef struct _TtsProvider TtsProvider;

#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_VOICES__H
