# The following functions contains all the flags passed to the different build stages.

set(PACK_REPO_PATH "C:/Users/A18434/.mchp_packs" CACHE PATH "Path to the root of a pack repository.")

function(SCCP1_dsPIC33CK_default_default_XC_DSC_assemble_rule target)
    set(options
        "-g"
        "-mcpu=33CK32MP102"
        "-Wa,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-g,--no-relax"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=1"
        PRIVATE "XPRJ_default=default")
endfunction()
function(SCCP1_dsPIC33CK_default_default_XC_DSC_assemblePreproc_rule target)
    set(options
        "-x"
        "assembler-with-cpp"
        "-g"
        "-mcpu=33CK32MP102"
        "-Wa,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-g,--no-relax"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=1"
        PRIVATE "XPRJ_default=default")
endfunction()
function(SCCP1_dsPIC33CK_default_default_XC_DSC_compile_rule target)
    set(options
        "-g"
        "-mcpu=33CK32MP102"
        "-O0"
        "-msmart-io=1"
        "-Wall"
        "-msfr-warn=off"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG"
        PRIVATE "XPRJ_default=default")
endfunction()
function(SCCP1_dsPIC33CK_default_default_XC_DSC_compile_cpp_rule target)
    set(options
        "-g"
        "${CC_PRE}"
        "${DEBUGGER_NAME_AS_MACRO}"
        "-fframe-base-loclist"
        "-mcpu=33CK32MP102"
        "-frtti"
        "-fexceptions"
        "-fno-check-new"
        "-fenforce-eh-specs"
        "-fno-common"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG"
        PRIVATE "XPRJ_default=default")
endfunction()
function(SCCP1_dsPIC33CK_default_dependentObject_rule target)
    set(options
        "-c"
        "-mcpu=33CK32MP102"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
endfunction()
function(SCCP1_dsPIC33CK_default_link_rule target)
    set(options
        "-g"
        "-mcpu=33CK32MP102"
        "-Wl,--script=p33CK32MP102.gld,--local-stack,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,--report-mem,--memorysummary,memoryfile.xml"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16")
    list(REMOVE_ITEM options "")
    target_link_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=__DEBUG"
        PRIVATE "XPRJ_default=default")
endfunction()
function(SCCP1_dsPIC33CK_default_bin2hex_rule target)
    add_custom_target(
        SCCP1_dsPIC33CK_default_Bin2Hex ALL
        COMMAND ${MP_BIN2HEX} ${SCCP1_dsPIC33CK_default_image_name} -a -mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16
        WORKING_DIRECTORY ${SCCP1_dsPIC33CK_default_output_dir}
        BYPRODUCTS "${SCCP1_dsPIC33CK_default_output_dir}/${SCCP1_dsPIC33CK_default_image_base_name}.hex"
        COMMENT "Convert build file to .hex")
    add_dependencies(SCCP1_dsPIC33CK_default_Bin2Hex ${target})
endfunction()
function(SCCP1_dsPIC33CK_default_objcopy_lss_rule target)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${OBJDUMP}
        ARGS --disassemble --wide --demangle --line-numbers --section-headers -mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16 --source ${SCCP1_dsPIC33CK_default_image_name} > ${SCCP1_dsPIC33CK_default_image_base_name}.lss
        WORKING_DIRECTORY ${SCCP1_dsPIC33CK_default_output_dir})
endfunction()
