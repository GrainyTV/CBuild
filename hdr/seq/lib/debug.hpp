#pragma once

#ifndef NDEBUG
    #include <cstdlib>
    #include <iostream>
    #include <source_location>

    #define ASSERT_WITH_ORIGIN(condition, message, sl)                           \
        if ((condition) == false)                                                \
        {                                                                        \
            std::cout << "[[ASSERT_FAILURE]]" << '\n';                           \
            std::cout << (message) << '\n';                                      \
            std::cout << "@ " << (sl).file_name() << ":" << (sl).line() << '\n'; \
            std::abort();                                                        \
        }

    #define ASSERT(condition, message) ASSERT_WITH_ORIGIN(condition, message, std::source_location::current());

#else
    #define ASSERT(condition, message) ((void) 0)

#endif
