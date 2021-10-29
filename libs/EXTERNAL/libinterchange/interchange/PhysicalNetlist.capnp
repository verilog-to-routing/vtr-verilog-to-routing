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
@0xcb2ccd67aa912968;
using Java = import "/capnp/java.capnp";
$Java.package("com.xilinx.rapidwright.interchange");
$Java.outerClassname("PhysicalNetlist");

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("PhysicalNetlist");

using Ref = import "References.capnp";

struct StringRef {
    type  @0 :Ref.ReferenceType = rootValue;
    field @1 :Text = "strList";
}
annotation stringRef(*) :StringRef;
using StringIdx = UInt32;

struct HashSet {
    type  @0 : Ref.ImplementationType = enumerator;
    hide  @1 : Bool = true;
}
annotation hashSet(*) :HashSet;

struct PhysNetlist {

  part         @0 : Text;
  placements   @1 : List(CellPlacement);
  physNets     @2 : List(PhysNet);
  physCells    @3 : List(PhysCell);
  strList      @4 : List(Text) $hashSet();
  siteInsts    @5 : List(SiteInstance);
  properties   @6 : List(Property);
  nullNet      @7 : PhysNet;

  struct PinMapping {
    cellPin    @0 : StringIdx $stringRef();
    bel        @1 : StringIdx $stringRef();
    belPin     @2 : StringIdx $stringRef();
    isFixed    @3 : Bool;
    union {
      multi     @4 : Void;
      otherCell @5 : MultiCellPinMapping;
    }
  }

  struct MultiCellPinMapping {
    multiCell  @0 : StringIdx $stringRef();
    multiType  @1 : StringIdx $stringRef();
  }

  struct CellPlacement {
    cellName      @0 : StringIdx $stringRef();
    type          @1 : StringIdx $stringRef();
    site          @2 : StringIdx $stringRef();
    bel           @3 : StringIdx $stringRef();
    pinMap        @4 : List(PinMapping);
    otherBels     @5 : List(StringIdx) $stringRef();
    isBelFixed    @6 : Bool;
    isSiteFixed   @7 : Bool;
    altSiteType   @8 : StringIdx $stringRef();
  }

  struct PhysCell {
    cellName    @0 : StringIdx $stringRef();
    physType    @1 : PhysCellType;
  }

  enum PhysCellType {
    locked  @0;
    port    @1;
    gnd     @2;
    vcc     @3;
  }

  struct PhysNet {
    name      @0 : StringIdx $stringRef();
    sources   @1 : List(RouteBranch);
    stubs     @2 : List(RouteBranch);
    type      @3 : NetType = signal;
  }

  enum NetType {
    signal   @0;
    gnd      @1;
    vcc      @2;
  }


  struct RouteBranch {
    routeSegment : union {
      belPin  @0 : PhysBelPin;
      sitePin @1 : PhysSitePin;
      pip     @2 : PhysPIP;
      sitePIP @3 : PhysSitePIP;
    }
    branches @4 : List(RouteBranch);
  }

  struct PhysBel {
    site @0 : StringIdx $stringRef();
    bel  @1 : StringIdx $stringRef();
  }

  struct PhysBelPin {
    site @0 : StringIdx $stringRef();
    bel  @1 : StringIdx $stringRef();
    pin  @2 : StringIdx $stringRef();
  }

  struct PhysSitePin {
    site @0 : StringIdx $stringRef();
    pin  @1 : StringIdx $stringRef();
  }

  struct PhysPIP {
    tile    @0 : StringIdx $stringRef();
    wire0   @1 : StringIdx $stringRef();
    wire1   @2 : StringIdx $stringRef();
    forward @3 : Bool;
    isFixed @4 : Bool;
    # In case of a pseudo PIP also the traversed site
    # needs to be added to the PhysPIP object
    union {
      noSite @5: Void;
      # It is assumed that a pseudo PIP can traverse one
      # site only
      site   @6: StringIdx $stringRef();
    }
  }

  struct PhysSitePIP {
    site    @0 : StringIdx $stringRef();
    bel     @1 : StringIdx $stringRef();
    pin     @2 : StringIdx $stringRef();
    isFixed @3 : Bool;
    union {
      isInverting @4 : Bool;
      inverts     @5 : Void;
    }
  }

  struct SiteInstance {
    site  @0 : StringIdx $stringRef();
    type  @1 : StringIdx $stringRef();
  }

  struct Property {
    key   @0 : StringIdx $stringRef();
    value @1 : StringIdx $stringRef();
  }
}
