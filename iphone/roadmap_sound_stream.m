/* roadmap_sound_stream.m - Play sound from url
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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
 *   See roadmap_sound_stream.h
 */

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_messagebox.h"
#include "roadmap_sound_stream.h"

#include <CFNetwork/CFHTTPMessage.h>
#include <CFNetwork/CFHTTPStream.h>
#include <CFNetwork/CFHTTPAuthentication.h>
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>


#define ROADMAP_SOUND_STREAM_UN @"waze-mobi" //TODO: hide this
#define ROADMAP_SOUND_STREAM_PW @"n8brp43j"

#define PRINTERROR(LABEL)	printf("%s err %4.4s %d\n", LABEL, &err, err)


static AudioFileStreamID af_parser; //TODO: rename
static CFReadStreamRef readStream; //TODO: rename
static CFHTTPMessageRef httpRequest = NULL;
static int parseFlag = kAudioFileStreamParseFlag_Discontinuity;
static int shouldEndThread = 0;
static int isStreamActive = 0;

void applyAuthentication ();
void cancelLoad ();
void stream_client_cb (CFReadStreamRef stream,
                       CFStreamEventType eventType,
                       void *clientCallBackInfo);


#define kNumAQBufs 32			// number of audio queue buffers we allocate
#define kAQBufSize 2048       // number of bytes in each audio queue buffer
#define kAQMaxPacketDescs 512	// number of packet descriptions in our array

struct MyData
{
	AudioFileStreamID audioFileStream;	// the audio file stream parser
   
	AudioQueueRef audioQueue;								// the audio queue
	AudioQueueBufferRef audioQueueBuffer[kNumAQBufs];		// audio queue buffers
	
	AudioStreamPacketDescription packetDescs[kAQMaxPacketDescs];	// packet descriptions for enqueuing audio
	
	unsigned int fillBufferIndex;	// the index of the audioQueueBuffer that is being filled
	size_t bytesFilled;				// how many bytes have been filled
	size_t packetsFilled;			// how many packets have been filled
   
	bool inuse[kNumAQBufs];			// flags to indicate that a buffer is still in use
	bool started;					// flag to indicate that the queue has been started
	bool failed;					// flag to indicate an error occurred
   bool paused;
   
	pthread_mutex_t mutex;			// a mutex to protect the inuse flags
	pthread_cond_t cond;			// a condition varable for handling the inuse flags
};
typedef struct MyData MyData;

static MyData* gMyData;




/////////////////////////////////////////////////
//Audio Queue

int MyFindQueueBuffer(MyData* myData, AudioQueueBufferRef inBuffer)
{
	for (unsigned int i = 0; i < kNumAQBufs; ++i) {
		if (inBuffer == myData->audioQueueBuffer[i]) 
			return i;
	}
	return -1;
}



void MyAudioQueueOutputCallback(	void*					inClientData, 
                                AudioQueueRef			inAQ, 
                                AudioQueueBufferRef		inBuffer)
{
	// this is called by the audio queue when it has finished decoding our data. 
	// The buffer is now free to be reused.
	MyData* myData = (MyData*)inClientData;
	unsigned int bufIndex = MyFindQueueBuffer(myData, inBuffer);
	
	// signal waiting thread that the buffer is free.
	pthread_mutex_lock(&myData->mutex);
	myData->inuse[bufIndex] = false;
	pthread_cond_signal(&myData->cond);
	pthread_mutex_unlock(&myData->mutex);
   
   if ((bufIndex < kNumAQBufs - 1 && !myData->inuse[bufIndex+1]) ||
       (bufIndex == kNumAQBufs - 1 && !myData->inuse[0])) {
      NSLog(@"discontinuity");
      parseFlag = kAudioFileStreamParseFlag_Discontinuity;
   }
   else
      parseFlag = 0;
}

void stopPlayback () {
   //TODO: any cleanup here...
   cancelLoad();
   AudioFileStreamClose(af_parser);
   AudioQueueDispose(gMyData->audioQueue, TRUE);
   gMyData->paused = false;
   gMyData->started = false;
   isStreamActive = 0;
}

void isRunningPropertyChanged (void                  *inUserData,
                               AudioQueueRef         inAQ,
                               AudioQueuePropertyID  inID)
{
   NSLog(@"Is running property");
   OSStatus status;
   UInt32 dataSize;
   UInt32 isRunning;
   
   status = AudioQueueGetPropertySize(inAQ, kAudioQueueProperty_IsRunning, &dataSize);
   status = AudioQueueGetProperty(inAQ, kAudioQueueProperty_IsRunning, &isRunning, &dataSize);
   
   if ((isRunning == 0) && isStreamActive){
      //NSLog(@"STOPPED");
      //stopPlayback();
      shouldEndThread = 1;
   }
}

OSStatus MyEnqueueBuffer(MyData* myData)
{
	OSStatus err = noErr;
	myData->inuse[myData->fillBufferIndex] = true;		// set in use flag
	
	// enqueue buffer
	AudioQueueBufferRef fillBuf = myData->audioQueueBuffer[myData->fillBufferIndex];
	fillBuf->mAudioDataByteSize = myData->bytesFilled;		
	err = AudioQueueEnqueueBuffer(myData->audioQueue, fillBuf, myData->packetsFilled, myData->packetDescs);
   
	if (err) { PRINTERROR("AudioQueueEnqueueBuffer"); myData->failed = true; return err; }
   
   // go to next buffer
	if (++myData->fillBufferIndex >= kNumAQBufs) myData->fillBufferIndex = 0;
   pthread_mutex_lock(&myData->mutex);
	myData->bytesFilled = 0;		// reset bytes filled
	myData->packetsFilled = 0;		// reset packets filled
   pthread_mutex_unlock(&myData->mutex);
     
	if (!myData->started) {		// start the queue if it has not been started already
		err = AudioQueueStart(myData->audioQueue, NULL);
      err = AudioQueueAddPropertyListener(myData->audioQueue, kAudioQueueProperty_IsRunning, isRunningPropertyChanged, NULL);
		if (err) { PRINTERROR("AudioQueueStart"); myData->failed = true; return err; }		
		myData->started = true;
      myData->paused = false;
		printf("started\n");
	}
  
   
	// wait until next buffer is not in use
	printf("->lock\n");
	pthread_mutex_lock(&myData->mutex); 
	while ((myData->inuse[myData->fillBufferIndex]) && !shouldEndThread) {
		printf("... WAITING ...\n");
		pthread_cond_wait(&myData->cond, &myData->mutex);
	}
	pthread_mutex_unlock(&myData->mutex);
	printf("<-unlock\n");
  
	return err;
}



/////////////////////////////////////////////////
//Audio file stream


void af_packets_cb (void                          *inClientData,
                    UInt32                        inNumberBytes,
                    UInt32                        inNumberPackets,
                    const void                    *inInputData,
                    AudioStreamPacketDescription  *inPacketDescriptions)
{
   //NSLog(@"af_packets_cb");
   //isDataParsed = 1;
   
  	// this is called by audio file stream when it finds packets of audio
	MyData* myData = (MyData*)inClientData;
	printf("got data.  bytes: %d  packets: %d\n", inNumberBytes, inNumberPackets);
   
	// the following code assumes we're streaming VBR data. for CBR data, you'd need another code branch here.
   
	for (int i = 0; i < inNumberPackets; ++i) {
		SInt64 packetOffset = inPacketDescriptions[i].mStartOffset;
		SInt64 packetSize   = inPacketDescriptions[i].mDataByteSize;
		
		// if the space remaining in the buffer is not enough for this packet, then enqueue the buffer.
		size_t bufSpaceRemaining = kAQBufSize - myData->bytesFilled;
		if (bufSpaceRemaining < packetSize) {
         //printf("af_packets_cb - No space in buffer, calling enqueue\n");
			MyEnqueueBuffer(myData);
		}
		
		// copy data to the audio queue buffer
		AudioQueueBufferRef fillBuf = myData->audioQueueBuffer[myData->fillBufferIndex];
		memcpy((char*)fillBuf->mAudioData + myData->bytesFilled, (const char*)inInputData + packetOffset, packetSize);
		// fill out packet description
		myData->packetDescs[myData->packetsFilled] = inPacketDescriptions[i];
		myData->packetDescs[myData->packetsFilled].mStartOffset = myData->bytesFilled;
		// keep track of bytes filled and packets filled
		myData->bytesFilled += packetSize;
		myData->packetsFilled += 1;
		
		// if that was the last free packet description, then enqueue the buffer.
		size_t packetsDescsRemaining = kAQMaxPacketDescs - myData->packetsFilled;
		if (packetsDescsRemaining == 0) {
			MyEnqueueBuffer(myData);
		}
	}
   
   //parseFlag = 0;
}


void af_property_cb (void                        *inClientData,
                     AudioFileStreamID           inAudioFileStream,
                     AudioFileStreamPropertyID   inPropertyID,
                     UInt32                      *ioFlags)
{
   //NSLog(@"af_property_cb");
   roadmap_main_set_cursor(ROADMAP_CURSOR_NORMAL);
  
   // this is called by audio file stream when it finds property values
	MyData* myData = (MyData*)inClientData;
	OSStatus err = noErr;
   
	printf("found property '%c%c%c%c'\n", (inPropertyID>>24)&255, (inPropertyID>>16)&255, (inPropertyID>>8)&255, inPropertyID&255);
   
	switch (inPropertyID) {
		case kAudioFileStreamProperty_ReadyToProducePackets :
		{
			// the file stream parser is now ready to produce audio packets.
			// get the stream format.
			AudioStreamBasicDescription asbd;
			UInt32 asbdSize = sizeof(asbd);
			err = AudioFileStreamGetProperty(inAudioFileStream, kAudioFileStreamProperty_DataFormat, &asbdSize, &asbd);
			if (err) { PRINTERROR("get kAudioFileStreamProperty_DataFormat"); myData->failed = true; break; }
			
			// create the audio queue
			err = AudioQueueNewOutput(&asbd, MyAudioQueueOutputCallback, myData, NULL, NULL, 0, &myData->audioQueue);
			if (err) { PRINTERROR("AudioQueueNewOutput"); myData->failed = true; break; }
			
			// allocate audio queue buffers
			for (unsigned int i = 0; i < kNumAQBufs; ++i) {
				err = AudioQueueAllocateBuffer(myData->audioQueue, kAQBufSize, &myData->audioQueueBuffer[i]);
				if (err) { PRINTERROR("AudioQueueAllocateBuffer"); myData->failed = true; break; }
			}
         
			// get the cookie size
			UInt32 cookieSize;
			Boolean writable;
			err = AudioFileStreamGetPropertyInfo(inAudioFileStream, kAudioFileStreamProperty_MagicCookieData, &cookieSize, &writable);
			if (err) { PRINTERROR("info kAudioFileStreamProperty_MagicCookieData"); break; }
			printf("cookieSize %d\n", cookieSize);
         
			// get the cookie data
			void* cookieData = calloc(1, cookieSize);
			err = AudioFileStreamGetProperty(inAudioFileStream, kAudioFileStreamProperty_MagicCookieData, &cookieSize, cookieData);
			if (err) { PRINTERROR("get kAudioFileStreamProperty_MagicCookieData"); free(cookieData); break; }
         
			// set the cookie on the queue.
			err = AudioQueueSetProperty(myData->audioQueue, kAudioQueueProperty_MagicCookie, cookieData, cookieSize);
			free(cookieData);
			if (err) { PRINTERROR("set kAudioQueueProperty_MagicCookie"); break; }
			break;
		}
      case kAudioFileStreamProperty_DataFormat:
      {
         // get the stream format.
			AudioStreamBasicDescription asbd;
			UInt32 asbdSize = sizeof(asbd);
			err = AudioFileStreamGetProperty(inAudioFileStream, kAudioFileStreamProperty_DataFormat, &asbdSize, &asbd);
         break;
      }
	}
}



/////////////////////////////////////////////////
//Network

void cancelLoad ()
{
   if (readStream) {
      CFReadStreamClose(readStream);
      readStream = NULL;
   }
}


void loadRequest ()
{
   cancelLoad();

   
   readStream = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, httpRequest);
   
   CFReadStreamSetProperty(readStream,
                           kCFStreamPropertyHTTPShouldAutoredirect,
                           kCFBooleanTrue);
   CFReadStreamOpen(readStream);
   
   
   CFStreamClientContext context = {0, NULL, NULL, NULL, NULL};
   CFReadStreamSetClient(readStream,
                         kCFStreamEventHasBytesAvailable | kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered,
                         stream_client_cb,
                         &context);
   CFReadStreamScheduleWithRunLoop(readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);

      
}

void* startStreamThread (void* data)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   
   double resolution = 0.1;
   
   loadRequest();
   
   while (!shouldEndThread) {
      NSDate* next = [NSDate dateWithTimeIntervalSinceNow:resolution];
      
      [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                               beforeDate:next];
   }
   
   stopPlayback();
   
   shouldEndThread = 0;
   
   //NSLog(@"Exit stream thread");
   [pool release];
   
   return 0;
}


void resumeWithCredentials(CFHTTPAuthenticationRef authentication,
                           CFDictionaryRef credentials) {
   
   // Apply whatever credentials we've built up to the old request
   
   if (!CFHTTPMessageApplyCredentialDictionary(httpRequest, authentication,
                                               
                                               credentials, NULL)) {
      
      //errorOccurredLoadingImage();
      
   } else {
      
      // Now that we've updated our request, retry the load
      
      loadRequest();
      
   }
   
}

void applyAuthentication ()
{
   static CFHTTPAuthenticationRef authentication;
   static CFMutableDictionaryRef credentials = NULL;
   CFStringRef user;
   CFStringRef pass;
   CFStringRef domain;
   
   if (!credentials) {
      credentials = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
      
      CFDictionarySetValue(credentials, kCFHTTPAuthenticationUsername,
                           (CFStringRef)ROADMAP_SOUND_STREAM_UN);
      
      CFDictionarySetValue(credentials, kCFHTTPAuthenticationPassword,                   
                           (CFStringRef)ROADMAP_SOUND_STREAM_PW);
   }
   
   if (!authentication) { 
      CFHTTPMessageRef responseHeader =
               (CFHTTPMessageRef) CFReadStreamCopyProperty(readStream,                                           
                                                  kCFStreamPropertyHTTPResponseHeader);

      
      // Get the authentication information from the response.
      authentication = CFHTTPAuthenticationCreateFromResponse(NULL, responseHeader);
      CFRelease(responseHeader);
   }
   
   CFStreamError err;
   
   if (!authentication) {
      // the newly created authentication object is bad, must return
      return;  
   } else if (!CFHTTPAuthenticationIsValid(authentication, &err)) {
      
      // destroy authentication and credentials
      if (credentials) {
         CFRelease(credentials);
         credentials = NULL;
      }
      
      CFRelease(authentication);
      authentication = NULL;
      
      
      
      // check for bad credentials (to be treated separately)
      if (err.domain == kCFStreamErrorDomainHTTP &&
          (err.error == kCFStreamErrorHTTPAuthenticationBadUserName
           || err.error == kCFStreamErrorHTTPAuthenticationBadPassword))
      {  
         //retryAuthorizationFailure(&authentication);
         return;
      } else {
         //errorOccurredLoadingImage(err); 
      }   
   }
   else {
      cancelLoad();
      if (credentials) {
         resumeWithCredentials(authentication, credentials);
      }
      
      // are a user name & password needed?
      else if (CFHTTPAuthenticationRequiresUserNameAndPassword(authentication))
      {
         CFStringRef realm = NULL;
         CFURLRef url = CFHTTPMessageCopyRequestURL(httpRequest);    
         
         // check if you need an account domain so you can display it if necessary
         if (!CFHTTPAuthenticationRequiresAccountDomain(authentication)) {   
            realm = CFHTTPAuthenticationCopyRealm(authentication);  
         }
         
         // ...prompt user for user name (user), password (pass)
         
         // and if necessary domain (domain) to give to the server...
         
         // Guarantee values
         
         if (!user) user = (CFStringRef)ROADMAP_SOUND_STREAM_UN;
         if (!pass) pass = (CFStringRef)ROADMAP_SOUND_STREAM_PW;
         
         CFDictionarySetValue(credentials, kCFHTTPAuthenticationUsername, user);
         CFDictionarySetValue(credentials, kCFHTTPAuthenticationPassword, pass);

         // Is an account domain needed? (used currently for NTLM only)
         
         if (CFHTTPAuthenticationRequiresAccountDomain(authentication)) { 
            if (!domain) domain = (CFStringRef)@"";
            CFDictionarySetValue(credentials,             
                                 kCFHTTPAuthenticationAccountDomain, domain);
         }
         if (realm) CFRelease(realm);
         
         CFRelease(url);
      }
      else {
         resumeWithCredentials(authentication, credentials);         
      }
   }
}


void stream_client_cb (CFReadStreamRef stream,
                       CFStreamEventType eventType,
                       void *clientCallBackInfo)
{
   UInt8 buffer[2048];
   OSStatus err;
   CFIndex bytes_read;
   
   if (readStream != stream)
      return;
   
   //TODO: increment traffic count
   
   CFHTTPMessageRef http_response = (CFHTTPMessageRef)CFReadStreamCopyProperty(stream, kCFStreamPropertyHTTPResponseHeader);
   UInt32 http_err = CFHTTPMessageGetResponseStatusCode(http_response);
   if (http_err == 401/* || http_err == 407*/)
      applyAuthentication();
   else if (http_err != 200) {
      //NSLog(@"Error connecting to stream server");
      cancelLoad();
      return;
   }

   
   if (eventType & kCFStreamEventErrorOccurred) {
      //NSLog(@"stream_client_cb: kCFStreamEventErrorOccurred");
      return;
   }
   
   if (eventType & kCFStreamEventEndEncountered) {
      //NSLog(@"stream_client_cb: kCFStreamEventEndEncountered");

      if (gMyData->bytesFilled > 0) {
         MyEnqueueBuffer(gMyData);
         err = AudioQueueFlush(gMyData->audioQueue);
         err = AudioQueueStop(gMyData->audioQueue, false);
      }
      return;
   }
   
   if (eventType & kCFStreamEventHasBytesAvailable) {
      //NSLog(@"stream_client_cb: kCFStreamEventHasBytesAvailable");

      bytes_read = CFReadStreamRead (stream, buffer, sizeof(buffer));
      if (bytes_read == -1)
         return;
      
      err = AudioFileStreamParseBytes (af_parser, bytes_read, buffer, parseFlag);
      if (err)
         //NSLog(@"error AudioFileStreamParseBytes");
      return;
   }
}


int roadmap_sound_stream_play (const char *url, const char *username, const char *password) {
   

   CFURLRef cfURL;
   CFStringRef cfStr;
   static int initialized = 0;
   int i;
   
   if (!initialized) {
      // allocate a struct for storing our state
      gMyData = (MyData*)calloc(1, sizeof(MyData));
      
      // initialize a mutex and condition so that we can b\ on buffers in use.
      pthread_mutex_init(&gMyData->mutex, NULL);
      pthread_cond_init(&gMyData->cond, NULL);
      
      initialized = 1;
   }
   
   if (isStreamActive) {
      //roadmap_messagebox("Info", "Already playing");
      //roadmap_sound_stream_pause();
      return 0;
   }
   
   isStreamActive = 1;
   roadmap_main_set_cursor(ROADMAP_CURSOR_WAIT);
   
   gMyData->bytesFilled = 0;		// reset bytes filled
	gMyData->packetsFilled = 0;		// reset packets filled
   
   for (i = 0; i < kNumAQBufs; ++i) {
      gMyData->inuse[i] = false;
   }
   
   cfStr = CFStringCreateWithCString (kCFAllocatorDefault, url, kCFStringEncodingUTF8);
   cfURL = CFURLCreateWithString(kCFAllocatorDefault, cfStr, NULL);
   CFRelease(cfStr);
   
   cfStr = CFSTR("GET");
   httpRequest =  CFHTTPMessageCreateRequest (kCFAllocatorDefault, cfStr, cfURL, kCFHTTPVersion1_1);
   CFRelease(cfURL);
   
   AudioFileStreamOpen(gMyData, af_property_cb, af_packets_cb, kAudioFileMP3Type, &af_parser);

   
   
   //Load as a new thread
   pthread_attr_t  attr;
   pthread_t       posixThreadID;
   int             returnVal;  
   
   
   returnVal = pthread_attr_init(&attr);
   assert(!returnVal);
   
   returnVal =  pthread_attr_setstacksize(&attr, 1048576);
   assert(!returnVal);
   
   returnVal = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   assert(!returnVal);
   
   int     threadError = pthread_create(&posixThreadID, &attr, &startStreamThread, NULL);
   
   returnVal = pthread_attr_destroy(&attr);
   assert(!returnVal);
   
   if (threadError != 0) {
      // Report an error.
   }
   
   
   return 0;
}


void roadmap_sound_stream_stop (void) {
   if (isStreamActive) {
      //stopPlayback();
      shouldEndThread = 1;
      pthread_cond_signal(&gMyData->cond);
   }
}



void roadmap_sound_stream_pause (void) {
   if (gMyData->paused) {
      AudioQueueStart(gMyData->audioQueue, NULL);
      gMyData->paused = false;
   } else {
      AudioQueuePause(gMyData->audioQueue);
      gMyData->paused = true;
   }
}

int roadmap_sound_stream_is_active (void) {
   return isStreamActive;
}
