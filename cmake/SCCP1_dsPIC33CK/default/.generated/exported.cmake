set(DEPENDENT_MP_BIN2HEXSCCP1_dsPIC33CK_default_Qj2FgPm2 "c:/Program Files/Microchip/xc-dsc/v3.31/bin/xc-dsc-bin2hex.exe")
set(DEPENDENT_DEPENDENT_TARGET_ELFSCCP1_dsPIC33CK_default_Qj2FgPm2 ${CMAKE_CURRENT_LIST_DIR}/../../../../out/SCCP1_dsPIC33CK/default.elf)
set(DEPENDENT_TARGET_DIRSCCP1_dsPIC33CK_default_Qj2FgPm2 ${CMAKE_CURRENT_LIST_DIR}/../../../../out/SCCP1_dsPIC33CK)
set(DEPENDENT_BYPRODUCTSSCCP1_dsPIC33CK_default_Qj2FgPm2 ${DEPENDENT_TARGET_DIRSCCP1_dsPIC33CK_default_Qj2FgPm2}/${sourceFileNameSCCP1_dsPIC33CK_default_Qj2FgPm2}.s)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRSCCP1_dsPIC33CK_default_Qj2FgPm2}/${sourceFileNameSCCP1_dsPIC33CK_default_Qj2FgPm2}.s
    COMMAND ${DEPENDENT_MP_BIN2HEXSCCP1_dsPIC33CK_default_Qj2FgPm2} ${DEPENDENT_DEPENDENT_TARGET_ELFSCCP1_dsPIC33CK_default_Qj2FgPm2} --image ${sourceFileNameSCCP1_dsPIC33CK_default_Qj2FgPm2} ${addressSCCP1_dsPIC33CK_default_Qj2FgPm2} ${modeSCCP1_dsPIC33CK_default_Qj2FgPm2} -mdfp=C:/Users/A18434/.mchp_packs/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRSCCP1_dsPIC33CK_default_Qj2FgPm2}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFSCCP1_dsPIC33CK_default_Qj2FgPm2})
add_custom_target(
    dependent_produced_source_artifactSCCP1_dsPIC33CK_default_Qj2FgPm2 
    DEPENDS ${DEPENDENT_TARGET_DIRSCCP1_dsPIC33CK_default_Qj2FgPm2}/${sourceFileNameSCCP1_dsPIC33CK_default_Qj2FgPm2}.s
    )
