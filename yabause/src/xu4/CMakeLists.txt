project(yabause-xu4)

yab_port_start()

find_package(SDL2 COMPONENTS sdl)
find_package(SDL2 REQUIRED)
find_package(OpenGLES)
find_package(PkgConfig REQUIRED)

find_package(Boost REQUIRED COMPONENTS system filesystem date_time locale)

if (NOT OPENGLES2_FOUND)
	return()
endif (NOT OPENGLES2_FOUND)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(pugixml)
add_subdirectory(nanogui-sdl)

include( ../glfw/external_zlib.cmake )
include( ../glfw/external_libpng.cmake )


set(PORT_INCLUDE_DIRS ${SDL2_INCLUDE_DIR} ${png_INCLUDE_DIR} ${zlib_INCLUDE_DIR} )
set(PORT_LIBRARIES ${SDL2_LIBRARY} ${OPENGLES2_LIBRARIES} ${png_STATIC_LIBRARIES} ${zlib_STATIC_LIBRARIES} )


set(yabause_xu4_SOURCES
	main.cpp
	InputManager.h
	InputManager.cpp
	InputConfig.h
	InputConfig.cpp
	MenuScreen.h
	MenuScreen.cpp
) 

include_directories( ${PORT_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} "nanogui-sdl/include" "nanogui-sdl/ext/eigen" )
add_executable(yabause-xu4 ${yabause_xu4_SOURCES})
target_link_libraries(yabause-xu4 yabause  ${Boost_LIBRARIES} pugixml ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES} nanogui )

yab_port_success(yabause-xu4)

#install(TARGETS yabause-xu4 DESTINATION "/opt/retropie/emulators/yabasanshiro/")
#install(FILES "README.md" DESTINATION "/opt/retropie/emulators/yabasanshiro/" )

install(TARGETS yabause-xu4 DESTINATION "bin")
install(FILES "README.md" DESTINATION "." )
