add_executable(iman-parser 
    ../iman.h
    
    iman_lexer.h
    iman_lexer.c
    
    iman_reference.h
    iman_reference.c
    
    iman_form_parser.h
    iman_form_parser.c
    
    iman_parser.h
    iman_parser.c
    
    iman_binary_writer.h
    iman_binary_writer.c
    
    iman_ref_writer.h
    iman_ref_writer.c
    
    parser.c
)

target_link_libraries(iman-parser z)