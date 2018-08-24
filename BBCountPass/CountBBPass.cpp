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
// #include "SourceInfo.hpp"

using namespace llvm;
using patches_info = std::map<std::string, std::map<std::string, std::vector<int>>>;

static cl::opt<std::string> JSON("repo-patches");
static cl::opt<std::string> Repo("repo-name");

// auto compare = [](const SourceInfo& lhs, const SourceInfo& rhs) {
//     std::string mixedLhs {lhs.getFilename() + ":" + std::to_string(lhs.getLineNo())};
//     std::string mixedRhs {rhs.getFilename() + ":" + std::to_string(rhs.getLineNo())};
//     return mixedLhs.compare(mixedRhs);
// };

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
              // std::cout << "Inserting... Path: " << canon << " -- " << "Line: " << lineNo << "\n";
              // SourceInfo srcInfo{canon, lineNo};
              auto result = basicBlockMap.insert(std::make_pair(canon + ":" + std::to_string(lineNo), &BB));
              // if (result.second) {
                // std::cout << "Insert successful!\n";
              // }
            
              // SourceInfo tmp {canon, lineNo};
              // std::cout << "Basic block at " << tmp << ": " << basicBlockMap.at(tmp) << "\n";
              auto found2 = basicBlockMap.find(canon + ":" + std::to_string(lineNo));
              // if (found2 != basicBlockMap.end()) {
                // std::cout << "Found: " << canon + ":" + std::to_string(lineNo) << "\n";
              // } else {
                // std::cout << "Didn't find " << canon + ":" + std::to_string(lineNo) << "\n";
              // }
              // std::cout << "\n";

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

      // for (auto const& [srcInfo, bb] : bb_map) {
      //   std::cout << srcInfo << " -> " << bb->getName().str() << "\n";
      // }
      // std::ifstream input {JSON};
      // nlohmann::json patchesJSON;
      // input >> patchesJSON;
      std::string actualJSON;
      if (JSON[0] == '\'') {
        actualJSON = JSON.substr(1, JSON.length() - 2);
      } else {
        actualJSON = JSON;
      }
      
      std::cout << "le json: " << actualJSON << "\n";
      nlohmann::json patchesJSON = nlohmann::json::parse(actualJSON);

      patches_info patches = patchesJSON;

      // std::map<std::string, unsigned> commitsToBBCount;
      // std::cout << "\n";

      for (auto const& [commit, filenameToLines] : patches) {
        // errs() << commit << "\n";
        std::set<BasicBlock*> bbs;
        for (auto const& [filename, lines] : filenameToLines) {
          // errs() << filename << ": ";
          for (auto const& lineNo : lines) {
            // errs() << lineNo << " ";
            // SourceInfo temp {filename, lineNo};
            // errs() << "Looking for: " << filename + ":" + std::to_string(lineNo) << "\n";
            auto found = basicBlockMap.find(filename + ":" + std::to_string(lineNo));
            if (found != basicBlockMap.end()) {
              bbs.insert(found->second);
              // errs() << "main:Found: " << filename + ":" + std::to_string(lineNo) << "\n";
            }
          }
          // errs() << "\n";
        }
        // std::cout << "Commit: " << commit << " updates basic blocks: ";
        // for (auto const& bb : bbs) {
        //   errs() << bb << " ";
        // }
        // commitsToBBCount.emplace(commit, bbs.size());
        // errs() << commit << ":" << bbs.size();

        nlohmann::json jsonOutput;

        std::ifstream json_exists("BBCount.json");
        if (json_exists.good()) {
          std::ifstream input {"BBCount.json"};
          input >> jsonOutput;
        }

        if (jsonOutput.find(commit) != jsonOutput.end()) {
          // errs() << "got here 0\n";
          // unsigned bbCount = jsonOutput[commit];
          // jsonOutput[commit] = bbCount + bbs.size();
          // errs() << "got here 01\n";
        } else {
          // errs() << "got here 1\n";
          jsonOutput.emplace(commit, bbs.size());
          // errs() << "got here 11\n";
        }
        

        std::ofstream outfile("BBCount.json");
        outfile << std::setw(4) << jsonOutput << std::endl;

        // outfile.open("BBCount.json", std::ios_base::app);
        // outfile << commit << ":" << bbs.size() << "\n"; 
        // std::cout << "\n";
      }

      // nlohmann::json mapJSON {commitsToBBCount};
      // std::ofstream output {"BBCount.json"};
      // output << std::setw(4) << mapJSON << std::endl;

      // for (auto& F : M) {
      //   std::cout << "Function " << F.getName().str() << "\n";
      //   for (auto& BB : F) {
      //     std::cout << "Basic block " << BB.getName().str() << "\n";
      //     for (auto& instr : BB) {
      //       DebugLoc dbgLoc {instr.getDebugLoc()};
      //       MDNode *md  {dbgLoc.getAsMDNode(BB.getContext())};
      //       DILocation loc {md};
      //       std::cout << loc.getDirectory().str() << "/" << loc.getFilename().str() << " -- " << loc.getLineNumber() << "\n";
      //     }
      //   }
      // }
      return false;
    }

  }; // end of struct ShortestPathPass
}  // end of anonymous namespace

char CountBBPass::ID = 0;
static RegisterPass<CountBBPass> X("countBB", "Counts updated basic blocks in a patch",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);                          