/* roadmap_main.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Ehud Shabtai
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
 *
 * SYNOPSYS:
 *
 *   int main (int argc, char **argv);
 */

#include <stdlib.h>
#include <string.h>

#ifdef ROADMAP_USES_GPE
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <libdisplaymigration/displaymigration.h>
#endif

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "navigate/navigate_bar.h"
#include "roadmap_canvas.h"
#include "roadmap_history.h"
#include "roadmap_keyboard.h"
#include "roadmap_screen.h"
#include "../editor/editor_main.h"
#include "roadmap_main.h"
#include "roadmap_androidmain.h"
#include "roadmap_androidcanvas.h"
#include "roadmap_androidbrowser.h"
#include "roadmap_messagebox.h"
#include "JNI/FreeMapJNI.h"
#
#include "roadmap_android_msg.h"
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <roadmap_sound.h>
#include "auto_hide_dlg.h"
#include "roadmap_search.h"
#include "roadmap_bar.h"
#include "roadmap_device_events.h"
#include "roadmap_androidmenu.h"
#include "roadmap_androidogl.h"
#include "roadmap_browser.h"
// Crash handling
#include <signal.h>
#include <sigcontext.h>


int USING_PHONE_KEYPAD = 0;

unsigned char ANDR_APP_SHUTDOWN_FLAG = 0;	// The indicator if the application is in the exiting process

time_t timegm(struct tm *tm) { return 0; }

static BOOL sgFirstRun = FALSE;	// The indicator if current application run is after the upgrade (first execution)
static int sgHandledSignals[] = {SIGSEGV, SIGSTOP, SIGILL, SIGABRT, SIGBUS, SIGFPE, SIGSTKFLT};

static int sgAndroidBuildSdkVersion = -1;
static char sgAndroidDeviceName[128] = {0};
static char sgAndroidDeviceManufacturer[128] = {0};
static char sgAndroidDeviceModel[128] = {0};

//======= IO section ========
typedef enum
{
	_IO_DIR_UNDEFINED = 0,
	_IO_DIR_READ,
	_IO_DIR_WRITE,
	_IO_DIR_READWRITE
} io_direction_type;

#define IO_VALID( ioId ) ( ( ioId ) >= 0 )
#define IO_INVALID_VAL (-1)

typedef struct roadmap_main_io {
   RoadMapIO io;
   RoadMapInput callback;

   int io_id;						// If is not valid < 0
   pthread_t handler_thread;
   pthread_mutex_t mutex;			// Mutex for the condition variable
   pthread_cond_t cond;				// Condition variable for the thread
   io_direction_type io_type;
   int pending_close;				// Indicator for the IO to be closed
   time_t start_time;
} roadmap_main_io;

/*
 * Kernel structure. Added in order to prevent includes problem
 *
 */
struct ucontext {
	unsigned long	  uc_flags;
	struct ucontext  *uc_link;
	stack_t		  uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t	  uc_sigmask;
	/* Allow for uc_sigmask growth.  Glibc uses a 1024-bit sigset_t.  */
	int		  unused[32 - (sizeof (sigset_t) / sizeof (int))];
	/* Last for extensibility.  Eight byte aligned because some
	   coprocessors require eight byte alignment.  */
 	unsigned long	  uc_regspace[128] __attribute__((__aligned__(8)));
};
typedef struct ucontext ucontext_t;

#define SOCKET_READ_SELECT_TIMEOUT {30, 0} // {sec, u sec}
#define SOCKET_WRITE_SELECT_TIMEOUT {30, 0} // {sec, u sec}
#define MESSAGE_DISPATCHER_TIMEOUT {10, 0} // {sec, nano sec}

#define CRASH_DUMP_ADDR_NUM		200		/* The number of addresses to dump when crash is occured */
#define CRASH_DUMP_ADDR_NUM_ON_SHUTDOWN      20      /* The number of addresses to dump when crash is occured on shutdown
                                                      * to not blow up the log on each exit
                                                     */
#define ROADMAP_MAX_IO 64
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];

static void roadmap_main_close_IO();
static void roadmap_main_reset_IO( roadmap_main_io *data );
static int roadmap_main_handler_post_wait( int ioMsg, roadmap_main_io *aIO );
static void roadmap_start_event (int event);
static void roadmap_main_signals_setup( void );
static void roadmap_main_termination_handler( int signum, siginfo_t *info, void *secret );
static void roadmap_main_dump_memory( volatile unsigned long *addr, int addr_num );
static const char *roadmap_main_get_signame( int sig );
static EVirtualKey roadmap_main_back_btn_handler( void );
static void roadmap_main_handle_menu();
static void on_device_event( device_event event, void* context );
static void roadmap_main_set_bottom_bar( BOOL force_hidden );
static BOOL roadmap_main_is_pending_close( roadmap_main_io *data );

//===============================================================

typedef enum
{
	_KEY_NOT_HANDLED = 0,
	_KEY_HANDLED = 1
} key_handling_status_type;

//////// Timers section ////////////////
#define ROADMAP_MAX_TIMER 24

typedef struct roadmap_main_timer {
   int id;
   RoadMapCallback callback;
}  roadmap_main_timer;
static roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];

////////////////////////////////////////

static char *RoadMapMainTitle = NULL;

static RoadMapKeyInput RoadMapMainInput = NULL;

static const int sgNavigateBarOffset = 50;


/*************************************************************************************************
 * roadmap_main_key_pressed()
 * Key pressed event handler. Android key codes mapping
 *
 */
int roadmap_main_key_pressed( int aKeyCode, int aIsSpecial, const char* aUtf8Bytes )
{

	char regular_key[2] = {0};
	EVirtualKey vk      = VK_None;
	const char* pCode;

	roadmap_log( ROADMAP_DEBUG, "Key event. Code %d, Is special %d, UTF8 : %s", aKeyCode, aIsSpecial, aUtf8Bytes );

	// First of all handle virtual keys
	switch( aKeyCode )
	{
		case _andr_keycode_dpad_center	  :
		case _andr_keycode_enter		  :
		{
			regular_key[0] = ENTER_KEY;
			roadmap_keyboard_handler__key_pressed( regular_key, KEYBOARD_ASCII );
			return _KEY_HANDLED;
		}
		case _andr_keycode_del			  :
		{
			regular_key[0] = BACKSPACE;
			roadmap_keyboard_handler__key_pressed( regular_key, KEYBOARD_ASCII );
			return _KEY_HANDLED;
		}
		case _andr_keycode_call           : vk = VK_Call_Start;     break;
		case _andr_keycode_menu			  : roadmap_main_handle_menu(); break;
		case _andr_keycode_soft_left      : break;
		case _andr_keycode_home			  :	break;
		case _andr_keycode_return		  :
		case _andr_keycode_soft_right     :	{ vk = roadmap_main_back_btn_handler(); break; };
		case _andr_keycode_dpad_up        : vk = VK_Arrow_up;       break;
		case _andr_keycode_dpad_down      : vk = VK_Arrow_down;     break;
		case _andr_keycode_dpad_left      : vk = VK_Arrow_left;     break;
		case _andr_keycode_dpad_right     : vk = VK_Arrow_right;    break;
		case _andr_keycode_camera         : break;
		case _andr_keycode_search         : roadmap_search_menu();  break;
		case _andr_keycode_end_call       : roadmap_main_exit();     return _KEY_NOT_HANDLED;
		default:
			break;
	}

	// Handle virtual key if necessary
	if( vk != VK_None )
	{
		regular_key[0] = ( char ) ( vk & 0xFF );
		regular_key[1] = '\0';
		if ( roadmap_keyboard_handler__key_pressed( regular_key, KEYBOARD_VIRTUAL_KEY ) )
		{
			return _KEY_HANDLED;
		}
	}
	// Handle non-special keys (having utf8)
	if ( !aIsSpecial )	// Not a special key
	{
		// Handle regular keys. pCode - pointer to the null terminated bytes
		// of the utf character
		if( roadmap_keyboard_handler__key_pressed( aUtf8Bytes, KEYBOARD_ASCII ) )
		{
			return _KEY_HANDLED;
		}
	}
	// Any other case
	return _KEY_NOT_HANDLED;
}

/*************************************************************************************************
 * void roadmap_main_android_os_build_sdk_version( int aVersion )
 * Sets the current os build sdk version
 *
 */
void roadmap_main_set_build_sdk_version( int aVersion )
{
	sgAndroidBuildSdkVersion = aVersion;
}

/*************************************************************************************************
 * int roadmap_main_set_build_sdk_version( void )
 * Gets the current os build sdk version
 *
 */
int roadmap_main_get_build_sdk_version( void )
{
	return sgAndroidBuildSdkVersion;
}

/*************************************************************************************************
 * BOOL roadmap_main_mtouch_supported( void )
 * Indicates if multitouch supported
 *
 */
BOOL roadmap_main_mtouch_supported( void )
{
   return ( sgAndroidBuildSdkVersion >= ANDROID_OS_VER_ECLAIR );
}


/*************************************************************************************************
 * void roadmap_main_set_device_name( const char* device_name )
 * Sets the current device name
 *
 */
void roadmap_main_set_device_name( const char* device_name )
{
	strncpy( sgAndroidDeviceName, device_name, sizeof( sgAndroidDeviceName )-1 );
}

/*************************************************************************************************
 * void roadmap_main_set_device_model( const char* model )
 * Sets the current device model
 *
 */
void roadmap_main_set_device_model( const char* model )
{
   strncpy( sgAndroidDeviceModel, model, sizeof( sgAndroidDeviceModel )-1 );
}

/*************************************************************************************************
 * void roadmap_main_set_device_manufacturer( const char* manufacturer )
 * Sets the current device manufacturer
 *
 */
void roadmap_main_set_device_manufacturer( const char* manufacturer )
{
   strncpy( sgAndroidDeviceManufacturer, manufacturer, sizeof( sgAndroidDeviceManufacturer )-1 );
}


/*************************************************************************************************
 * const char* roadmap_main_get_device_name( void )
 * Returns the device name
 *
 */
const char* roadmap_main_get_device_name( void )
{
	return sgAndroidDeviceName;
}

/*************************************************************************************************
 * void roadmap_main_show_contacts( void )
 * Requests the system to show the contacts activity
 *
 */
void roadmap_main_show_contacts( void )
{
   FreeMapNativeManager_ShowContacts();
}

/*************************************************************************************************
 * int LogResult( int aVal, int aVerbose, const char *aStrPrefix)
 * Logs the system api call error if occurred
 * aVal - the tested call return value
 * aVerbose - the log severity
 * level, source and line are hidden in the ROADMAP_DEBUG, INFO etc macros
 */
int LogResult( int aVal, const char *aStrPrefix, int level, char *source, int line )
{
	if ( aVal == 0 )
		return 0;
	if ( aVal == -1 )
		aVal = errno;

	roadmap_log( level, source, line, "%s (Thread %d). Error %d: %s", aStrPrefix, pthread_self(), aVal, strerror( aVal ) );

	return aVal;
}


/*************************************************************************************************
 * roadmap_main_handler_post_wait( int ioMsg, roadmap_main_io *aIO )
 * Causes calling thread to wait on the condition variable for this IO
 * Returns error code
 */
static int roadmap_main_handler_post_wait( int ioMsg, roadmap_main_io *aIO )
{
	int waitRes, retVal = 0;
	struct timespec sleepWaitFor;
    struct timespec sleepTimeOut;

	sleepTimeOut = ( struct timespec ) MESSAGE_DISPATCHER_TIMEOUT;

    // Compute the timeout
	clock_gettime( CLOCK_REALTIME, &sleepWaitFor );
	sleepWaitFor.tv_sec += sleepTimeOut.tv_sec;
	sleepWaitFor.tv_nsec += sleepTimeOut.tv_nsec;

	retVal = pthread_mutex_lock( &aIO->mutex );
	if ( LogResult( retVal, "Mutex lock failed", ROADMAP_WARNING ) )
	{
		return retVal;
	}

	// POST THE MESSAGE TO THE MAIN LOOP
	// The main application thread has its message loop implemented
	// at the Java side calling JNI functions for handling
	// Supposed to be very small and efficient code.
	// Called inside the mutex to prevent signaling before waitin
	// TODO :: Check possibility to implement this message queue at the lower level
	// TODO :: Start POSIX thread as the main thread
	FreeMapNativeManager_PostNativeMessage( ioMsg );

	// Waiting for the callback to finish
	retVal = pthread_cond_timedwait( &aIO->cond, &aIO->mutex, &sleepWaitFor );
	if ( LogResult( retVal, "Condition wait", ROADMAP_WARNING ) )
	{
		pthread_mutex_unlock( &aIO->mutex );
		return retVal;
	}

	retVal = pthread_mutex_unlock( &aIO->mutex );
	if ( LogResult( retVal, "Mutex unlock failed", ROADMAP_WARNING ) )
	{
		return retVal;
	}


	return retVal;
}

/*************************************************************************************************
 * BOOL roadmap_main_invalidate_pending_close( roadmap_main_io *data )
 * Thread safe, invalidating the IO pending for close
 * Returns TRUE if the IO was invalidated
 */
static BOOL roadmap_main_invalidate_pending_close( roadmap_main_io *data )
{
	BOOL res = FALSE;
	pthread_mutex_lock( &data->mutex );
	if ( data->pending_close )
	{
		data->pending_close = 0;
		res = TRUE;
	}
	pthread_mutex_unlock( &data->mutex );

	return res;
}

/*************************************************************************************************
 * BOOL roadmap_main_is_pending_close( roadmap_main_io *data )
 * Thread safe, reading the IO pending close
 * (Necessary for the non-atomic integer reads (M-Core cache implementations) and optimizer caching
 */
static BOOL roadmap_main_is_pending_close( roadmap_main_io *data )
{
	BOOL res = FALSE;
	pthread_mutex_lock( &data->mutex );
	res = data->pending_close;
	pthread_mutex_unlock( &data->mutex );

	return res;
}

/*************************************************************************************************
 * roadmap_main_socket_handler()
 * Socket handler thread body. Polling on the file descriptor.
 * ( Blocks with timeout )
 * Posts message to the main thread in case of file descriptor change
 * Waits on the condition variable before continue polling
 */
static void *roadmap_main_socket_handler( void* aParams )
{
	// IO data
	roadmap_main_io *data = (roadmap_main_io*) aParams;
	RoadMapIO *io = &data->io;
	int io_id = data->io_id;
	// Sockets data
	int fd = roadmap_net_get_fd(io->os.socket);
	fd_set fdSet;
	struct timeval selectReadTO = SOCKET_READ_SELECT_TIMEOUT;
	struct timeval selectWriteTO = SOCKET_WRITE_SELECT_TIMEOUT;
	int retVal, ioMsg;
    const char *handler_dir;

	// Empty the set
	FD_ZERO( &fdSet );

	handler_dir = ( data->io_type == _IO_DIR_WRITE ) ? "WRITE" : "READ";
//	roadmap_log( ROADMAP_INFO, "Starting the %s socket handler %d for socket %d IO ID %d", handler_dir, pthread_self(), io->os.socket, data->io_id );
	// Polling loop
	while( !roadmap_main_invalidate_pending_close( data ) &&
			IO_VALID( data->io_id ) &&
			( io->subsystem != ROADMAP_IO_INVALID ) &&
			!ANDR_APP_SHUTDOWN_FLAG )
	{
		// Add the file descriptor to the set if necessary
		if ( !FD_ISSET( fd, &fdSet ) )
		{
//			roadmap_log( ROADMAP_DEBUG, "Thread %d. Calling FD_SET for FD: %d", pthread_self(), fd );
			FD_SET( fd, &fdSet );
		}
		//selectTO = (struct timeval) SOCKET_SELECT_TIMEOUT;
		// Try to read or write from the file descriptor. fd + 1 is the max + 1 of the fd-s set!
		if ( data->io_type == _IO_DIR_WRITE )
		{
			retVal = select( fd+1, NULL, &fdSet, NULL, &selectWriteTO );
//			roadmap_log( ROADMAP_DEBUG, "Thread %d. IO %d WRITE : %d. FD: %d", pthread_self(), data->io_id, retVal, fd );
		}
		else
		{
			retVal = select( fd+1, &fdSet, NULL, NULL, &selectReadTO );
//			roadmap_log( ROADMAP_DEBUG, "Thread %d. IO %d READ : %d. FD: %d", pthread_self(), data->io_id, retVal, fd );
		}
		// Cancellation point - if IO is marked for invalidation - thread has to be closed
		if ( roadmap_main_invalidate_pending_close( data ) )
		{
//			roadmap_log( ROADMAP_INFO, "IO %d invalidated. Thread %d going to exit...", io_id, pthread_self() );
			break;
		}

		if ( retVal == 0 )
		{
//			roadmap_log( ROADMAP_INFO, "IO %d Socket %d timeout", data->io_id, io->os.socket );
		}
		if( retVal  < 0 )
		{
			// Error in file descriptor polling
			roadmap_log( ROADMAP_ERROR, "IO %d Socket %d error for thread %d: Error # %d, %s", data->io_id, io->os.socket, pthread_self(), errno, strerror( errno ) );
			break;
		}
		/* Check if this input was unregistered while we were
		 * sleeping.
		 */
		if ( io->subsystem == ROADMAP_IO_INVALID || !IO_VALID( data->io_id ) )
		{
			break;
		}

		if ( retVal && !ANDR_APP_SHUTDOWN_FLAG ) // Non zero if data available
		{
			ioMsg = ( data->io_id & ANDROID_MSG_ID_MASK ) | ANDROID_MSG_CATEGORY_IO_CALLBACK;
//			roadmap_log( ROADMAP_DEBUG, "Handling data for IO %d. Posting the message", data->io_id );
			// Waiting for the callback to finish its work
			if ( roadmap_main_handler_post_wait( ioMsg, data ) != 0 )
			{
				// The message dispatching is not performed !!!
				roadmap_log( ROADMAP_ERROR, "IO %d message dispatching failed. Thread %d going to finilize", data->io_id, pthread_self() );
				break;
			}
		}
    }
//	roadmap_log( ROADMAP_INFO, "Finalizing the %s socket handler %d for socket %d IO ID %d", handler_dir, pthread_self(), io->os.socket, data->io_id );
	ioMsg = ( data->io_id & ANDROID_MSG_ID_MASK ) | ANDROID_MSG_CATEGORY_IO_CLOSE;
	FreeMapNativeManager_PostNativeMessage( ioMsg );

	return (NULL);
}

/*************************************************************************************************
 * void roadmap_main_init_IO()
 * Initialization of the IO pool
 * void
 */
static void roadmap_main_init_IO()
{
	int i;
	int retVal;
	for( i = 0; i < ROADMAP_MAX_IO; ++i )
	{
		RoadMapMainIo[i].io_id = IO_INVALID_VAL;
		retVal = pthread_mutex_init( &RoadMapMainIo[i].mutex, NULL );
		LogResult( retVal, "Mutex init. ", ROADMAP_ERROR );
		retVal = pthread_cond_init( &RoadMapMainIo[i].cond, NULL );
		LogResult( retVal, "Condition init init. ", ROADMAP_ERROR );
		RoadMapMainIo[i].pending_close = 0;
    }
}


#define ROW_BUF_SIZE 64



/*************************************************************************************************
 * static void roadmap_main_dump_memory( volatile unsigned long *addr, int addr_num )
 * Dumps the memory starting from the provided address
 *
 */
static void roadmap_main_dump_memory( volatile unsigned long *addr, int addr_num )
{
	int i, cur_len = 0, available_size = 0;
	char row_buf[ROW_BUF_SIZE];
	volatile unsigned long *pAddr;

	roadmap_log_raw_data( "\n" );
	for(  i = 0; i < addr_num; ++i )
	{
		pAddr = addr + i;
		snprintf( row_buf, ROW_BUF_SIZE, "%08x : %08x\n", (unsigned int) pAddr, (unsigned int) *pAddr );
		roadmap_log_raw_data( row_buf );
	}
}


/*************************************************************************************************
 * const char *roadmap_main_get_signame( int sig )
 * Returns the literal interpretation of the signal number
 *
 */
static const char *roadmap_main_get_signame( int sig )
{
    switch(sig) {
    case SIGILL:     return "SIGILL";
    case SIGABRT:    return "SIGABRT";
    case SIGBUS:     return "SIGBUS";
    case SIGSTOP:    return "SIGSTOP";
    case SIGFPE:     return "SIGFPE";
    case SIGSEGV:    return "SIGSEGV";
    case SIGSTKFLT:  return "SIGSTKFLT";
    default:         return "?";
    }
}

/*************************************************************************************************
 * void roadmap_main_termination_handler( int signum, siginfo_t *info, void *secret )
 * The routine called by the signal. Prints the registers and stack. Gracefully exits the application
 *
 */
static void roadmap_main_termination_handler( int signum, siginfo_t *info, void *secret )
{
   static BOOL handling = FALSE;
	ucontext_t *uc = (ucontext_t *)secret;
	struct sigcontext* sig_ctx;
	unsigned long sig_fault_addr = 0xFFFFFFFF;
	int sig_errno = 0;
	int sig_code = 0;
	int i;
	int addr_num = CRASH_DUMP_ADDR_NUM;

	if ( handling )
	   return;

	handling = TRUE;

	if ( info )
	{
		 sig_fault_addr = (unsigned long) info->si_addr;
		 sig_errno = info->si_errno;
		 sig_code = info->si_code;
	}

   /*
     * AGA NOTE:
     * Temprorary solution. Prints limited stack on exit crash because of consistent crashes on milestone 2.1
     * Should be removed after full analysis
     */
    if ( ANDR_APP_SHUTDOWN_FLAG )
    {
       roadmap_log_raw_data( "!!!! Crash on application shutdown !!!!!! \n" );
       addr_num = CRASH_DUMP_ADDR_NUM_ON_SHUTDOWN;
    }

	roadmap_log( ROADMAP_ERROR, "Received signal %s %d. Fault address: %x. Error: %d. Code: %d  \n " \
			"********************************* STACK DUMP ******************************** ",
			roadmap_main_get_signame( signum ), signum, sig_fault_addr, sig_errno, sig_code );


	if ( !uc )
	{
		roadmap_log( ROADMAP_ERROR, "No context information in secret!" );
	}
	else
	{
		sig_ctx = &uc->uc_mcontext;

		if ( sig_ctx )
		{
			roadmap_log_raw_data_fmt(
					"Registers: \n " \
					"R0 %08x  R1 %08x  R2 %08x  R3 %08x  R4 %08x  R5 %08x  R6 %08x\n" \
					"R7 %08x  R8 %08x  R9 %08x  R10 %08x\n" \
					"fp %08x  ip %08x  sp %08x  lr %08x  pc %08x  cpsr %08x  fault %08x\n " \
					"Stack data: Flags: %x, Size: %d, SP: %x",
					sig_ctx->arm_r0, sig_ctx->arm_r1, sig_ctx->arm_r2, sig_ctx->arm_r3, sig_ctx->arm_r4,
					sig_ctx->arm_r5, sig_ctx->arm_r6, sig_ctx->arm_r7, sig_ctx->arm_r8, sig_ctx->arm_r9,
					sig_ctx->arm_r10,
					sig_ctx->arm_fp, sig_ctx->arm_ip, sig_ctx->arm_sp, sig_ctx->arm_lr, sig_ctx->arm_pc,
					sig_ctx->arm_cpsr, sig_ctx->fault_address, uc->uc_stack.ss_flags, uc->uc_stack.ss_size, uc->uc_stack.ss_sp );

	      roadmap_main_dump_memory( (volatile unsigned long*) sig_ctx->arm_sp, addr_num );
			fflush( NULL );
		}
		else
		{
			roadmap_log( ROADMAP_ERROR, "No signal context in secret!" );
		}

	}
	roadmap_main_exit();
}
/*************************************************************************************************
 * void roadmap_main_start_init()
 * Initialization of the application before start
 * void
 */
void roadmap_main_start_init()
{
	int i;

	roadmap_main_init_IO();

	roadmap_start_subscribe ( roadmap_start_event );

	roadmap_main_signals_setup();

}

/*************************************************************************************************
 * void roadmap_main_signal_setup()
 * Sets up the signals for handling
 *
 */

static void roadmap_main_signals_setup( void )
{
	int i;
	int sig_count = sizeof( sgHandledSignals ) / sizeof( sgHandledSignals[0] );
	struct sigaction new_action, old_action;

	/*
	 * Setting up the action params
	 */
    new_action.sa_sigaction = roadmap_main_termination_handler;
    sigemptyset (&new_action.sa_mask);
	 new_action.sa_flags = SA_SIGINFO;

	/*
	 * Set up the action for each signal in set
	 */
	for ( i = 0; i < sig_count; ++i )
	{
		sigaction( sgHandledSignals[i], &new_action, &old_action );
	}
}


void roadmap_main_new ( const char *title, int width, int height )
{

	roadmap_canvas_new();

	editor_main_set (1);
}


void roadmap_main_set_keyboard ( struct RoadMapFactoryKeyMap *bindings,
                                RoadMapKeyInput callback ) {
   RoadMapMainInput = callback;
}


void roadmap_main_add_separator (RoadMapMenu menu)
{
   roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
}


/*************************************************************************************************
 * roadmap_main_set_handler()
 * Creates the handler thread matching the io subsystem
 */
static void roadmap_main_set_handler( roadmap_main_io* aIO )
{
	RoadMapIO *io = &aIO->io;
	int retVal = 0;

	// Handler threads for the IO
	switch ( io->subsystem )
	{
	   case ROADMAP_IO_SERIAL:
		   roadmap_log ( ROADMAP_ERROR, "Serial IO is roadmap_main_set_handlernot supported" );
		   retVal = 0;
		  break;
	   case ROADMAP_IO_NET:
       {
    	    const char *handler_dir;
    		handler_dir = ( aIO->io_type == _IO_DIR_WRITE ) ? "WRITE" : "READ";

		   retVal = pthread_create( &aIO->handler_thread, NULL,
										roadmap_main_socket_handler, aIO );

		   roadmap_log ( ROADMAP_DEBUG, "Creating handler thread for the net subsystem. ID: %d. Socket: %d. Thread: %d. Direction: %s",
													   aIO->io_id, aIO->io.os.socket, aIO->handler_thread, handler_dir );
		   break;
	   }

	   case ROADMAP_IO_FILE:
		   roadmap_log ( ROADMAP_ERROR, "FILE IO is not supported" );
		   retVal = 0;
		   break;
	}
	// Check the handler creation status
	if ( retVal != 0 )
	{
	   aIO->io_id = IO_INVALID_VAL;
	   roadmap_log ( ROADMAP_ERROR, "handler thread creation has failed with error # %d, %s", errno, strerror( errno ) );
	}
}
/*************************************************************************************************
 * roadmap_main_set_input()
 * Allocates the entry for the io and creates the handler thread
 */
void roadmap_main_set_input ( RoadMapIO *io, RoadMapInput callback )
{
	int i;
	int retVal;
	int fd;

	roadmap_log( ROADMAP_DEBUG, "Setting the input for the subsystem : %d\n", io->subsystem );

   if (io->subsystem == ROADMAP_IO_NET) fd = roadmap_net_get_fd(io->os.socket);
   else fd = io->os.file; /* All the same on UNIX except sockets. */

	for (i = 0; i < ROADMAP_MAX_IO; ++i)
	{
		if ( !IO_VALID( RoadMapMainIo[i].io_id ) )
		{
			RoadMapMainIo[i].io = *io;
			RoadMapMainIo[i].callback = callback;
			RoadMapMainIo[i].io_type = _IO_DIR_READ;
			RoadMapMainIo[i].io_id = i;
			break;
		}
   }
   if ( i == ROADMAP_MAX_IO )
   {
      roadmap_log ( ROADMAP_FATAL, "Too many set input calls" );
      return;
   }

   // Setting the handler
   roadmap_main_set_handler( &RoadMapMainIo[i] );
}


/*************************************************************************************************
 * roadmap_main_set_output()
 * Allocates the entry for the io and creates the handler thread
 */
void roadmap_main_set_output ( RoadMapIO *io, RoadMapInput callback )
{

	int i;
	int fd;

	roadmap_log( ROADMAP_DEBUG, "Setting the output for the subsystem : %d\n", io->subsystem );

   if (io->subsystem == ROADMAP_IO_NET) fd = roadmap_net_get_fd(io->os.socket);
   else fd = io->os.file; /* All the same on UNIX except sockets. */

	for ( i = 0; i < ROADMAP_MAX_IO; ++i )
	{
		if ( !IO_VALID( RoadMapMainIo[i].io_id ) )
		{
			RoadMapMainIo[i].io = *io;
			RoadMapMainIo[i].callback = callback;
			RoadMapMainIo[i].io_id = i;
			RoadMapMainIo[i].io_type = _IO_DIR_WRITE;
			RoadMapMainIo[i].start_time = time(NULL);
			break;
		}
	}

	if ( i == ROADMAP_MAX_IO )
	{
	   roadmap_log ( ROADMAP_FATAL, "Too many set output calls" );
	   return;
	}

	// Setting the handler
	roadmap_main_set_handler( &RoadMapMainIo[i] );

}


RoadMapIO *roadmap_main_output_timedout(time_t timeout) {
   int i;

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if ( IO_VALID( RoadMapMainIo[i].io_id ) ) {
         if (RoadMapMainIo[i].start_time &&
               (timeout > RoadMapMainIo[i].start_time)) {
            return &RoadMapMainIo[i].io;
         }
      }
   }

   return NULL;
}


/*************************************************************************************************
 * roadmap_main_remove_input()
 * The IO entry is marked for deallocation. Will be available again when the thread will finish
 */
void roadmap_main_remove_input ( RoadMapIO *io )
{
	int i;

	for (i = 0; i < ROADMAP_MAX_IO; ++i)
	{

	   if ( IO_VALID( RoadMapMainIo[i].io_id ) && roadmap_io_same(&RoadMapMainIo[i].io, io))
	   {
			 // Cancel the thread and set is valid to zero
 			 roadmap_log( ROADMAP_DEBUG, "Canceling IO # %d thread %d\n", i, RoadMapMainIo[i].handler_thread );
			 RoadMapMainIo[i].io.os.file = -1;
			 pthread_mutex_lock( &RoadMapMainIo[i].mutex );
			 RoadMapMainIo[i].pending_close = 1;
			 RoadMapMainIo[i].start_time = 0;
			 pthread_mutex_unlock( &RoadMapMainIo[i].mutex );

			 roadmap_log( ROADMAP_DEBUG, "Removing the input id %d for the subsystem : %d \n", i, io->subsystem );
			 break;
	   }
	}

	if ( i == ROADMAP_MAX_IO )
	{
	   roadmap_log ( ROADMAP_ERROR, "Can't find input to remove!" );
	}
}


/*************************************************************************************************
 * roadmap_main_reset_IO()
 * Resetting the IO entry fields
 */
static void roadmap_main_reset_IO( roadmap_main_io *data )
{
	roadmap_log( ROADMAP_DEBUG, "Reset IO: %d \n", data->io_id );

	data->callback = NULL;
	data->handler_thread = 0;
	data->io.os.file = -1;
	data->io_id = IO_INVALID_VAL;
	data->pending_close = 0;
}

/*************************************************************************************************
 * roadmap_main_timeout()
 * Called upon timeout expiration - calls the timer callback matching the received id
 */
static int roadmap_main_timeout ( int aTimerId )
{
	roadmap_main_timer lTimer;

	if( aTimerId < 0 || aTimerId > ROADMAP_MAX_TIMER )
		return FALSE;

	// Getting the appropriate timer
	lTimer = RoadMapMainPeriodicTimer[aTimerId];

	RoadMapCallback callback = ( RoadMapCallback ) ( lTimer.callback );

	// roadmap_log( ROADMAP_DEBUG, "Timer expired for timer : %d. Callback : %x\n", aTimerId, callback );

	if (callback != NULL)
	{
	   // Apply the callback
	  (*callback) ();
	}

	return TRUE;
}

/*************************************************************************************************
 * roadmap_main_set_periodic()
 * Allocates entry in the Timers table and schedules the new timer using the JNI layer
 *
 */
void roadmap_main_set_periodic (int interval, RoadMapCallback callback)
{

   int index;
   int timerMessage; // Main loop message associated with this timer
   struct roadmap_main_timer *timer = NULL;

   // roadmap_log( ROADMAP_INFO, "Setting the periodic timer. Callback address : %x\n", callback );
   if ( ANDR_APP_SHUTDOWN_FLAG )
	   return;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {
         return;
      }
      if (timer == NULL) {
         if (RoadMapMainPeriodicTimer[index].callback == NULL) {
            timer = RoadMapMainPeriodicTimer + index;
            timer->id = index;
         }
      }
   }


   if (timer == NULL) {
      roadmap_log (ROADMAP_FATAL, "Timer table saturated");
   }

   // Update the structure
   timer->callback = callback;
   // Update the Android layer
   timerMessage = ( 0xFFFF & timer->id ) | ANDROID_MSG_CATEGORY_TIMER;
   FreeMapNativeTimerManager_AddTask( timer->id, timerMessage, interval );

   // roadmap_log( ROADMAP_DEBUG, "The timer %d is set up successfully with interval %d. Callback: %x \n", timer->id, interval,  timer->callback );
}

/*************************************************************************************************
 * roadmap_main_remove_periodic()
 * Deallocates timer and cancels it using the JNI layer
 */

void roadmap_main_remove_periodic (RoadMapCallback callback) {

   int index;

   // roadmap_log( ROADMAP_DEBUG, "Removing the periodic timer. Callback address : %x\n", callback );

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {

          RoadMapMainPeriodicTimer[index].callback = NULL;
          // Update the Android layer - at this time it still be a call
          FreeMapNativeTimerManager_RemoveTask( index );
          // roadmap_log( ROADMAP_DEBUG, "Timer %d with callback address : %x is removed", index, callback );

          return;
      }
   }
   roadmap_log (ROADMAP_ERROR, "timer 0x%08x not found", callback);

}


/*************************************************************************************************
 * roadmap_main_message_dispatcher()
 * Main thread messages dispatching routine. Calls the appropriate callback
 * according to the message information
 */
// Main loop message dispatcher
void roadmap_main_message_dispatcher( int aMsg )
{
	//roadmap_log( ROADMAP_DEBUG, "Dispatching the message: %d", aMsg );
	// Dispatching process
	//roadmap_main_time_interval( 0 );
	if ( aMsg & ANDROID_MSG_CATEGORY_IO_CALLBACK )	// IO callback message type
	{
		int retVal;
		int indexIo = aMsg & ANDROID_MSG_ID_MASK;
		//roadmap_log( ROADMAP_DEBUG, "Dispatching the message for IO %d", indexIo );
		// Call the handler
		if ( IO_VALID( RoadMapMainIo[indexIo].io_id ) && RoadMapMainIo[indexIo].callback &&
				!roadmap_main_is_pending_close( &RoadMapMainIo[indexIo] ) )
		{
			RoadMapIO *io = &RoadMapMainIo[indexIo].io;

			RoadMapMainIo[indexIo].callback( io );
 		    roadmap_log( ROADMAP_DEBUG, "Callback %x. IO %d", RoadMapMainIo[indexIo].callback, RoadMapMainIo[indexIo].io_id );

			// Send the signal to the thread if the IO is valid
			roadmap_log( ROADMAP_INFO, "Signaling thread %d", RoadMapMainIo[indexIo].handler_thread );
			retVal = pthread_mutex_lock( &RoadMapMainIo[indexIo].mutex );
			LogResult( retVal, "Mutex lock failed", ROADMAP_WARNING );

			retVal = pthread_cond_signal( &RoadMapMainIo[indexIo].cond );
			LogResult( retVal, "Condition wait", ROADMAP_INFO );

			pthread_mutex_unlock( &RoadMapMainIo[indexIo].mutex );
			LogResult( retVal, "Condition unlock failed", ROADMAP_WARNING );
		}
		else
		{
			roadmap_log( ROADMAP_INFO, "The IO %d is undefined (Index: %d). Dispatching is impossible for message %d",RoadMapMainIo[indexIo].io_id, indexIo, aMsg );
		}
	}
	// IO close message type - the thread was finalized
	if ( aMsg & ANDROID_MSG_CATEGORY_IO_CLOSE )
	{
		int indexIo = aMsg & ANDROID_MSG_ID_MASK;
		roadmap_main_reset_IO( &RoadMapMainIo[indexIo] );
	}
	if ( aMsg & ANDROID_MSG_CATEGORY_TIMER )	// Timer message type
	{
		int indexTimer = aMsg & ANDROID_MSG_ID_MASK;
		//roadmap_log( ROADMAP_DEBUG, "Dispatching the message for Timer %d", indexTimer );
		// Handle timeout
		roadmap_main_timeout( indexTimer );
	}
	// Menu events message type
	if ( aMsg & ANDROID_MSG_CATEGORY_MENU )	// Timer message type
	{
		int itemId = aMsg & ANDROID_MSG_ID_MASK;
		roadmap_androidmenu_handler( itemId );
	}
}

void roadmap_main_set_status (const char *text) {}


void roadmap_main_flush (void)
{
	// Signal the java layer to update the screen
	FreeMapNativeManager_Flush();
}

/*************************************************************************************************
 * roadmap_main_exit()
 * Starts the shutdown process
 *
 */
void roadmap_main_exit (void)
{
   if ( !ANDR_APP_SHUTDOWN_FLAG )
   {
      roadmap_log( ROADMAP_WARNING, "Exiting the application\n" );

      ANDR_APP_SHUTDOWN_FLAG = 1;
      // Pass the control over the shutdown process to the java layer
      FreeMapNativeManager_ShutDown();
   }
   /*
    * AGA NOTE: Not supposed to arrive to this point. We have System.exit() call in Java layer
    * Just in case that something was wrong we are here
    *
    */
   exit( 0 );
}
/*************************************************************************************************
 * roadmap_main_exit()
 * Starts the shutdown process
 *
 */
void roadmap_main_shutdown()
{
	roadmap_log( ROADMAP_INFO, "Native shutdown start" );
	// Free the resources
	roadmap_start_exit ();

	// Free the canvas resources
	roadmap_log( ROADMAP_WARNING, "Closing the canvas" );

#ifdef AGG
	roadmap_canvas_agg_close();
#endif

	// Free the JNI resourses
	roadmap_log( ROADMAP_WARNING, "Freeing the JNI objects" );
	CloseJNIObjects();

	roadmap_log( ROADMAP_WARNING, "Closing the IO" );

	// Close the mutexes and conditions
	roadmap_main_close_IO();

	roadmap_log( ROADMAP_WARNING, "Native shutdown end" );
}


/*************************************************************************************************
 * int roadmap_main_time_interval( int aCallIndex )
 * For index 0 - returns the current time in milliseconds.
 * For index 1 - reutrns the interval from the last call with index 0
 *
 */
inline int roadmap_main_time_interval( int aCallIndex, int aPrintFlag )
{
	static struct timeval last_time = {0,-1};
	int delta = -1;
	struct timezone tz;
	struct timeval cur_time;
	// Get the current time
	if ( gettimeofday( &cur_time, &tz ) == -1 )
	{
		roadmap_log( ROADMAP_ERROR, "Error in obtaining current time\n" );
		return -1;
	}
	if ( aCallIndex == 0 )
	{
		last_time = cur_time;
		delta = 0;
		if ( aPrintFlag )
			roadmap_log( ROADMAP_WARNING, "Benchmark. Starting ... Thread: %d", pthread_self() );
	}
	if ( aCallIndex == 1 )
	{
		if ( last_time.tv_usec == - 1 )
			return 0;

		delta = ( cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000 ) -
					( last_time.tv_sec * 1000 + last_time.tv_usec / 1000 );
		if ( aPrintFlag )
		{
			roadmap_log( ROADMAP_WARNING, "Benchmark. Interval: %d msec. Thread: %d", delta, pthread_self() );
//			if ( delta > 200 )
//				roadmap_log( ROADMAP_WARNING, "WARNING. PERFORMANCE LEAK!" );

		}
	}

	return delta;
}

/*************************************************************************************************
 * int roadmap_main_time_msec()
 * Returns time in millisec
 *
 *
 */
long roadmap_main_time_msec()
{
	static struct timeval last_time = {0,-1};
	struct timezone tz;
	struct timeval cur_time;
	long val;
	// Get the current time
	if ( gettimeofday( &cur_time, &tz ) == -1 )
	{
		roadmap_log( ROADMAP_ERROR, "Error in obtaining current time\n" );
		return -1;
	}
	val = ( cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000 );
	return val;
}

/*************************************************************************************************
 * void roadmap_main_close_IO()
 * Deallocate IO associated resources
 *
 */
static void roadmap_main_close_IO()
{
	int i;
	int retVal;

	for (i = 0; i < ROADMAP_MAX_IO; ++i)
	{
		pthread_cond_destroy( &RoadMapMainIo[i].cond );
		pthread_mutex_destroy( &RoadMapMainIo[i].mutex );
    }
}

/*************************************************************************************************
 * void roadmap_main_minimize( void )
 * Minimize the window. Calls the Android layer through JNI
 *
 */
void roadmap_gui_minimize( void )
{
	FreeMapNativeManager_MinimizeApplication( -1 );
}

/*************************************************************************************************
 * void roadmap_main_minimize( void )
 * Minimize the window. Calls the Android layer through JNI
 *
 */
void roadmap_gui_maximize( void )
{
	FreeMapNativeManager_MaximizeApplication();
}
/*************************************************************************************************
 * void on_auto_hide_dialog_close( int exit_code, void* context )
 * Auto hide dialog on close callback
 *
 */
static void on_auto_hide_dialog_close( int exit_code, void* context )
{
}
/*************************************************************************************************
 * void roadmap_main_minimize( void )
 * Minimize button callback
 *
 */
void roadmap_main_minimize( void )
{

   auto_hide_dlg(on_auto_hide_dialog_close);
}

/*************************************************************************************************
 * EVirtualKey roadmap_main_back_btn_handler( void )
 * Handles the Android back button
 *
 */
static EVirtualKey roadmap_main_back_btn_handler( void )
{
	EVirtualKey vk = VK_None;
	if ( !ssd_dialog_is_currently_active() )
	{
		roadmap_main_minimize();
		if ( !roadmap_screen_refresh() )
			roadmap_screen_redraw();
	}
	else
	{
		vk = VK_Softkey_right;
	}
	return vk;
}


/*************************************************************************************************
 * void roadmap_start_event (int event)
 * Start event hanler
 *
 */
static void roadmap_start_event (int event) {
   switch (event) {
	   case ROADMAP_START_INIT:
	   {
	#ifdef FREEMAP_IL
		  editor_main_check_map ();
	#endif
		  roadmap_device_events_register( on_device_event, NULL);
		  roadmap_main_set_bottom_bar( TRUE );
		  roadmap_androidbrowser_init();
		  break;
	   }
   }
}


/*************************************************************************************************
 * static void roadmap_main_handle_menu()
 * Handle the android menu key
 *
 */
static void roadmap_main_handle_menu()
{
	if ( roadmap_bottom_bar_shown() )
	{
		roadmap_bottom_bar_hide();
	}
	else
	{
		roadmap_bottom_bar_show();
	}
	navigate_bar_resize();
	if ( !roadmap_screen_refresh() )
	   roadmap_screen_redraw();
}

/*************************************************************************************************
 * static void on_device_event( device_event event, void* context )
 * Device event handler
 *
 */
static void on_device_event( device_event event, void* context )
{
   if( device_event_window_orientation_changed == event )
   {
	   roadmap_main_set_bottom_bar( FALSE );
   }
}

/*************************************************************************************************
 * static void roadmap_main_set_bottom_bar( void )
 * Sets the bottom bar depending on the screen orientation
 *
 */
static void roadmap_main_set_bottom_bar( BOOL force_hidden )
{
   if ( force_hidden )
   {
	   roadmap_bottom_bar_hide();
   }
   navigate_bar_resize();
   if ( !roadmap_screen_refresh() )
	   roadmap_screen_redraw();
}

void roadmap_main_set_first_run( BOOL value )
{
	sgFirstRun = value;
}

BOOL roadmap_horizontal_screen_orientation()
{
	return roadmap_canvas_is_landscape();
}

void roadmap_main_show_gps_disabled_warning()
{
	//roadmap_messagebox_custom( "Your phone's GPS is  turned OFF", "Waze requires GPS connection. Please turn on your GPS from Menu > Settings > Security & Location > Enable GPS satellites", 21, "#ffffff", 17, NULL );
	roadmap_messagebox_timeout( "Your phone's GPS is  turned OFF", "Waze requires GPS connection. Please turn on your GPS from Menu > Settings > Security & Location > Enable GPS satellites", 5 );
}


RoadMapMenu roadmap_main_new_menu (void) { return NULL; }


void roadmap_main_free_menu (RoadMapMenu menu) {}


void roadmap_main_add_menu (RoadMapMenu menu, const char *label) {}


void roadmap_main_add_menu_item (RoadMapMenu menu,
                                 const char *label,
                                 const char *tip,
                                 RoadMapCallback callback) {}


void roadmap_main_popup_menu (RoadMapMenu menu, int x, int y) {}

void roadmap_main_set_window_size( void *w, int width, int height ) {}

void roadmap_main_toggle_full_screen (void) {}

void roadmap_main_add_tool (const char *label,
                            const char *icon,
                            const char *tip,
                            RoadMapCallback callback) {}


void roadmap_main_add_tool_space (void) {}


void roadmap_main_add_canvas (void) {}


void roadmap_main_add_status (void)
{
// TODO:: Check the necessity
}

void roadmap_main_show (void) {}

void roadmap_main_set_cursor (int cursor) {}
