#include <istream>
#include <string>
#include <sstream>
#include <utility>
#include <functional>
#include <tcl/tcl.h>

//#include "DeviceResources.capnp.h"
//#include "LogicalNetlist.capnp.h"
#include "vpr_constraints.h"
#include "atom_netlist.h"

#define OV_TOSTRING(e) virtual operator std::string() { return (e); }

/* XDC ERRORS */

class XDC_eException {
public:
    OV_TOSTRING("Unknown error")
};

class XDC_eCustomException : XDC_eException {
public:
    std::string message;
    XDC_eCustomException(std::string&& message) {
        this->message = std::move(message);
    }
    OV_TOSTRING(this->message)
};

class XDC_eFailedToInitTcl : XDC_eException {
public:
    int error_code;
    XDC_eFailedToInitTcl(int error_code) {
        this->error_code = error_code;
    }
    OV_TOSTRING("Can't initialize TCL (code " + std::to_string(error_code) +")")
};

class XDC_eErroneousXDC : XDC_eException {
public:
    int line, column;
    std::string filename;
    OV_TOSTRING(this->filename + ":" + std::to_string(this->line) + "," + std::to_string(this->column) + ": Unknown error")
};

/* TCL COMMANDS */

using TclCommandError = std::string;
enum class e_TclCommandStatus : int {
    TCL_CMD_SUCCESS,
    TCL_CMD_FAIL
};
class TclClientData {
public:
    e_TclCommandStatus cmd_status;
    TclCommandError error;
    VprConstraints& constraints;
    AtomNetlist& netlist;

    TclClientData(VprConstraints& constraints, AtomNetlist& netlist) :
        cmd_status(e_TclCommandStatus::TCL_CMD_SUCCESS),
        error("No errors"),
        constraints(constraints),
        netlist(netlist)
    {}

    void set_error(TclCommandError&& error) {
        this->error = std::move(error);
    }
};

enum class e_XDCProperty {
    XDC_PROP_PACKAGE_PIN,
    XDC_PROP_UNKNOWN
};

static e_XDCProperty xdc_prop_from_str(const char* str) {
    if (str == "PACKAGE_PIN")
        return e_XDCProperty::XDC_PROP_PACKAGE_PIN;
    
    return e_XDCProperty::XDC_PROP_UNKNOWN;
}

static int do_set_property_package_pin(
    VprConstraints& constraints,
    AtomNetlist& netlist,
    const char* pin_name,
    const char* port_name,
    std::function<void(std::string)> on_error)
{
    AtomPinId pin = netlist.find_pin(pin_name);
    if (pin == AtomPinId::INVALID()) {
        on_error("set_property: Can't find PIN named `" + std::string(pin_name) + "`");
        return 0;
    }
    /* TODO: Handle pins W/O ports properly */
    AtomPortId port = netlist.pin_port(pin);
    //constraints.add_constrained_atom()
    return 0;
}

static int tcl_set_property_package_pin(TclClientData* cd, Tcl_Interp* tcl_interp, Tcl_Obj* tcl_pin_name, Tcl_Obj* tcl_port) {
    int error;
    
    const char* pin_name = Tcl_GetString(tcl_pin_name);
    if (pin_name == nullptr) {
        cd->set_error("set_property: pin_name of PACKAGE_PIN should be a string.");
        return 0;
    }

    const char* port_name = Tcl_GetString(tcl_port);
    if (port_name == nullptr) {
        cd->set_error("set_property: pin_name of PACKAGE_PIN should be a string.");
        return 0;
    }
    
    error = do_set_property_package_pin(cd->constraints, cd->netlist, pin_name, port_name, [&](std::string msg) { cd->set_error(std::move(msg)); });
}

static int tcl_set_property(TclClientData* cd, Tcl_Interp* tcl_interp, int objc, Tcl_Obj* const objvp[]) {
    if (objc < 2) {
        cd->set_error("set_property: Expected at least 2 arguments.");
        return 0;
    }

    const char* property_name = Tcl_GetString(objvp[0]);
    if (property_name == nullptr) {
        cd->set_error("set_property: First argument should be a string.");
        return 0;
    }
    const e_XDCProperty property = xdc_prop_from_str(property_name);

    switch (property) {
    case e_XDCProperty::XDC_PROP_PACKAGE_PIN:
        if (objc != 3) {
            cd->set_error("set_property: Property `PACKAGE_PIN` requires one target and one value.");
            return 0;
        }
        return tcl_set_property_package_pin(cd, tcl_interp, objvp[2], objvp[3]);
    case e_XDCProperty::XDC_PROP_UNKNOWN:
        cd->set_error("set_property: Property `" + std::string(property_name) + "` is not recognized.");
        return 0;
    default:
        cd->set_error("set_property: Property `" + std::string(property_name) + "` is not supported.");
        return 0;
    }

}

/* API */

VprConstraints read_xdc_constraints_to_vpr(AtomNetlist& netlist, const char* argv0, std::istream& xdc_stream) {
    int error;

    Tcl_FindExecutable(argv0);

    Tcl_Interp* tcl_interp = Tcl_CreateInterp();
    if ((error = Tcl_Init(tcl_interp)) != TCL_OK) {
        Tcl_DeleteInterp(tcl_interp);
        throw XDC_eFailedToInitTcl(error);
    }

    auto constraints = VprConstraints();
    auto client_data = TclClientData(constraints, netlist); /* ! Shares ownership of constraints and netlist */

    throw XDC_eCustomException("XDC parser is not fully implemented");
}