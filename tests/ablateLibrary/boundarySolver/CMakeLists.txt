target_sources(libraryTests
        PRIVATE
        boundarySolverPointTests.cpp
        boundarySolverDistributedTests.cpp
        )

add_subdirectory(lodi)
add_subdirectory(physics)