# cmake version
cmake_minimum_required(VERSION 3.13)

## gcc 10 default: -fno-common
add_compile_options(-fcommon)
## or
#add_compile_options(-DNO_COMMON)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(ttl2vga)

# initialize the sdk
pico_sdk_init()

#######

# name anything you want
add_executable(ttl2vga)

# must match with pio filename and executable name from above
pico_generate_pio_header(ttl2vga ${CMAKE_CURRENT_LIST_DIR}/hsync_out.pio)
pico_generate_pio_header(ttl2vga ${CMAKE_CURRENT_LIST_DIR}/vsync_out.pio)
pico_generate_pio_header(ttl2vga ${CMAKE_CURRENT_LIST_DIR}/rgb_out.pio)

pico_generate_pio_header(ttl2vga ${CMAKE_CURRENT_LIST_DIR}/hsync_in.pio)
pico_generate_pio_header(ttl2vga ${CMAKE_CURRENT_LIST_DIR}/vsync_in_16M.pio)
pico_generate_pio_header(ttl2vga ${CMAKE_CURRENT_LIST_DIR}/vsync_in_14M.pio)
pico_generate_pio_header(ttl2vga ${CMAKE_CURRENT_LIST_DIR}/getpix.pio)

# must match with executable name and source file names
target_sources(ttl2vga PRIVATE ttl2vga.c vga_out.c ttl_in_16M.c ttl_in_14M.c)

# must match with executable name
target_link_libraries(ttl2vga PRIVATE pico_stdlib pico_multicore hardware_pio hardware_dma)

# must match with executable name
pico_add_extra_outputs(ttl2vga)


# (sdk submodule tinyUSB)
## enable usb output, disable uart output
#pico_enable_stdio_usb(ttl2vga 1)
pico_enable_stdio_usb(ttl2vga 0)
pico_enable_stdio_uart(ttl2vga 0)

