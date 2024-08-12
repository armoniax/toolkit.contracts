add_contract(amaxapplybps amaxapplybps ${CMAKE_CURRENT_SOURCE_DIR}/src/amaxapplybps.cpp)

target_include_directories(amaxapplybps
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(amaxapplybps
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/amaxapplybps.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/amaxapplybps.contracts.md @ONLY )

target_compile_options( amaxapplybps PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
