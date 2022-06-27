# Copyright 2020-2021 Xilinx, Inc. and Google, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

@0x9d262c6ba6512325;
using Java = import "/capnp/java.capnp";
using Ref = import "References.capnp";
using Dir = import "LogicalNetlist.capnp";
$Java.package("com.xilinx.rapidwright.interchange");
$Java.outerClassname("DeviceResources");

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("DeviceResources");

struct HashSet {
    type  @0 : Ref.ImplementationType = enumerator;
    hide  @1 : Bool = true;
}
annotation hashSet(*) :HashSet;

struct StringRef {
    type  @0 :Ref.ReferenceType = rootValue;
    field @1 :Text = "strList";
}
annotation stringRef(*) :StringRef;
using StringIdx = UInt32;

struct SiteTypeRef {
    type  @0 :Ref.ReferenceType = root;
    field @1 :Text = "siteTypeList";
}
annotation siteTypeRef(*) :SiteTypeRef;
using SiteTypeIdx = UInt32;

struct BELPinRef {
    type  @0 :Ref.ReferenceType = parent;
    field @1 :Text = "belPins";
    depth @2 :Int32 = 1;
}
annotation belPinRef(*) :BELPinRef;
using BELPinIdx = UInt32;

struct WireRef {
    type  @0 :Ref.ReferenceType = parent;
    field @1 :Text = "wires";
    depth @2 :Int32 = 1;
}
annotation wireRef(*) :WireRef;
using WireIdx = UInt32;

struct WireTypeRef {
    type  @0 :Ref.ReferenceType = rootValue;
    field @1 :Text = "wireTypes";
}
annotation wireTypeRef(*) :WireTypeRef;
using WireTypeIdx = UInt32;

using WireIDInTileType = UInt32; # ID in Tile Type
using SitePinIdx = UInt32;

struct TileTypeRef {
    type  @0 :Ref.ReferenceType = parent;
    field @1 :Text = "tileTypeList";
    depth @2 :Int32 = 1;
}
annotation tileTypeRef(*) :TileTypeRef;
using TileTypeIdx = UInt32;

using TileTypeSiteTypeIdx = UInt32;
using TileTypeSubTileIdx = UInt16;

struct PIPTimingRef {
  type  @0 :Ref.ReferenceType = rootValue;
  field @1 :Text = "pipTimings";
}
annotation pipTimingRef(*) :PIPTimingRef;
using PipTimingIdx = UInt32;

struct NodeTimingRef {
  type  @0 :Ref.ReferenceType = rootValue;
  field @1 :Text = "nodeTimings";
}
annotation nodeTimingRef(*) :NodeTimingRef;
using NodeTimingIdx = UInt32;

struct Device {

  name            @0 : Text;
  strList         @1 : List(Text) $hashSet();
  siteTypeList    @2 : List(SiteType);
  tileTypeList    @3 : List(TileType);
  tileList        @4 : List(Tile);
  wires           @5 : List(Wire);
  nodes           @6 : List(Node);
  # Netlist libraries of Unisim primitives and macros
  # The library containing primitives should be called "primitives", and
  # the library containing macros called "macros".
  primLibs        @7 : Dir.Netlist;
  exceptionMap    @8 : List(PrimToMacroExpansion); # Prims to macros expand w/same name, except these
  cellBelMap      @9 : List(CellBelMapping);
  cellInversions @10 : List(CellInversion);
  packages       @11 : List(Package);
  constants      @12 : Constants;
  constraints    @13 : Constraints;
  lutDefinitions @14 : LutDefinitions;
  parameterDefs  @15 : ParameterDefinitions;
  wireTypes      @16 : List(WireType);
  pipTimings     @17 : List(PIPTiming);
  nodeTimings    @18 : List(NodeTiming);

  #######################################
  # Placement definition objects
  #######################################
  struct SiteType {
    name         @0 : StringIdx $stringRef();
    belPins      @1 : List(BELPin); # All BEL Pins in site type
    pins         @2 : List(SitePin);
    lastInput    @3 : UInt32; # Index of the last input pin
    bels         @4 : List(BEL);
    sitePIPs     @5 : List(SitePIP);
    siteWires    @6 : List(SiteWire);
    altSiteTypes @7 : List(SiteTypeIdx);
  }

  # Maps site pins from alternative site types to the parent primary site pins
  struct ParentPins {
    # pins[0] is the mapping of the siteTypeList[altSiteType].pins[0] to the
    # primary site pin index.
    #
    # To determine the tile wire for a alternative site type, first get the
    # site pin index for primary site, then use primaryPinsToTileWires.
    pins  @0 : List(SitePinIdx);
  }

  struct SiteTypeInTileType {
    primaryType @0 : SiteTypeIdx $siteTypeRef();

    # primaryPinsToTileWires[0] is the tile wire that matches
    # siteTypeList[primaryType].pins[0], etc.
    primaryPinsToTileWires @1 : List(StringIdx) $stringRef();

    # altPinsToPrimaryPins[0] is the mapping for
    # siteTypeList[primaryType].altSiteTypes[0], etc.
    altPinsToPrimaryPins @2 : List(ParentPins);
  }

  struct TileType {
    name       @0 : StringIdx $stringRef();
    siteTypes  @1 : List(SiteTypeInTileType);
    wires      @2 : List(StringIdx) $stringRef();
    pips       @3 : List(PIP);
    constants  @4 : List(WireConstantSources);
  }

  #######################################
  # Placement instance objects
  #######################################
  struct BELInverter {
    nonInvertingPin @0 : BELPinIdx $belPinRef(depth = 2);
    invertingPin    @1 : BELPinIdx $belPinRef(depth = 2);
  }

  struct BEL {
    name      @0 : StringIdx $stringRef();
    type      @1 : StringIdx $stringRef();
    pins      @2 : List(BELPinIdx) $belPinRef();
    category  @3 : BELCategory; # This would be BELClass/class, but conflicts with Java language
    union {
        nonInverting @4 : Void;
        inverting    @5 : BELInverter;
    }
  }

  enum BELCategory {
    logic    @0;
    routing  @1;
    sitePort @2;
  }

  struct Site {
    name      @0 : StringIdx $stringRef();
    type      @1 : TileTypeSiteTypeIdx; # Index into TileType.siteTypes
  }

  struct Tile {
    name       @0 : StringIdx $stringRef();
    type       @1 : TileTypeIdx $tileTypeRef();
    sites      @2 : List(Site);
    row        @3 : UInt16;
    col        @4 : UInt16;

    # Field ordinal 5 was deleted.
    deleted    @5 : UInt32;

    # Sub-tiles enable PIPs inside a tile to use different FASM prefices
    # This is needed for the Nexus, where there can be multiple tiles from a
    # bitstream perspective at the same (row, col) grid location. PIP.subTile
    # indexes into this list to get a prefix for FASM purposes.
    # If sub-tiles are not used; then this list is empty and an implicit
    # sub-tile index 0 has the same prefix as the tile name.
    subTilesPrefices @6 : List(StringIdx) $stringRef();
  }

  ######################################
  # Intra-site routing resources
  ######################################
  struct BELPin {
    name   @0 : StringIdx $stringRef();
    dir    @1 : Dir.Netlist.Direction;
    bel    @2 : StringIdx $stringRef();
  }

  struct SiteWire {
    name   @0 : StringIdx $stringRef();
    pins   @1 : List(BELPinIdx) $belPinRef();
  }

  struct SitePIP {
    inpin  @0 : BELPinIdx $belPinRef();
    outpin @1 : BELPinIdx $belPinRef();
    # Interconnect delay
    delay  @2 : CornerModel;
  }

  struct SitePin {
    name     @0 : StringIdx $stringRef();
    dir      @1 : Dir.Netlist.Direction;
    belpin   @2 : BELPinIdx $belPinRef();
    model : union {
      noModel     @5 : Void;
      resistance  @3 : CornerModel;
      capacitance @4 : CornerModel;
    }
    delay    @6 : CornerModel;
  }

  ######################################
  # Inter-site routing resources
  ######################################

  struct Wire {
    tile      @0 : StringIdx $stringRef();
    wire      @1 : StringIdx $stringRef();
    type      @2 : WireTypeIdx $wireTypeRef();
  }

  enum WireCategory {
    # general interconnect, usually with many uphill and downhill pips and spanning multiple tiles
    general @0;
    # pin/local wires, carry chains, dedicated paths, everything else
    special @1;
    # the global clock network
    global  @2;
  }

  # This is used to distinguish between different types of wires, in order to provide extra hints
  # during routing, such as the category of wires. It is also intended to be able to describe
  # complex routing requirements, such as global routing which requires a series of different types
  # of wires to be used in succession
  struct WireType {
    name     @0 : StringIdx $stringRef();
    category @1 : WireCategory;
  }

  struct Node {
    wires      @0 : List(WireIdx) $wireRef();
    nodeTiming @1 : NodeTimingIdx $nodeTimingRef();
  }

  struct PIP {
    wire0        @0 : WireIDInTileType;
    wire1        @1 : WireIDInTileType;
    directional  @2 : Bool;
    buffered20   @3 : Bool;
    buffered21   @4 : Bool;
    union {
      conventional @5 : Void;
      pseudoCells  @6 : List(PseudoCell);
    }
    subTile      @7 : TileTypeSubTileIdx; # Index into Tile.subTilesPrefices
    timing       @8 : PipTimingIdx $pipTimingRef();
  }

  struct PseudoCell {
    bel          @0 : StringIdx $stringRef();
    pins         @1 : List(StringIdx) $stringRef();
  }

  struct WireConstantSources {
      wires        @0 : List(WireIDInTileType);
      constant     @1 : ConstantType;
  }

  # Table lookup map, for string parameters
  struct ParameterMapEntry {
    # If the primitive parameter matches 'from', then the macro
    # instance parameter will be set to 'to'.
    from @0 : StringIdx $stringRef();
    to   @1 : StringIdx $stringRef();
  }

  # This describes how we map a parameter from primitive to its
  # macro expansion.
  struct ParameterMapRule {
    # Name of the parameter in its parent primitive
    primParam @0 : StringIdx $stringRef();
    # Name of the cell instance to set the derived parameter on
    instName  @1 : StringIdx $stringRef();
    # Name of the parameter on the cell instance to set
    instParam @2 : StringIdx $stringRef();
    # How to derive the new parameter value
    union {
      # Copy the value directly across with no transform applied
      copyValue   @3 : Void;
      # Apply an arbitrary mapping of bits while deriving the new value.
      # Bit i of the derived value will be taken from bit bitSlice[i] of the
      # parent primitive parameter. This way bit ranges; every Nth bit and
      # permutation can all be represented.
      bitSlice    @4 : List(UInt32);
      # Use a table lookup to derive a new value for a string parameter.
      tableLookup @5 : List(ParameterMapEntry);
    }
  }

  ######################################
  # Macro expansion exception map for
  # primitives that don't expand to a
  # macro of the same name. This is also
  # used for conditional matches on
  # parameter values and parameter
  # transforms from primitive to
  # expansion.
  ######################################
  struct PrimToMacroExpansion {
    primName  @0 : StringIdx $stringRef();
    macroName @1 : StringIdx $stringRef();
    # Optionally, primitive to macro expansions can be conditional on a
    # parameter match. For example, I/O buffer expansions might be
    # different between true and pseudo differential IO types. The
    # expansion is used if **any** of the parameters specified match.
    union {
      always     @2 : Void;
      parameters @3 : List(Dir.Netlist.PropertyMap.Entry);
    }
    # These rules are used to map parameters from the primitive to the
    # constituent cell instances for the macro. For example; a LUTRAM
    # primitive might have its init value split up to the init values
    # of its constituent LUTs.
    paramMapping @4 : List(ParameterMapRule);
  }

  # Cell <-> BEL and Cell pin <-> BEL Pin mapping
  struct CellBelMapping {
    cell          @0 : StringIdx $stringRef();
    commonPins    @1 : List(CommonCellBelPinMaps);
    parameterPins @2 : List(ParameterCellBelPinMaps);
    pinsDelay     @3 : List(PinsDelay);
  }

  # Map one cell pin to one BEL pin.
  # Note: There may be more than one BEL pin mapped to one cell pin.
  # If cellPin is "GND" then the BEL pin is tied to ground, and if cellPin
  # is "VCC" then the BEL pin is tied to Vcc. This is used where BEL pins
  # must be tied to a constant, but there is no corresponding cell pin.
  struct CellBelPinEntry {
    cellPin @0 : StringIdx $stringRef();
    belPin  @1 : StringIdx $stringRef();
  }

  # Specifies BELs located in a specific site type.
  struct SiteTypeBelEntry {
    siteType @0 : StringIdx $stringRef();
    bels     @1 : List(StringIdx) $stringRef();
  }

  # This is the portion of Cell <-> BEL pin mapping that is common across all
  # parameter settings for a specific site type and BELs within that site
  # type.
  struct CommonCellBelPinMaps {
    siteTypes @0 : List(SiteTypeBelEntry);
    pins      @1 : List(CellBelPinEntry);
  }

  # This is the portion of the Cell <-> BEL pin mapping that is parameter
  # specific.
  struct ParameterSiteTypeBelEntry {
    bel       @0 : StringIdx $stringRef();
    siteType  @1 : StringIdx $stringRef();
    parameter @2 : Dir.Netlist.PropertyMap.Entry;
  }

  struct ParameterCellBelPinMaps {
    parametersSiteTypes @0 : List(ParameterSiteTypeBelEntry);
    pins                @1 : List(CellBelPinEntry);
  }

  struct Package {
    struct PackagePin {
        packagePin @0 : StringIdx $stringRef();
        site : union {
            noSite     @1 : Void;
            site       @2 : StringIdx $stringRef();
        }
        bel : union {
            noBel      @3 : Void;
            bel        @4 : StringIdx $stringRef();
        }
    }

    struct Grade {
        name             @0 : StringIdx $stringRef();
        speedGrade       @1 : StringIdx $stringRef();
        temperatureGrade @2 : StringIdx $stringRef();
    }

    name        @0 : StringIdx $stringRef();
    packagePins @1 : List(PackagePin);
    grades      @2 : List(Grade);
  }

  # Constants
  enum ConstantType {
      # Routing a VCC or GND are equal cost.
      noPreference @0;
      # Routing a GND has the best cost.
      gnd          @1;
      # Routing a VCC has the best cost.
      vcc          @2;
  }

  struct Constants {
    struct SitePinConstantExceptions {
        siteType     @0 : StringIdx $stringRef();
        sitePin      @1 : StringIdx $stringRef();
        bestConstant @2 : ConstantType;
    }

    struct SiteConstantSource {
        siteType     @0 : StringIdx $stringRef();
        bel          @1 : StringIdx $stringRef();
        belPin       @2 : StringIdx $stringRef();
        constant     @3 : ConstantType;
    }

    struct NodeConstantSource {
        tile     @0 : StringIdx $stringRef();
        wire     @1 : StringIdx $stringRef();
        constant @2 : ConstantType;
    }

    # These structures are used to define default constant values for unused
    # or missing cell pins. For each cell type, we have a list of pins and
    # what to do with those pins (which will be to tie it to 0 or 1 in most
    # cases).
    enum CellPinValue {
      # leave floating
      float        @0;
      # connect to ground
      gnd          @1;
      # connect to vcc
      vcc          @2;
    }

    struct DefaultCellConnection {
      # What is the name of this cell pin?
      name    @0 : StringIdx $stringRef();
      # The default constant value for the pin if missing or disconnected
      value   @1 : CellPinValue;
    }

    struct DefaultCellConnections {
      # The type of the cell we're providing a list of defaults for
      cellType @0 : StringIdx $stringRef();
      # The list of default cell pin values
      pins     @1 : List(DefaultCellConnection);
    }

    # When either constant signal can be routed to an input site pin, which
    # constant should be used by default?
    #
    # For example, if a site pin has a local inverter and a cell requires a
    # constant signal, then either a gnd or vcc could be routed to the site.
    # The inverter can be used to select which ever constant is needed,
    # regardless of what constant the cell requires.  In some fabrics, routing
    # a VCC or routing a GND is significantly easier than the other.
    defaultBestConstant    @0 : ConstantType;

    # If there are exceptions to the default best constant, then this list
    # specifies which site pins use a different constant.
    bestConstantExceptions @1 : List(SitePinConstantExceptions);

    # List of constants that can be found within the routing graph without
    # consuming a BEL.
    #
    # Tools can always generate a constant source from a LUT BEL type.
    siteSources            @2 : List(SiteConstantSource);

    # Most tied nodes are handled under TileType.constants, however in some
    # exceptional cases, the tying is inconsistent between tile types.
    # nodeSources should be used to explicitly list nodes that fall into this
    # case.
    nodeSources            @3 : List(NodeConstantSource);

    # Name of GND and VCC cells and their pins that are tied to GND/VCC.
    gndCellType            @4 : StringIdx $stringRef();
    gndCellPin             @5 : StringIdx $stringRef();

    vccCellType            @6 : StringIdx $stringRef();
    vccCellPin             @7 : StringIdx $stringRef();

    # If the format requires a specific gnd/vcc net name, what name should be
    # used?
    gndNetName : union {
        anyName @8 : Void;
        name    @9 : StringIdx $stringRef();
    }

    vccNetName : union {
        anyName @10 : Void;
        name    @11 : StringIdx $stringRef();
    }

    # How to treat missing/disconnected cell pins
    defaultCellConns       @12 : List(DefaultCellConnections);
  }

  ######################################
  # Inverting pins description
  #
  # This block describes local site wire
  # inverters, site routing BELs, and
  # parameters.
  ######################################
  struct CellPinInversionParameter {
    union {
      # This inverter cannot be controlled by parameter, only via tool merging
      # of INV cells or other means.
      invOnly      @0 : Void;
      # This inverter can be controlled by a parameter.
      # What parameter value configures this setting?
      parameter    @1 : Dir.Netlist.PropertyMap.Entry;
    }
  }

  struct CellPinInversion {
    # Which cell pin supports a local site inverter?
    cellPin      @0 : StringIdx $stringRef();

    # What parameters are used for the non-inverting case, and how to route
    # through the inversion routing bels (if any).
    notInverting @1 : CellPinInversionParameter;

    # What parameters are used for the inverting case, and how to route
    # through the inversion routing bels (if any).
    inverting    @2 : CellPinInversionParameter;
  }

  struct CellInversion {
    # Which cell is being described?
    cell     @0 : StringIdx $stringRef();

    # Which cell have site local inverters?
    cellPins @1 : List(CellPinInversion);
  }

  ######################################
  # Timing modeling
  #
  # This section defines the timing model represantation
  # for the interchange schema.
  #
  # Even though there is no strict standard to define how many
  # corner models need to be defined for a given architecture,
  # a timing delay model usually includes a "fast" and a
  # "slow" corner process, each with three delay measures:
  #  - minimum
  #  - typical
  #  - maximum
  #
  #  The idea is to have a static definition of the corner models
  #  so to standardize the interchange format to use at maximum
  #  two process corner models (fast and slow), with the respective
  #  delay measures. If an architecture does not include one or two
  #  (but not all of three) delay measures, the timing model is still valid.
  #
  # There are three main location where timing delays must be defined:
  #  - BEL/Cell pins
  #  - Wires
  #  - PIPs
  #
  #  * BEL/Cell pins:
  #  These delays can be sequential (associated to a clock pin) or combinatorial
  #  (associated to an input/output pair).
  #  At minimum there are four types of pin delays that need to be considered:
  #   - comb    : input to output delay
  #   - setup   : associated to the input and a clock pin
  #   - hold    : associated to the input and a clock pin
  #   - clock2q : associated to the clock and the output pin
  #
  #  * Wires:
  #  These delays are associated to wires, or better nodes which are collection
  #  of electrically connected wires.
  #  They are described following the RC (Resistance/Capacitance) modeling.
  #
  # * PIPs:
  # These are delays corresponding to the connections between two wires.
  #
  ######################################

  # BEL/Cell pins delays
  struct PinsDelay {
    firstPin      @0 : PinDelay;
    secondPin     @1 : PinDelay;
    cornerModel   @2 : CornerModel;
    pinsDelayType @3 : PinsDelayType;
    site          @4 : SiteTypeIdx $siteTypeRef();
  }

  struct PinDelay {
    pin   @0 : BELPinIdx;
    union {
      noClock   @1 : Void;
      clockEdge @2 : ClockEdge;
    }
  }

  enum ClockEdge {
    rise @0;
    fall @1;
  }

  enum PinsDelayType {
    comb  @0;
    setup @1;
    hold  @2;
    clk2q @3;
  }

  # Wire (nodes) delays
  struct NodeTiming {
    capacitance @0 : CornerModel;
    resistance  @1 : CornerModel;
  }

  # PIP (switches) delays
  struct PIPTiming {
    inputCapacitance    @0 : CornerModel;
    internalCapacitance @1 : CornerModel;
    internalDelay       @2 : CornerModel;
    outputResistance    @3 : CornerModel;
    outputCapacitance   @4 : CornerModel;
  }

  struct CornerModel {
    slow : union {
      noSlow @0 : Void;
      slow   @1 : CornerModelValues;
    }

    fast : union {
      noFast @2 : Void;
      fast   @3 : CornerModelValues;
    }
  }

  struct CornerModelValues {
    min : union {
      noMin @0 : Void;
      min   @1 : Float32;
    }

    typ : union {
      noTyp @2 : Void;
      typ   @3 : Float32;
    }

    max : union {
      noMax @4 : Void;
      max   @5 : Float32;
    }
  }

  ######################################
  # Placement constraints
  #
  # This section defines constraints required for valid placement above and
  # beyond routing constraints.
  #
  # This section has three sections:
  #  - Tags
  #  - Routed tags
  #  - Cell constraints
  #
  # The tags sections defines a list of tags that take one of an exclusive set
  # of states.  Tags are attached to an object within the FPGA fabric.
  # Currently tags can only be attached to either a site type or tile type.
  # All instances of that site type or tile type will have it's own unique
  # instance of the tag.  In order for the constraints to be considered "met",
  # each tag instance must be constrained to either 0 or 1 states.  In the
  # event that a tag instance is constrained to 0 states, then that tag
  # instance value will be the "default" state.
  #
  # Routed tags are required to express instances where a cell constraint
  # relates through a site routing mux to a tag. Routed tags route from a tag
  # or routed tag to either another routed tag or a cell constraint.
  #
  # Cell constraints are the list of constraints that are applied (or removed)
  # when a cell is placed (or unplaced).  The format of the cell constraint
  # can be read as:
  #  - When a cell of a specific type is (cell/cells field)
  #  - Is placed at (siteTypes and bel field)
  #  - Apply the following constraints (implies/requires field).
  #
  # In the case where a cell constraint references a routed tag, then the
  # constraint also applies to the upstream tag or routed tag.
  ######################################
  struct Constraints {
    struct State {
      state       @0 :Text;
      description @1 :Text;
    }

    struct Tag {
      tag         @0 :Text;
      description @1 :Text;
      default     @2 :Text;
      union {
        siteTypes @3 :List(Text);
        tileTypes @4 :List(Text);
      }
      states      @5 :List(State);
    }

    struct RoutedTagPin {
      pin @0 :Text;
      tag @1 :Text;
    }

    struct RoutedTag {
      routedTag  @0 :Text;
      routingBel @1 :Text;
      belPins    @2 :List(RoutedTagPin);
    }

    struct RoutedTagPort {
      tag  @0 :Text;
      port @1 :Text;
    }

    struct TagPair {
      union {
        tag       @0 :Text;
        routedTag @1 :RoutedTagPort;
      }
      state       @2 :Text;
    }

    struct TagStates {
      union {
        tag       @0 :Text;
        routedTag @1 :RoutedTagPort;
      }
      states      @2 :List(Text);
    }

    struct BELLocation {
      union {
        anyBel @0 :Void;
        name   @1 :Text;
        bels   @2 :List(Text);
      }
    }

    struct ConstraintLocation {
      siteTypes  @0 :List(Text);
      bel        @1 :BELLocation;
      union {
        implies  @2 :List(TagPair);
        requires @3 :List(TagStates);
      }
    }

    struct CellConstraint {
      union {
        cell    @0 :Text;
        cells   @1 :List(Text);
      }
      locations @2 :List(ConstraintLocation);
    }

    tags            @0 :List(Tag);
    routedTags      @1 :List(RoutedTag);
    cellConstraints @2 :List(CellConstraint);
  }

  ######################################
  # LUT definitions
  ######################################
  struct LutDefinitions {
    struct LutCell {
      # What cell type is this?
      cell        @0 : Text;

      # What pins are part of the LUT equations?
      # Pins are listed in LSB first order.
      inputPins   @1 : List(Text);

      # How is the LUT equation stored?
      equation : union {
        # Equation is stored as an INIT style parameter.
        # For a LUT2, INIT is 4 bits.
        #   INIT[0] is output when all pins are 0.
        #   INIT[1] is output when first pin is 1 and all other pins are 0.
        #   INIT[2] is output when second pin is 1 and all other pins are 0.
        #   INIT[3] is output when both pins are 1.
        initParam @2 : Text;
        invalid   @3 : Void;
      }
    }

    struct LutBel {
      # Name of the BEL that is part of this element
      name      @0 : Text;
      # What pins are part of this BEL?
      # Pins are listed in LSB first order.
      inputPins @1 : List(Text);
      # What is the output pin of this LUT?
      outputPin @2 : Text;
      # What bits within the LutElement does this BEL use?
      # Bits must be consecutive.
      lowBit    @3 : Int8;
      highBit   @4 : Int8;
    }

    # Each LUT element in the site should have a width.
    struct LutElement {
      width @0 : Int8;
      # If 2 or more BELs share the same underlying equation shortage,
      # how are the BELs related?
      bels  @1 : List(LutBel);
    }

    # How are LUT BELs laid out in a site?
    struct LutElements {
      site @0 : Text;
      luts @1 : List(LutElement);
    }

    # Which cells are LUT equations?
    lutCells    @0 : List(LutCell);

    # Which sites have LUT BELs?
    lutElements @1 : List(LutElements);
  }

  enum ParameterFormat {
    string        @0;
    # TRUE/FALSE/1/0
    boolean       @1;
    # 0/1/256
    integer       @2;
    # 0.0
    floatingPoint @3;
    # 1'b0
    verilogBinary @4;
    # 8'hF
    verilogHex    @5;
    # 0b10
    cBinary       @6;
    # 0xF
    cHex          @7;
  }

  struct ParameterDefinition {
    # What is the name of this parameter?
    name    @0 : StringIdx;

    # When a parameter is stored as a string, what presentation should it use?
    #
    # If the default is stored as a string, it must be in this presentation.
    # If a logical netlist stores this parameter as a string, it must be in
    # this presentation.
    format  @1 : ParameterFormat;

    # Default parameter value
    #
    # Note: key should also be set for ease of copy into
    # LogicalNetlist.
    default @2 : Dir.Netlist.PropertyMap.Entry;
  }

  struct CellParameterDefinition {
    cellType   @0 : StringIdx;
    parameters @1 : List(ParameterDefinition);
  }

  struct ParameterDefinitions {
    cells @0 : List(CellParameterDefinition);
  }
}
