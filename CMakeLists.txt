cmake_minimum_required(VERSION 3.18)
set(PICO_BOARD pico_w)
set(PICO_SDK_FETCH_FROM_GIT ON)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
include(FetchContent)

FetchContent_Declare(
    Universal_hal
    GIT_REPOSITORY https://github.com/Hoog-V/Universal_hal
    GIT_TAG stable
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/Universal_hal
)

FetchContent_Declare(
    Manikin_Software_Libaries_V2
    GIT_REPOSITORY https://github.com/RobotPatient/Manikin_Software_Libraries_V2.git
    GIT_TAG main
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/Manikin_Software_Libaries_V2
)

FetchContent_Declare(
  FreeRTOS
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel
  GIT_TAG        V11.0.1
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/FreeRTOS
)

project(thumbs_track_module_eai C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif ()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)

if (PICO_CYW43_SUPPORTED) # set by BOARD=pico-w
    if (NOT TARGET pico_cyw43_arch)
        message("Skipping build as support is not available")
    else ()
        add_executable(thumbs_track_module_eai "src/main.cpp")
        # enable usb output, disable uart output
        pico_enable_stdio_usb(thumbs_track_module_eai 1)
        pico_enable_stdio_uart(thumbs_track_module_eai 0)

        target_include_directories(thumbs_track_module_eai PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/src
        )

        # Select the heap port.  values between 1-4 will pick a heap.
        set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)

        # Select the native compile PORT
        set(FREERTOS_PORT "GCC_ARM_CM0" CACHE STRING "" FORCE)

        add_library(freertos_config INTERFACE)
        FetchContent_MakeAvailable(FreeRTOS)
        target_include_directories(freertos_config INTERFACE "config/")

        add_library(FreeRTOS ALIAS freertos_kernel)
        set(PLATFORM_DEFINED PICO)
        FetchContent_MakeAvailable(Universal_hal)
        FetchContent_MakeAvailable(Manikin_Software_Libaries_V2)

        target_link_libraries(thumbs_track_module_eai pico_cyw43_arch_lwip_poll
            pico_stdlib
            pico_lwip_mbedtls
            pico_mbedtls
            freertos_kernel
            freertos_config
            Universal_hal
            sensor_fingerposition
        )

        pico_add_extra_outputs(thumbs_track_module_eai)

    endif ()
endif()
