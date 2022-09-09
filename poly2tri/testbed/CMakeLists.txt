# Dependencies
# find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

# Build testbed
add_executable(p2t
    main.cc
)

target_link_libraries(p2t
    PRIVATE
	${ALL_LIBS}
    poly2tri
)
