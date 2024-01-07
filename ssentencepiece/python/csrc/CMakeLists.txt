pybind11_add_module(_ssentencepiece
  ssentencepiece.cc
)

target_link_libraries(_ssentencepiece PRIVATE ssentencepiece_core)

install(TARGETS _ssentencepiece DESTINATION ${CMAKE_INSTALL_PREFIX})
