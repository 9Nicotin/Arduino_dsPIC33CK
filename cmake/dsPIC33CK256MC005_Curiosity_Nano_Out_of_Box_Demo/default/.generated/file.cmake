# The following variables contains the files used by the different stages of the build process.
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_assemble
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/dmt_asm.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/where_was_i.s")
set_source_files_properties(${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_assemble} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_assemble})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_assemblePreproc)
set_source_files_properties(${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_assemblePreproc} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_assemblePreproc})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_compile
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/HardwareSerial.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/system_config.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/wiring.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/wiring_analog.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/wiring_digital.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/wiring_interrupts.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/wiring_shift.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/cores/arduino/wiring_tone.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/libraries/HRPWM/src/HRPWM.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/libraries/SPI/src/SPI.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/libraries/Wire/src/Wire.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/variants/dspic33ck256mc002/variant.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/variants/dspic33ck256mc005/variant.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/variants/dspic33ck256mp508/variant.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../arduino-platform/microchip/dspic33ck/variants/dspic33ck32mp102/variant.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/clock.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/config_bits.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/dmt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/interrupt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/pins.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/reset.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/system.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.mcc/mcc_generated_files/system/src/traps.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../test_led/arduino_build/sketch.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../test_led/test_led.c")
set_source_files_properties(${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_compile} PROPERTIES LANGUAGE C)
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_compile_cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp_support/minimal_cxx.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp_support/test_arduino_cpp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp_support/test_cpp.cpp")
set_source_files_properties(${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_compile_cpp} PROPERTIES LANGUAGE CXX)
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_link "${CMAKE_CURRENT_SOURCE_DIR}/../../../test_led/arduino_build/core.a")
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_bin2hex)
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_FILE_TYPE_objcopy_lss)
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_image_name "default.elf")
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_image_base_name "default")

# The output directory of the final image.
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_output_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../out/dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo")

# The full path to the final image.
set(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_full_path_to_image ${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_output_dir}/${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_image_name})
