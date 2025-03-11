add_library(freertos_kernel_port_headers INTERFACE)

target_sources(freertos_kernel_port_headers
    INTERFACE
        FILE_SET headers TYPE HEADERS BASE_DIRS source/include FILES 
            source/include/portmacro.h
)

add_library(freertos_kernel_port OBJECT)

target_sources(freertos_kernel_port
    PRIVATE
        source/src/port.cpp
    PUBLIC
        FILE_SET headers TYPE HEADERS BASE_DIRS source/include FILES 
            source/include/portmacro.h
            source/include/posix_v2/interrupt_state.hpp
            source/include/posix_v2/interrupt_state.tpp
            source/include/posix_v2/pause.hpp
            source/include/posix_v2/posix.hpp
            source/include/posix_v2/spinlock.hpp
            source/include/posix_v2/spinlock.tpp
            source/include/posix_v2/task_context.hpp
)

target_compile_features(freertos_kernel_port PRIVATE cxx_std_23)

if(CYGWIN)
    target_compile_definitions(freertos_kernel_port PRIVATE _GNU_SOURCE)
endif()

target_link_libraries(freertos_kernel_port
    PRIVATE
        freertos_kernel_port_headers
        freertos_kernel_include
)
