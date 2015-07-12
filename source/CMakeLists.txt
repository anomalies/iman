set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c90 -Wall -Wextra -Werror -pedantic -O2")

add_executable(iman 
    iman.h 
    iman.c 
    iman_options.c
)

install(TARGETS iman RUNTIME DESTINATION bin)