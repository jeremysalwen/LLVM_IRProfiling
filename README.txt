LLVM_IRProf is a set of passes for edge and path profiling LLVM IR.  
They used to be part of LLVM, but were removed in commit 191835 (and 
some subsequent cleanup commits).  However, the profiling code which is 
meant to replace these old passes is part of Clang, meaning that we lose 
the ability to profile arbitrary LLVM IR.  In addition, they currently 
only provide edge profiling.

To build:

Run scons

To use:

    opt -load libIRProf.so -insert-path-profiling

(or whatever pass you wish to use instead of insert-path-profiling).

If you are instrumenting code, the output will need to be linked with 
libprofile.so, which is built as part of this project.

If you want more in-depth documentation, let me know and I probably can 
scrounge some up from the old llvm docs.


-Jeremy (jeremysalwen@gmail.com)
