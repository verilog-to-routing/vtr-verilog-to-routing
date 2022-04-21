#include <istream>
#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <functional>
#include <tcl/tcl.h>

#include "globals.h"
#include "vpr_constraints.h"
#include "atom_netlist.h"

#include "oo_tcl.h"
#include "xdc_constraints.h"


enum class e_XDCProperty {
    XDC_PROP_PACKAGE_PIN,
    XDC_PROP_IOSTANDARD,
    XDC_PROP_UNKNOWN
};

static e_XDCProperty xdc_prop_from_str(const char* str) {
    if (!std::strcmp(str, "PACKAGE_PIN"))
        return e_XDCProperty::XDC_PROP_PACKAGE_PIN;
    if (!std::strcmp(str, "IOSTANDARD"))
        return e_XDCProperty::XDC_PROP_IOSTANDARD;
    
    return e_XDCProperty::XDC_PROP_UNKNOWN;
}

DECLARE_TCL_TYPE(AtomPortId, port_tcl_t)

class TclPhysicalConstraintsClient : public TclClient {
public:
    VprConstraints& constraints;
    const t_arch& arch;
    AtomNetlist& netlist;

    TclPhysicalConstraintsClient(VprConstraints& constraints_, const t_arch& arch_, AtomNetlist& netlist_) :
        constraints(constraints_),
        arch(arch_),
        netlist(netlist_)
    {}

    typedef int(TclPhysicalConstraintsClient::*TclMethod)(int objc, Tcl_Obj* const objvp[]);
    void register_methods(std::function<void(const char* name, TclMethod)> register_method) {
        register_method("get_ports", &TclPhysicalConstraintsClient::get_ports);
        register_method("set_property", &TclPhysicalConstraintsClient::set_property);
    }

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
        case e_XDCProperty::XDC_PROP_IOSTANDARD:
            if (objc != 4)
                return this->_ret_error("set_property: Property `IOSTANDARD` "
                                        "requires one value and one target.");
            return this->_set_property_iostandard(objvp[2], objvp[3]);
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
        const char* pin_name = Tcl_GetString(tcl_pin_name);
        if (pin_name == nullptr)
            return this->_ret_error("set_property: pin_name of PACKAGE_PIN should be a string.");
        
        auto* port = tcl_obj_getptr<AtomPortId>(tcl_port);
        if (port == nullptr)
            return this->_ret_error("set_property: port__name of PACKAGE_PIN should have a `port` type.");

        return this->_do_set_property_package_pin(pin_name, *port);
    }

    int _do_set_property_package_pin(const char* pin_name, AtomPortId port) {
        /* Find associated AtomBlock */

        const auto& phys_grid_mapping = this->arch.phys_grid_mapping;
        
        std::string pin_str(pin_name);
        auto it = phys_grid_mapping.find(pin_str);
        if (it == phys_grid_mapping.end())
            return this->_ret_error("Can't find physical PIN named `" +
                                    pin_str + "`");
        const t_phys_map_region& package_pin_region = it->second;
        
        AtomBlockId port_block = this->netlist.port_block(port);

        /* Create a 1x1 VprConstraints Partition to contrain the block */

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

        /* Constrain the AtomBlock */

        this->constraints.add_constrained_atom(port_block, part_id);

        VTR_LOG("XDC: Assigned PACKAGE_PIN %s = [%d, %d, %d, %d].%d\n",
                pin_name, package_pin_region.x, package_pin_region.y,
                package_pin_region.w, package_pin_region.h, package_pin_region.subtile);

        return this->_ret_ok();
    }

    int _set_property_iostandard(Tcl_Obj* tcl_io_standard, Tcl_Obj* tcl_port) {
        const char* io_standard = Tcl_GetString(tcl_io_standard);
        if (tcl_io_standard == nullptr)
            return this->_ret_error("set_property: iostandard should be a string.");
        
        auto* port = tcl_obj_getptr<AtomPortId>(tcl_port);
        if (port == nullptr)
            return this->_ret_error("set_property: port_name of IOSTANDARD should have a `port` type.");
        
        return this->_do_set_property_iostandard(io_standard, *port);
    }

    int _do_set_property_iostandard(const char* io_standard, AtomPortId port) {
        AtomBlockId block = this->netlist.port_block(port);
        this->netlist.set_block_param(block, "IOSTANDARD", io_standard);
        VTR_LOG("XDC: Assigned IOSTANDARD %s = %s\n", this->netlist.block_name(block).c_str(), io_standard);
    }

    int _do_get_ports(const char* pin_name) {
        AtomBlockId pin_block = this->netlist.find_block(pin_name);

        if (pin_block == AtomBlockId::INVALID()) {
            std::string available_ports;
            const auto& pins = this->netlist.blocks();
            int pins_left = pins.size();
            for (auto& pin_block_ : pins) {
                auto ports = this->netlist.block_ports(pin_block_);
                auto port_it = this->netlist.block_ports(pin_block_).begin();
                if (port_it != ports.end()) {
                    available_ports += this->netlist.block_name(pin_block_);
                    if (pins_left != 1)
                        available_ports += ", ";
                }
                pins_left--;
            }

            return this->_ret_error("set_property: Can't find ports for `" +
                                    std::string(pin_name) + "`. Available blocks: " +
                                    available_ports);
        }
        
        auto ports = this->netlist.block_ports(pin_block);
        auto port_it = this->netlist.block_ports(pin_block).begin();
        if (port_it == ports.end())
            return this->_ret_error("set_property: '" + std::string(pin_name) + "' - no ports found.");

        /* TODO: this assumes there's only one port, which just happens to be true in case of fpga interchange.
         * If we want to handle it better, we should return a list of port ids instead.
         */
        AtomPortId port = *this->netlist.block_ports(pin_block).begin();
        std::string port_name = this->netlist.block_name(pin_block);

        VTR_LOG("get_ports: Got port `%s` (%llu) for pin `%s`\n", port_name.c_str(), (size_t)port, pin_name);

        return this->_ret_obj<AtomPortId>(this, std::move(port));
    }
};

REGISTER_TCL_TYPE_W_STR_UPDATE(AtomPortId, port_tcl_t) {
    const auto* port = tcl_obj_getptr<AtomPortId>(obj);
    const auto* client = tcl_obj_get_ctx_ptr<TclPhysicalConstraintsClient>(obj);
    std::string port_name = client->netlist.port_name(*port);
    tcl_set_obj_string(obj, port_name);
} END_REGISTER_TCL_TYPE;

VprConstraints read_xdc_constraints_to_vpr(std::istream& xdc_stream, const t_arch& arch, AtomNetlist& netlist) {
    //VTR_ASSERT(_xdc_initialized);

    VprConstraints constraints;

    TclPhysicalConstraintsClient pc_client(constraints, arch, netlist);
    TclCtx ctx;
    ctx.init();
    ctx.add_tcl_type<AtomPortId>();
    ctx.add_tcl_client<TclPhysicalConstraintsClient>(pc_client);
    ctx.read_tcl(xdc_stream);

    return constraints;
}

void load_xdc_constraints_file(const char* read_xdc_constraints_name, const t_arch& arch, AtomNetlist& netlist) {
    VTR_ASSERT(vtr::check_file_name_extension(read_xdc_constraints_name, ".xdc"));
    VTR_LOG("Reading XDC %s...\n", read_xdc_constraints_name);

    std::ifstream file(read_xdc_constraints_name);
    FloorplanningContext& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();

    try {
        floorplanning_ctx.constraints = read_xdc_constraints_to_vpr(file, arch, netlist);
    } catch (TCL_eErroneousTCL& e) {
        e.filename = std::string(read_xdc_constraints_name);
        throw e;
    }
}
