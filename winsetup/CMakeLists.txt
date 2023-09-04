file(GLOB_RECURSE source CONFIGURE_DEPENDS winsetup.h zip.h winsetup.c zip.c)

if(MSVC)
    add_compile_options(
        $<$<CONFIG:>:/MT> #---------|
        $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
        $<$<CONFIG:Release>:/MT> #--|
    )
endif()

add_library(winsetup STATIC ${source})
