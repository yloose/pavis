cmake_minimum_required(VERSION 3.0)
project(pavis)

###############################################################################
## file globbing ##############################################################
###############################################################################

file(GLOB_RECURSE sources src/*.c include/*.h)
file(GLOB_RECURSE data res/* )

set_source_files_properties(${data} PROPERTIES HEADER_FILE_ONLY TRUE)

###############################################################################
## target definitions #########################################################
###############################################################################

add_executable(pavis-daemon ${sources} ${data})

#target_compile_options(pavis-daemon PUBLIC -std=c++1y -Wall -Wfloat-conversion)

target_include_directories(pavis-daemon PUBLIC include)

file(COPY ${data} DESTINATION res)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -g")

###############################################################################
## dependencies ###############################################################
###############################################################################

# gcc -o visualizer.out main.c audio_handling.c color_algorithm.c e131_client.c socket_server.c config_parser.c state_file.c -lpulse -lpulse-simple -lfftw3 -lm -le131 -lpthread -ldl -lconfig

add_subdirectory( lib/libe131 )

target_link_libraries(pavis-daemon PUBLIC
	pulse
	pulse-simple
	fftw3
	m
	e131
	pthread
	dl
	config
)


###############################################################################
## packaging ##################################################################
###############################################################################

install(TARGETS pavis-daemon DESTINATION bin)

# Create directory for the pavis-daemon to write its state file to
install(DIRECTORY DESTINATION /var/lib/pavis DIRECTORY_PERMISSIONS WORLD_READ WORLD_WRITE WORLD_EXECUTE)

# Compile helper script
install(FILES res/compile_algorithms.sh DESTINATION /usr/share/doc/pavis/)

# Config files
install(FILES res/pavis.conf.default DESTINATION /usr/share/doc/pavis/)
install(DIRECTORY res/algorithms DESTINATION /usr/share/doc/pavis)

# Systemd unit file
install(FILES res/pavis.service DESTINATION /usr/lib/systemd/user)

set(CPACK_PACKAGE_NAME "pavis")
set(CPACK_PACKAGE_VERSION "0.1.0")

set(CPACK_MONOLITHIC_INSTALL 1)

include(CPack)
