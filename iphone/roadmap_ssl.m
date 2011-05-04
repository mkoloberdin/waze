/* roadmap_ssl.c - SSL connection for iPhone
 *
 * LICENSE:
 *
 *   Copyright 2011, Avi R.
 *   Copyright 2011, Waze Ltd
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
 */


#include <stdlib.h>

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_net.h"
#include "roadmap_ssl.h"

#import <CFNetwork/CFProxySupport.h>


struct roadmap_ssl_io_t {
   RoadMapSocket  s;
   void           *data;
   CFReadStreamRef readStream;
   CFWriteStreamRef writeStream;
   RoadMapNetSslConnectCallback onOpenCallback;
   RoadMapInput callback;
};


//////////////////////////////////////////////////////////////////////////////////////////////////
//void roadmap_ssl_set_secured_input (RoadMapIO *io, RoadMapInput callback, void *ssl_context) {
//   RoadMapSslIO ssl_io;
//   ssl_io->callback = callback;
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_ssl_read (void *context, void *buffer, int buffer_size) {
  RoadMapSslIO ssl_io = (RoadMapSslIO) context;
   int bytes_read = CFReadStreamRead (ssl_io->readStream, buffer, buffer_size);
   
   return bytes_read;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void read_stream_client_cb (CFReadStreamRef stream,
                                   CFStreamEventType eventType,
                                   void *clientCallBackInfo)
{
   //printf("read_stream_client_cb status: %d\n", CFReadStreamGetStatus(stream));
   if (eventType & kCFStreamEventOpenCompleted) {
      //CFStreamClientContext *context = (CFStreamClientContext*) clientCallBackInfo;
      //struct roadmap_main_ssl_io *io = clientCallBackInfo;//context->info;
      //NSLog(@"read_stream_client_cb: kCFStreamEventOpenCompleted");
      //      on_ssl_open_callback(io->s, io->data, io);
      //      free(io);
      //      return;
   }
   if (eventType & kCFStreamEventErrorOccurred) {
      CFErrorRef err = CFReadStreamCopyError(stream);
      if (err) {
         CFStringRef str = CFErrorCopyDescription(err);
         char buf[1024];
         
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "read_stream_client_cb: kCFStreamEventErrorOccurred description: '%s'", buf);
            CFRelease(str);
         }
         
         str = CFErrorCopyFailureReason (err);
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "read_stream_client_cb: kCFStreamEventErrorOccurred reason: '%s'", buf);
            CFRelease(str);
         }
         
         str = CFErrorCopyRecoverySuggestion (err);
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "read_stream_client_cb: kCFStreamEventErrorOccurred recovery: '%s'", buf);
            CFRelease(str);
         }
         
         CFRelease(err);
      }
      
      RoadMapSslIO io = clientCallBackInfo;//context->info;
      io->onOpenCallback(io->s, io->data, io, err_net_failed);
      roadmap_ssl_close(io);
      
      return;
   }
   
   if (eventType & kCFStreamEventEndEncountered) {
      //NSLog(@"read_stream_client_cb: kCFStreamEventEndEncountered");
      return;
   }
   
   if (eventType & kCFStreamEventHasBytesAvailable) {
      //NSLog(@"read_stream_client_cb: kCFStreamEventHasBytesAvailable");
      //      
      //UInt8 buffer[512];
      //            int bytes_read = CFReadStreamRead (stream, buffer, sizeof(buffer));
      //            if (bytes_read == -1)
      //               return;
      //      struct roadmap_main_ssl_io *io = clientCallBackInfo;//context->info;
      //on_ssl_read_callback(io->s, io->data, buffer, bytes_read);
      // io->callback();
      //      
      //      err = AudioFileStreamParseBytes (af_parser, bytes_read, buffer, parseFlag);
      //      if (err)
      //         //NSLog(@"error AudioFileStreamParseBytes");
      return;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void write_stream_client_cb (CFWriteStreamRef stream,
                                    CFStreamEventType eventType,
                                    void *clientCallBackInfo)
{
   //printf("write_stream_client_cb status: %d\n", CFReadStreamGetStatus(stream));
   if (eventType & kCFStreamEventOpenCompleted) {
      //NSLog(@"write_stream_client_cb: kCFStreamEventOpenCompleted");
      return;
   }
   if (eventType & kCFStreamEventErrorOccurred) {
      CFErrorRef err = CFWriteStreamCopyError(stream);
      if (err) {
         CFStringRef str = CFErrorCopyDescription(err);
         char buf[1024];
         
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "write_stream_client_cb: kCFStreamEventErrorOccurred description: '%s'", buf);
            CFRelease(str);
         }
         
         str = CFErrorCopyFailureReason (err);
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "write_stream_client_cb: kCFStreamEventErrorOccurred reason: '%s'", buf);
            CFRelease(str);
         }
         
         str = CFErrorCopyRecoverySuggestion (err);
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "write_stream_client_cb: kCFStreamEventErrorOccurred recovery: '%s'", buf);
            CFRelease(str);
         }
         
         CFRelease(err);
      }
      
      RoadMapSslIO io = clientCallBackInfo;//context->info;
      io->onOpenCallback(io->s, io->data, io, err_net_failed);
      roadmap_ssl_close(io);
      
      return;
   }
   
   if (eventType & kCFStreamEventEndEncountered) {
      //NSLog(@"write_stream_client_cb: kCFStreamEventEndEncountered");
      return;
   }
   
   if (eventType & kCFStreamEventCanAcceptBytes) {
      CFWriteStreamSetClient(stream, 0, NULL, NULL);
      //NSLog(@"write_stream_client_cb: kCFStreamEventCanAcceptBytes");
      RoadMapSslIO io = clientCallBackInfo;//context->info;
      io->onOpenCallback(io->s, io->data, io, succeeded);
      return;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_ssl_send (RoadMapSslIO io, const void *data, int length) {
   int res = CFWriteStreamWrite(io->writeStream, data, length);
   //printf("res: %d\n", res);
   if (res < 0) {
      CFErrorRef err = CFWriteStreamCopyError(io->writeStream);
      if (err) {
         CFStringRef str = CFErrorCopyDescription(err);
         char buf[1024];
         CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
         roadmap_log(ROADMAP_ERROR, "Error writing to SSL stream: '%s'", buf);
         CFRelease(str);
         CFRelease(err);
      }
   }
   
   return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_ssl_open (RoadMapSocket s, void *data, RoadMapNetSslConnectCallback callback) {
   RoadMapSslIO ssl_io;
   CFReadStreamRef readStream;
   CFWriteStreamRef writeStream;
   CFStreamCreatePairWithSocket(NULL, roadmap_net_get_fd(s), &readStream, &writeStream);
   if (!readStream || !writeStream) {
      roadmap_log(ROADMAP_ERROR, "CFStreamCreatePairWithSocket() failed");
      return 0;
   }
   //printf("open_ssl status: %d\n", CFReadStreamGetStatus(readStream));
   
   ssl_io = malloc(sizeof (*ssl_io));
   ssl_io->s = s;
   ssl_io->data = data;
   ssl_io->readStream = readStream;
   ssl_io->writeStream = writeStream;
   ssl_io->onOpenCallback = callback;
   CFStreamClientContext context = {0, ssl_io, NULL, NULL, NULL};
   CFReadStreamSetClient(readStream,
                         kCFStreamEventHasBytesAvailable | kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered
                         |kCFStreamEventOpenCompleted,
                         read_stream_client_cb,
                         &context);
   CFReadStreamScheduleWithRunLoop(readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   CFWriteStreamSetClient(writeStream,
                          kCFStreamEventCanAcceptBytes | kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered
                          |kCFStreamEventOpenCompleted,
                          write_stream_client_cb,
                          &context);
   CFWriteStreamScheduleWithRunLoop(writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   
   //Open read stream
   NSInputStream *inputStream = (NSInputStream *)readStream;
   [inputStream setProperty:NSStreamSocketSecurityLevelNegotiatedSSL forKey:NSStreamSocketSecurityLevelKey];
   
   if (!CFReadStreamOpen(readStream)) {
      roadmap_log(ROADMAP_ERROR, "CFReadStreamOpen() failed");
      return 0;
   }
   //printf("open_ssl status: %d\n", CFReadStreamGetStatus(readStream));
   
   //Open write stream
   NSOutputStream *outputStream = (NSOutputStream *)writeStream;
   [outputStream setProperty:NSStreamSocketSecurityLevelNegotiatedSSL forKey:NSStreamSocketSecurityLevelKey];
   
   if (!CFWriteStreamOpen(writeStream)) {
      roadmap_log(ROADMAP_ERROR, "CFWriteStreamOpen() failed");
      return 0;
   }
   //printf("open_ssl status: %d\n", CFWriteStreamGetStatus(writeStream));
   
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_ssl_close (void *context) {
   RoadMapSslIO ssl_io = (RoadMapSslIO) context;
   
   if (!context) {
      roadmap_log(ROADMAP_WARNING, "roadmap_ssl_close() - NULL context");
      return;
   }
   
   CFWriteStreamUnscheduleFromRunLoop(ssl_io->writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   CFWriteStreamClose (ssl_io->writeStream);
   CFRelease(ssl_io->writeStream);
   
   CFReadStreamUnscheduleFromRunLoop(ssl_io->readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   CFReadStreamClose (ssl_io->readStream);
   CFRelease(ssl_io->readStream);
   
   free (ssl_io);
}