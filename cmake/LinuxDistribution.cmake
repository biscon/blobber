# LD03 does not alter the accepted LD02 toolchain/image identity.
if(BLOBBER_LINUX_BASELINE)
    add_custom_target(dist
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/distribution/archive.py"
            --build "${CMAKE_BINARY_DIR}" --source "${CMAKE_SOURCE_DIR}" --version "${PROJECT_VERSION}"
        VERBATIM)
else()
    add_custom_target(dist
        COMMAND ${CMAKE_COMMAND} -E echo "dist requires the controlled Linux baseline; use tools/linux_release.py dist"
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM)
endif()
