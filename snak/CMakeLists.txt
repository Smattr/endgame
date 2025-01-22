add_executable(snak src/main.c)
target_link_libraries(snak PRIVATE endgame)
install(TARGETS snak RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
