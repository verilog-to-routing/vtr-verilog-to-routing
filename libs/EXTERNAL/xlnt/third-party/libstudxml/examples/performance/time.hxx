// file      : examples/performance/time.hxx
// copyright : not copyrighted - public domain

#ifndef TIME_HXX
#define TIME_HXX

#include <iosfwd> // std::ostream&

namespace os
{
  class time
  {
  public:
    class failed {};

    // Create a time object representing the current time.
    //
    time ();

    time (unsigned long long nsec)
    {
      sec_  = static_cast<unsigned long> (nsec / 1000000000ULL);
      nsec_ = static_cast<unsigned long> (nsec % 1000000000ULL);
    }

    time (unsigned long sec, unsigned long nsec)
    {
      sec_  = sec;
      nsec_ = nsec;
    }

  public:
    unsigned long
    sec () const
    {
      return sec_;
    }

    unsigned long
    nsec () const
    {
      return nsec_;
    }

  public:
    class overflow {};
    class underflow {};

    time
    operator+= (time const& b)
    {
      unsigned long long tmp = 0ULL + nsec_ + b.nsec_;

      sec_ += static_cast<unsigned long> (b.sec_ + tmp / 1000000000ULL);
      nsec_ = static_cast<unsigned long> (tmp % 1000000000ULL);

      return *this;
    }

    time
    operator-= (time const& b)
    {
      if (*this < b)
        throw underflow ();

      sec_ -= b.sec_;

      if (nsec_ < b.nsec_)
      {
        --sec_;
        nsec_ += 1000000000ULL - b.nsec_;
      }
      else
        nsec_ -= b.nsec_;

      return *this;
    }

    friend time
    operator+ (time const& a, time const& b)
    {
      time r (a);
      r += b;
      return r;
    }

    friend time
    operator- (time const& a, time const& b)
    {
      time r (a);
      r -= b;
      return r;
    }

    friend bool
    operator < (time const& a, time const& b)
    {
      return (a.sec_ < b.sec_) || (a.sec_ == b.sec_ && a.nsec_ < b.nsec_);
    }

  private:
    unsigned long sec_;
    unsigned long nsec_;
  };

  std::ostream&
  operator<< (std::ostream&, time const&);
}

#endif // TIME_HXX
