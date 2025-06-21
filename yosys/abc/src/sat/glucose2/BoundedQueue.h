/***********************************************************************************[BoundedQueue.h]
 Glucose -- Copyright (c) 2009, Gilles Audemard, Laurent Simon
                CRIL - Univ. Artois, France
                LRI  - Univ. Paris Sud, France
 
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/


#ifndef BoundedQueue_h
#define BoundedQueue_h

#include "sat/glucose2/Vec.h"

ABC_NAMESPACE_CXX_HEADER_START

//=================================================================================================

namespace Gluco2 {

template <class T>
class bqueue {
    vec<T>  elems;
    int     first;
    int        last;
    uint64_t sumofqueue;
    int     maxsize;
    int     queuesize; // Number of current elements (must be < maxsize !)
    bool expComputed;
    double exp,value;
public:
 bqueue(void) : first(0), last(0), sumofqueue(0), maxsize(0), queuesize(0),expComputed(false) { } 
    
    void initSize(int size) {growTo(size);exp = 2.0/(size+1);} // Init size of bounded size queue
    
    void push(T x) {
      expComputed = false;
        if (queuesize==maxsize) {
            assert(last==first); // The queue is full, next value to enter will replace oldest one
            sumofqueue -= elems[last];
            if ((++last) == maxsize) last = 0;
        } else 
            queuesize++;
        sumofqueue += x;
        elems[first] = x;
        if ((++first) == maxsize) {first = 0;last = 0;}
    }

    T peek() { assert(queuesize>0); return elems[last]; }
    void pop() {sumofqueue-=elems[last]; queuesize--; if ((++last) == maxsize) last = 0;}
    
    uint64_t getsum() const {return sumofqueue;}
    unsigned int getavg() const {return (unsigned int)(sumofqueue/((uint64_t)queuesize));}
    int maxSize() const {return maxsize;}
    double getavgDouble() const {
      double tmp = 0;
      for(int i=0;i<elems.size();i++) {
        tmp+=elems[i];
      }
      return tmp/elems.size();
    }
    int isvalid() const {return (queuesize==maxsize);}
    
    void growTo(int size) {
        elems.growTo(size); 
        first=0; maxsize=size; queuesize = 0;last = 0;
        for(int i=0;i<size;i++) elems[i]=0; 
    }
    
    double getAvgExp() {
      if(expComputed) return value;
      double a=exp;
      value = elems[first];
      for(int i  = first;i<maxsize;i++) {
        value+=a*((double)elems[i]);
        a=a*exp;
      }
      for(int i  = 0;i<last;i++) {
        value+=a*((double)elems[i]);
        a=a*exp;
      }
      value = value*(1-exp)/(1-a);
      expComputed = true;
      return value;
      

    }
    void fastclear() {first = 0; last = 0; queuesize=0; sumofqueue=0;} // to be called after restarts... Discard the queue
    
    int  size(void)    { return queuesize; }

    void clear(bool dealloc = false)   { elems.clear(dealloc); first = 0; maxsize=0; queuesize=0;sumofqueue=0;}


};
}
//=================================================================================================

ABC_NAMESPACE_CXX_HEADER_END

#endif
