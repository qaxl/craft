# add_custom_target(hlsl_shaders)

# set(HLSL_SHADER_FILES VertexShader.hlsl PixelShader.hlsl)

# set_source_files_properties(VertexShader.hlsl PROPERTIES ShaderType "vs")
# set_source_files_properties(PixelShader.hlsl PROPERTIES ShaderType "ps")
# set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "4_0")

# foreach(FILE ${HLSL_SHADER_FILES})
#   get_filename_component(FILE_WE ${FILE} NAME_WE)
#   get_source_file_property(shadertype ${FILE} ShaderType)
#   get_source_file_property(shadermodel ${FILE} ShaderModel)
#   add_custom_command(TARGET hlsl_shaders # TODO: use dxc.exe
#                      COMMAND fxc.exe /nologo /Emain /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fo ${CMAKE_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
#                      MAIN_DEPENDENCY ${FILE}
#                      COMMENT "HLSL ${FILE}"
#                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#                      VERBATIM)
# endforeach(FILE)

add_custom_target(glsl_shaders ALL)

file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
file(GLOB_RECURSE GLSL_SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.frag ${CMAKE_CURRENT_SOURCE_DIR}/*.vert)

foreach(FILE ${GLSL_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME)
  set(SPV_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${FILE_WE}.spv)

  add_custom_command(
    OUTPUT ${SPV_FILE}
    COMMAND glslc -o ${SPV_FILE} ${FILE}
    COMMENT "Compiling into SPV ${FILE} -> ${SPV_FILE}"
    VERBATIM
  )

  add_custom_target(glsl_shader_${FILE_WE} DEPENDS ${SPV_FILE})
  add_dependencies(glsl_shaders glsl_shader_${FILE_WE})
endforeach()

