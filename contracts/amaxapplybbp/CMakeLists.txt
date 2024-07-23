add_contract(amaxapplybbp amaxapplybbp ${CMAKE_CURRENT_SOURCE_DIR}/src/amaxapplybbp.cpp)

target_include_directories(amaxapplybbp
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/../amax.bootdao/include)

set_target_properties(amaxapplybbp
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/amaxapplybbp.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/amaxapplybbp.contracts.md @ONLY )

target_compile_options( amaxapplybbp PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
