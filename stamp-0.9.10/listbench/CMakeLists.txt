
#
#  Copyright (C) 2011
#  University of Rochester Department of Computer Science
#    and
#  Lehigh University Department of Computer Science and Engineering
# 
# License: Modified BSD
#          Please see the file LICENSE.RSTM for licensing information

set(
  sources
  ../../libstm/lib_hicamp.c
  ../lib/list.c
  ../lib/mt19937ar.c
  ../lib/pair.c
  ../lib/queue.c
  ../lib/random.c
  ../lib/rbtree.c
  ../lib/thread.c
  ../lib/vector.c
  listbench.cpp)

# build the different executables
foreach (arch ${rstm_archs})
  add_stamp_executable(exec listbench ${arch} ${sources})
  add_target_definitions(${exec} MAP_USE_RBTREE)
endforeach ()
