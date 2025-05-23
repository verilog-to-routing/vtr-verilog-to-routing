#ifndef _file_hpp_INCLUDED
#define _file_hpp_INCLUDED

#include "global.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#ifndef CADICAL_NDEBUG
#include <climits>
#endif

/*------------------------------------------------------------------------*/
#ifdef WIN32
#define cadical_putc_unlocked putc
#define cadical_getc_unlocked getc
#else
#ifndef NUNLOCKED
#define cadical_putc_unlocked putc_unlocked
#define cadical_getc_unlocked getc_unlocked
#else
#define cadical_putc_unlocked putc
#define cadical_getc_unlocked getc
#endif
#endif
/*------------------------------------------------------------------------*/

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// Wraps a 'C' file 'FILE' with name and supports zipped reading and writing
// through 'popen' using external helper tools.  Reading has line numbers.
// Compression and decompression relies on external utilities, e.g., 'gzip',
// 'bzip2', 'xz', and '7z', which should be in the 'PATH'.

struct Internal;

class File {

  Internal *internal;
#if !defined(CADICAL_QUIET) || !defined(CADICAL_NDEBUG)
  bool writing;
#endif

  int close_file; // need to close file (1=fclose, 2=pclose, 3=pipe)
  int child_pid;
  FILE *file;
  char *_name;
  uint64_t _lineno;
  uint64_t _bytes;

  File (Internal *, bool, int, int, FILE *, const char *);

  static FILE *open_file (Internal *, const char *path, const char *mode);
  static FILE *read_file (Internal *, const char *path);
  static FILE *write_file (Internal *, const char *path);

  static void split_str (const char *, std::vector<char *> &);
  static void delete_str_vector (std::vector<char *> &);

  static FILE *open_pipe (Internal *, const char *fmt, const char *path,
                          const char *mode);
  static FILE *read_pipe (Internal *, const char *fmt, const int *sig,
                          const char *path);
#if !defined(_WIN32) && !defined(__wasm)
  static FILE *write_pipe (Internal *, const char *fmt, const char *path,
                           int &child_pid);
#endif

public:
  static char *find_program (const char *prg); // search in 'PATH'
  static bool exists (const char *path);       // file exists?
  static bool writable (const char *path);     // can write to that file?
  static size_t size (const char *path);       // file size in bytes

  bool piping (); // Is opened file a pipe?

  // Does the file match the file type signature.
  //
  static bool match (Internal *, const char *path, const int *sig);

  // Read from existing file. Assume given name.
  //
  static File *read (Internal *, FILE *f, const char *name);

  // Open file from path name for reading (possibly through opening a pipe
  // to a decompression utility, based on the suffix).
  //
  static File *read (Internal *, const char *path);

  // Same for writing as for reading above.
  //
  static File *write (Internal *, FILE *, const char *name);
  static File *write (Internal *, const char *path);

  ~File ();

  // Using the 'unlocked' versions here is way faster but
  // not thread safe if the same file is used by different
  // threads, which on the other hand currently is impossible.

  int get () {
    CADICAL_assert (!writing);
    int res = cadical_getc_unlocked (file);
    if (res == '\n')
      _lineno++;
    if (res != EOF)
      _bytes++;
    return res;
  }

  bool put (char ch) {
    CADICAL_assert (writing);
    if (cadical_putc_unlocked (ch, file) == EOF)
      return false;
    _bytes++;
    return true;
  }

  bool endl () { return put ('\n'); }

  bool put (unsigned char ch) {
    CADICAL_assert (writing);
    if (cadical_putc_unlocked (ch, file) == EOF)
      return false;
    _bytes++;
    return true;
  }

  bool put (const char *s) {
    for (const char *p = s; *p; p++)
      if (!put (*p))
        return false;
    return true;
  }

  bool put (int lit) {
    CADICAL_assert (writing);
    if (!lit)
      return put ('0');
    else if (lit == -2147483648) {
      CADICAL_assert (lit == INT_MIN);
      return put ("-2147483648");
    } else {
      char buffer[11];
      int i = sizeof buffer;
      buffer[--i] = 0;
      CADICAL_assert (lit != INT_MIN);
      unsigned idx = abs (lit);
      while (idx) {
        CADICAL_assert (i > 0);
        buffer[--i] = '0' + idx % 10;
        idx /= 10;
      }
      if (lit < 0 && !put ('-'))
        return false;
      return put (buffer + i);
    }
  }

  bool put (int64_t l) {
    CADICAL_assert (writing);
    if (!l)
      return put ('0');
    else if (l == INT64_MIN) {
      CADICAL_assert (sizeof l == 8);
      return put ("-9223372036854775808");
    } else {
      char buffer[21];
      int i = sizeof buffer;
      buffer[--i] = 0;
      CADICAL_assert (l != INT64_MIN);
      uint64_t k = l < 0 ? -l : l;
      while (k) {
        CADICAL_assert (i > 0);
        buffer[--i] = '0' + k % 10;
        k /= 10;
      }
      if (l < 0 && !put ('-'))
        return false;
      return put (buffer + i);
    }
  }

  bool put (uint64_t l) {
    CADICAL_assert (writing);
    if (!l)
      return put ('0');
    else {
      char buffer[22];
      int i = sizeof buffer;
      buffer[--i] = 0;
      while (l) {
        CADICAL_assert (i > 0);
        buffer[--i] = '0' + l % 10;
        l /= 10;
      }
      return put (buffer + i);
    }
  }

  const char *name () const { return _name; }
  uint64_t lineno () const { return _lineno; }
  uint64_t bytes () const { return _bytes; }

  void connect_internal (Internal *i) { internal = i; }
  bool closed () { return !file; }

  void close (bool print = false);
  void flush ();
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
