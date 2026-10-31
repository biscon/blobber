function(blobber_world_publication target baker source output)
    file(GLOB_RECURSE world_inputs CONFIGURE_DEPENDS
        "${source}/npcs/*.json" "${source}/npcs/*.png"
        "${source}/textures/*.png" "${source}/fixtures/*.png")
    # A changed inventory also rebuilds when a dependency was removed.
    set(inventory "${CMAKE_CURRENT_BINARY_DIR}/${target}-inputs.txt")
    file(GENERATE OUTPUT "${inventory}" CONTENT "${world_inputs}\n")
    add_custom_command(OUTPUT "${output}/textures.bin" "${output}/metadata.json" "${output}/sources.json"
        COMMAND ${baker} "${source}" "${output}"
        DEPENDS ${baker} ${world_inputs} "${inventory}" "${source}/ui/items/loot-bag.png"
        COMMENT "Preparing reusable world textures and registered sprite mip chains" VERBATIM)
    add_custom_target(${target} DEPENDS "${output}/textures.bin" "${output}/metadata.json" "${output}/sources.json")
endfunction()
