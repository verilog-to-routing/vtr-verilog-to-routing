// Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

`include "../../../third_party/mips32r1_core/mips32r1/Add.v"
`include "../../../third_party/mips32r1_core/mips32r1/Compare.v"
`include "../../../third_party/mips32r1_core/mips32r1/CPZero.v"
`include "../../../third_party/mips32r1_core/mips32r1/EXMEM_Stage.v"
`include "../../../third_party/mips32r1_core/mips32r1/IDEX_Stage.v"
`include "../../../third_party/mips32r1_core/mips32r1/MemControl.v"
`include "../../../third_party/mips32r1_core/mips32r1/MIPS_Parameters.v"
`include "../../../third_party/mips32r1_core/mips32r1/Mux4.v"
`include "../../../third_party/mips32r1_core/mips32r1/RegisterFile.v"
`include "../../../third_party/mips32r1_core/mips32r1/TrapDetect.v"
`include "../../../third_party/mips32r1_core/mips32r1/ALU.v"
`include "../../../third_party/mips32r1_core/mips32r1/Control.v"
`include "../../../third_party/mips32r1_core/mips32r1/Divide.v"
`include "../../../third_party/mips32r1_core/mips32r1/Hazard_Detection.v"
`include "../../../third_party/mips32r1_core/mips32r1/IFID_Stage.v"
`include "../../../third_party/mips32r1_core/mips32r1/MEMWB_Stage.v"
`include "../../../third_party/mips32r1_core/mips32r1/Mux2.v"
`include "../../../third_party/mips32r1_core/mips32r1/Processor.v"
`include "../../../third_party/mips32r1_core/mips32r1/Register.v"