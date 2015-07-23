set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -Werror -pedantic")

add_executable(iman 
    iman.h
    iman.c
    
    iman_config.h
    
    iman_architecture.h
    
    iman_options.h
    iman_options.c
)

install(TARGETS iman RUNTIME DESTINATION bin)

add_subdirectory(tools)
add_subdirectory(parser)