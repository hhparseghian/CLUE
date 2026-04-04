cmake_minimum_required(VERSION 3.10)
project(CLUE VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---------- Backend options ----------
option(UI_BACKEND_WAYLAND "Build the Wayland backend" ON)
option(UI_BACKEND_X11     "Build the X11 backend"     ON)
option(UI_BACKEND_DRM     "Build the DRM/KMS backend" ON)

# ---------- Core library ----------
set(CLUE_CORE_SOURCES
    src/core/window.c
    src/core/event.c
    src/core/focus.c
    src/core/widget.c
    src/core/backend_select.c
    src/renderer/gl_renderer.c
    src/renderer/font.c
    src/widgets/signal.c
    src/widgets/clue_widget.c
    src/widgets/draw.c
    src/widgets/label.c
    src/widgets/button.c
    src/widgets/box.c
    src/widgets/text_input.c
    src/widgets/scroll.c
    src/widgets/image.c
    src/widgets/checkbox.c
    src/widgets/radio.c
    src/widgets/separator.c
    src/widgets/slider.c
    src/widgets/dropdown.c
    src/widgets/theme.c
    src/widgets/grid.c
    src/widgets/tabs.c
    src/widgets/dialog.c
    src/widgets/listview.c
    src/widgets/progress.c
    src/widgets/tooltip.c
    src/widgets/menu.c
    src/widgets/treeview.c
    src/widgets/table.c
    src/widgets/timer.c
    src/widgets/file_dialog.c
    src/widgets/toggle.c
    src/widgets/spinbox.c
    src/widgets/statusbar.c
    src/widgets/toolbar.c
    src/widgets/canvas.c
    src/widgets/splitter.c
    src/widgets/text_editor.c
    src/widgets/clipboard.c
    src/widgets/shortcut.c
    src/widgets/app.c
    src/third_party/stb_image_impl.c
)

add_library(clue STATIC ${CLUE_CORE_SOURCES})

target_include_directories(clue
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party
)

# GL renderer needs EGL/GLES2 headers and libs
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLES REQUIRED egl glesv2 freetype2)
target_include_directories(clue PRIVATE ${GLES_INCLUDE_DIRS})
target_link_libraries(clue PRIVATE ${GLES_LIBRARIES} m)

# ---------- Wayland backend ----------
if(UI_BACKEND_WAYLAND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED
        wayland-client
        wayland-cursor
        wayland-egl
        egl
        glesv2
        xkbcommon
    )

    # --- Generate xdg-shell protocol sources ---
    find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)
    pkg_get_variable(WL_PROTOCOLS_DIR wayland-protocols pkgdatadir)

    set(XDG_SHELL_XML "${WL_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
    set(XDG_SHELL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/xdg-shell-client-protocol.h")
    set(XDG_SHELL_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/xdg-shell-protocol.c")

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

    add_custom_command(
        OUTPUT  ${XDG_SHELL_HEADER}
        COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${XDG_SHELL_HEADER}
        DEPENDS ${XDG_SHELL_XML}
        COMMENT "Generating xdg-shell client header"
    )
    add_custom_command(
        OUTPUT  ${XDG_SHELL_SOURCE}
        COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${XDG_SHELL_SOURCE}
        DEPENDS ${XDG_SHELL_XML}
        COMMENT "Generating xdg-shell protocol code"
    )

    target_sources(clue PRIVATE
        src/backends/wayland/wayland.c
        ${XDG_SHELL_SOURCE}
        ${XDG_SHELL_HEADER}
    )
    target_include_directories(clue PRIVATE
        ${WAYLAND_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    )
    target_link_libraries(clue PRIVATE ${WAYLAND_LIBRARIES})
    target_compile_definitions(clue PRIVATE CLUE_HAS_WAYLAND)
endif()

# ---------- X11 backend ----------
if(UI_BACKEND_X11)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED
        x11
        egl
        glesv2
        xkbcommon
    )

    target_sources(clue PRIVATE src/backends/x11/x11.c)
    target_include_directories(clue PRIVATE ${X11_INCLUDE_DIRS})
    target_link_libraries(clue PRIVATE ${X11_LIBRARIES})
    target_compile_definitions(clue PRIVATE CLUE_HAS_X11)
endif()

# ---------- DRM/KMS backend ----------
if(UI_BACKEND_DRM)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DRM REQUIRED
        libdrm
        gbm
        egl
        glesv2
        libinput
        libudev
        xkbcommon
    )

    target_sources(clue PRIVATE src/backends/drm/drm.c)
    target_include_directories(clue PRIVATE ${DRM_INCLUDE_DIRS})
    target_link_libraries(clue PRIVATE ${DRM_LIBRARIES})
    target_compile_definitions(clue PRIVATE CLUE_HAS_DRM)
endif()

# ---------- Install ----------
include(GNUInstallDirs)

# Collect all dependency libs for pkg-config
set(CLUE_PC_LIBS "-lfreetype -lEGL -lGLESv2 -lxkbcommon -lm")
if(UI_BACKEND_X11)
    string(APPEND CLUE_PC_LIBS " -lX11")
endif()
if(UI_BACKEND_WAYLAND)
    string(APPEND CLUE_PC_LIBS " -lwayland-client -lwayland-cursor -lwayland-egl")
endif()
if(UI_BACKEND_DRM)
    string(APPEND CLUE_PC_LIBS " -ldrm -lgbm -linput -ludev")
endif()

configure_file(clue.pc.in clue.pc @ONLY)

install(TARGETS clue
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/clue
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/clue.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# ---------- Hello example ----------
add_executable(clue_demo examples/demo/main.c)
target_link_libraries(clue_demo PRIVATE clue)
