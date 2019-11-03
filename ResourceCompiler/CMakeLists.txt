add_executable(ResourceCompiler ResourceCompiler.cpp)

function(add_resource FILE_PATH RESOURCE_NAME TARGET)
    get_filename_component(FILE_NAME ${FILE_PATH} NAME)
    set("${CMAKE_BINARY_DIR}/Resources/${FILE_NAME}.cpp")
    add_custom_command(
        OUTPUT ${GENERATED_FILE_PATH}
        COMMAND $<TARGET_FILE:ResourceCompiler> -input ${FILE_PATH} -output ${GENERATED_FILE_PATH} -name ${RESOURCE_NAME}
        DEPENDS ${FILE_PATH}
    )
    target_sources(${TARGET} PUBLIC ${GENERATED_FILE_PATH})
endfunction()
