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

#include "tclcpp.h"
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

DECLARE_TCL_TYPE(AtomPortId)

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

    template <typename F>
    void register_methods(F register_method) {
        register_method("get_ports", &TclPhysicalConstraintsClient::get_ports);
        register_method("set_property", &TclPhysicalConstraintsClient::set_property);
        register_method("get_cells", &TclPhysicalConstraintsClient::get_cells);
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
        if (objc < 2)
            return this->_ret_error("get_ports: Expected one or more arguments.");
        
        std::vector<const char*> port_names;

        for (size_t i = 1; i < objc; i++) {
            const char* pin_name = Tcl_GetString(objvp[i]);
            if (pin_name == nullptr)
                return this->_ret_error("get_ports: pin_name should be a string.");
            port_names.push_back(pin_name);
        }
        
        return this->_do_get_ports(std::move(port_names));
    }

    int get_cells(int objc, Tcl_Obj* const objvp[]) {
        return this->_ret_error("get_cells: unimplemented");
    }

protected:
    int _set_property_package_pin(Tcl_Obj* tcl_pin_name, Tcl_Obj* tcl_ports) {
        const char* pin_name = Tcl_GetString(tcl_pin_name);
        if (pin_name == nullptr)
            return this->_ret_error("set_property: pin_name of PACKAGE_PIN should be a string.");
        
        auto port_list = tcl_obj_getlist<AtomPortId>(static_cast<TclClient*>(this), tcl_ports);
        size_t port_count = port_list.size();
        if (port_count != 1)
            return this->_ret_error("set_property PACKAGE_PIN: Expected 1 port, got " + 
                                    std::to_string(port_count) + ".");
        auto* port = *port_list.begin();
        if (port == nullptr)
            return this->_ret_error("set_property: port_name of PACKAGE_PIN should have a `AtomPortId` type.");

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

    int _set_property_iostandard(Tcl_Obj* tcl_io_standard, Tcl_Obj* tcl_ports) {
        const char* io_standard = Tcl_GetString(tcl_io_standard);
        
        auto ports = tcl_obj_getlist<AtomPortId>(static_cast<TclClient*>(this), tcl_ports);
        
        return this->_do_set_property_iostandard(io_standard, ports);
    }

    int _do_set_property_iostandard(const char* io_standard, TclList<AtomPortId>& ports) {
        auto iterator = ports.begin();
        for (auto port : ports) {
            if (port == nullptr)
                return this->_ret_error("set_property: port_name of IOSTANDARD should have a `AtomPortId` type.");
            AtomBlockId block = this->netlist.port_block(*port);
            this->netlist.set_block_param(block, "IOSTANDARD", io_standard);
            VTR_LOG("XDC: Assigned IOSTANDARD %s = %s\n", this->netlist.block_name(block).c_str(), io_standard);
        }

        return this->_ret_ok();
    }

    int _do_get_ports(std::vector<const char*> pin_names) {
        std::vector<AtomPortId> port_ids;
        for (auto pin_name : pin_names) {
            AtomPortId port;
            int result = this->_do_get_port(pin_name, &port);
            if (result != TCL_OK)
                return result;
            port_ids.push_back(port);
        }

        auto port_list = this->_list<AtomPortId, decltype(port_ids)>(this, std::move(port_ids));
        this->_ret_list<AtomPortId>(reinterpret_cast<void*>(this), port_list);
    }

    int _do_get_port(const char* pin_name, AtomPortId* port_) {
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

            return this->_ret_error("get_ports: Can't find ports for `" +
                                    std::string(pin_name) + "`. Available blocks: " +
                                    available_ports);
        }
        
        auto ports = this->netlist.block_ports(pin_block);

        auto port_it = this->netlist.block_ports(pin_block).begin();
        if (port_it == ports.end())
            return this->_ret_error("get_ports: '" + std::string(pin_name) + "' - no ports found.");

        /* We assume that there's only one port, which just happens to be true in case of fpga interchange.
         * If we ever want to handle it better, we should return a list of port ids instead.
         */
        VTR_ASSERT(ports.size() == 1);
        AtomPortId port = *ports.begin();
        std::string port_name = this->netlist.block_name(pin_block);

        VTR_LOG("get_ports: Got port `%s` (%llu) for pin `%s`\n", port_name.c_str(), (size_t)port, pin_name);

        *port_ = port;
        return this->_ret_ok();
    }
};

REGISTER_TCL_TYPE_W_STR_UPDATE(AtomPortId) (Tcl_Obj* obj) {
    const auto* port = tcl_obj_getptr<AtomPortId>(obj);
    const auto* client = tcl_obj_get_ctx_ptr<TclPhysicalConstraintsClient>(obj);
    std::string port_name = client->netlist.port_name(*port);
    tcl_set_obj_string(obj, port_name);
} END_REGISTER_TCL_TYPE;

VprConstraints read_xdc_constraints_to_vpr(std::istream& xdc_stream, const t_arch& arch, AtomNetlist& netlist) {

    VprConstraints constraints;
    TclPhysicalConstraintsClient pc_client(constraints, arch, netlist);

    TclCtx::with_ctx<void>([&](TclCtx& ctx) {
        /*
         * Tcl by default will interpret bus indices as calls to commands
         * (eg. in[0] gets interpreted as call to command `i` with argument being a
         * result of calling `0`). This overrides this behaviour.
         */
        std::istringstream fix_port_indexing(
            "rename unknown _original_unknown\n"
            "proc unknown args {\n"
            "  set result [scan [lindex $args 0] \"%d\" value]\n"
            "  if { $result == 1 && [llength $args] == 1 } {\n"
            "    return \\[$value\\]\n"
            "  } else {\n"
            "    uplevel 1 [list _original_unknown {*}$args]\n"
            "  }\n"
            "}"
        );
        ctx.read_tcl(fix_port_indexing);
        
        /* Add types and commands to handle XDC files */
        ctx.add_tcl_type<AtomPortId>();
        ctx.add_tcl_client<TclPhysicalConstraintsClient>(pc_client);

        /* Read and interpret XDC */
        ctx.read_tcl(xdc_stream);
    });

    /* At this point `pc_client` has written the contraints */
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
