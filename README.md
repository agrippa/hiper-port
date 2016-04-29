## HIPER-port is an LLVM libtooling plugin that converts C/C++ programs that use OpenMP pragmas, OpenSHMEM APIs, and other legacy APIs to use a task-based parallel programming model

HIPER-port currently targets the HClib task-parallel library:
https://github.com/habanero-rice/hcpp. However, it could be made to target other
models, such as Cilk. HIPER-port is primarily a porting tool: it does not have any
embedded knowledge about the target model that allows it to auto-optimize the
generated code. That would be the responsibility of the user following the
transformation pass.

HIPER-port is primarily motivated by the development of many alternative
programming models in recent years, and is intended to make the process of
trying out new programming models straightforward, fast, and low-cost for
application developers.

Please direct questions, comments, or feature requests for HIPER-port to
jmaxg3 AT gmail.com.
