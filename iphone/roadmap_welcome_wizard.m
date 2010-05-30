/* roadmap_welcome_wizard.m - Welcome Wizard
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
 *   Copyright 2009, Waze Ltd
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
 * SYNOPSYS:
 *
 *   See roadmap_welcome_wizard.h
 */

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_path.h"
#include "roadmap_lang.h"
#include "roadmap_config.h"
#include "Realtime.h"
#include "RealtimeDefs.h"
#include "address_search.h"
#include "roadmap_messagebox.h"
#include "roadmap_list_menu.h"
#include "roadmap_history.h"
#include "roadmap_social.h"
#include "roadmap_geo_location_info.h"
#include "roadmap_iphoneimage.h"
#include "widgets/iphoneLabel.h"
#include "roadmap_login.h"
#include "ssd_progress_msg_dialog.h"
#include "roadmap_start.h"
#include "roadmap_introduction.h"

#include "roadmap_welcome_wizard.h"
#include "roadmap_iphonewelcome_wizard.h"


extern RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_PASSWORD_Var;
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






static CGRect gRectTerm = {20.0f, 15.0f, 280.0f, 300.0f};

//static CGSize gSizeTermAccepted = {75.0f, 30.0f};
//static CGSize gSizeNormalBtn = {100.0f, 30.0f};
static CGSize gSizePersonalizeText1 = {270.0f, 60.0f};
static CGSize gSizePersonalizeText2 = {270.0f, 80.0f};
static CGSize gSizePersonalizeLater = {270.0f, 60.0f};

#define TEXT_HEIGHT 30.0f

enum ids {
	ID_TWITTER_USERNAME = 1,
	ID_TWITTER_PASSWORD
};

static RoadMapCallback gCallback;
static int is_wizard_shown = 0;
static int gShowIntro = 0;



const char* Hebrew_Disclaimer = 
   "ברוכים הבאים ל Waze!\n\n" \
   "מידע חי על מצב התנועה ונווט עוקף פקקים, אשר מיוצר בשיתוף ותרומה של חברי הקהילה\n" \
   "בלחיצה על 'אשר' הנך מאשר כי:\n" \
   "1. קראת והנך מסכים לתנאי השימוש ומדיניות הפרטיות. השימוש בשירות Waze כפוף להסכמים המחייבים ומעיד על הסכמתך להם. תקציר זה אינו בא במקומם, והוא מיועד לצרכי נוחות בלבד.\n" \
   "2. השימוש בשירות הינו באחריותך בלבד. הינך מאשר כי תשתמש בשירות בהתאם להוראות כל דין, לרבות דיני התעבורה." \
   "3. שירות Waze מחייב חיבור לרשת האינטרנט לצורך השימוש בו, וקבלת עדכונים בזמן אמת. חברת Waze אינה מספקת קישור כזה. באחריותך לרכוש אותו ולוודא שתנאיו מתאימים לצרכיך. חברת Waze אינה נושאת בכל אחריות לקישור ו/או לכל שיבוש, תקלה או קלקול בו. \n" \
   "4. תוכנת המפוי Waze המותקנת על-גבי מכשיר הטלפון הסלולרי שלך, היא תוכנה חופשית; "  \
   "אתה יכול להפיצה מחדש ו/או לשנות את התוכנה על פי תנאי הרישיון הציבורי הכללי של GNU, אם גרסה 2 של הרישיון, " \
   "ובין אם (לפי בחירתך) כל גרסה מאוחרת שלו. התוכנה מופצת בתקווה שתהיה מועילה, אבל בלא אחריות כלשהי. לפרטים נוספים, ראה את הרישיון הציבורי הכללי של GNU. אם רצונך לקבל את קוד המקור של התכנה, פנה אלינו בכתב. \n" \
   "5. כשאתה מספק מידע ותוכן לשירות, אתה מאשר שאתה בעלים בלעדי של כל הזכויות בו ורשאי להקנות בו זכויות. מידע ותוכן כזה נשאר בבעלותך ואתה מקנה בו לחברת Waze רישיון חינם כלל עולמי, לא בלעדי, בלתי חוזר ובלתי מוגבל בזמן, שניתן להעברה וכולל זכות למתן רשיונות-משנה, להשתמש, להעתיק, להפיץ, ליצור יצירות נגזרות, להציג ולבצע בפומבי מידע ותוכן כזה. בכפוף לכך, בסיס הנתונים של השירות הוא קניינה של החברה ואין להשתמש בו אלא לצרכים לא מסחריים ופרטיים בלבד.\n" \
   "6. שירות Waze מוצע בחינם, בתקווה שתמצא אותו מועיל. עם זאת, Waze ו/או עובדיה, מנהליה, בעלי מניותיה, יועציה ו/או מי מטעמה, לא ישאו בחבות כלשהיא כלפיך ו/או כלפי צדדים שלישים, מכל סיבה שהיא, בקשר עם מוצריה Waze ו/או שירותיה, לרבות (אך לא רק) בגין כל הפסד, אבדן רווח, פגיעה במוניטין, תשלום, הוצאה ו/או נזק, ישיר או עקיף, ממוני או בלתי-ממוני.\n\n\n";


const char* English_Disclaimer = 
   "Welcome to Waze!\n\n"
   "You're about to join the first network of drivers working together to build and share real-time road intelligence.\n\n"
   "Since Waze is 100% user generated, we need your collaboration and patience!"
   "By clicking 'Accept' you confirm that:\n\n" \
   "1. You have read and agreed to the 'Terms of Use' http://www.waze.com/legal/tos/ and 'Privacy Policy' http://www.waze.com/legal/privacy/ ('Binding Agreements'). The use of Waze service (the 'Service') is subject to the Binding Agreements and indicates your consent to them. This summary is not meant to replace the Binding Agreements. It is intended for convenience purposes only.\n"
   "2. Use of Service is made at your sole risk.  You hereby agree to use the Service only in accordance with any applicable law, including but not limited to, all transportation laws and regulations.\n"
   "3. The Service requires Internet connection for real time updates. Waze does not provide such connection. It is your responsibility to purchase connection services and to make sure its terms matches your needs. Waze has no control over or responsibility to the connection and/or any disruption, failure or breakdown therein.\n\n" \
   "4. The Waze client, that you install on your cellular device, is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY. See the GNU General Public License http://www.gnu.org/licenses/gpl-2.0.html for more details. If you wish to obtain the program's source code, please write to us.\n"
   "5. When you provide data and or content to the Service, you hereby confirm that you own all exclusive rights at it and may assign or license such rights. You keep all title and rights to such data and/or content you provide to the Service, but you grant Waze Inc. and/or Waze Mobile (the Company or 'Waze') a worldwide, free, non-exclusive, irrevocable, sublicensable, transferable and perpetual license to use, copy, distribute, create derivative works of, publicly display, publicly perform and exploit in any other manner such data and content. Subject to the aforementioned, the Company keeps title and all rights to the Service's database and you may use it for non-commercial and private purposes only.\n\n" \
   "6. Waze service is offered for free, with hope that you find it useful. However, Waze and/or its employees, directors shareholders, advisors and/or on anyone of it's behalf (collectively: 'Waze') shall not be liable to you and/or to any third party, for any reason whatsoever, as result with the use of the Company's product and/or Service. You hereby irrevocably release Waze from any liability of any kind, for any consequence arising from use of the Company's products and/or Service, Including (but not only) for any loss, loss of profit, damage to reputation, fee, expense and/or damage, direct or indirect, financial or non-financial.\n\n";




const char* Spanish_Disclaimer = 
   "Usted está a punto de unirse a la primera red de conductores que trabajan conjuntamente para crear y compartir información de tráfico y eventos sobre las vías en tiempo real. Dado que Waze es un servicio 100% generado por los usuarios, requerimos de su colaboración y paciencia."
   "Al hacer clic (pinchar) en \"Aceptar\", usted confirma que:"
   "1.Ha leído y acepta  los \"Términos de Uso\" http://www.waze.com/legal/tos/ y la \"Política de Privacidad\" http://www.waze.com/legal/privacy/ (\"Acuerdos Obligatorios\"). El uso del servicio Waze (el \"Servicio\") está sujeto a los Acuerdos Obligatorios y determina su consentimiento a ellos.  Éste resumen no pretende sustituir los Acuerdos Obligatorios. Solamente se lo presenta para facilidad."
   "2. El uso del Servicio se hace a su propio riesgo. Por el presente usted acepta usar el Servicio únicamente de acuerdo a cualquier ley aplicable, incluyendo pero no limitado a, todas las leyes y reglamentos y demás normas sobre transporte."
   "3. El Servicio requiere de conexión inalámbrica a Internet para actualizaciones a tiempo real. Waze no provee tal conexión. Es su responsabilidad adquirir los servicios de conexión y asegurarse que sus términos satisfagan sus necesidades. Waze no tiene control o es responsable de la conexión y/o interrupción,  fallos o averías en ella al igual que no es responsable de la cobertura ofrecida por el operador respectivo."
   "4. El aplicativo móvil Waze, que usted instale en su dispositivo móvil consiste en un programa libre que usted puede redistribuirlo y/o modificarlo bajo los términos de la Licencia General Pública GNU, ya fuere la versión 2 de la Licencia o (a su elección) cualquier versión posterior.  Este programa se distribuye con la intención de que sea útil, pero SIN NINGUNA GARANTIA. Ver la Licencia General Pública GNU http://www.gnu.org/licenses/gpl-2.0.html para mayores detalles. Si desea obtener el código de fuente del programa, por favor contáctese con Waze."
   "5. Cuando proporcione datos y/o contenidos al Servicio, por el presente usted confirma que es el titular exclusivo de todos los derechos sobre ellos y que podrá ceder o licenciar tales derechos. Usted mantiene toda titularidad y derechos de dichos datos y/o contenidos proporcionados al Servicio, pero concede a Waze Mobile (la \"Compañía\" o \"Waze\") una licencia a nivel mundial, gratuita, no exclusiva, irrevocable, sublicenciable, transferible y perpetua para el uso, reproducción, distribución,  creación de obras derivadas de, exhibición pública, ejecución pública y cualquier otra forma de explotación de esos datos y contenidos. Sujeto a lo antes mencionado, la Compañía mantiene la propiedad y todos los derechos sobre la base de datos del Servicio y usted podrá usarlo exclusivamente para fines  no comerciales y privados."
   "6. El Servicio Waze se ofrece en forma gratuita, con el objetivo de que usted lo encuentre útil. Sin embargo, Waze y/o sus empleados, directores, accionistas,  asesores, partners y/o cualquiera en su nombre (conjuntamente \"Waze\") no será responsable ante usted y/o cualquier tercero, por cualquier motivo, como resultado del uso del producto o Servicio de la Compañía. Por el presente usted libera irrevocablemente a Waze de toda responsabilidad de cualquier tipo, de cualquier consecuencia derivada del uso del producto o Servicio de la Compañía, incluyendo (pero no limitado) a cualquier pérdida, lucro cesante, pérdida de beneficios, daño a la reputación, tasa, gasto y/o perjuicio, directo o indirecto, económico o no económico.\n\n\n";





/////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////
static void set_terms_accepted(){
	roadmap_config_set(&WELCOME_WIZ_TERMS_OF_USE_Var, WELCOME_WIZ_FIRST_TIME_Yes);
	roadmap_config_save(TRUE);
}



/////////////////////////////////////////////////////////////////////
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
BOOL roadmap_welcome_wizard_is_first_time(void){
	roadmap_config_declare_enumeration( WELCOME_WIZ_CONFIG_TYPE, 
                                       &WELCOME_WIZ_FIRST_TIME_Var, 
                                       NULL, 
                                       WELCOME_WIZ_FIRST_TIME_Yes, 
                                       WELCOME_WIZ_FIRST_TIME_No, 
                                       NULL);
	if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_FIRST_TIME_Var), WELCOME_WIZ_FIRST_TIME_No))
		return FALSE;
	return TRUE;
}

//////////////////////////////////////////////////////////////
void roadmap_term_of_use(RoadMapCallback callback){
	if (is_terms_accepted()){
		if (callback)
			(*callback)();
		return;
	}else
		gCallback = callback;

	TermOfUseView *termDialog = [[TermOfUseView alloc] init];
	[termDialog show];
}

/////////////////////////////////////////////////////////////////////
void welcome_wizard_twitter_dialog(int showIntro){
   WelcomeWizardView *twitterDialog = [[WelcomeWizardView alloc] init];
	[twitterDialog showTwitter:showIntro];
}

void roadmap_welcome_personalize_later_dialog() {
   WelcomeWizardView *personalizeLater = [[WelcomeWizardView alloc] init];
	[personalizeLater showPersonalizeLater];
}

void roadmap_welcome_personalize_dialog() {
   WelcomeWizardView *welcomeDialog = [[WelcomeWizardView alloc] init];
	[welcomeDialog showPersonalize];
}

void roadmap_welcome_wizard_set_first_time_no () {
   roadmap_config_set(&WELCOME_WIZ_FIRST_TIME_Var, WELCOME_WIZ_FIRST_TIME_No);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////
void roadmap_welcome_wizard(void){
   if (roadmap_welcome_wizard_is_first_time())
		roadmap_login_new_existing_dlg();
}





/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
@implementation TermOfUseView

- (void) onAccept
{
	set_terms_accepted();
	
	if (gCallback)
		(*gCallback)();
}

- (void)show
{
	CGRect rect;
	UIImage *image;
	UIImageView *bgView;
	NSString *text;
	UILabel *label;
	UIButton *button;
   int viewPosY = 0;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
	
	[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Hi there!")]];
	
	//hide left button
	UINavigationItem *navItem = [self navigationItem];
	[navItem setHidesBackButton:YES];
	
	
	//set UI elements
	
	//background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     scrollView.bounds.size.height - 30);//,
//					  roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		UIImage *strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
	
	//Message
   const char* system_lang = roadmap_lang_get_system_lang();
   if(!strcmp("heb",system_lang))
      text = [NSString stringWithUTF8String:Hebrew_Disclaimer];
   else if (!strcmp("espanol",system_lang))
      text = [NSString stringWithUTF8String:Spanish_Disclaimer];
   else
      text = [NSString stringWithUTF8String:English_Disclaimer];
   
   gRectTerm.size.width = scrollView.bounds.size.width - 40;
   label = [[UILabel alloc] initWithFrame:gRectTerm];
	[label setText:text];
	[label setNumberOfLines:0];
   [label setLineBreakMode:UILineBreakModeWordWrap];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setFont:[UIFont systemFontOfSize:15]];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleHeight];
   [label sizeToFit];
	[scrollView addSubview:label];
	[label release];
   viewPosY = label.bounds.size.height + 20;

	//Accept button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Accept")] forState:UIControlStateNormal];
	image = roadmap_iphoneimage_load("button_up");
	if (image) {
		[button setBackgroundImage:image forState:UIControlStateNormal];
		[image release];
	}
	image = roadmap_iphoneimage_load("button_down");
	if (image) {
		[button setBackgroundImage:image forState:UIControlStateHighlighted];
		[image release];
	}
   [button sizeToFit];
   rect.size = button.bounds.size;
   rect.origin.x = (scrollView.bounds.size.width - button.bounds.size.width) / 2;
   rect.origin.y = viewPosY;
	[button setFrame:rect];
   viewPosY += button.bounds.size.height + 10;
	[button addTarget:self action:@selector(onAccept) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
	
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;

	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY+10)];
	
	roadmap_main_push_view(self);
}



- (void)dealloc
{
	[super dealloc];
}

@end



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

@implementation WelcomeWizardView

- (void) onFinish
{
	roadmap_welcome_wizard_set_first_time_no();
   is_wizard_shown = 0;
   
   if (gShowIntro)
      roadmap_introduction_show_auto();
   else
      roadmap_main_show_root(NO);
}

- (void) onTwitterNext
{
   int i;
   int verified = 1;
   NSString *username, *password;
   UIView *view;
   UIScrollView *scrollView = (UIScrollView *) self.view;
   
   username = @"";
   password = @"";
   
   for (i = 0; i < [[scrollView subviews] count]; ++i) {
      view = [[scrollView subviews] objectAtIndex:i];
      switch (view.tag) {
         case ID_TWITTER_USERNAME:
            if (((UILabel *)view).text)
               username = ((UILabel *)view).text;
            break;
         case ID_TWITTER_PASSWORD:
            if (((UILabel *)view).text)
               password = ((UILabel *)view).text;
            break;
         default:
            break;
      }
   }
   
   if (([username compare:@""] != NSOrderedSame) &&
       ([password compare:@""] == NSOrderedSame)) {
      verified = 0;
      roadmap_messagebox("Error", "Invalid username");
   }
   else if (([username compare:@""] == NSOrderedSame) &&
              ([password compare:@""] != NSOrderedSame)) {
      verified = 0;
      roadmap_messagebox("Error", "Invalid password");
   }
   
   
   if (verified) {
      WelcomeWizardView *endDialog = [[WelcomeWizardView alloc] init];
      [endDialog showEnd];
   }
}

- (void) onPersonalizeClose
{
   is_wizard_shown = 0;
   roadmap_login_on_signup_skip ();
   roadmap_welcome_wizard_set_first_time_no();
}

- (void) onPersonalizeNext
{
   roadmap_login_update_dlg_show();
}

- (void) onPersonalizeSkip
{
   WelcomeWizardView *personalizeLaterDialog = [[WelcomeWizardView alloc] init];
	[personalizeLaterDialog showPersonalizeLater];
}


- (void)viewDidAppear:(BOOL)animated
{
	UIScrollView *scrollView = (UIScrollView *) self.view;
	CGRect rect = [scrollView frame];
	rect.size.height = roadmap_main_get_mainbox_height();
	[scrollView setFrame:rect];
   
//TODO: make twitter text field first responder ?
	
	[super viewDidAppear:animated];
}

////////////////////
// End Dialog
- (void)showEnd
{
	CGRect rect;
   CGFloat viewPosY = 5.0f;
	UIImage *image;
   UIImage *strechedImage;
	UIImageView *bgView;
	NSString *text;
	UILabel *label;
	UIButton *button;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
   UIImage *imgButtonUp;
   UIImage *imgButtonDown;
   
   image = roadmap_iphoneimage_load("button_up");
   if (image) {
      imgButtonUp = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
   image = roadmap_iphoneimage_load("button_down");
   if (image) {
      imgButtonDown = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
	
	//[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Personalize later")]];
	
	
	//set UI elements
	
	//background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
   
   
	//Way to go text
	text = [NSString stringWithUTF8String:
           roadmap_lang_get("Way to go!")];
   rect = bgView.frame;
   rect.size.width = bgView.bounds.size.width - 80;
   rect.size.height = TEXT_HEIGHT;
   rect.origin.y += viewPosY;
   rect.origin.x += 40;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setTextAlignment:UITextAlignmentCenter];
   [label setFont:[UIFont boldSystemFontOfSize:18]];
   [label setTextColor:[UIColor colorWithRed:0.941f green:0.0f blue:0.376f alpha:1.0f]];
	[scrollView addSubview:label];
	[label release];
   viewPosY += label.bounds.size.height + 5;
   
   //Accound updated text
   text = [NSString stringWithUTF8String:
           roadmap_lang_get("Your account has been updated")];
   rect = bgView.frame;
   rect.size.width = bgView.bounds.size.width - 120;
   rect.size.height = TEXT_HEIGHT * 2;
   rect.origin.y += viewPosY;
   rect.origin.x += 60;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setTextAlignment:UITextAlignmentCenter];
   [label setFont:[UIFont systemFontOfSize:16]];
   [label setNumberOfLines:2];
	[bgView addSubview:label];
	[label release];
   viewPosY += label.bounds.size.height + 5;
   
	//Close button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
   [button sizeToFit];
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += (bgView.bounds.size.width - button.bounds.size.width) /2;
   rect.size = button.bounds.size;
	[button setFrame:rect];
	[button addTarget:self action:@selector(onFinish) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
   
   viewPosY += button.bounds.size.height + 5;
	
   
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;
   
	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	
	roadmap_main_push_view(self);
}


////////////////////
// Twitter Dialog
- (void)showTwitter: (int)showIntro
{
	CGRect rect;
	UIImage *image;
	UIImageView *imageView;
   UIImage *strechedImage;
	UIImageView *bgView;
	UILabel *label;
	UITextField *textField;
   CGFloat viewPosY = 5.0f;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
	
	[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Enter twitter details")]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Next")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onTwitterNext)];
	[navItem setRightBarButtonItem:barButton];
	
   gShowIntro = showIntro;
   
   //UI elements
	
   //background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
   
   //Twitter icon
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   image = roadmap_iphoneimage_load("Tweeter-logo");
   if (image) {
      if (!roadmap_lang_rtl())
         rect.origin.x += 5;
      else
         rect.origin.x += bgView.bounds.size.width - 5 - image.size.width;
      rect.size = image.size;
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      [imageView setFrame:rect];
      [scrollView addSubview:imageView];
      [imageView release];
   }
	
	//Note text
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   if (!roadmap_lang_rtl())
      rect.origin.x += 5 + imageView.frame.size.width + 10;
   else
      rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = bgView.bounds.size.width - imageView.frame.size.width - 20;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Tweet your road reports")]];
	//[label setBackgroundColor:[UIColor clearColor]];
   [label setAdjustsFontSizeToFitWidth:YES];
	[scrollView addSubview:label];
	[label release];
   
   viewPosY += imageView.bounds.size.height + 5;
   
	
   
   //Username
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Username")]];
	[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_TWITTER_USERNAME];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	//[textField setReturnKeyType:UIReturnKeyDone];
	[textField setDelegate:self];
	[textField setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<skip>")]];
	[scrollView addSubview:textField];
	//[textField becomeFirstResponder];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 10;
	
	//Password
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Password")]];
	[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_TWITTER_PASSWORD];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	[textField setSecureTextEntry:YES];
	//[textField setReturnKeyType:UIReturnKeyDone];
	[textField setDelegate:self];
	[textField setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<skip>")]];
	[scrollView addSubview:textField];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 5;
   
   
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;
		
	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	roadmap_main_push_view(self);
}


////////////////////
// Personalize Later Dialog
- (void)showPersonalizeLater
{
	CGRect rect;
   CGFloat viewPosY = 5.0f;
	UIImage *image;
   UIImage *strechedImage;
	UIImageView *bgView;
	NSString *text;
	UILabel *label;
	UIButton *button;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
   UIImage *imgButtonUp;
   UIImage *imgButtonDown;
   
   image = roadmap_iphoneimage_load("button_up");
   if (image) {
      imgButtonUp = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
   image = roadmap_iphoneimage_load("button_down");
   if (image) {
      imgButtonDown = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
	
	//[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Personalize later")]];
	
	
	//set UI elements
	
	//background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
   
     
	
	//Personalize later text
	text = [NSString stringWithUTF8String:
           roadmap_lang_get("You can personalize your account from Settings->profile")];
   rect.size = gSizePersonalizeLater;
   rect.origin.y = viewPosY;
   rect.origin.x = (bgView.bounds.size.width - gSizePersonalizeLater.width) /2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setNumberOfLines:3];
	[bgView addSubview:label];
	[label release];
   viewPosY += label.bounds.size.height + 5;
   
	//Close button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
   [button sizeToFit];
   rect.origin.y = viewPosY;
   rect.origin.x = (bgView.bounds.size.width - button.bounds.size.width) /2;
   rect.size = button.bounds.size;
	[button setFrame:rect];
	[button addTarget:self action:@selector(onPersonalizeClose) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
   
   viewPosY += button.bounds.size.height + 5;
	
	   
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;
   
	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	
	roadmap_main_push_view(self);
}

////////////////////
// Personalize Dialog
- (void)showPersonalize
{
	CGRect rect;
   CGFloat viewPosY = 5.0f;
	UIImage *image;
   UIImage *strechedImage;
	UIImageView *bgView;
   UIImageView *imageView;
	NSString *text;
	UILabel *label;
	UIButton *button;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
   UIImage *imgButtonUp;
   UIImage *imgButtonDown;
   
   image = roadmap_iphoneimage_load("button_up");
   if (image) {
      imgButtonUp = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
   image = roadmap_iphoneimage_load("button_down");
   if (image) {
      imgButtonDown = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
	
	[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Personalize your account")]];
	
   //hide left button
	UINavigationItem *navItem = [self navigationItem];
	[navItem setHidesBackButton:YES];
   
	
	//set UI elements
	
	//background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
   
   //Info icon
   rect.origin.y = viewPosY;
   image = roadmap_iphoneimage_load("Info");
   if (image) {
      if (!roadmap_lang_rtl())
         rect.origin.x = 5;
      else
         rect.origin.x = bgView.bounds.size.width - 5 - image.size.width;
      rect.size = image.size;
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      [imageView setFrame:rect];
      [bgView addSubview:imageView];
      [imageView release];
   }
   viewPosY += imageView.bounds.size.height + 5;
    
	
	//Personalize text 1
	text = [NSString stringWithUTF8String:
           roadmap_lang_get("You are signed-in with a randomly generated login")];
   rect.size = gSizePersonalizeText1;
   rect.origin.y = viewPosY;
   rect.origin.x = (bgView.bounds.size.width - gSizePersonalizeText1.width) /2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setNumberOfLines:2];
	//[label setTextAlignment:UITextAlignmentCenter];
   [label setFont:[UIFont boldSystemFontOfSize:18]];
	[bgView addSubview:label];
	[label release];
   viewPosY += label.bounds.size.height + 5;
   
   //Personalize text 2
	text = [NSString stringWithUTF8String:
           roadmap_lang_get("To easily access your web account, send road tweets and more, personalize your account now")];
   rect.size = gSizePersonalizeText2;
   rect.origin.y = viewPosY;
   rect.origin.x = (bgView.bounds.size.width - gSizePersonalizeText2.width) /2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setNumberOfLines:4];
	//[label setTextAlignment:UITextAlignmentCenter];
   [label setFont:[UIFont systemFontOfSize:16]];
	[bgView addSubview:label];
	[label release];
   viewPosY += label.bounds.size.height + 60;
	
	//Next button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Next")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
   [button sizeToFit];
   rect.origin.y = viewPosY;
   rect.origin.x = bgView.bounds.size.width/2 + 5;
   rect.size = button.bounds.size;
	[button setFrame:rect];
	[button addTarget:self action:@selector(onPersonalizeNext) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
	
	//Skip button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Skip")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
   [button sizeToFit];
   rect.origin.y = viewPosY;
   rect.origin.x = bgView.bounds.size.width/2 - button.bounds.size.width - 5;
   rect.size = button.bounds.size;
	[button setFrame:rect];
	[button addTarget:self action:@selector(onPersonalizeSkip) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
	
   viewPosY += button.bounds.size.height + 5;
   
   
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;

	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	
	roadmap_main_push_view(self);
}



- (void)dealloc
{
	[super dealloc];
}

//Text field delegate
- (void)textFieldDidEndEditing:(UITextField *)textField {
	switch ([textField tag]) {
		case ID_TWITTER_USERNAME:
			roadmap_twitter_set_username([[textField text] UTF8String]);
			break;
		case ID_TWITTER_PASSWORD:
			roadmap_twitter_set_password([[textField text] UTF8String]);		
			break;
		default:
			break;
	}
   
   [textField resignFirstResponder];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
		[textField resignFirstResponder];
   
	return NO;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
   if (roadmap_keyboard_typing_locked(TRUE)) {
      [textField resignFirstResponder];
      return NO;
   } else
      return YES;
}

@end


