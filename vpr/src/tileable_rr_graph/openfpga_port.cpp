#include <numeric>
#include <algorithm>
#include <limits>

/* Headers from vtrutil library */
#include "vtr_assert.h"

#include "openfpga_port.h"

/************************************************************************
 * Member functions for BasicPort class
 ***********************************************************************/

/************************************************************************
 * Constructors
 ***********************************************************************/
/* Default constructor */
BasicPort::BasicPort() {
    /* By default we set an invalid port, which size is 0 */
    lsb_ = 1;
    msb_ = 0;

    origin_port_width_ = -1;
}

/* Quick constructor */
BasicPort::BasicPort(const char* name, const size_t& lsb, const size_t& msb) {
    set_name(std::string(name));
    set_width(lsb, msb);
    set_origin_port_width(-1);
}

BasicPort::BasicPort(const std::string& name, const size_t& lsb, const size_t& msb) {
    set_name(name);
    set_width(lsb, msb);
    set_origin_port_width(-1);
}

BasicPort::BasicPort(const char* name, const size_t& width) {
    set_name(std::string(name));
    set_width(width);
    set_origin_port_width(-1);
}

BasicPort::BasicPort(const std::string& name, const size_t& width) {
    set_name(name);
    set_width(width);
    set_origin_port_width(-1);
}

/************************************************************************
 * Accessors
 ***********************************************************************/
/* get the port width */
size_t BasicPort::get_width() const {
    if (true == is_valid()) {
        return msb_ - lsb_ + 1;
    }
    return 0; /* invalid port has a zero width */
}

/* get the LSB */
size_t BasicPort::get_msb() const {
    return msb_;
}

/* get the LSB */
size_t BasicPort::get_lsb() const {
    return lsb_;
}

/* get the name */
std::string BasicPort::get_name() const {
    return name_;
}

/* Make a range of the pin indices */
std::vector<size_t> BasicPort::pins() const {
    std::vector<size_t> pin_indices;

    /* Return if the port is invalid */
    if (false == is_valid()) {
        return pin_indices; /* Return an empty vector */
    }
    /* For valid ports, create a vector whose length is the port width */
    pin_indices.resize(get_width());
    /* Fill in an incremental sequence */
    std::iota(pin_indices.begin(), pin_indices.end(), get_lsb());
    /* Ensure the last one is MSB */
    VTR_ASSERT(get_msb() == pin_indices.back());

    return pin_indices;
}

/* Check if a port can be merged with this port: their name should be the same */
bool BasicPort::mergeable(const BasicPort& portA) const {
    return (0 == this->get_name().compare(portA.get_name()));
}

/* Check if a port is contained by this port:
 * this function will check if the (LSB, MSB) of portA
 * is contained by the (LSB, MSB) of this port
 */
bool BasicPort::contained(const BasicPort& portA) const {
    return (lsb_ <= portA.get_lsb() && portA.get_msb() <= msb_);
}

/* Set original port width */
size_t BasicPort::get_origin_port_width() const {
    return origin_port_width_;
}

/************************************************************************
 * Overloaded operators
 ***********************************************************************/
/* Two ports are the same only when:
 * 1. port names are the same
 * 2. LSBs are the same
 * 3. MSBs are the same
 */
bool BasicPort::operator==(const BasicPort& portA) const {
    if ((0 == this->get_name().compare(portA.get_name()))
        && (this->get_lsb() == portA.get_lsb())
        && (this->get_msb() == portA.get_msb())) {
        return true;
    }
    return false;
}

bool BasicPort::operator<(const BasicPort& portA) const {
    if ((0 == this->get_name().compare(portA.get_name()))
        && (this->get_lsb() < portA.get_lsb())
        && (this->get_msb() < portA.get_msb())) {
        return true;
    }
    return false;
}

/************************************************************************
 * Mutators
 ***********************************************************************/
/* copy */
void BasicPort::set(const BasicPort& basic_port) {
    name_ = basic_port.get_name();
    lsb_ = basic_port.get_lsb();
    msb_ = basic_port.get_msb();
    origin_port_width_ = basic_port.get_origin_port_width();

    return;
}

/* set the port LSB and MSB */
void BasicPort::set_name(const std::string& name) {
    name_ = name;
    return;
}

/* set the port LSB and MSB */
void BasicPort::set_width(const size_t& width) {
    if (0 == width) {
        make_invalid();
        return;
    }
    lsb_ = 0;
    msb_ = width - 1;
    return;
}

/* set the port LSB and MSB */
void BasicPort::set_width(const size_t& lsb, const size_t& msb) {
    /* If lsb and msb is invalid, we make a default port */
    if (lsb > msb) {
        make_invalid();
        return;
    }
    set_lsb(lsb);
    set_msb(msb);
    return;
}

void BasicPort::set_lsb(const size_t& lsb) {
    lsb_ = lsb;
    return;
}

void BasicPort::set_msb(const size_t& msb) {
    msb_ = msb;
    return;
}

void BasicPort::set_origin_port_width(const size_t& origin_port_width) {
    origin_port_width_ = origin_port_width;
    return;
}

/* Increase the port width */
void BasicPort::expand(const size_t& width) {
    if (0 == width) {
        return; /* ignore zero-width port */
    }
    /* If current port is invalid, we do not combine */
    if (0 == get_width()) {
        lsb_ = 0;
        msb_ = width;
        return;
    }
    /* Increase MSB */
    msb_ += width;
    return;
}

/* Swap lsb and msb */
void BasicPort::revert() {
    std::swap(lsb_, msb_);
    return;
}

/* rotate: increase both lsb and msb by an offset  */
bool BasicPort::rotate(const size_t& offset) {
    /* If offset is 0, we do nothing */
    if (0 == offset) {
        return true;
    }

    /* If current width is 0, we set a width using the offset! */
    if (0 == get_width()) {
        set_width(offset);
        return true;
    }
    /* check if leads to overflow:
     * if limits - msb is larger than offset
     */
    if ((std::numeric_limits<size_t>::max() - msb_ < offset)) {
        return false;
    }
    /* Increase LSB and MSB */
    lsb_ += offset;
    msb_ += offset;
    return true;
}

/* rotate: decrease both lsb and msb by an offset  */
bool BasicPort::counter_rotate(const size_t& offset) {
    /* If current port is invalid or offset is 0,
     * we do nothing
     */
    if ((0 == offset) || (0 == get_width())) {
        return true;
    }
    /* check if leads to overflow:
     * if limits is larger than offset
     */
    if ((std::numeric_limits<size_t>::min() + lsb_ < offset)) {
        return false;
    }
    /* decrease LSB and MSB */
    lsb_ -= offset;
    msb_ -= offset;
    return true;
}

/* Reset to initial port */
void BasicPort::reset() {
    make_invalid();
    return;
}

/* Combine two ports */
void BasicPort::combine(const BasicPort& port) {
    /* LSB follows the current LSB */
    /* MSB increases */
    VTR_ASSERT(0 < port.get_width()); /* Make sure port is valid */
    /* If current port is invalid, we do not combine */
    if (0 == get_width()) {
        return;
    }
    /* Increase MSB */
    msb_ += port.get_width();
    return;
}

/* A restricted combine function for two ports,
 * Following conditions will be applied:
 * 1. the two ports have the same name
 *    Note: you must run mergable() function first
 *          to make sure this assumption is valid
 * 2. the new MSB will be the maximum MSB of the two ports
 * 3. the new LSB will be the minimum LSB of the two ports
 * 4. both ports should be valid!!!
 */
void BasicPort::merge(const BasicPort& portA) {
    VTR_ASSERT(true == this->mergeable(portA));
    VTR_ASSERT(true == this->is_valid() && true == portA.is_valid());
    /* We skip merging if the portA is already contained by this port */
    if (true == this->contained(portA)) {
        return;
    }
    /* LSB follows the minium LSB of the two ports */
    lsb_ = std::min((int)lsb_, (int)portA.get_lsb());
    /* MSB follows the minium MSB of the two ports */
    msb_ = std::max((int)msb_, (int)portA.get_msb());
    /* Origin port width follows the maximum of the two ports */
    msb_ = std::max((int)origin_port_width_, (int)portA.get_origin_port_width());
    return;
}

/* Internal functions */
/* Make a port to be invalid: msb < lsb */
void BasicPort::make_invalid() {
    /* set a default invalid port */
    lsb_ = 1;
    msb_ = 0;
    return;
}

/* check if port size is valid > 0 */
bool BasicPort::is_valid() const {
    /* msb should be equal or greater than lsb, if this is a valid port */
    if (msb_ < lsb_) {
        return false;
    }
    return true;
}

/************************************************************************
 * ConfPorts member functions
 ***********************************************************************/

/************************************************************************
 * Constructor
 ***********************************************************************/
/* Default constructor */
ConfPorts::ConfPorts() {
    /* default port */
    reserved_.reset();
    regular_.reset();
}

/* copy */
ConfPorts::ConfPorts(const ConfPorts& conf_ports) {
    set(conf_ports);
}

/************************************************************************
 * Accessors
 ***********************************************************************/
size_t ConfPorts::get_reserved_port_width() const {
    return reserved_.get_width();
}

size_t ConfPorts::get_reserved_port_lsb() const {
    return reserved_.get_lsb();
}

size_t ConfPorts::get_reserved_port_msb() const {
    return reserved_.get_msb();
}

size_t ConfPorts::get_regular_port_width() const {
    return regular_.get_width();
}

size_t ConfPorts::get_regular_port_lsb() const {
    return regular_.get_lsb();
}

size_t ConfPorts::get_regular_port_msb() const {
    return regular_.get_msb();
}

/************************************************************************
 * Mutators
 ***********************************************************************/
void ConfPorts::set(const ConfPorts& conf_ports) {
    set_reserved_port(conf_ports.get_reserved_port_width());
    set_regular_port(conf_ports.get_regular_port_lsb(), conf_ports.get_regular_port_msb());
    return;
}

void ConfPorts::set_reserved_port(size_t width) {
    reserved_.set_width(width);
    return;
}

void ConfPorts::set_regular_port(size_t width) {
    regular_.set_width(width);
    return;
}

void ConfPorts::set_regular_port(size_t lsb, size_t msb) {
    regular_.set_width(lsb, msb);
    return;
}

void ConfPorts::set_regular_port_lsb(size_t lsb) {
    regular_.set_lsb(lsb);
    return;
}

void ConfPorts::set_regular_port_msb(size_t msb) {
    regular_.set_msb(msb);
    return;
}

/* Increase the port width of reserved port */
void ConfPorts::expand_reserved_port(size_t width) {
    reserved_.expand(width);
    return;
}

/* Increase the port width of regular port */
void ConfPorts::expand_regular_port(size_t width) {
    regular_.expand(width);
    return;
}

/* Increase the port width of both ports */
void ConfPorts::expand(size_t width) {
    expand_reserved_port(width);
    expand_regular_port(width);
}

/* rotate */
bool ConfPorts::rotate_regular_port(size_t offset) {
    return regular_.rotate(offset);
}

/* counter rotate */
bool ConfPorts::counter_rotate_regular_port(size_t offset) {
    return regular_.counter_rotate(offset);
}

/* Reset to initial port */
void ConfPorts::reset() {
    reserved_.reset();
    regular_.reset();
    return;
}
