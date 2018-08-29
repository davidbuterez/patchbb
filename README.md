# patchbb
Simple tool to see how many basic blocks are updated/changed between different versions (patches) of a repository. Uses the output from [diffanalyze](https://github.com/davidbuterez/diffanalyze) to provide targets for the LLVM Pass. Consists of 3 parts:
- `patchbb.py` - this is the main script that walks through the different versions of the repository and calls the other parts as needed
- `build.sh` - this is a customizable script that has all the instructions to build the application we are inspecting (e.g. `./configure; make`)
- A LLVM Pass that looks at the bitcode file, using the output from **diffanalyze** and LLVM debug information to count basic blocks.

## Notes 
This is built and tested with LLVM 3.4, as this is the stable version that is used by KLEE. The results are as accurate as the debug information from LLVM.

## Usage
First, run **diffanalyze** to get some targets in JSON format. Example:

`./diffanalyze.py https://git.savannah.gnu.org/git/findutils.git -s --save-json --track diff --rangeInt 15`

You can then call patchbb. Example:

`./patchbb.py https://git.savannah.gnu.org/git/findutils.git CountBBPass.so --name findutils --patch-history output.json`

This will produce a `BBCount.json` with the resulting information.

Arguments:
1. First argument is the git URL of the repository
2. Second argument is the path to the LLVM Pass (see *Installation* for build instructions)
- `--name` - this is the name of the repository (e.g. `findutils`)
- `--patch-history` - this is the output, in JSON format, from **diffanalyze**

(Not shown in the example):
- `--llvm-link-path` - if you have an out-of-source build of LLVM, you will need to provide the path to **llvm-link**
- `--opt-path` - if you have an out-of-source build of LLVM, you will need to provide the path to **opt**

Overall sample usage:

`./patchbb.py <git-url> <path-to-pass> --name <name> --llvm-link-path <path-to-llvm-link> --patch-history <history.json> --opt-path <path-to-opt>`

## Installation
Since you need **diffanalyze** to run this, I will assume that you have succesfully installed it. There are detailed instructions on how to do so on the [GitHub page](https://github.com/davidbuterez/diffanalyze). Note that, in particular, you must have `universalctags` available.

Additionally, you will need:
- LLVM 3.4
- wllvm: `pip3 install wllvm`

Once cloned, run in the main directory:

`cmake CMakeLists.txt`
`make`

This will build the pass. You can now call the script, as instructed above.


