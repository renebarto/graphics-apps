/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2017 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>

#include "rtString.h"
#include "rtRemote.h"
#include <pthread.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

#include <string>
#include <vector>

static pthread_mutex_t gMutex= PTHREAD_MUTEX_INITIALIZER;
static std::vector<std::string> gAvailablePipelines= std::vector<std::string>();
static bool gRunning= false;

static void signalHandler(int signum)
{
   printf("signalHandler: signum %d\n", signum);
	gRunning= false;
}

class rtCaptureRegistryObject : public rtObject
{
   public:
     rtCaptureRegistryObject()
     {
     }

     rtDeclareObject(rtCaptureRegistryObject, rtObject);     

     rtMethod1ArgAndNoReturn("registerPipeline", registerPipeline, rtString);
     rtMethod1ArgAndNoReturn("unregisterPipeline", unregisterPipeline, rtString);
     rtMethodNoArgAndReturn("getAvailablePipelines", getAvailablePipelines, rtString);

     rtError registerPipeline( rtString pipelineName );
     rtError unregisterPipeline( rtString pipelineName );
     rtError getAvailablePipelines( rtString &result );

     void validateAvailablePipelines();

   private:
};
rtDefineObject(rtCaptureRegistryObject, rtObject);
rtDefineMethod(rtCaptureRegistryObject, registerPipeline);
rtDefineMethod(rtCaptureRegistryObject, unregisterPipeline);
rtDefineMethod(rtCaptureRegistryObject, getAvailablePipelines);

rtError rtCaptureRegistryObject::registerPipeline( rtString pipelineName )
{
   bool found= false;
   std::string strNew= pipelineName.cString();
   pthread_mutex_lock( &gMutex );
   for ( std::vector<std::string>::iterator it= gAvailablePipelines.begin();
         it != gAvailablePipelines.end();
         ++it )
   {
      std::string str= (*it);
      if ( !str.compare( strNew ) )
      {
         found= true;
      }
   }
   if ( !found )
   {
      gAvailablePipelines.push_back( std::string(strNew) );
   }
   pthread_mutex_unlock( &gMutex );

   return RT_OK;
}

rtError rtCaptureRegistryObject::unregisterPipeline( rtString pipelineName )
{
   bool found= false;
   std::string strRemove= pipelineName.cString();
   pthread_mutex_lock( &gMutex );
   for ( std::vector<std::string>::iterator it= gAvailablePipelines.begin();
         it != gAvailablePipelines.end();
         ++it )
   {
      std::string str= (*it);
      if ( !str.compare( strRemove ) )
      {
         found= true;
         gAvailablePipelines.erase(it);
         break;
      }
   }
   pthread_mutex_unlock( &gMutex );

   return RT_OK;
}

rtError rtCaptureRegistryObject::getAvailablePipelines( rtString &result )
{
   int i;
   char work[16];

   result= "";

   validateAvailablePipelines();

   pthread_mutex_lock( &gMutex );
   snprintf( work, sizeof(work), "%d", gAvailablePipelines.size() );
   result.append(work);
   i= 0;
   for ( std::vector<std::string>::iterator it= gAvailablePipelines.begin();
         it != gAvailablePipelines.end();
         ++it )
   {
      std::string str= (*it);
      result.append(",");
      result.append(str.c_str());
      ++i;
   }
   pthread_mutex_unlock( &gMutex );

   return RT_OK;
}

void rtCaptureRegistryObject::validateAvailablePipelines()
{
   bool needAnotherPass;

   do
   {
      needAnotherPass= false;
      pthread_mutex_lock( &gMutex );
      for ( std::vector<std::string>::iterator it= gAvailablePipelines.begin();
            it != gAvailablePipelines.end();
            ++it )
      {
         int pid;
         std::string str= (*it);
         if ( sscanf( str.c_str(), "media-%d-", &pid ) == 1 )
         {
            int rc;
            char work[64];
            struct stat fileinfo;

            snprintf( work, sizeof(work), "/proc/%d", pid );
            rc= stat( work, &fileinfo );
            if ( rc )
            {
               gAvailablePipelines.erase(it);
               needAnotherPass= true;
               break;
            }
         }
      }
      pthread_mutex_unlock( &gMutex );
   }
   while( needAnotherPass );
}

int main( int argc, const char **argv )
{
   rtError rc;
   rtRemoteEnvironment *env= 0;

   printf("mediacapture-daemon v1.0\n");

   env= rtEnvironmentGetGlobal();

   rc= rtRemoteInit(env);
   if ( rc == RT_OK )
   {
      struct sigaction sigint;
      rtObjectRef registryObj;

      registryObj= new rtCaptureRegistryObject();
      if ( registryObj.ptr() )
      {
         rc= rtRemoteRegisterObject( env, "mediacaptureregistry", registryObj );
         printf("rtRemoteRegisterObject rc %d\n", rc);
         if ( rc != RT_OK )
         {
            printf("error: unable to register capture registry object: rc %d\n", rc);
         }

         sigint.sa_handler= signalHandler;
         sigemptyset(&sigint.sa_mask);
	      sigint.sa_flags= SA_RESETHAND;
         sigaction(SIGINT, &sigint, NULL);

         gRunning= true;
         while( gRunning )
         {
            rtRemoteProcessSingleItem( env );
            usleep( 10000 );
         }
      }
   }

   printf("meidacapture-daemon: exit\n");

   return 0;
}

