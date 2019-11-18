add_custom_command(
    OUTPUT ${LIBSBOX_SOURCE_DIR}/generated/request_schema.c ${LIBSBOX_SOURCE_DIR}/generated/request_schema.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/request.json ${CMAKE_CURRENT_SOURCE_DIR}/dump.sh
    COMMAND mkdir -p ${LIBSBOX_SOURCE_DIR}/generated
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/dump.sh ${CMAKE_CURRENT_SOURCE_DIR}/request.json request_schema ${LIBSBOX_SOURCE_DIR}/generated/request_schema.c ${LIBSBOX_SOURCE_DIR}/generated/request_schema.h
)

add_custom_command(
    OUTPUT ${LIBSBOX_SOURCE_DIR}/generated/response_schema.c ${LIBSBOX_SOURCE_DIR}/generated/response_schema.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/response.json ${CMAKE_CURRENT_SOURCE_DIR}/dump.sh
    COMMAND mkdir -p ${LIBSBOX_SOURCE_DIR}/generated
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/dump.sh ${CMAKE_CURRENT_SOURCE_DIR}/response.json response_schema ${LIBSBOX_SOURCE_DIR}/generated/response_schema.c ${LIBSBOX_SOURCE_DIR}/generated/response_schema.h
)

add_custom_target(
    json_schemas
    DEPENDS ${LIBSBOX_SOURCE_DIR}/generated/request_schema.c
    DEPENDS ${LIBSBOX_SOURCE_DIR}/generated/request_schema.h
    DEPENDS ${LIBSBOX_SOURCE_DIR}/generated/response_schema.c
    DEPENDS ${LIBSBOX_SOURCE_DIR}/generated/response_schema.h
)
