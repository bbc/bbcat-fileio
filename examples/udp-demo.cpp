
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

// for system detection
#include <bbcat-base/OSCompiler.h>

#ifndef TARGET_OS_WINDOWS
// for usleep()
#include <unistd.h>
#endif

#include <vector>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-base/UDPSocket.h>
#include <bbcat-base/Thread.h>
#include <bbcat-base/LockFreeBuffer.h>

using namespace bbcat;

// ensure the version numbers of the linked libraries and registered
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_base_version);

typedef struct
{
  uint64_t             ticks;
  std::vector<uint8_t> data;
} PACKET;
  
typedef struct
{
  UDPSocket              udp;
  LockFreeBuffer<PACKET> packets;
} THREAD_CONTEXT;
  
// thread routine
void *receivethread(Thread& thread, void *arg)
{
  THREAD_CONTEXT& context = *(THREAD_CONTEXT *)arg;
  UDPSocket& udp = context.udp;

  printf("Thread started\n");

  while (!thread.StopRequested())
  {
    PACKET *packet;
    sint_t bytes;

    // wait for UDP data
    if (udp.wait(200))
    {
      // find out if bytes available
      if ((bytes = udp.recv(NULL, 0)) > 0)
      {
        // try to get a writable buffer
        if ((packet = context.packets.GetWriteBuffer()) != NULL)
        {
          sint_t bytes1;

          // ensure buffer is big enough for the data
          if (packet->data.size() < (size_t)bytes) packet->data.resize(bytes);

          // save time of receive
          packet->ticks = GetNanosecondTicks();
          
          // receive data
          if ((bytes1 = udp.recv(&packet->data[0], bytes)) == bytes)
          {
            // increment write index
            context.packets.IncrementWrite();
          }
          else
          {
            fprintf(stderr, "Failed to receive %d bytes from UDP socket (received %d bytes)\n", bytes, bytes1);
            thread.Stop(false); // MUST use false here otherwise the thread will hang waiting for itself to complete!
          }
        }
        else printf("No data slots available!\n");
      }
      else
      {
        // probably socket closed
        fprintf(stderr, "Socket error\n");
        thread.Stop(false); // MUST use false here otherwise the thread will hang waiting for itself to complete!
      }
    }
  }

  printf("Thread complete\n");
  
  return NULL;
}

// CTRL-C handling
bool quit = false;
void chkabort(int sig)
{
  (void)sig;
  quit = true;
}

int main(int argc, char *argv[])
{
  uint_t port = 4244, buflen = 100;

  // allow CTRL-C to gracefully stop program
  signal(SIGINT, chkabort);
  
  // print library versions (the actual loaded versions, if dynamically linked)
  printf("Versions:\n%s\n", LoadedVersions::Get().GetVersionsList().c_str());

  if ((argc >= 2) && ((strcmp(argv[1], "-help") == 0) || (strcmp(argv[1], "-h") == 0)))
  {
    fprintf(stderr, "Usage: udp-demp [-p <udp-port>] [-l <buffer-len>]\n");
    exit(1);
  }

  int i;
  for (i = 1; i < argc; i++)
  {
    if      (strcmp(argv[i], "-p") == 0) port   = atoi(argv[++i]);
    else if (strcmp(argv[i], "-l") == 0) buflen = atoi(argv[++i]);
    else
    {
      fprintf(stderr, "Unrecognized option '%s'\n", argv[i]);
    }    
  }
  
  THREAD_CONTEXT context;
  if (context.udp.bind(port))
  {
    Thread th;
    
    // set number of queued packets for thread
    context.packets.Resize(buflen);

    if (th.Start(&receivethread, &context))
    {
      while (!quit && th.IsRunning() && !th.HasCompleted())
      {
        PACKET *packet;

        // see if there is a packet available
        if ((packet = context.packets.GetReadBuffer()) != NULL)
        {
          // data available
          printf("Packet received at %sns: %u bytes (delay %sns)\n", StringFrom(packet->ticks, "016x").c_str(), (uint_t)packet->data.size(), StringFrom(GetNanosecondTicks() - packet->ticks, "016x").c_str());

          context.packets.IncrementRead();
        }
        else
        {
          // nothing to read
          usleep(10000);
        }
      }

      th.Stop();
    }
    else fprintf(stderr, "Failed to start thread: %s\n", strerror(errno));
  }
  else fprintf(stderr, "Unable to bind UDP socket to port %u: %s\n", port, strerror(errno));
  
  return 0;
}
