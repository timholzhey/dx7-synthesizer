cmake_minimum_required(VERSION 3.16)
project(fm_synthesizer C)

set(CMAKE_C_STANDARD 99)

set(PORTAUDIO_DIR "${CMAKE_SOURCE_DIR}/libs/portaudio")
set(HTTP_SERVER_DIR "${CMAKE_SOURCE_DIR}/libs/http-server")

add_executable(fm_synthesizer
        src/main.c
        src/synthesizer/patch_file.c
        src/synthesizer/synthesizer.c
        src/synthesizer/voice.c
        src/visualization/visualization.c
        src/audio_driver/audio_driver.c
        src/web_server/web_server.c

        ${HTTP_SERVER_DIR}/src/http/server/http_server.c
        ${HTTP_SERVER_DIR}/src/http/headers/http_headers.c
        ${HTTP_SERVER_DIR}/src/http/request/http_request.c
        ${HTTP_SERVER_DIR}/src/http/request/parser/http_request_parser.c
        ${HTTP_SERVER_DIR}/src/http/request/handler/http_request_handler.c
        ${HTTP_SERVER_DIR}/src/http/request/params/http_request_params.c
        ${HTTP_SERVER_DIR}/src/http/response/http_response.c
        ${HTTP_SERVER_DIR}/src/http/response/builder/http_response_builder.c
        ${HTTP_SERVER_DIR}/src/http/method/http_method.c
        ${HTTP_SERVER_DIR}/src/http/status/http_status.c
        ${HTTP_SERVER_DIR}/src/http/route/http_route.c
        ${HTTP_SERVER_DIR}/src/http/interface/http_interface.c
        ${HTTP_SERVER_DIR}/src/http/static/http_static.c
        ${HTTP_SERVER_DIR}/src/http/mime/http_mime.c

        ${HTTP_SERVER_DIR}/src/websocket/server/websocket_server.c
        ${HTTP_SERVER_DIR}/src/websocket/handshake/websocket_handshake.c
        ${HTTP_SERVER_DIR}/src/websocket/route/websocket_route.c
        ${HTTP_SERVER_DIR}/src/websocket/frame/websocket_frame.c
        ${HTTP_SERVER_DIR}/src/websocket/interface/websocket_interface.c

        ${HTTP_SERVER_DIR}/src/json/json.c
        ${HTTP_SERVER_DIR}/src/json/lexer/json_lexer.c
        ${HTTP_SERVER_DIR}/src/json/parser/json_parser.c
        ${HTTP_SERVER_DIR}/src/json/stringifier/json_stringifier.c

        ${HTTP_SERVER_DIR}/src/lib/sha1/sha1.c
        ${HTTP_SERVER_DIR}/src/lib/base64/base64.c
)

include_directories(
        src
        src/synthesizer
        src/visualization
        src/audio_driver
        src/web_server

        libs/portaudio/include

        ${HTTP_SERVER_DIR}/src
        ${HTTP_SERVER_DIR}/src/http
        ${HTTP_SERVER_DIR}/src/http/server
        ${HTTP_SERVER_DIR}/src/http/headers
        ${HTTP_SERVER_DIR}/src/http/request
        ${HTTP_SERVER_DIR}/src/http/request/parser
        ${HTTP_SERVER_DIR}/src/http/request/handler
        ${HTTP_SERVER_DIR}/src/http/request/params
        ${HTTP_SERVER_DIR}/src/http/response
        ${HTTP_SERVER_DIR}/src/http/response/builder
        ${HTTP_SERVER_DIR}/src/http/method
        ${HTTP_SERVER_DIR}/src/http/status
        ${HTTP_SERVER_DIR}/src/http/route
        ${HTTP_SERVER_DIR}/src/http/interface
        ${HTTP_SERVER_DIR}/src/http/static
        ${HTTP_SERVER_DIR}/src/http/mime

        ${HTTP_SERVER_DIR}/src/websocket
        ${HTTP_SERVER_DIR}/src/websocket/server
        ${HTTP_SERVER_DIR}/src/websocket/handshake
        ${HTTP_SERVER_DIR}/src/websocket/route
        ${HTTP_SERVER_DIR}/src/websocket/frame
        ${HTTP_SERVER_DIR}/src/websocket/interface

        ${HTTP_SERVER_DIR}/src/json
        ${HTTP_SERVER_DIR}/src/json/lexer
        ${HTTP_SERVER_DIR}/src/json/parser
        ${HTTP_SERVER_DIR}/src/json/stringifier

        ${HTTP_SERVER_DIR}/src/lib
        ${HTTP_SERVER_DIR}/src/lib/sha1
        ${HTTP_SERVER_DIR}/src/lib/base64
)

find_library(PORTAUDIO_LIB
        NAMES portaudio
        HINTS "${CMAKE_SOURCE_DIR}/libs/portaudio/*"
)

if (NOT PORTAUDIO_LIB)
    message(FATAL_ERROR "PortAudio library not found")
endif()

target_link_libraries(fm_synthesizer ${PORTAUDIO_LIB})

add_definitions(
        -DSOURCE_DIR="${CMAKE_SOURCE_DIR}"
        -DHTTP_SERVER_ROOT_DIR="${CMAKE_SOURCE_DIR}"
        -DDEBUG_GLOBAL=1
)
