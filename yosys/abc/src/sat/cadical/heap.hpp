#ifndef _heap_hpp_INCLUDED
#define _heap_hpp_INCLUDED

#include "global.h"

#include "util.hpp" // Alphabetically after 'heap.hpp'.

#include <cassert>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

using namespace std;

// This is a priority queue with updates for unsigned integers implemented
// as binary heap.  We need to map integer elements added (through
// 'push_back') to positions on the binary heap in 'array'. This map is
// stored in the 'pos' array. This approach is really wasteful (at least in
// terms of memory) if only few and a sparse set of integers is added.  So
// it should not be used in this situation.  A generic priority queue would
// implement the mapping externally provided by another template parameter.
// Since we use 'UINT_MAX' as 'not contained' flag, we can only have
// 'UINT_MAX - 1' elements in the heap.

const unsigned invalid_heap_position = UINT_MAX;

template <class C> class heap {

  vector<unsigned> array; // actual binary heap
  vector<unsigned> pos;   // positions of elements in array
  C less;                 // less-than for elements

  // Map an element to its position entry in the 'pos' map.
  //
  unsigned &index (unsigned e) {
    if (e >= pos.size ())
      pos.resize (1 + (size_t) e, invalid_heap_position);
    unsigned &res = pos[e];
    CADICAL_assert (res == invalid_heap_position || (size_t) res < array.size ());
    return res;
  }

  bool has_parent (unsigned e) { return index (e) > 0; }
  bool has_left (unsigned e) {
    return (size_t) 2 * index (e) + 1 < size ();
  }
  bool has_right (unsigned e) {
    return (size_t) 2 * index (e) + 2 < size ();
  }

  unsigned parent (unsigned e) {
    CADICAL_assert (has_parent (e));
    return array[(index (e) - 1) / 2];
  }

  unsigned left (unsigned e) {
    CADICAL_assert (has_left (e));
    return array[2 * index (e) + 1];
  }

  unsigned right (unsigned e) {
    CADICAL_assert (has_right (e));
    return array[2 * index (e) + 2];
  }

  // Exchange elements 'a' and 'b' in 'array' and fix their positions.
  //
  void exchange (unsigned a, unsigned b) {
    unsigned &i = index (a), &j = index (b);
    swap (array[i], array[j]);
    swap (i, j);
  }

  // Bubble up an element as far as necessary.
  //
  void up (unsigned e) {
    unsigned p;
    while (has_parent (e) && less ((p = parent (e)), e))
      exchange (p, e);
  }

  // Bubble down an element as far as necessary.
  //
  void down (unsigned e) {
    while (has_left (e)) {
      unsigned c = left (e);
      if (has_right (e)) {
        unsigned r = right (e);
        if (less (c, r))
          c = r;
      }
      if (!less (e, c))
        break;
      exchange (e, c);
    }
  }

  // Very expensive checker for the main 'heap' invariant.  Can be enabled
  // to find violations of antisymmetry in the client implementation of
  // 'less' and as well of course bugs in this heap implementation.  It
  // should be enabled during testing applications of the heap.
  //
  void check () {
#if 0 // EXPENSIVE HEAP CHECKING IF ENABLED
#warning "expensive checking in heap enabled"
    CADICAL_assert (array.size () <= invalid_heap_position);
    for (size_t i = 0; i < array.size (); i++) {
      size_t l = 2*i + 1, r = 2*i + 2;
      if (l < array.size ()) CADICAL_assert (!less (array[i], array[l]));
      if (r < array.size ()) CADICAL_assert (!less (array[i], array[r]));
      CADICAL_assert (array[i] >= 0);
      {
        CADICAL_assert ((size_t) array[i] < pos.size ());
        CADICAL_assert (i == (size_t) pos[array[i]]);
      }
    }
    for (size_t i = 0; i < pos.size (); i++) {
      if (pos[i] == invalid_heap_position) continue;
      CADICAL_assert (pos[i] < array.size ());
      CADICAL_assert (array[pos[i]] == (unsigned) i);
    }
#endif
  }

public:
  heap (const C &c) : less (c) {}

  // Number of elements in the heap.
  //
  size_t size () const { return array.size (); }

  // Check if no more elements are in the heap.
  //
  bool empty () const { return array.empty (); }

  // Check whether 'e' is already in the heap.
  //
  bool contains (unsigned e) const {
    if ((size_t) e >= pos.size ())
      return false;
    return pos[e] != invalid_heap_position;
  }

  // Add a new (not contained) element 'e' to the heap.
  //
  void push_back (unsigned e) {
    CADICAL_assert (!contains (e));
    size_t i = array.size ();
    CADICAL_assert (i < (size_t) invalid_heap_position);
    array.push_back (e);
    index (e) = (unsigned) i;
    up (e);
    down (e);
    check ();
  }

  // Returns the maximum element in the heap.
  //
  unsigned front () const {
    CADICAL_assert (!empty ());
    return array[0];
  }

  // Removes the maximum element in the heap.
  //
  unsigned pop_front () {
    CADICAL_assert (!empty ());
    unsigned res = array[0], last = array.back ();
    if (size () > 1)
      exchange (res, last);
    index (res) = invalid_heap_position;
    array.pop_back ();
    if (size () > 1)
      down (last);
    check ();
    return res;
  }

  // Notify the heap, that evaluation of 'less' has changed for 'e'.
  //
  void update (unsigned e) {
    CADICAL_assert (contains (e));
    up (e);
    down (e);
    check ();
  }

  void clear () {
    array.clear ();
    pos.clear ();
  }

  void erase () {
    erase_vector (array);
    erase_vector (pos);
  }

  void shrink () {
    shrink_vector (array);
    shrink_vector (pos);
  }

  // Standard iterators 'inherited' from 'vector'.
  //
  typedef typename vector<unsigned>::iterator iterator;
  typedef typename vector<unsigned>::const_iterator const_iterator;
  iterator begin () { return array.begin (); }
  iterator end () { return array.end (); }
  const_iterator begin () const { return array.begin (); }
  const_iterator end () const { return array.end (); }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
