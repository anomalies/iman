set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c90 -Wall -Wextra -Werror -pedantic")

add_executable(iman 
    iman.h
    iman.c 
    iman_architecture.h
    
    iman_options.h
    iman_options.c
    
    parser/iman_lexer.h
    parser/iman_lexer.c
    
    parser/iman_parser.h
    parser/iman_parser.c
)

install(TARGETS iman RUNTIME DESTINATION bin)