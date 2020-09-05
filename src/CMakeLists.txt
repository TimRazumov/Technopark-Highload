set(TARGET static_server)

file(GLOB FILES "*.cc")

add_executable(${TARGET} ${FILES})

target_link_libraries(${TARGET}
    ${LIBS}
)

target_include_directories(${TARGET} PRIVATE
    ${INCLUDE_DIRS}
)

target_link_libraries(${TARGET}
    Threads::Threads
    Boost::boost
    Boost::system
    Boost::program_options
    Boost::filesystem
    Boost::coroutine
    Boost::log 
    Boost::log_setup
)