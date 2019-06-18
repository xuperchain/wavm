add_definitions(-std=c++11 -fPIC -O0)
add_library(rpc OBJECT rpc_inproc.cpp)
add_custom_target(bridge ALL
  COMMAND ${CMAKE_AR} -crs libbridge.a $<TARGET_OBJECTS:rpc>)
add_dependencies(bridge rpc)
