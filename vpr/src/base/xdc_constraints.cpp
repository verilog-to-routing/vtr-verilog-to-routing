#include <istream>
#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <functional>
#include <list>
#include <tcl/tcl.h>
#include <cstring>

//#include "DeviceResources.capnp.h"
//#include "LogicalNetlist.capnp.h"
#include "globals.h"
#include "vpr_constraints.h"
#include "atom_netlist.h"

#include "xdc_constraints.h"

#define OV_TOSTRING(c, e) c::operator std::string() const { return (e); }

/* XDC ERRORS */

OV_TOSTRING(XDC_eException, "Unknown error")

XDC_eCustomException::XDC_eCustomException(std::string&& message_) : message(std::move(message_)) {}
XDC_eCustomException::operator std::string() const { return (this->message); }

XDC_eFailedToInitTcl::XDC_eFailedToInitTcl(int error_code_) : error_code(error_code_) {}
OV_TOSTRING(XDC_eFailedToInitTcl, "Can't initialize TCL (code " + std::to_string(error_code) +")")

XDC_eErroneousXDC::XDC_eErroneousXDC(
    std::string&& filename_,
    int line_, int column_,
    std::string&& message_)
    : filename(std::move(filename_)), line(line_), column(column_), message(message_) {}
OV_TOSTRING(XDC_eErroneousXDC, this->filename + ":" + std::to_string(this->line) + "," +
                               std::to_string(this->column) + ": " + this->message)


/* TCL COMMANDS */

/* Stolen from nextpnr */
static void Tcl_SetStringResult(Tcl_Interp *interp, const std::string &s) {
    char *copy = Tcl_Alloc(s.length() + 1);
    std::strcpy(copy, s.c_str());
    Tcl_SetResult(interp, copy, TCL_DYNAMIC);
}

using TclCommandError = std::string;
enum class e_TclCommandStatus : int {
    TCL_CMD_SUCCESS,
    TCL_CMD_SUCCESS_STRING,
    TCL_CMD_SUCCESS_OBJECT,
    TCL_CMD_FAIL
};

enum class e_XDCProperty {
    XDC_PROP_PACKAGE_PIN,
    XDC_PROP_UNKNOWN
};

static e_XDCProperty xdc_prop_from_str(const char* str) {
    if (!std::strcmp(str, "PACKAGE_PIN"))
        return e_XDCProperty::XDC_PROP_PACKAGE_PIN;
    
    return e_XDCProperty::XDC_PROP_UNKNOWN;
}

/* TCL Object Types */

extern const Tcl_ObjType port_tcl_t;

template <typename T> constexpr const Tcl_ObjType* _tcl_type_of();
template <>
constexpr const Tcl_ObjType* _tcl_type_of<AtomPortId>() {
    return &port_tcl_t;
}

class XDCTclClient;

template <typename T>
static T* _tcl_obj_getptr(Tcl_Obj* obj) {
    if (obj->typePtr != _tcl_type_of<T>())
        return nullptr;
    return static_cast<T*>(obj->internalRep.twoPtrValue.ptr2);
}

template <>
XDCTclClient* _tcl_obj_getptr(Tcl_Obj* obj) {
    return static_cast<XDCTclClient*>(obj->internalRep.twoPtrValue.ptr1);
}

template <>
const XDCTclClient* _tcl_obj_getptr(Tcl_Obj* obj) {
    return static_cast<const XDCTclClient*>(obj->internalRep.twoPtrValue.ptr1);
}

/* XDCTclClient - XDC commands are implemented here */

class XDCTclClient {
public:
    VprConstraints& constraints;
    e_TclCommandStatus cmd_status;
    std::string string;
    Tcl_Obj* object;
    const t_arch& arch;
    const AtomNetlist& netlist;

    XDCTclClient(VprConstraints& constraints_, const t_arch& arch_, const AtomNetlist& netlist_) :
        constraints(constraints_),
        cmd_status(e_TclCommandStatus::TCL_CMD_SUCCESS),
        string("No errors"),
        arch(arch_),
        netlist(netlist_)
    {}

    /* TCL functions */

    int set_property(int objc, Tcl_Obj* const objvp[]) {        
        if (objc < 3)
            return this->_ret_error("set_property: Expected at least 2 arguments, got " + 
                                    std::to_string(objc) + ".");

        const char* property_name = Tcl_GetString(objvp[1]);
        if (property_name == nullptr)
            return this->_ret_error("set_property: First argument should be a string.");
        
        e_XDCProperty property = xdc_prop_from_str(property_name);

        switch (property) {
        case e_XDCProperty::XDC_PROP_PACKAGE_PIN:
            if (objc != 4)
                return this->_ret_error("set_property: Property `PACKAGE_PIN` "
                                        "requires one target and one value.");
            return this->_set_property_package_pin(objvp[2], objvp[3]);
        case e_XDCProperty::XDC_PROP_UNKNOWN:
            return this->_ret_error("set_property: Property `" + std::string(property_name) +
                            "` is not recognized.");
        default: break;
        }
        return this->_ret_error("set_property: Property `" + std::string(property_name) +
                                "` is not supported.");
    }

    int get_ports(int objc, Tcl_Obj* const objvp[]) {
        if (objc != 2)
            return this->_ret_error("get_ports: Expected one argument.");
        
        const char* pin_name = Tcl_GetString(objvp[1]);
        if (pin_name == nullptr)
            return this->_ret_error("get_ports: pin_name should be a string.");
        
        return this->_do_get_ports(pin_name);
    }

protected:
    int _set_property_package_pin(Tcl_Obj* tcl_pin_name, Tcl_Obj* tcl_port) {
        int error;
        
        const char* pin_name = Tcl_GetString(tcl_pin_name);
        if (pin_name == nullptr)
            return this->_ret_error("set_property: pin_name of PACKAGE_PIN should be a string.");
        
        auto* port = _tcl_obj_getptr<AtomPortId>(tcl_port);
        if (port == nullptr)
            return this->_ret_error("set_property: pin_name of PACKAGE_PIN should have a `port` type.");

        return this->_do_set_property_package_pin(pin_name, *port);

        return error;
    }

    int _do_set_property_package_pin(const char* pin_name, AtomPortId port) {
        const auto& phys_grid_mapping = this->arch.phys_grid_mapping;

        
        std::string pin_str(pin_name);
        auto it = phys_grid_mapping.find(pin_str);
        if (it == phys_grid_mapping.end())
            return this->_ret_error("Can't find physical PIN named `" +
                                    pin_str + "`");
        const t_phys_map_region& package_pin_region = it->second;
        
        AtomBlockId port_block = this->netlist.port_block(port);
        Partition part;
        part.set_name(pin_str + ".PART");
        Region r;
        r.set_region_rect(
            package_pin_region.x,
            package_pin_region.y,
            package_pin_region.x + package_pin_region.w - 1,
            package_pin_region.y + package_pin_region.h - 1
        );
        r.set_sub_tile(package_pin_region.subtile);
        PartitionRegion pr;
        pr.add_to_part_region(r);
        part.set_part_region(pr);
        PartitionId part_id = this->constraints.add_partition(part);
        this->constraints.add_constrained_atom(port_block, part_id);

        VTR_LOG("XDC: Assigned PACKAGE_PIN %s = [%d, %d, %d, %d].%d\n",
                pin_name, package_pin_region.x, package_pin_region.y,
                package_pin_region.w, package_pin_region.h, package_pin_region.subtile);

        return this->_ret_ok();
    }

    int _do_get_ports(const char* pin_name) {
        AtomPinId pin = this->netlist.find_pin(pin_name);
        if (pin == AtomPinId::INVALID()) {
            std::string available_pins;
            const auto& pins = this->netlist.pins();
            int pins_left = pins.size();
            for (auto& pin : pins) {
                available_pins += this->netlist.pin_name(pin);
                if (pins_left != 1)
                    available_pins += ", ";
                pins_left--;
            }

            return this->_ret_error("set_property: Can't find PORT named `" +
                                    std::string(pin_name) + "`. Available pins: " +
                                    available_pins);
        }
        
        AtomPortId port = this->netlist.pin_port(pin);
        std::string port_name = this->netlist.port_name(port);

        VTR_LOG("get_ports: Got port `%s` (%llu) for pin `%s`\n", port_name.c_str(), (size_t)port, pin_name);

        return this->_ret_obj<AtomPortId>(port);
    }

    /* Return methods */

    inline int _ret_error(TclCommandError&& error) {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_FAIL;
        this->string = std::move(error);
        return TCL_ERROR;
    }

    inline int _ret_ok() {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS;
        return TCL_OK;
    }

    inline int _ret_string(std::string&& string_) {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS_STRING;
        this->string = std::move(string_);
        return TCL_OK;
    }

    template <typename T>
    inline int _ret_obj(const T& object_data) {
        const Tcl_ObjType* type = _tcl_type_of<T>();
        Tcl_Obj* obj = Tcl_NewObj();
        obj->bytes = nullptr;
        obj->typePtr = type;
        obj->internalRep.twoPtrValue.ptr1 = this;
        obj->internalRep.twoPtrValue.ptr2 = Tcl_Alloc(sizeof(T));
        new(obj->internalRep.twoPtrValue.ptr2) T(object_data);
        if (type->updateStringProc != nullptr)
            type->updateStringProc(obj);
        
        this->object = obj;
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS_OBJECT;

        return TCL_OK;
    }
};

template <typename T>
static void _tcl_obj_free(Tcl_Obj* obj) {
    T* obj_data = _tcl_obj_getptr<T>(obj);
    obj_data->~T();
    Tcl_Free(reinterpret_cast<char*>(obj_data));
}

static void _tcl_obj_dup(Tcl_Obj* src, Tcl_Obj* dst) {
    dst->internalRep.twoPtrValue = src->internalRep.twoPtrValue;
    dst->bytes = nullptr;
    dst->length = 0;
}

static void _tcl_set_obj_string(Tcl_Obj* obj, const std::string& str) {
    if (obj->bytes != nullptr)
        Tcl_Free(obj->bytes);
    obj->bytes = Tcl_Alloc(str.length() + 1);
    obj->length = str.length();
    std::strcpy(obj->bytes, str.c_str());
}

static int _tcl_set_from_none(Tcl_Interp *tcl_interp, Tcl_Obj *obj) {
    (void)(obj); /* Surpress "unused parameter" macro */
    /* TODO: Better error message */
    Tcl_SetStringResult(tcl_interp, "Attempted an invalid conversion.");
    return TCL_ERROR;
}

const Tcl_ObjType port_tcl_t = (Tcl_ObjType){
    .name = "port",
    .freeIntRepProc = _tcl_obj_free<AtomPortId>,
    .dupIntRepProc = _tcl_obj_dup,
    .updateStringProc = [](Tcl_Obj* obj) {
        const auto* port = _tcl_obj_getptr<AtomPortId>(obj);
        const auto* client = _tcl_obj_getptr<XDCTclClient>(obj);
        std::string port_name = client->netlist.port_name(*port);
        _tcl_set_obj_string(obj, port_name);
    },
    .setFromAnyProc = _tcl_set_from_none
};

static bool _xdc_initialized = false;

/* API */

void xdc_init() {
    if (!_xdc_initialized)
        Tcl_FindExecutable(_argv0);
    _xdc_initialized = true;

    VTR_LOG("Initialized TCL subsystem");
}

/**
 * \brief Provide TCL runtime for parsing XDC files.
 */
class XDCCtx {
public:
    XDCCtx(const t_arch& arch_, const AtomNetlist& netlist_) :
        constraints(VprConstraints()),
        _client(this->constraints, arch_, netlist_)
    {
        this->_tcl_interp = Tcl_CreateInterp();
#ifdef DEBUG
        this->_init = false;
#endif
        VTR_LOG("Created XDC context.\n");
    }

    ~XDCCtx() {
        Tcl_DeleteInterp(this->_tcl_interp);

        VTR_LOG("Deleted XDC context.\n");
    }

    void init() {
        int error;

        if ((error = Tcl_Init(this->_tcl_interp)) != TCL_OK)
            throw XDC_eFailedToInitTcl(error);

        error = this->_fix_port_indexing();
        VTR_ASSERT(error == TCL_OK);
        this->_extend_tcl();

#ifdef DEBUG
        this->_init = true;
#endif
    }

    /**
     * \brief Extend (VPR) constraints with constraints read from and XDC script
     */
    void read_xdc(std::istream& xdc_stream) {
        int error;

        this->_debug_init();

        std::ostringstream os;
        xdc_stream >> os.rdbuf();
        std::string xdc = os.str();

        error = Tcl_Eval(this->_tcl_interp, xdc.c_str());
        /* TODO: More precise error */
        if (error != TCL_OK) {
            int error_line = Tcl_GetErrorLine(this->_tcl_interp);
            const char* msg = Tcl_GetStringResult(this->_tcl_interp);
            throw XDC_eErroneousXDC("<unknown file>", error_line, 0, std::string(msg));
        }
    }

    VprConstraints constraints;

protected:

    /**
     * \brief Add TCL commands used in XDC files
     */
    void _extend_tcl() {
        this->_add_tcl_type(&port_tcl_t);
        this->_add_tcl_cmd("get_ports", &XDCTclClient::get_ports);
        this->_add_tcl_cmd("set_property", &XDCTclClient::set_property);
    }

    /**
     * \brief Tcl by default will interpret bus indices as calls to commands
     *        (eg. in[0] gets interpreted as call to command `i` with argument being a
     *        result of calling `0`). This overrides this behaviour.
     * 
     * Code taken from nextpnr.
     */
    int _fix_port_indexing() {
        int error;
        error = Tcl_Eval(this->_tcl_interp, "rename unknown _original_unknown");
        if (error != TCL_OK)
            return error;
        error = Tcl_Eval(
            this->_tcl_interp,
            "proc unknown args {\n"
            "  set result [scan [lindex $args 0] \"%d\" value]\n"
            "  if { $result == 1 && [llength $args] == 1 } {\n"
            "    return \\[$value\\]\n"
            "  } else {\n"
            "    uplevel 1 [list _original_unknown {*}$args]\n"
            "  }\n"
            "}"
        );
        return error;
    }

    typedef int(XDCTclClient::*TclMethod)(int objc, Tcl_Obj* const objvp[]);

    struct TclMethodDispatch {
        TclMethodDispatch(TclMethod method_, XDCCtx& ctx_) : method(method_), ctx(ctx_) {}
        
        TclMethod method;
        XDCCtx& ctx;
    };

    static int _tcl_do_method(
        ClientData cd,
        Tcl_Interp* tcl_interp,
        int objc,
        Tcl_Obj* const objvp[]
    ) {
        TclMethodDispatch* d = static_cast<TclMethodDispatch*>(cd);
        auto method = d->method;
        auto& client = d->ctx._client;

        (client.*method)(objc, objvp);

        switch (client.cmd_status) {
        case e_TclCommandStatus::TCL_CMD_FAIL:
            Tcl_SetStringResult(tcl_interp, d->ctx._client.string);
            return TCL_ERROR;
        case e_TclCommandStatus::TCL_CMD_SUCCESS_STRING:
            Tcl_SetStringResult(tcl_interp, d->ctx._client.string);
            return TCL_OK;
        case e_TclCommandStatus::TCL_CMD_SUCCESS_OBJECT:
            Tcl_SetObjResult(tcl_interp, d->ctx._client.object);
        default: break;
        }
        return TCL_OK;
    }

    void _add_tcl_cmd(const char* name, TclMethod method) {
        this->_method_dispatch_list.push_front(TclMethodDispatch(method, *this));
        auto& md = this->_method_dispatch_list.front();
        Tcl_CreateObjCommand(this->_tcl_interp, name, XDCCtx::_tcl_do_method, &md, nullptr);
    }

    void _add_tcl_type(const Tcl_ObjType* ty) {
        Tcl_RegisterObjType(ty);
    }

    Tcl_Interp* _tcl_interp;
    XDCTclClient _client;
    std::list<TclMethodDispatch> _method_dispatch_list;

private:
#ifdef DEBUG
    inline void _debug_init() const {
        VTR_ASSERT(this->_init);
    }

    bool _init;
#else
    inline void _debug_init() const {}
#endif
};

VprConstraints read_xdc_constraints_to_vpr(std::istream& xdc_stream, const t_arch& arch, const AtomNetlist& netlist) {
    VTR_ASSERT(_xdc_initialized);

    XDCCtx ctx(arch, netlist);
    ctx.init();
    ctx.read_xdc(xdc_stream);
    return std::move(ctx.constraints);
}

void load_xdc_constraints_file(const char* read_xdc_constraints_name, const t_arch& arch, const AtomNetlist& netlist) {
    VTR_ASSERT(vtr::check_file_name_extension(read_xdc_constraints_name, ".xdc"));
    VTR_LOG("Reading XDC %s...\n", read_xdc_constraints_name);

    std::ifstream file(read_xdc_constraints_name);
    FloorplanningContext& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();

    try {
        floorplanning_ctx.constraints = read_xdc_constraints_to_vpr(file, arch, netlist);
    } catch (XDC_eErroneousXDC& e) {
        e.filename = std::string(read_xdc_constraints_name);
        throw e;
    }
}
