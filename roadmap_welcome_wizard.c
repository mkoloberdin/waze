/* roadmap_welcome_wizard.c - First time settup wizard
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
  *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_bar.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "roadmap_start.h"
#include "roadmap_login.h"
#include "roadmap_welcome_wizard.h"
#include "roadmap_twitter.h"
#include "roadmap_history.h"
#include "roadmap_screen.h"
#include "ssd/ssd_dialog.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_bitmap.h"
#include "roadmap_mood.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_checkbox.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "Realtime/RealtimeAlerts.h"
#include "roadmap_native_keyboard.h"

#ifdef   TOUCH_SCREEN
   #include "ssd/ssd_keyboard.h"
#endif //TOUCH_SCREEN

const char* Hebrew_Disclaimer1 =
         "ברוכים הבאים ל Waze!\n"
         "מידע חי על מצב התנועה ונווט עוקף פקקים, אשר מיוצר בשיתוף ותרומה של חברי הקהילה\n";
const char* Hebrew_Disclaimer2 =
		 "בלחיצה על 'אשר' הנך מאשר כי:\n" 
         "1. קראת והנך מסכים לתנאי השימוש ומדיניות הפרטיות. השימוש בשירות Waze כפוף להסכמים המחייבים ומעיד על הסכמתך להם. תקציר זה אינו בא במקומם, והוא מיועד לצרכי נוחות בלבד.\n" \
         "2. השימוש בשירות הינו באחריותך בלבד. הינך מאשר כי תשתמש בשירות בהתאם להוראות כל דין, לרבות דיני התעבורה.";
         
  
const char* Hebrew_Disclaimer3 =
         "3. שירות Waze מחייב חיבור לרשת האינטרנט לצורך השימוש בו, וקבלת עדכונים בזמן אמת. חברת Waze אינה מספקת קישור כזה. באחריותך לרכוש אותו ולוודא שתנאיו מתאימים לצרכיך. חברת Waze אינה נושאת בכל אחריות לקישור ו/או לכל שיבוש, תקלה או קלקול בו. \n" \
         "4. תוכנת המפוי Waze המותקנת על-גבי מכשיר הטלפון הסלולרי שלך, היא תוכנה חופשית; "  \
         "אתה יכול להפיצה מחדש ו/או לשנות את התוכנה על פי תנאי הרישיון הציבורי הכללי של GNU, אם גרסה 2 של הרישיון, "\
         "ובין אם (לפי בחירתך) כל גרסה מאוחרת שלו. התוכנה מופצת בתקווה שתהיה מועילה, אבל בלא אחריות כלשהי. לפרטים נוספים, ראה את הרישיון הציבורי הכללי של GNU. אם רצונך לקבל את קוד המקור של התכנה, פנה אלינו בכתב. ";

const char* Hebrew_Disclaimer4 =
	     "5. כשאתה מספק מידע ותוכן לשירות, אתה מאשר שאתה בעלים בלעדי של כל הזכויות בו ורשאי להקנות בו זכויות. מידע ותוכן כזה נשאר בבעלותך ואתה מקנה בו לחברת Waze רישיון חינם כלל עולמי, לא בלעדי, בלתי חוזר ובלתי מוגבל בזמן, שניתן להעברה וכולל זכות למתן רשיונות-משנה, להשתמש, להעתיק, להפיץ, ליצור יצירות נגזרות, להציג ולבצע בפומבי מידע ותוכן כזה. בכפוף לכך, בסיס הנתונים של השירות הוא קניינה של החברה ואין להשתמש בו אלא לצרכים לא מסחריים ופרטיים בלבד.";
	
const char* Hebrew_Disclaimer5 = 
		 "6. שירות Waze מוצע בחינם, בתקווה שתמצא אותו מועיל. עם זאת, Waze ו/או עובדיה, מנהליה, בעלי מניותיה, יועציה ו/או מי מטעמה, לא ישאו בחבות כלשהיא כלפיך ו/או כלפי צדדים שלישים, מכל סיבה שהיא, בקשר עם מוצריה Waze ו/או שירותיה, לרבות (אך לא רק) בגין כל הפסד, אבדן רווח, פגיעה במוניטין, תשלום, הוצאה ו/או נזק, ישיר או עקיף, ממוני או בלתי-ממוני.";











const char* English_Disclaimer1 =
   "Welcome to Waze!\n\n"
   "You're about to join the first network of drivers working together to build and share real-time road intelligence.\n\n"
   "Since Waze is 100% user generated, we need your collaboration and patience!";

const char* English_Disclaimer2 =
      "By clicking 'Accept' you confirm that:\n\n" \
      "1. You have read and agreed to the 'Terms of Use' http://www.waze.com/legal/tos/ and 'Privacy Policy' http://www.waze.com/legal/privacy/ ('Binding Agreements'). The use of Waze service (the 'Service') is subject to the Binding Agreements and indicates your consent to them. This summary is not meant to replace the Binding Agreements. It is intended for convenience purposes only.";
const char* English_Disclaimer3 =
      "2. Use of Service is made at your sole risk.  You hereby agree to use the Service only in accordance with any applicable law, including but not limited to, all transportation laws and regulations.";
const char* English_Disclaimer4 =      
      "3. The Service requires Internet connection for real time updates. Waze does not provide such connection. It is your responsibility to purchase connection services and to make sure its terms matches your needs. Waze has no control over or responsibility to the connection and/or any disruption, failure or breakdown therein.\n\n" \
      "4. The Waze client, that you install on your cellular device, is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY. See the GNU General Public License http://www.gnu.org/licenses/gpl-2.0.html for more details. If you wish to obtain the program's source code, please write to us.";
      
const char* English_Disclaimer5 =
	  "5. When you provide data and or content to the Service, you hereby confirm that you own all exclusive rights at it and may assign or license such rights. You keep all title and rights to such data and/or content you provide to the Service, but you grant Waze Inc. and/or Waze Mobile (the Company or 'Waze') a worldwide, free, non-exclusive, irrevocable, sublicensable, transferable and perpetual license to use, copy, distribute, create derivative works of, publicly display, publicly perform and exploit in any other manner such data and content. Subject to the aforementioned, the Company keeps title and all rights to the Service's database and you may use it for non-commercial and private purposes only.\n\n" \
	  "6. Waze service is offered for free, with hope that you find it useful. However, Waze and/or its employees, directors shareholders, advisors and/or on anyone of it's behalf (collectively: 'Waze') shall not be liable to you and/or to any third party, for any reason whatsoever, as result with the use of the Company's product and/or Service. You hereby irrevocably release Waze from any liability of any kind, for any consequence arising from use of the Company's products and/or Service, Including (but not only) for any loss, loss of profit, damage to reputation, fee, expense and/or damage, direct or indirect, financial or non-financial.";

	  
     
const char * EnglishIntro1 = 
	"Waze is a social mobile navigation application that allows users to build and use live maps, real-time traffic updates and turn-by-turn navigation to improve daily commuting."; 
	

const char * EnglishIntro2 = 
"The mapping in your country has only recently begun so it'll be a little while until the app delivers its full value.\n"	
"In many areas  there will be no map at all and you're likely to be paving roads as you start out. ";

const char * EnglishIntro3 =  "If you like mapping, now's a great time to get involved and map your area - you'll have a ton of fun!\n";

	
extern RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var;
////   First time
static RoadMapConfigDescriptor WELCOME_WIZ_FIRST_TIME_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELCOME_WIZ_FIRST_TIME_Name);

static RoadMapConfigDescriptor WELCOME_WIZ_ENABLED_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELOCME_WIZ_ENABLED_Name);

static RoadMapConfigDescriptor WELCOME_WIZ_TERMS_OF_USE_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELCOME_WIZ_TERMS_OF_USE_Name);

static RoadMapConfigDescriptor WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Name);
                                    
static RoadMapConfigDescriptor WELCOME_WIZ_SHOW_INTRO_SCREEN_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELCOME_WIZ_SHOW_INTRO_SCREEN_Name);                              

static SsdWidget gAddressResultList = NULL;
static RoadMapCallback gCallback;
static RoadMapCallback gLoginCallBack = NULL;

static RMNativeKBParams s_gNativeKBParams = {  _native_kb_type_default, 1, _native_kb_action_done };

/////////////////////////////////////////////////////////////////////
static BOOL is_first_time(void){
   roadmap_config_declare_enumeration( WELCOME_WIZ_CONFIG_TYPE,
                                       &WELCOME_WIZ_FIRST_TIME_Var,
                                       NULL,
                                       WELCOME_WIZ_FIRST_TIME_Yes,
                                       WELCOME_WIZ_FIRST_TIME_No,
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_FIRST_TIME_Var), WELCOME_WIZ_FIRST_TIME_Yes))
      return TRUE;
   return FALSE;
}

static BOOL is_terms_accepted(void){
   roadmap_config_declare_enumeration( WELCOME_WIZ_CONFIG_TYPE,
                                       &WELCOME_WIZ_TERMS_OF_USE_Var,
                                       NULL,
                                       WELCOME_WIZ_FIRST_TIME_No,
                                       WELCOME_WIZ_FIRST_TIME_Yes,
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_TERMS_OF_USE_Var), WELCOME_WIZ_FIRST_TIME_Yes))
      return TRUE;
   return FALSE;
}

static BOOL is_show_intro_screen(void){
   roadmap_config_declare_enumeration( WELCOME_WIZ_SHOW_INTRO_CONFIG_TYPE,
                                       &WELCOME_WIZ_SHOW_INTRO_SCREEN_Var,
                                       NULL,
                                       WELCOME_WIZ_SHOW_INTRO_SCREEN_No,
                                       WELCOME_WIZ_SHOW_INTRO_SCREEN_Yes,
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_SHOW_INTRO_SCREEN_Var), WELCOME_WIZ_SHOW_INTRO_SCREEN_Yes))
      return TRUE;
   return FALSE;
}

static void set_terms_accepted(){
   roadmap_config_set(&WELCOME_WIZ_TERMS_OF_USE_Var, WELCOME_WIZ_FIRST_TIME_Yes);
   roadmap_config_save(TRUE);
}

static BOOL is_wizard_enabled(void){
   roadmap_config_declare_enumeration( WELCOME_WIZ_ENABLE_CONFIG_TYPE,
                                       &WELCOME_WIZ_ENABLED_Var,
                                       NULL,
                                       WELCOME_WIZ_FIRST_TIME_No,
                                       WELCOME_WIZ_FIRST_TIME_Yes,
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_ENABLED_Var), WELCOME_WIZ_FIRST_TIME_Yes))
      return TRUE;
   return FALSE;

}
/////////////////////////////////////////////////////////////////////
static void set_first_time_no(){
   roadmap_config_set(&WELCOME_WIZ_FIRST_TIME_Var, WELCOME_WIZ_FIRST_TIME_No);
   roadmap_config_save(TRUE);
}

#ifdef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static BOOL on_keyboard_pressed( void* context, const char* utf8char, uint32_t flags)
{
   SsdWidget edt = (SsdWidget)context;

   return edt->key_pressed( edt, utf8char, flags);
}

#endif //TOUCH_SCREEN

/////////////////////////////////////////////////////////////////////
static void add_address_to_history( int category,
                                    const char* city,
                                    const char* street,
                                    const char* house,
                                    const char* state,
                                    const char *name,
                                    RoadMapPosition *position)
{
   const char* address[ahi__count];
   char temp[20];

   address[ahi_city]          = city;
   address[ahi_street]        = street;
   address[ahi_house_number]  = house;
   address[ahi_state]         = state;
   if(name)
      address[ahi_name]   = name;
   else
      address[ahi_name]   = "";

   sprintf(temp, "%d", position->latitude);
   address[ahi_latitude] = strdup(temp);
   sprintf(temp, "%d", position->longitude);
   address[ahi_longtitude] = strdup(temp);

   roadmap_history_add( category, address);
   roadmap_history_save();
}

/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height){
   SsdWidget space;

#if (defined(WIN32))
   height = height / 2;
#endif
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (space, NULL,NULL);
   return space;
}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_softkey_do_nothing(SsdWidget widget, const char *new_value, void *context){
   return 0;
}

#endif //TOUCH_SCREEN

#ifdef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int end_button_callback (SsdWidget widget, const char *new_value) {

   set_first_time_no();
   ssd_dialog_hide_all (dec_ok);

   return 1;
}
#else //TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_softkey_finish(SsdWidget widget, const char *new_value, void *context){
   set_first_time_no();
   ssd_dialog_hide_all (dec_ok);
   return 0;
}
#endif //TOUCH_SCREEN

/////////////////////////////////////////////////////////////////////
// End
/////////////////////////////////////////////////////////////////////
static void end_dialog(){
   SsdWidget dialog;
    SsdWidget group;
   SsdWidget text;

   dialog = ssd_dialog_new ("Wiz_End",
                      roadmap_lang_get ("End"),
                      NULL,
                      SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);


   group = ssd_container_new ("End_con", NULL,
                        SSD_MIN_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_VCENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);
   ssd_widget_add (group, space(10));

   text = ssd_text_new ("Label", roadmap_lang_get("Way to go!"), 22,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_set_color(text,"#d52c6b", "#d52c6b");
   ssd_widget_add (group, text);
   ssd_widget_add (group, space(10));

   text = ssd_text_new ("Label", roadmap_lang_get("Your account has been updated"), 18,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(20));


#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Finish", roadmap_lang_get ("Finish"),
                        SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, end_button_callback));
#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Finish"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_softkey_finish);
#endif //TOUCH_SCREEN

   ssd_widget_add (dialog, group);
   ssd_dialog_activate ("Wiz_End", NULL);
}

#ifdef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int twitter_button_callback (SsdWidget widget, const char *new_value) {

   if ( ( widget != NULL ) && !strcmp(widget->name, "Skip"))
   {
      end_dialog();
   }
   else /* if (!strcmp(widget->name, "Next")) */
   {
     const char *username = ssd_dialog_get_value("TwitterUserName");
     const char *password = ssd_dialog_get_value("TwitterPassword");
     if (!username || !*username){
        roadmap_messagebox("Error", "Please enter a user name");
        return 0;
     }

     if (!password || !*password){
        roadmap_messagebox("Error", "Please enter password");
        return 0;
     }

     roadmap_twitter_set_username(username);
     roadmap_twitter_set_password(password);
     roadmap_config_save(TRUE);
     Realtime_TwitterConnect(ssd_dialog_get_value("TwitterUserName"), ssd_dialog_get_value("TwitterPassword"));
     end_dialog();
   }

   return 1;
}

#else //TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_softkey_next_twitter(SsdWidget widget, const char *new_value, void *context){
   roadmap_twitter_set_username(ssd_dialog_get_value("TwitterUserName"));
   roadmap_twitter_set_password(ssd_dialog_get_value("TwitterPassword"));
   roadmap_config_save(TRUE);
   end_dialog();
   return 0;
}


#endif //else TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Twitter
/////////////////////////////////////////////////////////////////////
void welcome_wizard_twitter_dialog(void){
   SsdWidget dialog;
   SsdWidget group, box, box2;
   SsdWidget text;
   SsdWidget bitmap;

   int width = roadmap_canvas_width()/2;
   set_first_time_no();
   dialog = ssd_dialog_new ("Twitter",
                      roadmap_lang_get ("Enter twitter details"),
                      NULL,
                      SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);

   box = ssd_container_new ("twitter_conatiner", NULL,
                        SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_VCENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (box, NULL,NULL);

   ssd_widget_add (box, space(5));

   box2 = ssd_container_new ("twitter_conatiner", NULL,
                        SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (box2, NULL,NULL);
   bitmap = ssd_bitmap_new("twitter-logo","Tweeter-logo", SSD_ALIGN_VCENTER);
   ssd_widget_add(box2, bitmap);
   text = ssd_text_new ("Label", roadmap_lang_get("Tweet your road reports"), 18,SSD_TEXT_LABEL|SSD_ALIGN_VCENTER);
   ssd_widget_set_color(text, "#1bbff4", "#1bbff4");
   ssd_widget_add (box2, text);
   ssd_widget_add (box, box2);

   ssd_widget_add (box, space(5));

   //User name
   group = ssd_container_new ("Twitter Name group", NULL,
                               SSD_MAX_SIZE,30,
                               SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP|SSD_WS_DEFWIDGET);
   ssd_widget_set_color(group, NULL,NULL);
   text = ssd_text_new ("Label", roadmap_lang_get("User name"), 18,SSD_TEXT_LABEL|SSD_ALIGN_VCENTER);
   ssd_widget_add (group, text);

   ssd_widget_add(group, ssd_entry_new("TwitterUserName", "", SSD_ALIGN_VCENTER
         | SSD_ALIGN_RIGHT, 0, width, SSD_MIN_SIZE,
         roadmap_lang_get("User name")));


   ssd_widget_add (box, group);
   ssd_widget_add (box, space(5));

   group = ssd_container_new ("Twitter password group", NULL,
                               SSD_MAX_SIZE,30,
                               SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color(group, NULL,NULL);
   text = ssd_text_new ("Label", roadmap_lang_get("Password"), 18,SSD_TEXT_LABEL|SSD_ALIGN_VCENTER);
   ssd_widget_add (group, text);

   ssd_widget_add(group, ssd_entry_new("TwitterPassword", "", SSD_ALIGN_VCENTER
         | SSD_ALIGN_RIGHT, SSD_TEXT_PASSWORD, width, SSD_MIN_SIZE,
         roadmap_lang_get("Password")));

   ssd_widget_add (box, group);

#ifdef TOUCH_SCREEN
   ssd_widget_add (box, space(25));
   ssd_widget_add (box,
   ssd_button_label ("Next", roadmap_lang_get ("Next"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, twitter_button_callback));

   ssd_widget_add (box,
   ssd_button_label ("Skip", roadmap_lang_get ("Skip"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_END_ROW, twitter_button_callback));
   ssd_widget_add (box, space(10));

#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Next"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_softkey_next_twitter);
#endif //TOUCH_SCREEN

   ssd_widget_add (dialog, box);
   ssd_dialog_activate ("Twitter", NULL);
   ssd_dialog_draw();
   ssd_dialog_set_focus(ssd_widget_get(box, "TwitterUserName_con"));

}

/////////////////////////////////////////////////////////////////////
static SsdWidget create_results_dialog(const char *name)
{
   SsdWidget rcnt = NULL;
   SsdWidget list = NULL;

   rcnt = ssd_container_new( "Address_reslut_container",
                              NULL,
                              SSD_MAX_SIZE,
                              SSD_MAX_SIZE,
                              0);

   list = ssd_list_new(       "Address_result_list",
                              SSD_MAX_SIZE,
                              SSD_MAX_SIZE,
                              inputtype_free_text,
                              0,
                              NULL);

   ssd_list_resize( list, 0);

   gAddressResultList = list;

   ssd_widget_add( rcnt, list);

   return rcnt;
}

/////////////////////////////////////////////////////////////////////
static int get_selected_list_item()
{
   const void * value = ssd_list_selected_value(gAddressResultList);
   if (value == NULL)
      return 0;

   return atoi((char *)value);
}


/////////////////////////////////////////////////////////////////////
static int on_list_item_selected_work(SsdWidget widget, const char *new_value, const void *value)
{
   RoadMapPosition position;
   BOOL success;

   int               selected_list_item   = get_selected_list_item();
   const address_candidate*
                     selection            = generic_search_results( selected_list_item);

   position.latitude =   (int)selection->latitude*1000000;
   position.longitude =   (int)selection->longtitude*1000000;
   success = Realtime_TripServer_CreatePOI(roadmap_lang_get("Work"), &position, TRUE);
   if (success){
      roadmap_history_declare ('F', 7);
         add_address_to_history( ADDRESS_FAVORITE_CATEGORY,
                           selection->city,
                           selection->street,
                           get_house_number__str( selection->house),
                           ADDRESS_HISTORY_STATE,
                           roadmap_lang_get("Work"),
                           &position);
   }
   welcome_wizard_twitter_dialog();
   return 0;
}


/////////////////////////////////////////////////////////////////////
static void on_address_resolved_work( void*                context,
                                 address_candidate*   array,
                                 int                  size,
                                 roadmap_result       rc)
{
   static   const char* results[ADSR_MAX_RESULTS];
   static   void*       indexes[ADSR_MAX_RESULTS];
   SsdWidget dlg  = NULL;

   SsdWidget list_cont = (SsdWidget)context;
   SsdWidget list;
   int       i;

   assert(list_cont);

   list = ssd_widget_get( list_cont, "Address_result_list");

   if( succeeded != rc)
   {
      if( err_as_could_not_find_matches == rc)
         roadmap_messagebox ( roadmap_lang_get( "Resolve Address"),
                              roadmap_lang_get( "String could not be resolved\n"
                                                "to a valid address"));
      else
      {
         char msg[64];

         sprintf( msg, "Search failed\nError %d", rc);

         roadmap_messagebox ( roadmap_lang_get( "Resolve Address"), msg);
      }

      roadmap_log(ROADMAP_ERROR,
                  "roadmap_welcome_wizard::on_address_resolved() - Resolve process failed with error '%s' (%d)",
                  roadmap_result_string( rc), rc);
      return;
   }

   if( !size)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "roadmap_welcome_wizard::on_address_resolved() - NO RESULTS for the address-resolve process");
      return;
   }

   for( i=0; i<size; i++)
   {
      char value[20];
      results[i] = array[i].address;
      sprintf(value, "%d",i);
      indexes[i] = (void*)strdup(value);
   }

   dlg = ssd_dialog_new ("Work Address",
                      roadmap_lang_get ("Work address 3/4"),
                      NULL,
                      SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);

   ssd_widget_add(dlg,list_cont);

   ssd_list_populate(list,
                     size,
                     results,
                     (const void **)&indexes[0],
                     NULL,
                     0,
                     on_list_item_selected_work,
                     NULL,
                     FALSE);
#ifndef TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dlg, "");
   ssd_widget_set_left_softkey_callback   ( dlg, on_softkey_do_nothing);
#endif //TOUCH_SCREEN

   ssd_dialog_activate ("Work Address", NULL);
   ssd_dialog_draw ();
}

/////////////////////////////////////////////////////////////////////
static int on_search_work(SsdWidget widget, const char *new_value)
{
   SsdWidget      cont = widget->parent;
   SsdWidget      list_cont;
   SsdWidget      edit;
   const char*    text;
   roadmap_result rc;

   assert(cont);
   assert(cont->parent);
   assert(cont->parent->context);

   edit     = ssd_widget_get( cont, "Work");
   text     = ssd_text_get_text( edit);
   list_cont= cont->parent->context;


   rc = address_search_resolve_address( list_cont, on_address_resolved_work, text);
   if( succeeded == rc)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "roadmap_welcome_wizard::on_search() - Started Web-Service transaction: Resolve address");
   }
   else
   {
      const char* err = roadmap_result_string( rc);

      roadmap_log(ROADMAP_ERROR,
                  "roadmap_welcome_wizard::on_search() - Resolve process transaction failed to start: '%s'",
                  err);

      roadmap_messagebox ( roadmap_lang_get( "Resolve Address"),
                           roadmap_lang_get( err));
   }

   return 0;
}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_search_workaddress(SsdWidget widget, const char *new_value, void *context){
   on_search_work(widget->children->children, NULL);
   return 0;
}

#endif //TOUCH_SCREEN

/////////////////////////////////////////////////////////////////////
static BOOL workaddress_on_key_pressed_delegate_to_editbox(
                     SsdWidget   this,
                     const char* utf8char,
                     uint32_t    flags)
{
   SsdWidget editbox  = NULL;
   //   Special case:   move focus to the list
   if( KEY_IS_ENTER)
   {
      on_search_work(this, NULL);
      return TRUE;
   }

#ifndef TOUCH_SCREEN
   {
   SsdWidget dialog;
   dialog = this->parent->parent;
   if (!(KEYBOARD_VIRTUAL_KEY & flags)){
      ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Search"));
      ssd_widget_set_left_softkey_callback   ( dialog, on_search_workaddress);
      ssd_dialog_refresh_current_softkeys();
      roadmap_screen_refresh();
   }
   }
#endif //TOUCH_SCREEN

   editbox = this->children;
   return editbox->key_pressed( editbox, utf8char, flags);
}

/////////////////////////////////////////////////////////////////////
#ifdef TOUCH_SCREEN
static int workaddress_button_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "Skip")){
      welcome_wizard_twitter_dialog();
   }
   else if (!strcmp(widget->name, "Search")){
         on_search_work(widget, NULL);
   }
   return 1;
}
#else //TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_skip_workaddress(SsdWidget widget, const char *new_value, void *context){
   welcome_wizard_twitter_dialog();
   return 0;
}

#endif //TOUCH_SCREEN

/////////////////////////////////////////////////////////////////////
// Work address
/////////////////////////////////////////////////////////////////////
static void workaddress_dialog(){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget text, edit_con, edit, bitmap;
   int txt_box_height = 40;
#ifndef TOUCH_SCREEN
   txt_box_height = 23;
#endif

   dialog = ssd_dialog_new ("Work_Addresse",
                      roadmap_lang_get ("Work address 3/4"),
                      NULL,
                      SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);

   ssd_widget_add (dialog, space(5));

   group = ssd_container_new ("work_con", NULL,
                        SSD_MAX_SIZE,SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_VCENTER);
   ssd_widget_set_color (group, NULL,NULL);

   text = ssd_text_new ("Label", roadmap_lang_get("Enter your work address"), 18,SSD_END_ROW|SSD_TEXT_LABEL);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(10));

   edit_con = ssd_container_new ("Edit_con", NULL,
                        SSD_MAX_SIZE,txt_box_height,SSD_WS_TABSTOP|SSD_END_ROW|SSD_CONTAINER_TXT_BOX);
   ssd_widget_set_color (edit_con, "#ffffff","#ffffff");

   bitmap = ssd_bitmap_new("serach", "search_icon", SSD_ALIGN_VCENTER);
   ssd_widget_add(edit_con, bitmap);

   edit = ssd_text_new     ( "Work","", 18, SSD_ALIGN_VCENTER);
   ssd_text_set_input_type( edit, inputtype_free_text);
   ssd_text_set_readonly  ( edit, FALSE);

   edit_con->key_pressed = workaddress_on_key_pressed_delegate_to_editbox;
   ssd_widget_add(edit_con, edit);
   ssd_widget_add(edit_con, ssd_bitmap_new("cursor", "cursor",SSD_ALIGN_VCENTER));
   ssd_widget_add (group, edit_con);

   ssd_widget_add (group, space(20));

#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Search", roadmap_lang_get ("Search"),
                        SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, workaddress_button_callback));

   ssd_widget_add (group,
   ssd_button_label ("Skip", roadmap_lang_get ("Skip"),
                        SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_END_ROW, workaddress_button_callback));
   ssd_widget_add (group, space(10));

   if ( roadmap_native_keyboard_enabled() )
   {
	   ssd_dialog_set_ntv_keyboard_action( dialog->name, _ntv_kb_action_show );
	   ssd_dialog_set_ntv_keyboard_params( dialog->name, &s_gNativeKBParams );
   }
   else
   {
      ssd_create_keyboard( group,
                     on_keyboard_pressed,
                     NULL,
                     NULL,
                     edit);
   }

#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Skip"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_skip_workaddress);
#endif //TOUCH_SCREEN
   ssd_widget_add (dialog, group);
   dialog->context = create_results_dialog("Work address");
   ssd_dialog_activate ("Work_Addresse", NULL);
   ssd_dialog_draw();
   ssd_dialog_set_focus( edit_con );
}


/////////////////////////////////////////////////////////////////////
static int on_list_item_selected_home(SsdWidget widget, const char *new_value, const void *value)
{
   RoadMapPosition position;
   BOOL success;
   int               selected_list_item   = get_selected_list_item();
   const address_candidate*
                     selection            = generic_search_results( selected_list_item);

   position.latitude =   (int)(selection->latitude*1000000);
   position.longitude =   (int)(selection->longtitude*1000000);
   success = Realtime_TripServer_CreatePOI(roadmap_lang_get("Home"), &position, TRUE);
   if (success){
         roadmap_history_declare ('F', 7);

         add_address_to_history( ADDRESS_FAVORITE_CATEGORY,
                           selection->city,
                           selection->street,
                           get_house_number__str( selection->house),
                           ADDRESS_HISTORY_STATE,
                           roadmap_lang_get("Home"),
                           &position);
   }
   workaddress_dialog();
   return 0;
}


/////////////////////////////////////////////////////////////////////
static void on_address_resolved( void*                context,
                                 address_candidate*   array,
                                 int                  size,
                                 roadmap_result       rc)
{
   static   const char* results[ADSR_MAX_RESULTS];
   static   void*       indexes[ADSR_MAX_RESULTS];
   SsdWidget dlg  = NULL;

   SsdWidget list_cont = (SsdWidget)context;
   SsdWidget list;
   int       i;

   assert(list_cont);

   list = ssd_widget_get( list_cont, "Address_result_list");

   if( succeeded != rc)
   {
      if( err_as_could_not_find_matches == rc)
         roadmap_messagebox ( roadmap_lang_get( "Resolve Address"),
                              roadmap_lang_get( "String could not be resolved\n"
                                                "to a valid address"));
      else
      {
         char msg[64];

         sprintf( msg, "Search failed\nError %d", rc);

         roadmap_messagebox ( roadmap_lang_get( "Resolve Address"), msg);
      }

      roadmap_log(ROADMAP_ERROR,
                  "roadmap_welcome_wizard::on_address_resolved() - Resolve process failed with error '%s' (%d)",
                  roadmap_result_string( rc), rc);
      return;
   }

   if( !size)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "roadmap_welcome_wizard::on_address_resolved() - NO RESULTS for the address-resolve process");
      return;
   }

   for( i=0; i<size; i++)
   {
      char value[20];
      results[i] = array[i].address;
      sprintf(value, "%d",i);
      indexes[i] = (void*)strdup(value);
   }


   dlg = ssd_dialog_new ("Home Address",
                      roadmap_lang_get ("Home address 2/4"),
                      NULL,
                      SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);

   ssd_widget_add(dlg,list_cont);
   ssd_list_populate(list,
                     size,
                     results,
                     (const void **)&indexes[0],
                     NULL,
                     0,
                     on_list_item_selected_home,
                     NULL,
                     FALSE);

#ifndef TOUCH_SCREEN
    ssd_widget_set_left_softkey_text       ( dlg, "");
    ssd_widget_set_left_softkey_callback   ( dlg, on_softkey_do_nothing);
#endif //TOUCH_SCREEN

   ssd_dialog_activate ("Home Address", NULL);
   ssd_dialog_draw ();
}

/////////////////////////////////////////////////////////////////////
static int on_search_home(SsdWidget widget, const char *new_value)
{
   SsdWidget      cont = widget->parent;
   SsdWidget      list_cont;
   SsdWidget      edit;
   const char*    text;
   roadmap_result rc;

   assert(cont);
   assert(cont->parent);
   assert(cont->parent->context);

   edit     = ssd_widget_get( cont, "Home");
   text     = ssd_text_get_text( edit);
   list_cont= cont->parent->context;

   rc = address_search_resolve_address( list_cont, on_address_resolved, text);
   if( succeeded == rc)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "roadmap_welcome_wizard::on_search_home() - Started Web-Service transaction: Resolve address");
   }
   else
   {
      const char* err = roadmap_result_string( rc);

      roadmap_log(ROADMAP_ERROR,
                  "roadmap_welcome_wizard::on_search_home() - Resolve process transaction failed to start: '%s'",
                  err);

      roadmap_messagebox ( roadmap_lang_get( "Resolve Address"),
                           roadmap_lang_get( err));
   }

   return 0;
}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_search_homeaddress(SsdWidget widget, const char *new_value, void *context){
   on_search_home(widget->children->children, NULL);
   return 0;
}
#endif //TOUCH_SCREEN

/////////////////////////////////////////////////////////////////////
static BOOL homeaddress_on_key_pressed_delegate_to_editbox(
                     SsdWidget   this,
                     const char* utf8char,
                     uint32_t    flags)
{
   SsdWidget editbox  = NULL;


   //   Special case:   move focus to the list
   if( KEY_IS_ENTER)
   {
      on_search_home(this, NULL);
      return TRUE;
   }

#ifndef TOUCH_SCREEN
   {
   SsdWidget dialog;
   dialog = this->parent->parent;
   if (!(KEYBOARD_VIRTUAL_KEY & flags)){
      ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Search"));
      ssd_widget_set_left_softkey_callback   ( dialog, on_search_homeaddress);
      ssd_dialog_refresh_current_softkeys();
      roadmap_screen_refresh();
   }
   }
#endif //TOUCH_SCREEN

   editbox = this->children;
   return editbox->key_pressed( editbox, utf8char, flags);
}

#ifdef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int homeaddress_button_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "Skip")){
      workaddress_dialog();
   }
   else if (!strcmp(widget->name, "Search")){
         on_search_home(widget, NULL);
   }
   return 1;
}

#else //TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_skip_homeaddress(SsdWidget widget, const char *new_value, void *context){
   workaddress_dialog();
   return 0;
}
#endif //TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
// Home address
/////////////////////////////////////////////////////////////////////
static void homeaddress_dialog(){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget text, edit_con, edit, bitmap;
   int txt_box_height = 40;
#ifndef TOUCH_SCREEN
   txt_box_height = 23;
#endif
   dialog = ssd_dialog_new ("Home_Addresse",
                      roadmap_lang_get ("Home address 2/4"),
                      NULL,
                      SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);

   ssd_widget_add (dialog, space(5));

   group = ssd_container_new ("home_con", NULL,
                        SSD_MAX_SIZE,SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_VCENTER);
   ssd_widget_set_color (group, NULL,NULL);

   text = ssd_text_new ("Label", roadmap_lang_get("Enter your home address"), 18,SSD_END_ROW|SSD_TEXT_LABEL);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(5));

   text = ssd_text_new ("Label", roadmap_lang_get("We'll show you the fatest route to get there"), 14,SSD_END_ROW|SSD_TEXT_LABEL);
   ssd_widget_set_color(text, "#5987a9",NULL);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(5));

   edit_con = ssd_container_new ("Text_con", NULL,
                        SSD_MAX_SIZE,txt_box_height,SSD_WS_TABSTOP|SSD_END_ROW|SSD_CONTAINER_TXT_BOX);
   ssd_widget_set_color (edit_con, "#ffffff","#ffffff");

   bitmap = ssd_bitmap_new("serach", "search_icon", SSD_ALIGN_VCENTER);
   ssd_widget_add(edit_con, bitmap);
   edit = ssd_text_new     ( "Home","", 18, SSD_ALIGN_VCENTER);
   ssd_text_set_input_type( edit, inputtype_free_text);
   ssd_text_set_readonly  ( edit, FALSE);

   edit_con->key_pressed = homeaddress_on_key_pressed_delegate_to_editbox;
   ssd_widget_add(edit_con, edit);
   ssd_widget_add(edit_con, ssd_bitmap_new("cursor", "cursor",SSD_ALIGN_VCENTER));
   ssd_widget_add (group, edit_con);

   ssd_widget_add (group, space(20));

#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Search", roadmap_lang_get ("Search"),
                        SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, homeaddress_button_callback));

   ssd_widget_add (group,
   ssd_button_label ("Skip", roadmap_lang_get ("Skip"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_END_ROW, homeaddress_button_callback));
   ssd_widget_add (group, space(10));

   if ( roadmap_native_keyboard_enabled() )
   {
	   ssd_dialog_set_ntv_keyboard_action( dialog->name, _ntv_kb_action_show );
	   ssd_dialog_set_ntv_keyboard_params( dialog->name, &s_gNativeKBParams );
   }
   else
   {
      ssd_create_keyboard( group,
                     on_keyboard_pressed,
                     NULL,
                     NULL,
                     edit);
   }

#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Skip"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_skip_homeaddress);
#endif //TOUCH_SCREEN

   ssd_widget_add (dialog, group);
   dialog->context = create_results_dialog("Home address");
   ssd_dialog_activate ("Home_Addresse", NULL);
   ssd_dialog_draw();
   ssd_dialog_set_focus( edit_con);
}

/////////////////////////////////////////////////////////////////////
static int nickname_next_button_callback (SsdWidget widget, const char *new_value) {

   roadmap_config_set(&RT_CFG_PRM_NKNM_Var, ssd_dialog_get_value("NickName"));
   //homeaddress_dialog();
   welcome_wizard_twitter_dialog();
   return 1;
}

/////////////////////////////////////////////////////////////////////
static BOOL nickname_on_key_pressed_delegate_to_editbox(
                     SsdWidget   this,
                     const char* utf8char,
                     uint32_t    flags)
{
   SsdWidget editbox  = NULL;

   //   Special case:   move focus to the list
   if( KEY_IS_ENTER)
   {
      nickname_next_button_callback(NULL, NULL);
      return TRUE;
   }

   editbox = this->children;
   return editbox->key_pressed( editbox, utf8char, flags);
}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_softkey_next_nickname(SsdWidget widget, const char *new_value, void *context){
   nickname_next_button_callback(NULL, NULL);
   return 0;
}
#endif //TOUCH_SCREEN

/////////////////////////////////////////////////////////////////////
// Nickname
/////////////////////////////////////////////////////////////////////
static void nickname_dialog(){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget text, edit_con, edit;
   int txt_box_height = 40;
#ifndef TOUCH_SCREEN
   txt_box_height = 23;
#endif

   dialog = ssd_dialog_new ("Wiz_Nickname",
                      roadmap_lang_get ("Nickname 1/2"),
                      NULL,
                      SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);

   ssd_widget_add (dialog, space(5));

   group = ssd_container_new ("End_con", NULL,
                              SSD_MAX_SIZE,SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_VCENTER);
   ssd_widget_set_color (group, NULL,NULL);

   text = ssd_text_new ("Label", roadmap_lang_get("Choose a nickname"), 16,SSD_END_ROW|SSD_TEXT_LABEL);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(10));

   edit_con = ssd_container_new ("Text_con", NULL,
                                 SSD_MAX_SIZE,txt_box_height,SSD_WS_TABSTOP|SSD_END_ROW|SSD_CONTAINER_TXT_BOX);
   ssd_widget_set_color (edit_con, "#ffffff","#ffffff");

   edit = ssd_text_new     ( "NickName",roadmap_config_get( &RT_CFG_PRM_NKNM_Var), 16, SSD_ALIGN_VCENTER);
   ssd_text_set_input_type( edit, inputtype_free_text);
   ssd_text_set_readonly  ( edit, FALSE);

   edit_con->key_pressed = nickname_on_key_pressed_delegate_to_editbox;
   ssd_widget_add(edit_con, edit);

   ssd_widget_add(edit_con, ssd_bitmap_new("cursor", "cursor",SSD_ALIGN_VCENTER));
   ssd_widget_add (group, edit_con);

   ssd_widget_add (group, space(5));
#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Wizard_Next", roadmap_lang_get ("Next"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_END_ROW|SSD_START_NEW_ROW, nickname_next_button_callback));
   ssd_widget_add (group, space(10));

   if ( roadmap_native_keyboard_enabled() )
   {
	   ssd_dialog_set_ntv_keyboard_action( dialog->name, _ntv_kb_action_show );
	   ssd_dialog_set_ntv_keyboard_params( dialog->name, &s_gNativeKBParams );
   }
   else
   {
	  ssd_create_keyboard( group,
					 on_keyboard_pressed,
					 NULL,
					 NULL,
					 edit);
   }
#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Next"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_softkey_next_nickname);
#endif //TOUCH_SCREEN

   ssd_widget_add (dialog, group);

   ssd_dialog_activate ("Wiz_Nickname", NULL);
   ssd_dialog_draw();
   ssd_dialog_set_focus( edit_con);
}


#ifdef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int welcome_buttons_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "Skip")){
         set_first_time_no();
         ssd_dialog_hide_all(dec_close);
         roadmap_messagebox("","You can personalize your account from Settings->profile");
   }
   else if (!strcmp(widget->name, "Personalize")){
      roadmap_login_update_dlg_show();
   }

   return 1;
}
#else //TOUCH_SCREEN

static void intro_screen_left_key_callback(){
   return;  // do nothing if clicked left key, no back possible. 
}

/////////////////////////////////////////////////////////////////////
static int on_skip_welcome(SsdWidget widget, const char *new_value, void *context){
   set_first_time_no();
   ssd_dialog_hide_all(dec_close);
   return 0;
}

/////////////////////////////////////////////////////////////////////
static int on_personalize(SsdWidget widget, const char *new_value, void *context){
   roadmap_login_update_dlg_show();
   return 0;
}
#endif //TOUCH_SCREEN

static void on_dialog_closed( int exit_code, void* context){
#ifdef TOUCH_SCREEN
   roadmap_bottom_bar_show();
#endif
}
/////////////////////////////////////////////////////////////////////
// Welcome
/////////////////////////////////////////////////////////////////////
static void welcome_dialog(void){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box, text, bitmap;

#ifdef TOUCH_SCREEN
  roadmap_bottom_bar_hide();
#endif

   dialog = ssd_dialog_new ("Welcome",
                            roadmap_lang_get ("Personalize your account"),
                            NULL,
                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);

   group = ssd_container_new ("Welcome_group", NULL,
                              SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);

   ssd_widget_add (group, space(10));

   box = ssd_container_new ("spacer1", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (box, NULL,NULL);

   bitmap = ssd_bitmap_new("info", "Info", SSD_END_ROW);
   ssd_widget_add(box, bitmap);

   ssd_widget_add(group, box);

   ssd_widget_add (group, space(20));

   text = ssd_text_new ("Label", roadmap_lang_get("You are signed-in with a randomly generated login"), 18,SSD_ALIGN_RIGHT|SSD_END_ROW);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(20));

   text = ssd_text_new ("Label", roadmap_lang_get("To easily access your web account, send road tweets and more, personalize your account now"),14,SSD_END_ROW|SSD_ALIGN_RIGHT);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(20));


#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Skip", roadmap_lang_get ("Skip"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, welcome_buttons_callback));

   ssd_widget_add (group,
   ssd_button_label ("Personalize", roadmap_lang_get ("Personalize"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, welcome_buttons_callback));
#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Skip"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_skip_welcome);

   ssd_widget_set_right_softkey_text      ( dialog, roadmap_lang_get("Personalize"));
   ssd_widget_set_right_softkey_callback  ( dialog, on_personalize);
#endif //TOUCH_SCREEN
   ssd_widget_add (group, space(20));
   ssd_widget_add (dialog, group);
   ssd_dialog_activate ("Welcome", NULL);
   ssd_dialog_set_callback (on_dialog_closed);
   if (gLoginCallBack) {
      gLoginCallBack();
      gLoginCallBack = NULL;
   }

   roadmap_screen_refresh();
}




static void callGlobalCallback(){
	ssd_dialog_hide_all(dec_close);
	if (gCallback)
            (*gCallback)();
}

// callback for buttons in intro screen
static int intro_screen_buttons_callback(SsdWidget widget, const char *new_value){
	if (!strcmp(widget->name, "Next")){
		callGlobalCallback();
	}
	return 1;
}

// create the intro screen
static void create_intro_screen(){
	SsdWidget dialog;
	SsdWidget text;
	const char * introText;
	SsdWidget group;
	int tab_flag=0;
	int         width = roadmap_canvas_width() - 10;
	const char* system_lang;
#ifndef TOUCH_SCREEN
    tab_flag = SSD_WS_TABSTOP;
#endif	
	system_lang = roadmap_lang_get("lang");
	dialog = ssd_dialog_new ("Intro_screen",
                            roadmap_lang_get ("Hi there!"),
                            NULL,
                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_BACK);
   group = ssd_container_new ("Welcome", NULL,
            width, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_END_ROW|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);
   ssd_widget_add (group, space(8)); 
   
   if(0 == strcmp( system_lang, "Hebrew"))
       introText = EnglishIntro1;
    else
       introText = EnglishIntro1;
   text = ssd_text_new( "txt", "", 16, tab_flag|SSD_WS_DEFWIDGET);
   ssd_text_set_text_size( text, strlen(introText));
   ssd_text_set_text( text, introText);
   ssd_widget_add( group, text);  
   
   if(0 == strcmp( system_lang, "Hebrew"))
       introText = EnglishIntro2;
    else
       introText = EnglishIntro2;
   text = ssd_text_new( "txt", "", 16, tab_flag);
   ssd_text_set_text_size( text, strlen(introText));
   ssd_text_set_text( text, introText);
   ssd_widget_add( group, text);    
   
   if(0 == strcmp( system_lang, "Hebrew"))
       introText = EnglishIntro3;
    else
       introText = EnglishIntro3;
   text = ssd_text_new( "txt", "", 16, tab_flag);
   ssd_text_set_text_size( text, strlen(introText));
   ssd_text_set_text( text, introText);
   ssd_widget_add( group, text);     
   
#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Next", roadmap_lang_get ("Next"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, intro_screen_buttons_callback));

#else
   ssd_widget_set_right_softkey_text       ( dialog, roadmap_lang_get("Next"));
   ssd_widget_set_right_softkey_callback   ( dialog, callGlobalCallback);

   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get(""));
   ssd_widget_set_left_softkey_callback   ( dialog, intro_screen_left_key_callback);

#endif       

   ssd_widget_add (group, space(20));
   ssd_widget_add( dialog, group);
   ssd_dialog_activate ("Intro_screen", NULL);          
}


///////////////////////////////////////////////////////////////
// Terms of use
//////////////////////////////////////////////////////////////
static int term_of_use_dialog_buttons_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "Accept")){
         set_terms_accepted();
         if (is_show_intro_screen())
         	create_intro_screen(); 
         else 
         	callGlobalCallback();
         return 1;  
   }
   else if (!strcmp(widget->name, "Decline")){
      roadmap_main_exit();
   }

   return 1;
}



//////////////////////////////////////////////////////////////
static void on_dialog_closed_terms_of_use( int exit_code, void* context){
#ifdef TOUCH_SCREEN
   roadmap_bottom_bar_show();
#endif
}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int terms_of_use_accepted (SsdWidget widget, const char *new_value, void *context) {
  	set_terms_accepted();
    if (is_show_intro_screen())
         create_intro_screen(); 
    else 
         callGlobalCallback(); 
    return 1;  
}
/////////////////////////////////////////////////////////////////////
static int terms_of_use_declined (SsdWidget widget, const char *new_value, void *context){
   roadmap_main_exit();
   return 1;
}
#else
/*
 * Right soft key is defined for the touch devices with the hw back button: this callback makes it
 * impossible to be closed
 */
static int terms_of_use_right_softkey_touch( SsdWidget widget, const char *new_value, void *context )
{
	roadmap_main_minimize();
	return 1;
}
#endif //TOUCH_SCREEN


static BOOL OnKeyPressed( SsdWidget this, const char* utf8char, uint32_t flags){
   if( VK_Arrow_down == (*utf8char))
      return 1;
   else
      return 0;
}


//////////////////////////////////////////////////////////////
void term_of_use_dialog(void){

   static const char* Disclaimer = NULL;

   SsdWidget   dialog;
   SsdWidget   group;
   SsdWidget   text;
   int         width = roadmap_canvas_width() - 20;
   int         tab_flag = 0;
   const char* system_lang = roadmap_lang_get( "lang");

#ifndef TOUCH_SCREEN
   tab_flag = SSD_WS_TABSTOP;
#endif
   if( NULL == Disclaimer)
   {
      if(0 == strcmp( system_lang, "Hebrew"))
         Disclaimer = Hebrew_Disclaimer1;
      else
         Disclaimer = English_Disclaimer1;
   }

   
   dialog = ssd_dialog_new ("term_of_use",
                            roadmap_lang_get ("Terms of use"),
                            NULL,
                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_BACK);
   group = ssd_container_new ("Welcome", NULL,
            width, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_END_ROW|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);

   ssd_widget_add (group, space(20));


   
if(!(is_show_intro_screen())){
   text = ssd_text_new( "txt", "", 14, tab_flag|SSD_WS_DEFWIDGET);
   ssd_text_set_text_size( text, strlen(Disclaimer));
   ssd_text_set_text( text, Disclaimer);
   ssd_widget_add( group, text);
}


   if(0 == strcmp( system_lang, "Hebrew"))
       Disclaimer = Hebrew_Disclaimer2;
    else
       Disclaimer = English_Disclaimer2;
if(!(is_show_intro_screen()))
   text = ssd_text_new( "txt", "", 14, tab_flag);
else
  text = ssd_text_new( "txt", "", 14, tab_flag|SSD_WS_DEFWIDGET);
  
   ssd_text_set_text_size( text, strlen(Disclaimer));
   ssd_text_set_text( text, Disclaimer);
   ssd_widget_add( group, text);

   if(0 == strcmp( system_lang, "Hebrew"))
       Disclaimer = Hebrew_Disclaimer3;
    else
       Disclaimer = English_Disclaimer3;
   text = ssd_text_new( "txt", "", 14, tab_flag);
   ssd_text_set_text_size( text, strlen(Disclaimer));
   ssd_text_set_text( text, Disclaimer);
   ssd_widget_add( group, text);

   if(0 == strcmp( system_lang, "Hebrew"))
       Disclaimer = Hebrew_Disclaimer4;
    else
       Disclaimer = English_Disclaimer4;
   text = ssd_text_new( "txt", "", 14, tab_flag);
   ssd_text_set_text_size( text, strlen(Disclaimer));
   ssd_text_set_text( text, Disclaimer);
   ssd_widget_add( group, text);
   
   if(0 == strcmp( system_lang, "Hebrew"))
       Disclaimer = Hebrew_Disclaimer5;
    else
       Disclaimer = English_Disclaimer5;
   text = ssd_text_new( "txt", "", 14, tab_flag);
   ssd_text_set_text_size( text, strlen(Disclaimer));
   ssd_text_set_text( text, Disclaimer);
   ssd_widget_add( group, text);
   
   text->key_pressed     = OnKeyPressed;

   //ssd_widget_add (group, space(20));

#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Accept", roadmap_lang_get ("Accept"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, term_of_use_dialog_buttons_callback));

   ssd_widget_add (group,
   ssd_button_label ("Decline", roadmap_lang_get ("Decline"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, term_of_use_dialog_buttons_callback));
   ssd_widget_set_right_softkey_callback ( dialog, terms_of_use_right_softkey_touch );
#else
   ssd_widget_set_right_softkey_text       ( dialog, roadmap_lang_get("Accept"));
   ssd_widget_set_right_softkey_callback   ( dialog, terms_of_use_accepted);

   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Decline"));
   ssd_widget_set_left_softkey_callback   ( dialog, terms_of_use_declined);

#endif

   ssd_widget_add (group, space(20));
   ssd_widget_add( dialog, group);
   ssd_dialog_activate ("term_of_use", NULL);
   ssd_dialog_set_callback (on_dialog_closed_terms_of_use);
}


//////////////////////////////////////////////////////////////
void roadmap_term_of_use(RoadMapCallback callback){


   if (is_terms_accepted()){
      if (callback)
         (*callback)();
         return;
   }else
      gCallback = callback;

#ifdef TOUCH_SCREEN
  roadmap_bottom_bar_hide();
#endif

  term_of_use_dialog();
}


/////////////////////////////////////////////////////////////////////
static BOOL is_activation_time_reached(){

   time_t now;
   static BOOL initialized = FALSE;
   int minutes_to_activation;
   int first_time = roadmap_start_get_first_time_use();
   if (!initialized){
      roadmap_config_declare("preferences", &WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Var,
               WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Defaut, NULL);
      initialized = TRUE;
   }
   minutes_to_activation = roadmap_config_get_integer(&WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Var);
   now = time(NULL);
   if ((now - first_time) > (minutes_to_activation * 60))
      return TRUE;
   else
      return FALSE;

}

/////////////////////////////////////////////////////////////////////
void roadmap_welcome_wizard(void){

   if (!is_wizard_enabled())
      return;

   if (!is_first_time())
      return;

   if (!Realtime_is_random_user())
      return;

   if (!is_activation_time_reached())
      return;

   gLoginCallBack = Realtime_NotifyOnLogin (welcome_dialog);

}

/////////////////////////////////////////////////////////////////////
static int personalize_buttons_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "Skip")){
         set_first_time_no();
         ssd_dialog_hide_current(dec_close);
   }
   else if (!strcmp(widget->name, "Personalize")){
      roadmap_login_update_dlg_show();
   }

   return 1;
}

static int personalize_signin_callback( SsdWidget widget, const char *new_value )
{
	roadmap_login_details_dialog_show_un_pw();
	return 1;
}

void roadmap_welcome_personalize_dialog( void ){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box, text, bitmap;
   SsdWidget text_group;
   SsdClickOffsets offsets = {-10, -10, 10, 10 };
#ifdef TOUCH_SCREEN
  roadmap_bottom_bar_hide();
#endif

   dialog = ssd_dialog_new ( "Welcome",
                            roadmap_lang_get ("Personalize your account"),
                            NULL,
                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL );

   ssd_widget_add (dialog, space(5));

   group = ssd_container_new ("Welcome_group", NULL,
                              SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);

   ssd_widget_add (group, space(10));

   box = ssd_container_new ("spacer1", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (box, NULL,NULL);

   bitmap = ssd_bitmap_new("info", "Info", SSD_END_ROW);
   ssd_widget_add(box, bitmap);

   ssd_widget_add(group, box);

   ssd_widget_add (group, space(10));

   text = ssd_text_new ("Label", roadmap_lang_get("You are signed-in with a randomly generated login"), 18,SSD_END_ROW);
   ssd_widget_add (group, text);

   ssd_dialog_add_vspace( group, 5, 0 );

   // Sign in text
#ifdef TOUCH_SCREEN
   text_group = ssd_container_new ( "Text container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW );
   ssd_dialog_add_hspace( text_group, 1, 0 );
   ssd_widget_add( text_group, ssd_text_new("Label", roadmap_lang_get ( "Already have a login?  "), 18, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE ) );
   text = ssd_text_new( "Sign in", roadmap_lang_get ( "Sign in" ), 18, SSD_ALIGN_VCENTER );
   ssd_text_set_color( text, "#0000FF" );
   ssd_widget_add ( text_group, text );
   ssd_widget_add( group, text_group );
   ssd_widget_set_click_offsets( text_group, &offsets );
   ssd_widget_set_callback( text_group, personalize_signin_callback );

   // Space before next entry
   ssd_dialog_add_vspace( group, 3, 0 );
#endif

   ssd_widget_add (group, space(10));

   text = ssd_text_new ("Label", roadmap_lang_get("To easily access your web account, send road tweets and more, personalize your account now"), 14,SSD_END_ROW);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(10));


#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Skip", roadmap_lang_get ("Not now"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, personalize_buttons_callback));

   ssd_widget_add (group,
   ssd_button_label ("Personalize", roadmap_lang_get ("Personalize"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, personalize_buttons_callback));
#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Not now"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_skip_welcome);

   ssd_widget_set_right_softkey_text      ( dialog, roadmap_lang_get("Personalize"));
   ssd_widget_set_right_softkey_callback  ( dialog, on_personalize);
#endif //TOUCH_SCREEN

   ssd_widget_add (group, space(20));
   ssd_widget_add (dialog, group);
   ssd_dialog_activate ("Welcome", NULL);
   ssd_dialog_set_callback ( on_dialog_closed );

   roadmap_screen_refresh();

}

BOOL roadmap_welcome_on_preferences( void )
{
   if ( !Realtime_is_random_user() )
      return FALSE;

/*
   if (!is_wizard_enabled())
       return FALSE;
*/

   roadmap_welcome_personalize_dialog();

   return TRUE;
}
