/* roadmap_androidspeechtt.c - Android speech to text interface implementation
 *
 * LICENSE:
 *
  *   Copyright 2010 Alex Agranovich (AGA), Waze Ltd
 *
 *   This file is part of RoadMap.
 *
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
 *
 */

#include "roadmap_androidspeechtt.h"
#include "JNI/WazeSpeechttManager_JNI.h"
#include "JNI/FreeMapJNI.h"
#include <string.h>

static const char* get_lang_tag( void );

/***********************************************************/
/*  Name        : roadmap_androidspeechtt_launcher
 *  Purpose     : Prepare and call the speech to text engine on the Java layer
 *  Params      : result_cb - callback upon recognition process completion
 *              : timeout - the speech to text timeout
 *              : flags - special flags
 *  Returns     : void
 *              :
 */
static void roadmap_androidspeechtt_launcher( RMSpeechttResponseCb result_cb, RMSpeechttContext* context )
{
   AndrSpeechttCbContext* ctx = calloc( sizeof( AndrSpeechttCbContext ), 1 );
   ctx->callback = result_cb;
   ctx->context = context;
   // Meanwhile external always
   context->flags |= SPEECHTT_FLAG_EXTERNAL_RECOGNIZER;
   // Add "speak now" text
//   context->flags |= SPEECHTT_FLAG_EXTRA_PROMPT;
//   context->extra_prompt = roadmap_lang_get( "Speak now" );

   WazeSpeechttManager_StartNative( ctx, context->timeout, get_lang_tag(), context->extra_prompt, context->flags );

}


/***********************************************************/
/*  Name        : roadmap_androidspeechtt_close
 *  Purpose     : Prepare and call the speech to text engine stop routine on the Java layer
 *  Params      :
 *              :
 *              :
 *  Returns     : void
 *              :
 */
static void roadmap_androidspeechtt_close( void )
{
   WazeSpeechttManager_Stop();
}

/***********************************************************/
/*  Name        : roadmap_androidspeechtt_init
 *  Purpose     : Initialization routine. Registers the launcher and close callbacks
 *  Params      :
 *              :
 *  Returns     : void
 *              :
 */
void roadmap_androidspeechtt_init( void )
{
   if ( WazeSpeechttManager_IsAvailable() )
   {
      roadmap_speechtt_register_launcher( (RMSpeechttLauncherCb) roadmap_androidspeechtt_launcher );
      roadmap_speechtt_register_close( (RMSpeechttCloseCb) roadmap_androidspeechtt_close );
   }
   else
   {
      // AGA TODO: Reduce the log level
      roadmap_log( ROADMAP_WARNING, "Speech to text engine is not available!!!" );
   }
}

/***********************************************************/
/*  Name        : roadmap_get_lang_tag
 *  Purpose     : Returns the dispatched value of the system language according to the bcp 47 standard
 *  Params      : void
 *              :
 *              :
 *  Returns     : const char *
 *  Notes       : TODO:: Transfer this function to the roadmap_lang module
 */
static const char* get_lang_tag( void )
{
   const char *res = "en";
   if ( !strcmp( roadmap_lang_get_system_lang(), "eng" ) )
      res = "en";
   if ( !strcmp( roadmap_lang_get_system_lang(), "heb" ) )
      res = "he";
   if ( !strcmp( roadmap_lang_get_system_lang(), "esp" ) )
      res = "es";

   return res;
}


