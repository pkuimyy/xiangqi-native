# MSVC localized /showIncludes can use UTF-8 despite a non-UTF-8 active code page.
if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
  file(WRITE "${CMAKE_BINARY_DIR}/include-prefix-probe.cpp" "#include <stddef.h>\n")
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" /nologo /utf-8 /showIncludes /EP "${CMAKE_BINARY_DIR}/include-prefix-probe.cpp"
    OUTPUT_VARIABLE prefix_stdout ERROR_VARIABLE prefix_stderr
    ENCODING NONE RESULT_VARIABLE prefix_result
  )
  if(prefix_result EQUAL 0)
    string(CONCAT prefix_output "${prefix_stdout}" "\n" "${prefix_stderr}")
    string(REGEX MATCH "[^\r\n]*[A-Za-z]:[\\\\/][^\r\n]*stddef[.]h" prefix_line "${prefix_output}")
    if(prefix_line)
      string(REGEX REPLACE "[A-Za-z]:[\\\\/].*$" "" detected_prefix "${prefix_line}")
      if(detected_prefix)
        set(CMAKE_CL_SHOWINCLUDES_PREFIX "${detected_prefix}")
        set(CMAKE_C_CL_SHOWINCLUDES_PREFIX "${detected_prefix}")
        set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "${detected_prefix}")
      endif()
    endif()
  endif()
endif()
