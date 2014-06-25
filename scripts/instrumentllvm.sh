for i in *.bc;
do
opt-3.5 $i -load /path/to/libIRProfiling.so -insert-optimal-edge-profiling -o /tmp/lol.bc; 
clang-3.5  /path/to/libprofile.so /tmp/lol.bc -o `basename $i .bc`;
done
