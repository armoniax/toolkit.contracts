add_contract(l2amc.owner l2amc.owner ${CMAKE_CURRENT_SOURCE_DIR}/src/l2amc.owner.cpp)

target_include_directories(l2amc.owner
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(l2amc.owner
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/l2amc.owner.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/l2amc.owner.contracts.md @ONLY )

target_compile_options( l2amc.owner PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
