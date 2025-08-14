set(THREADS_PREFER_PTHREAD_FLAG ON)

add_llvm_executable(bracelet bracelet.cpp)

find_package(Threads REQUIRED)
target_link_libraries(bracelet PUBLIC Threads::Threads)
