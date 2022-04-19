#ifndef XDC_CONSTRAINTS_H
#define XDC_CONSTRAINTS_H

#include <string>

#include "atom_netlist.h"
#include "physical_types.h"
#include "vpr_constraints.h"

class XDC_eException {
public:
    virtual ~XDC_eException() = default;
    virtual operator std::string() const;
};

class XDC_eCustomException : XDC_eException {
public:
    XDC_eCustomException(std::string&& message_);
    ~XDC_eCustomException() override = default;
    operator std::string() const override;

    std::string message;
};

class XDC_eFailedToInitTcl : XDC_eException {
public:
    XDC_eFailedToInitTcl(int error_code_);
    ~XDC_eFailedToInitTcl() override = default;
    operator std::string() const override;

    int error_code;
};

class XDC_eErroneousXDC : XDC_eException {
public:
    XDC_eErroneousXDC(std::string&& filename_, int line_, int column_, std::string&& message_);
    ~XDC_eErroneousXDC() override = default;
    operator std::string() const override;

    std::string filename;
    int line, column;
    std::string message;
};

/**
 * \brief Init TCL susystem
 */
void xdc_init();

/**
 * \brief Creates VprConstraints from input stream in XDC format.
 * \throws XDC_eException: base class for all exceptions
 *         XDC_eFailedToInitTcl: Failure during initializations of TCL subsystem
 *         XDC_eErroneousXDC: Failure when parsing XDC
 *         XDC_eCustomException: Anything else that doesn't fit he above categories.
 */
VprConstraints read_xdc_constraints_to_vpr(std::istream& xdc_stream, const t_arch& arch, const AtomNetlist& netlist);

/**
 * \brief Parse a file in XDC format and apply it to global FloorplanningContext.
 * \throws XDC_eException: base class for all exceptions
 *         XDC_eFailedToInitTcl: Failure during initializations of TCL subsystem
 *         XDC_eErroneousXDC: Failure when parsing XDC
 *         XDC_eCustomException: Anything else that doesn't fit he above categories.
 */
void load_xdc_constraints_file(const char* read_xdc_constraints_name, const t_arch& arch, const AtomNetlist& netlist);

#endif
