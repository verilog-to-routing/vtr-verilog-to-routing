#ifndef OPENFPGA_PORT_H
#define OPENFPGA_PORT_H

/********************************************************************
 * Include header files that are required by data structure declaration
 *******************************************************************/
#include <string>
#include <vector>

/* namespace openfpga begins */
namespace openfpga {

/* A basic port */
class BasicPort {
  public: /* Constructors */
    BasicPort();
    BasicPort(const char* name, const size_t& lsb, const size_t& msb);
    BasicPort(const char* name, const size_t& width);
    BasicPort(const std::string& name, const size_t& lsb, const size_t& msb);
    BasicPort(const std::string& name, const size_t& width);
  public: /* Overloaded operators */
    bool operator== (const BasicPort& portA) const;
    bool operator< (const BasicPort& portA) const;
  public: /* Accessors */
    size_t get_width() const; /* get the port width */
    size_t get_msb() const; /* get the LSB */
    size_t get_lsb() const; /* get the LSB */
    std::string get_name() const; /* get the name */
    bool is_valid() const; /* check if port size is valid > 0 */
    std::vector<size_t> pins() const; /* Make a range of the pin indices */
    bool mergeable(const BasicPort& portA) const; /* Check if a port can be merged with this port */
    bool contained(const BasicPort& portA) const; /* Check if a port is contained by this port */
    size_t get_origin_port_width() const;
  public: /* Mutators */
    void set(const BasicPort& basic_port); /* copy */
    void set_name(const std::string& name); /* set the port LSB and MSB */
    void set_width(const size_t& width); /* set the port LSB and MSB */
    void set_width(const size_t& lsb, const size_t& msb); /* set the port LSB and MSB */
    void set_lsb(const size_t& lsb);
    void set_msb(const size_t& msb);
    void expand(const size_t& width); /* Increase the port width */
    void revert(); /* Swap lsb and msb */
    bool rotate(const size_t& offset); /* rotate */
    bool counter_rotate(const size_t& offset); /* counter rotate */
    void reset(); /* Reset to initial port */
    void combine(const BasicPort& port); /* Combine two ports */
    void merge(const BasicPort& portA);
    void set_origin_port_width(const size_t& origin_port_width);
  private: /* internal functions */
    void make_invalid(); /* Make a port invalid */
  private: /* Internal Data */
    std::string name_; /* Name of this port */
    size_t msb_; /* Most Significant Bit of this port */
    size_t lsb_; /* Least Significant Bit of this port */
    size_t origin_port_width_; /* Original port width of a port, used by traceback port conversion history  */
};

/* Configuration ports:
 * 1. reserved configuration port, which is used by RRAM FPGA architecture
 * 2. regular configuration port, which is used by any FPGA architecture 
 */
class ConfPorts {
  public: /* Constructors */
    ConfPorts(); /* default port */
    ConfPorts(const ConfPorts& conf_ports); /* copy */
  public: /* Accessors */
    size_t get_reserved_port_width() const;
    size_t get_reserved_port_lsb() const;
    size_t get_reserved_port_msb() const;
    size_t get_regular_port_width() const;
    size_t get_regular_port_lsb() const;
    size_t get_regular_port_msb() const;
  public: /* Mutators */
    void set(const ConfPorts& conf_ports);
    void set_reserved_port(size_t width);
    void set_regular_port(size_t width);
    void set_regular_port(size_t lsb, size_t msb);
    void set_regular_port_lsb(size_t lsb);
    void set_regular_port_msb(size_t msb);
    void expand_reserved_port(size_t width); /* Increase the port width of reserved port */
    void expand_regular_port(size_t width); /* Increase the port width of regular port */
    void expand(size_t width); /* Increase the port width of both ports */
    bool rotate_regular_port(size_t offset); /* rotate */
    bool counter_rotate_regular_port(size_t offset); /* counter rotate */
    void reset(); /* Reset to initial port */
  private: /* Internal Data */
    BasicPort reserved_;
    BasicPort regular_;
};

/* TODO: create a class for BL and WL ports */

} /* namespace openfpga ends */

#endif

