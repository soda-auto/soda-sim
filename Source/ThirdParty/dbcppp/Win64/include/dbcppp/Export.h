
#pragma once

#ifdef DBCPPP_STATIC
#    define DBCPPP_API
#else
#    ifdef _WIN32
#        ifdef DBCPPP_EXPORT
#            define DBCPPP_API __declspec(dllexport)
#        else
#            define DBCPPP_API __declspec(dllimport)
#        endif
#    else
#        define DBCPPP_API
#    endif
#endif
