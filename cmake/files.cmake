# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/syscall.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/sysmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/wrappers/reset_and_control_clock.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/wrappers/gpio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/wrappers/interrupt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/wrappers/timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/wrappers/pwm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/logic/init.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/wrappers/i2c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/logic/angles.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/logic/fsm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/startup_stm32c031xx.S"
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/stm32c031x6_flash.ld" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/stm32c031x6_flash.ld")
