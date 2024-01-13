set(SOURCE main.cpp
        Network/ListenSocket.cpp
        Network/ClientSocket.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE})

if (WIN32)
    target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
elseif (UNIX)
    message(FATAL_ERROR "Unix is not supported")
endif (WIN32)