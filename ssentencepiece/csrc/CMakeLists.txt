set(ssentencepiece_srcs
  ssentencepiece.cc
)

add_library(ssentencepiece_core ${ssentencepiece_srcs})
target_link_libraries(ssentencepiece_core pthread)

install(TARGETS ssentencepiece_core DESTINATION ${CMAKE_INSTALL_PREFIX})

function(ssentencepiece_add_test source)
  get_filename_component(name ${source} NAME_WE)
  add_executable(${name} "${source}")
  target_link_libraries(${name}
    PRIVATE
      ssentencepiece_core
      gtest
      gtest_main
  )

  add_test(NAME "Test.${name}"
    COMMAND
    $<TARGET_FILE:${name}>
    WORKING_DIRECTORY
    ${CMAKE_SOURCE_DIR}
  )
endfunction()

if(SBPE_ENABLE_TESTS)
  # please sort the source files alphabetically
  set(test_srcs
    ssentencepiece_test.cc
  )

  foreach(source IN LISTS test_srcs)
    ssentencepiece_add_test(${source})
  endforeach()
endif()
