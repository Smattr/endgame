add_executable(llamaterm
  src/main.c
)

target_link_libraries(llamaterm PRIVATE endgame)

install(TARGETS llamaterm
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
