#include "global.h"

#include "internal.hpp"

/*------------------------------------------------------------------------*/

// Our random number generator is seeded by default (i.e., in the default
// constructor) with random seeds, which should be unique across machines,
// processes and time.  This makes this code below rather operating system
// dependent.  We also use in essence defensive programming, overlaying
// several methods to get randomness since in the past we were bitten a
// couple of times (and got the same seeds).  Having several methods makes
// it also simpler to port randomly initializing seeds to different
// operating systems (even though currently it is only tested on Linux).
// This functionality is only used in the 'Mobical' model based tester at
// this point, since the main solver explicitly sets a random seed ('0' by
// default in 'options.hpp') and also currently only uses this seed in the
// local search procedure explicitly without using the default constructor.
// It is crucial for 'Mobical' to make sure that concurrent runs are really
// independent.

/*------------------------------------------------------------------------*/

// Uncomment the following definition to force printing the computed hash
// values for individual machine and process properties. This is only needed
// for testing, porting and debugging different ports of this seeding and
// hashing functions (uncomment and run 'mobical' for instance).

/*
#define DO_PRINT_HASH
*/

#ifdef DO_PRINT_HASH
#define PRINT_HASH(H) \
  do { \
    printf ("c PRINT_HASH %32s () = %020" PRIu64 "\n", __func__, H); \
    fflush (stdout); \
  } while (0)
#else
#define PRINT_HASH(...) \
  do { \
  } while (0)
#endif

/*------------------------------------------------------------------------*/

// This is Linux specific but if '/var/lib/dbus/machine-id' does not exist
// does not have any effect.  TODO: add a similar machine identity hashing
// function for other operating systems (Windows and macOS).

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

static uint64_t hash_machine_identifier () {
  FILE *file = fopen ("/var/lib/dbus/machine-id", "r");
  uint64_t res = 0;
  if (file) {
    char buffer[128];
    memset (buffer, 0, sizeof buffer);
    size_t bytes = fread (buffer, 1, sizeof buffer - 1, file);
    CADICAL_assert (bytes);
    fclose (file);
    if (bytes && bytes < sizeof buffer) {
      buffer[bytes] = 0;
      res = hash_string (buffer);
    }
  }
  PRINT_HASH (res);
  return res;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

/*------------------------------------------------------------------------*/

// On our Linux cluster where we used an NFS mounted root disk the
// 'machine-id' above (even on a locally mounted '/var' disk on each node)
// was copied from '/etc/machine-id' which was shared among all nodes
// (before figuring this out and fixing it). Thus the main idea of getting
// different hash values through this machine identifier machines did not
// work.  As an additional measure to increase the possibility to get
// different seeds we are now also using network addresses (explicitly).

#if !defined(_WIN32) && !defined(__wasm)

extern "C" {
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
}

#endif

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

static uint64_t hash_network_addresses () {
  uint64_t res = 0;

  // We still need to properly port this to Windows, but since accessing the
  // IP address is only required for better randomization during testing
  // (running 'mobical' on a cluster for instance) it is not crucial unless
  // you really need to run 'mobical' on a Windows cluster where each node
  // has identical IP addresses.

#if !defined(_WIN32) && !defined(__wasm)
  struct ifaddrs *addrs;
  if (!getifaddrs (&addrs)) {
    for (struct ifaddrs *addr = addrs; addr; addr = addr->ifa_next) {
      if (!addr->ifa_addr)
        continue;
      const int family = addr->ifa_addr->sa_family;
      if (family == AF_INET || family == AF_INET6) {
        const int size = (family == AF_INET) ? sizeof (struct sockaddr_in)
                                             : sizeof (struct sockaddr_in6);
        char buffer[128];
        if (!getnameinfo (addr->ifa_addr, size, buffer, sizeof buffer, 0, 0,
                          NI_NUMERICHOST)) {
          uint64_t tmp = hash_string (buffer);
#ifdef DO_PRINT_HASH
          printf ("c PRINT_HASH %35s = %020" PRIu64 "\n", buffer, tmp);
          fflush (stdout);
#endif
          res ^= tmp;
          res *= 10000000000000000051ul;
        }
      }
    }
    freeifaddrs (addrs);
  }
#endif

  PRINT_HASH (res);
  return res;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

/*------------------------------------------------------------------------*/

// Hash the current wall-clock time in seconds.

extern "C" {
#include <time.h>
}

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

static uint64_t hash_time () {
  uint64_t res = ::time (0);
  PRINT_HASH (res);
  return res;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

/*------------------------------------------------------------------------*/

// Hash the process identified.

extern "C" {
#include <sys/types.h>
#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
}

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

static uint64_t hash_process () {
  uint64_t res = getpid ();
  PRINT_HASH (res);
  return res;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

/*------------------------------------------------------------------------*/

// Hash the current number of clock cycles.

#include <ctime>

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

static uint64_t hash_clock_cycles () {
  uint64_t res = std::clock ();
  PRINT_HASH (res);
  return res;
}

} // namespace CaDiCaL

/*------------------------------------------------------------------------*/

namespace CaDiCaL {

Random::Random () : state (1) {
  add (hash_machine_identifier ());
  add (hash_network_addresses ());
  add (hash_clock_cycles ());
  add (hash_process ());
  add (hash_time ());
#ifdef DO_PRINT_HASH
  printf ("c PRINT_HASH %32s    = %020" PRIu64 "\n", "combined", state);
  fflush (stdout);
#endif
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
