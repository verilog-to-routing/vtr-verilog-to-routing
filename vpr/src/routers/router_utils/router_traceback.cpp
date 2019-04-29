
/* Standard header files required go first */

/* EXTERNAL library header files go second*/

/* VPR header files go then */
#include "router_traceback.h"
#include "router_common.h"
#include "vpr_types.h" //For t_trace

/* Finally we include global variables */

/* use a namespace for the shared functions used by routers */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */

namespace router {


t_traceback::t_traceback(const t_traceback& other) {

  VTR_ASSERT((other.head == nullptr && other.tail == nullptr) 
          || (other.head && other.tail && other.tail->next == nullptr));

  //Deep-copy of traceback
  t_trace* prev = nullptr;
  for (t_trace* other_curr = other.head; other_curr; other_curr = other_curr->next) {
    //VTR_LOG("Copying trace %p node: %d switch: %d\n", other_curr, other_curr->index, other_curr->iswitch);
    /* TODO: move alloc_trace_data() here, since the contructor requires it ?*/
    t_trace* curr = alloc_trace_data(); 

    curr->index = other_curr->index;
    curr->iswitch = other_curr->iswitch;

    if (prev) {
      prev->next = curr;
    } else {
      head = curr;
    }
    prev = curr;
  }

  //We may have gotten the last t_trace element from a free list,
  //make sure it's next ptr gets set to null
  if (prev) {
    prev->next = nullptr; 
  }

  tail = prev;
}

t_traceback::~t_traceback() {
  /* TODO: move free_traceback() here, since the decontructor requires it ?*/
  free_traceback(head);
}

t_traceback::t_traceback(t_traceback&& other)
  : t_traceback() {
  //Copy-swap
  swap(*this, other);
}

t_traceback t_traceback::operator=(t_traceback other) {
  //Copy-swap
  swap(*this, other);
  return *this;
}

void swap(t_traceback& first, t_traceback& second) {
  using std::swap;

  swap(first.head, second.head);
  swap(first.tail, second.tail);
}

}/* end namespace router */
