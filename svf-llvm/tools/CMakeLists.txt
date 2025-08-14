set(
  SVF_TOOLS
  "SABER;WPA;Bracelet;Example;DDA;MTA;CFL;LLVM2SVF;AE"
  CACHE STRING "SVF tools to build; entries may be tool directories or target names"
)

function(add_svf_tool _dir _target)
  if("${_dir}" IN_LIST SVF_TOOLS OR "${_target}" IN_LIST SVF_TOOLS)
    add_subdirectory(${_dir})
    list(APPEND SVF_ENABLED_TOOLS ${_target})
    set(SVF_ENABLED_TOOLS
        "${SVF_ENABLED_TOOLS}"
        PARENT_SCOPE
    )
  endif()
endfunction()

add_svf_tool(SABER saber)
add_svf_tool(WPA wpa)
add_svf_tool(Bracelet bracelet)
add_svf_tool(Example svf-ex)
add_svf_tool(DDA dvf)
add_svf_tool(MTA mta)
add_svf_tool(CFL cfl)
add_svf_tool(LLVM2SVF llvm2svf)
add_svf_tool(AE ae)

foreach(_tool IN LISTS SVF_ENABLED_TOOLS)
  # Link the SvfLLVM library against SVF's core & LLVM libraries
  target_link_libraries(${_tool} PRIVATE SvfCore SvfLLVM)

  # Ensure that the build artifacts are placed into lib/bin/include relative to the top of the build directory
  set_target_properties(
    ${_tool}
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SVF_BINARY_DIR}/bin
               LIBRARY_OUTPUT_DIRECTORY ${SVF_BINARY_DIR}/lib
               ARCHIVE_OUTPUT_DIRECTORY ${SVF_BINARY_DIR}/lib
  )

  install(
    TARGETS ${_tool}
    EXPORT SVFTargets
    RUNTIME DESTINATION ${SVF_INSTALL_BINDIR}
    LIBRARY DESTINATION ${SVF_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${SVF_INSTALL_LIBDIR}
            FILE_SET HEADERS
            DESTINATION ${SVF_INSTALL_INCLUDEDIR}
  )
endforeach()
