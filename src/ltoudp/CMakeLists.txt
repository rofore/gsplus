set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(atbridge atbridge.c llap.c port.c LToUDP.c)

target_compile_definitions(atbridge PUBLIC HAVE_ATBRIDGE)

if(WIN32)
	target_link_libraries(atbridge ws2_32) # winsock2
endif()
