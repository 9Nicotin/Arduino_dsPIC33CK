# The following functions contains all the flags passed to the different build stages.

set(PACK_REPO_PATH "C:/Users/A18434/.mchp_packs" CACHE PATH "Path to the root of a pack repository.")

function(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_assemble_rule target)
    set(options
        "-g"
        "-mcpu=33CK256MC005"
        "-Wa,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-g,--no-relax"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MC_DFP/1.11.412/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=1"
        PRIVATE "XPRJ_default=default")
endfunction()
function(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_assemblePreproc_rule target)
    set(options
        "-x"
        "assembler-with-cpp"
        "-g"
        "-mcpu=33CK256MC005"
        "-Wa,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-g,--no-relax"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MC_DFP/1.11.412/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=1"
        PRIVATE "XPRJ_default=default")
endfunction()
function(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_compile_rule target)
    set(options
        "-g"
        "-mcpu=33CK256MC005"
        "-ffunction-sections"
        "-O0"
        "-msmart-io=1"
        "-Wall"
        "-msfr-warn=off"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MC_DFP/1.11.412/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG"
        PRIVATE "XPRJ_default=default")
endfunction()
function(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_default_XC_DSC_compile_cpp_rule target)
    set(options
        "-g"
        "${CC_PRE}"
        "${DEBUGGER_NAME_AS_MACRO}"
        "-fframe-base-loclist"
        "-mcpu=33CK256MC005"
        "-frtti"
        "-fexceptions"
        "-fno-check-new"
        "-fenforce-eh-specs"
        "-ffunction-sections"
        "-O1"
        "-fno-common"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MC_DFP/1.11.412/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG"
        PRIVATE "XPRJ_default=default")
endfunction()
function(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_dependentObject_rule target)
    set(options
        "-c"
        "-mcpu=33CK256MC005"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MC_DFP/1.11.412/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
endfunction()
function(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_link_rule target)
    set(options
        "-g"
        "-mcpu=33CK256MC005"
        "-mreserve=data@0x1000:0x101b"
        "-mreserve=data@0x101c:0x101d"
        "-mreserve=data@0x101e:0x101f"
        "-mreserve=data@0x1020:0x1021"
        "-mreserve=data@0x1022:0x1023"
        "-mreserve=data@0x1024:0x1027"
        "-mreserve=data@0x1028:0x104f"
        "-Wl,--script=p33CK256MC005.gld,--local-stack,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,--heap=0,--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,--report-mem,--memorysummary,memoryfile.xml"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MC_DFP/1.11.412/xc16")
    list(REMOVE_ITEM options "")
    target_link_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=__DEBUG"
        PRIVATE "XPRJ_default=default")
endfunction()
function(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_bin2hex_rule target)
    add_custom_target(
        dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_Bin2Hex ALL
        COMMAND ${MP_BIN2HEX} ${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_image_name} -a -mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MC_DFP/1.11.412/xc16
        WORKING_DIRECTORY ${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_output_dir}
        BYPRODUCTS "${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_output_dir}/${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_image_base_name}.hex"
        COMMENT "Convert build file to .hex")
    add_dependencies(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_Bin2Hex ${target})
endfunction()
function(dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_objcopy_lss_rule target)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${OBJDUMP}
        ARGS --disassemble --wide --demangle --line-numbers --section-headers -mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MC_DFP/1.11.412/xc16 --source ${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_image_name} > ${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_image_base_name}.lss
        WORKING_DIRECTORY ${dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo_default_output_dir})
endfunction()
