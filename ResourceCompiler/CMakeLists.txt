add_executable(ResourceCompiler ResourceCompiler.cpp)

function(target_resource TARGET FILE_PATH RESOURCE_NAME)
    get_filename_component(FILE_NAME ${FILE_PATH} NAME)
    set(GENERATED_FILE_DIR "${CMAKE_BINARY_DIR}/Generated/Resources")
    set(GENERATED_FILE_PATH "${GENERATED_FILE_DIR}/${FILE_NAME}.c")
    add_custom_command(
        OUTPUT ${GENERATED_FILE_PATH}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_FILE_DIR}
        COMMAND $<TARGET_FILE:ResourceCompiler> -input ${FILE_PATH} -output ${GENERATED_FILE_PATH} -name ${RESOURCE_NAME}
        DEPENDS ${FILE_PATH} $<TARGET_FILE:ResourceCompiler>
    )
    target_sources(${TARGET} PRIVATE ${GENERATED_FILE_PATH})
endfunction()
