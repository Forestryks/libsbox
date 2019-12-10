add_custom_command(
    OUTPUT generated/request_schema.c generated/request_schema.h
    DEPENDS request.json
    COMMAND mkdir -p generated
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/dump.sh ${CMAKE_CURRENT_SOURCE_DIR}/request.json request_schema generated/request_schema.c generated/request_schema.h
    VERBATIM
)

add_custom_command(
    OUTPUT generated/response_schema.c generated/response_schema.h
    DEPENDS response.json
    COMMAND mkdir -p generated
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/dump.sh ${CMAKE_CURRENT_SOURCE_DIR}/response.json response_schema generated/response_schema.c generated/response_schema.h
    VERBATIM
)

add_custom_target(
    json_schemas
    DEPENDS generated/request_schema.c
    DEPENDS generated/request_schema.h
    DEPENDS generated/response_schema.c
    DEPENDS generated/response_schema.h
)
