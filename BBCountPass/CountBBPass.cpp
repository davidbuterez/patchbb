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
              auto result = basicBlockMap.insert(std::make_pair(canon + ":" + std::to_string(lineNo), &BB));
              if (result.second) {
                std::cout << "Added " << canon + ":" + std::to_string(lineNo) << "\n";
              }
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

      std::string actualJSON;
      if (JSON[0] == '\'') {
        actualJSON = JSON.substr(1, JSON.length() - 2);
      } else {
        actualJSON = JSON;
      }
      
      std::cout << "le json: " << actualJSON << "\n";
      nlohmann::json patchJSON = nlohmann::json::parse(actualJSON);

      patches_info patch = patchJSON;

      for (auto const& [commit, filenameToLines] : patch) {
        errs() << commit << "\n";
        std::set<BasicBlock*> bbs;
        for (auto const& [filename, lines] : filenameToLines) {
          errs() << filename << ": ";
          for (auto const& lineNo : lines) {
            errs() << lineNo << " ";
            auto found = basicBlockMap.find(filename + ":" + std::to_string(lineNo));
            if (found != basicBlockMap.end()) {
              bbs.insert(found->second);
              errs() << "Found: " << filename + ":" + std::to_string(lineNo) << "\n";
            } else {
              errs() << "not found\n";
            }
          }
          // errs() << "\n";
        }

        nlohmann::json jsonOutput;

        std::ifstream json_exists("BBCount.json");
        if (json_exists.good()) {
          errs() << "Out file exists\n";
          std::ifstream input {"BBCount.json"};
          input >> jsonOutput;
        }

        if (jsonOutput.find(commit) != jsonOutput.end()) {
          // errs() << "got here 0\n";
          errs() << "Adding to existing\n";
          unsigned bbCount = jsonOutput[commit];
          jsonOutput[commit] = bbCount + bbs.size();
          // errs() << "got here 01\n";
        } else {
          // errs() << "got here 1\n";
          errs() << "Emplacing new element\n";
          jsonOutput.emplace(commit, bbs.size());
          // errs() << "got here 11\n";
        }
        

        std::ofstream outfile("BBCount.json");
        outfile << std::setw(4) << jsonOutput << std::endl;
      }

      return false;
    }

  }; // end of struct
}  // end of anonymous namespace

char CountBBPass::ID = 0;
static RegisterPass<CountBBPass> X("countBB", "Counts updated basic blocks in a patch",
                             false,
                             false /* Analysis Pass */);                          