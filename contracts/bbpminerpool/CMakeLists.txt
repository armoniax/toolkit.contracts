add_contract(bbpminerpool bbpminerpool ${CMAKE_CURRENT_SOURCE_DIR}/src/bbpminerpool.cpp)

target_include_directories(bbpminerpool
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/../amax.bootdao/include)

set_target_properties(bbpminerpool
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/bbpminerpool.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/bbpminerpool.contracts.md @ONLY )

target_compile_options( bbpminerpool PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
