include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(SCCP1_dsPIC33CK_default_library_list )

# Handle files with suffix s, for group default-XC-DSC
if(SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_assemble)
add_library(SCCP1_dsPIC33CK_default_default_XC_DSC_assemble OBJECT ${SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_assemble})
    SCCP1_dsPIC33CK_default_default_XC_DSC_assemble_rule(SCCP1_dsPIC33CK_default_default_XC_DSC_assemble)
    list(APPEND SCCP1_dsPIC33CK_default_library_list "$<TARGET_OBJECTS:SCCP1_dsPIC33CK_default_default_XC_DSC_assemble>")

endif()

# Handle files with suffix S, for group default-XC-DSC
if(SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_assemblePreproc)
add_library(SCCP1_dsPIC33CK_default_default_XC_DSC_assemblePreproc OBJECT ${SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_assemblePreproc})
    SCCP1_dsPIC33CK_default_default_XC_DSC_assemblePreproc_rule(SCCP1_dsPIC33CK_default_default_XC_DSC_assemblePreproc)
    list(APPEND SCCP1_dsPIC33CK_default_library_list "$<TARGET_OBJECTS:SCCP1_dsPIC33CK_default_default_XC_DSC_assemblePreproc>")

endif()

# Handle files with suffix c, for group default-XC-DSC
if(SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_compile)
add_library(SCCP1_dsPIC33CK_default_default_XC_DSC_compile OBJECT ${SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_compile})
    SCCP1_dsPIC33CK_default_default_XC_DSC_compile_rule(SCCP1_dsPIC33CK_default_default_XC_DSC_compile)
    list(APPEND SCCP1_dsPIC33CK_default_library_list "$<TARGET_OBJECTS:SCCP1_dsPIC33CK_default_default_XC_DSC_compile>")

endif()

# Handle files with suffix cpp, for group default-XC-DSC
if(SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_compile_cpp)
add_library(SCCP1_dsPIC33CK_default_default_XC_DSC_compile_cpp OBJECT ${SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_compile_cpp})
    SCCP1_dsPIC33CK_default_default_XC_DSC_compile_cpp_rule(SCCP1_dsPIC33CK_default_default_XC_DSC_compile_cpp)
    list(APPEND SCCP1_dsPIC33CK_default_library_list "$<TARGET_OBJECTS:SCCP1_dsPIC33CK_default_default_XC_DSC_compile_cpp>")

endif()

# Handle files with suffix s, for group default-XC-DSC
if(SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_dependentObject)
add_library(SCCP1_dsPIC33CK_default_default_XC_DSC_dependentObject OBJECT ${SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_dependentObject})
    SCCP1_dsPIC33CK_default_default_XC_DSC_dependentObject_rule(SCCP1_dsPIC33CK_default_default_XC_DSC_dependentObject)
    list(APPEND SCCP1_dsPIC33CK_default_library_list "$<TARGET_OBJECTS:SCCP1_dsPIC33CK_default_default_XC_DSC_dependentObject>")

endif()

# Handle files with suffix elf, for group default-XC-DSC
if(SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_bin2hex)
add_library(SCCP1_dsPIC33CK_default_default_XC_DSC_bin2hex OBJECT ${SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_bin2hex})
    SCCP1_dsPIC33CK_default_default_XC_DSC_bin2hex_rule(SCCP1_dsPIC33CK_default_default_XC_DSC_bin2hex)
    list(APPEND SCCP1_dsPIC33CK_default_library_list "$<TARGET_OBJECTS:SCCP1_dsPIC33CK_default_default_XC_DSC_bin2hex>")

endif()

# Handle files with suffix elf, for group default-XC-DSC
if(SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_objcopy_lss)
add_library(SCCP1_dsPIC33CK_default_default_XC_DSC_objcopy_lss OBJECT ${SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_objcopy_lss})
    SCCP1_dsPIC33CK_default_default_XC_DSC_objcopy_lss_rule(SCCP1_dsPIC33CK_default_default_XC_DSC_objcopy_lss)
    list(APPEND SCCP1_dsPIC33CK_default_library_list "$<TARGET_OBJECTS:SCCP1_dsPIC33CK_default_default_XC_DSC_objcopy_lss>")

endif()


# Main target for this project
add_executable(SCCP1_dsPIC33CK_default_image_Qj2FgPm2 ${SCCP1_dsPIC33CK_default_library_list})

if(NOT CMAKE_HOST_WIN32)
    set_target_properties(SCCP1_dsPIC33CK_default_image_Qj2FgPm2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${SCCP1_dsPIC33CK_default_output_dir}")
endif()
set_target_properties(SCCP1_dsPIC33CK_default_image_Qj2FgPm2 PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf")
target_link_libraries(SCCP1_dsPIC33CK_default_image_Qj2FgPm2 PRIVATE ${SCCP1_dsPIC33CK_default_default_XC_DSC_FILE_TYPE_link})
# Add the link options from the rule file.
SCCP1_dsPIC33CK_default_link_rule( SCCP1_dsPIC33CK_default_image_Qj2FgPm2)

# Call bin2hex function from the rule file
SCCP1_dsPIC33CK_default_bin2hex_rule(SCCP1_dsPIC33CK_default_image_Qj2FgPm2)
if(CMAKE_HOST_WIN32)
    add_custom_command(
        TARGET SCCP1_dsPIC33CK_default_image_Qj2FgPm2
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SCCP1_dsPIC33CK_default_output_dir}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SCCP1_dsPIC33CK_default_image_Qj2FgPm2> ${SCCP1_dsPIC33CK_default_output_dir}/${SCCP1_dsPIC33CK_default_original_image_name}
        BYPRODUCTS ${SCCP1_dsPIC33CK_default_output_dir}/${SCCP1_dsPIC33CK_default_original_image_name}
        COMMENT "Copying elf to out location")
    set_property(
        TARGET SCCP1_dsPIC33CK_default_image_Qj2FgPm2
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES
        ${SCCP1_dsPIC33CK_default_output_dir}/${SCCP1_dsPIC33CK_default_original_image_name})
endif()

#Add objcopy steps
SCCP1_dsPIC33CK_default_objcopy_lss_rule(SCCP1_dsPIC33CK_default_image_Qj2FgPm2)

