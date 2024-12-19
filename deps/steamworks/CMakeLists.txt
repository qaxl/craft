if (BUILD_WITH_STEAMSDK)
    add_library(steamworks INTERFACE)

    target_include_directories(steamworks INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sdk/public)

    if (WIN32)
        target_link_libraries(steamworks INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sdk/redistributable_bin/win64/steam_api64.lib)
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sdk/redistributable_bin/win64/steam_api64.dll DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
    else()
        message(ERROR "Your platform is not supported.")
    endif()
endif()
