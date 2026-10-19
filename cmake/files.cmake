# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/syscall.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/sysmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/reset_and_control_clock.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/gpio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/interrupt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/pwm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/init.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/startup_stm32c031xx.S"
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/stm32c031x6_flash.ld" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/stm32c031x6_flash.ld")
