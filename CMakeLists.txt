project(rom_dump)

cmake_minimum_required( VERSION 3.5 )

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_ASM_COMPILER avr-gcc)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_AR avr-ar)

set(CPU_FREQ_MHZ 12000000)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmcu=atmega32 -Wall -DF_CPU=${CPU_FREQ_MHZ}UL")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

set(GENERATED_BINARY "avr_sst_flashrom")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mmcu=atmega32")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -Wl,-Map,${GENERATED_BINARY}.map")

add_executable(${GENERATED_BINARY}.elf
    atmega.c
    fuse.c
    main.c
    SST39SF020A.c
)

link_directories(
    ${PROJECT_SOURCE_DIR}/
)

include_directories(
    ${PROJECT_SOURCE_DIR}/
)

add_custom_target(hex ALL
    COMMAND avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex ${GENERATED_BINARY}.elf ${GENERATED_BINARY}.hex
    DEPENDS ${GENERATED_BINARY}.elf
)

add_custom_target(load 
    COMMAND avrdude -p atmega32 -P usb -c usbasp -U flash:w:${GENERATED_BINARY}.hex
)

add_custom_target(size
    COMMAND avr-size ${GENERATED_BINARY}.elf
)
