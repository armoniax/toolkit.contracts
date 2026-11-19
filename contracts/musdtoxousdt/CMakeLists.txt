add_contract(musdtoxousdt musdtoxousdt ${CMAKE_CURRENT_SOURCE_DIR}/src/musdtoxousdt.cpp)
target_include_directories(musdtoxousdt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(musdtoxousdt PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# CDT also discovers foreign multi_index tables; publish only our own tables.
find_package(PythonInterp 3 REQUIRED)
add_custom_command(TARGET musdtoxousdt POST_BUILD
   COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/clean_abi.py"
      "$<TARGET_FILE_DIR:musdtoxousdt>/musdtoxousdt.abi"
   VERBATIM)
set_property(TARGET musdtoxousdt APPEND PROPERTY LINK_DEPENDS
   "${CMAKE_CURRENT_SOURCE_DIR}/clean_abi.py")
