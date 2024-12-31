add_executable(onlyjump
  src/main.c
)

target_link_libraries(onlyjump PRIVATE endgame)

install(TARGETS onlyjump
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
