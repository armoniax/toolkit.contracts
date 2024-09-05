add_contract(l2amc.proxy l2amc.proxy 
      ${CMAKE_CURRENT_SOURCE_DIR}/src/source/curve25519_dh.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/source/curve25519_mehdi.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/source/curve25519_order.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/source/curve25519_utils.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/src/source/custom_blind.c
      ${CMAKE_CURRENT_SOURCE_DIR}/src/source/ed25519_sign.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/source/ed25519_verify.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/source/sha512.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/src/source/curve25519_mehdi_x64.c
      # ${CMAKE_CURRENT_SOURCE_DIR}/src/source/curve25519_order_x64.c
      # ${CMAKE_CURRENT_SOURCE_DIR}/src/source/curve25519_utils_x64.c
      ${CMAKE_CURRENT_SOURCE_DIR}/src/custom/random.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/custom/ed25519.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/src/C++/x25519.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/l2amc.proxy.cpp
                        )

if(DEFINED ENV{DAY_SECONDS_FOR_TEST})
   message(WARNING "ENV{DAY_SECONDS_FOR_TEST}=$ENV{DAY_SECONDS_FOR_TEST} should use only for test!!!")
   target_compile_definitions(l2amc.proxy PUBLIC "DAY_SECONDS_FOR_TEST=$ENV{DAY_SECONDS_FOR_TEST}")
endif()


target_include_directories(l2amc.proxy
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/src
   ${CMAKE_CURRENT_SOURCE_DIR}/src/include
   ${CMAKE_CURRENT_SOURCE_DIR}/src/custom
   ${CMAKE_CURRENT_SOURCE_DIR}/src/source
)

set_target_properties(l2amc.proxy
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/l2amc.proxy.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/l2amc.proxy.contracts.md @ONLY )

target_compile_options( l2amc.proxy PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )