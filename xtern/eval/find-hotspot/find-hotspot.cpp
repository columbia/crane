/* Copyright (c) 2013,  Regents of the Columbia University 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
 * materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "find-hotspot.h"
using namespace tern;

#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/Dominators.h"
#include "common/IDAssigner.h"
#include "common/util.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Instructions.h"
using namespace llvm;

#include <stdio.h>
#include <iostream>
#include <vector>
using namespace std;

static RegisterPass<FindHotspot> X(
		"find-hotspot",
		"Find Hotspot",
		false,
		true); // is analysis

char FindHotspot::ID = 0;

FindHotspot::FindHotspot(): ModulePass(&ID) {
}

FindHotspot::~FindHotspot() {
}

void FindHotspot::print(raw_ostream &O, const Module *M) const {
}

void FindHotspot::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<IDAssigner>();
  ModulePass::getAnalysisUsage(AU);
}

void FindHotspot::init(Module &M) {
  int_type = IntegerType::get(M.getContext(), 32);
  vector<const Type *> params;
  params.push_back(int_type);
  FunctionType *switch_fty = FunctionType::get(int_type, params, false);
  backedge_stat = M.getOrInsertFunction("backedge_stat", switch_fty);
  assert(backedge_stat);
}

bool FindHotspot::runOnModule(Module &M) {
  init(M);

  SmallVector<std::pair<const BasicBlock*,const BasicBlock*>, 32> edges;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function *f = I;
    if (f->isDeclaration())
      continue;
    FindFunctionBackedges(*f, edges);
  }

  SmallVector<std::pair<const BasicBlock*,const BasicBlock*>, 32>::iterator itr = edges.begin();
  for (; itr != edges.end(); itr++) {
    fprintf(stderr, "Back edge from function %s: bb %s to bb %s\n", 
      itr->first->getParent()->getNameStr().c_str(),
      itr->first->getNameStr().c_str(),
      itr->second->getNameStr().c_str()
    );
  }

  if (edges.size() > 0) {
    addBackEdgeStat(edges);
    return true;
  }
  
  return false;
}



void FindHotspot::addBackEdgeStat(SmallVector<std::pair<const BasicBlock*,const BasicBlock*>, 32> &back_edges) {
  for (size_t i = 0; i < back_edges.size(); ++i) {
    BasicBlock *x = (BasicBlock *)((long)back_edges[i].first);
    BasicBlock *y = (BasicBlock *)((long)back_edges[i].second);
    Function *f = x->getParent();
    BasicBlock *bb = BasicBlock::Create(f->getContext(), "", f);
    bb->setName("xtern_backedge_to_" + y->getNameStr());

    // Add the instrumented function.
    Value *patched = CallInst::Create(backedge_stat, ConstantInt::get(int_type, (int)i), "", bb);
    assert(patched);

    // Set its sucessor to y
    BranchInst::Create(y, bb);
        
    // Modify new_x's successor
    TerminatorInst *ti = x->getTerminator();
    assert(ti);
    unsigned idx = 0;
    for (succ_iterator it = succ_begin(x); it != succ_end(x); ++it, ++idx) {
      BasicBlock *z = *it;
      if (z == y)
        ti->setSuccessor(idx, bb);
    }

    // Modify PHI nodes of y;
    // For y, replace the incoming edge from x with from the back-edge BB
    for (BasicBlock::iterator ii = y->begin(); PHINode *phi = dyn_cast<PHINode>(ii); ++ii) {
      // Only replace the first one, because there may be multiple edges
      for (unsigned j = 0; j < phi->getNumIncomingValues(); ++j) {
        if (phi->getIncomingBlock(j) == x) {
            phi->setIncomingBlock(j, bb);
          break;
        }
      }
    }
  }
}


