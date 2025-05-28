#pragma once

#include "aig/gia/gia.h"
#include "base/main/main.h"
#include "base/cmd/cmd.h"

#include "rrrUtils.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  template <typename Ntk>
  void GiaReader(Gia_Man_t *pGia, Ntk *pNtk) {
    int i;
    Gia_Obj_t *pObj;
    pNtk->Reserve(Gia_ManObjNum(pGia));
    Gia_ManConst0(pGia)->Value = pNtk->GetConst0();
    Gia_ManForEachObj1(pGia, pObj, i) {
      if(Gia_ObjIsCi(pObj)) {
        pObj->Value = pNtk->AddPi();
      } else if(Gia_ObjIsCo(pObj)) {
        pNtk->AddPo(Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj));
      } else {
        // TODO: support XOR (and BUF and MUX?), maybe create another function
        pObj->Value = pNtk->AddAnd(Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj));
      }
    }
  }

  template <typename Ntk>
  Gia_Man_t *CreateGia(Ntk *pNtk) {
    Gia_Man_t *pGia = Gia_ManStart(pNtk->GetNumNodes());
    Gia_ManHashStart(pGia);
    std::vector<int> v(pNtk->GetNumNodes());
    v[0] = Gia_ManConst0Lit();
    pNtk->ForEachPi([&](int id) {
      v[id] = Gia_ManAppendCi(pGia);
    });
    pNtk->ForEachInt([&](int id) {
      assert(pNtk->GetNodeType(id) == rrr::AND);
      int x = -1;
      pNtk->ForEachFanin(id, [&](int fi, bool c) {
        if(x == -1) {
          x = Abc_LitNotCond(v[fi], c);
        } else {
          x = Gia_ManHashAnd(pGia, x, Abc_LitNotCond(v[fi], c));
        }
      });
      if(x == -1) {
        x = Abc_LitNot(v[0]);
      }
      v[id] = x;
    });
    pNtk->ForEachPoDriver([&](int fi, bool c) {
      Gia_ManAppendCo(pGia, Abc_LitNotCond(v[fi], c));
    });
    Gia_ManHashStop(pGia);
    return pGia;  
  }

  template <typename Ntk>
  void Abc9Execute(Ntk *pNtk, std::string Command) {
    Abc_Frame_t *pAbc = Abc_FrameGetGlobalFrame();
    Abc_FrameUpdateGia(pAbc, CreateGia(pNtk));
    if(Abc_FrameIsBatchMode()) {
      int r = Cmd_CommandExecute(pAbc, Command.c_str());
      assert(r == 0);
    } else {
      Abc_FrameSetBatchMode(1);
      int r = Cmd_CommandExecute(pAbc, Command.c_str());
      assert(r == 0);
      Abc_FrameSetBatchMode(0);
    }
    pNtk->Read(Abc_FrameReadGia(pAbc), GiaReader<Ntk>);
  }

}

ABC_NAMESPACE_CXX_HEADER_END
