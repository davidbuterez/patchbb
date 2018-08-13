#include <fstream>
#include <iostream>
#include <set>
#include <boost/algorithm/string/predicate.hpp>
#include <limits.h>
#include <stdlib.h>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/DebugInfo.h"
#include "json.hpp"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using patches_info = std::map<std::string, std::map<std::string, std::vector<int>>>;

static cl::opt<std::string> JSON("repo-patches");
static cl::opt<std::string> Repo("repo-name");

namespace {
  std::map<std::string, BasicBlock*> buildBBMap(Module &M) {
    std::map<std::string, BasicBlock*> basicBlockMap;

    for (auto& F : M) {
      for (auto& BB : F) {
        for (auto& instr : BB) {
          DebugLoc dbgLoc {instr.getDebugLoc()};
          MDNode *md  {dbgLoc.getAsMDNode(BB.getContext())};
          DILocation loc {md};

          std::string dir = loc.getDirectory().str();
          std::string filename = loc.getFilename().str();

          if (!dir.empty() && !filename.empty()) { 
            std::string pathStr {dir + "/" + filename};
            char *realPath = realpath(pathStr.c_str(), NULL);
            if (realPath) {
              std::string canon(realPath);
              free(realPath);

              std::size_t found = canon.find(Repo);
              if (found != std::string::npos) {
                canon = canon.substr(found + Repo.length() + 1);
              }
              unsigned lineNo = loc.getLineNumber();
              basicBlockMap.insert(std::make_pair(canon + ":" + std::to_string(lineNo), &BB));
            }
          }

        }
      }
    }

    return basicBlockMap;
  }
}

namespace llvm {
  struct CountBBPass : public ModulePass {
    static char ID;
    CountBBPass() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
      auto basicBlockMap = buildBBMap(M);

      std::ifstream input {JSON};
      nlohmann::json patchesJSON;
      input >> patchesJSON;

      patches_info patches = patchesJSON;

      std::map<std::string, unsigned> commitsToBBCount;

      for (auto const& [commit, filenameToLines] : patches) {
        std::set<BasicBlock*> bbs;

        for (auto const& [filename, lines] : filenameToLines) {
          for (auto const& lineNo : lines) {
            auto found = basicBlockMap.find(filename + ":" + std::to_string(lineNo));
            if (found != basicBlockMap.end()) {
              bbs.insert(found->second);
            }
          }
        }

        commitsToBBCount.emplace(commit, bbs.size());
      }

      nlohmann::json mapJSON {commitsToBBCount};
      std::ofstream output {"BBCount.json"};
      output << std::setw(4) << mapJSON << std::endl;

      return false;
    }

  }; // end of struct CountBBPass
}  // end of anonymous namespace

char CountBBPass::ID = 0;
static RegisterPass<CountBBPass> X("countBB", "Counts updated basic blocks in a patch",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);                          