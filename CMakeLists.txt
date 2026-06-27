cmake_minimum_required(VERSION 4.2)
project(raft_cpp)

set(CMAKE_CXX_STANDARD 20)

find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)

add_executable(raft_cpp main.cpp
        storage.cpp
        storage.h
        raftpb/utils.cpp
        util.cpp
        util.cpp
        util.cpp
        raft.cpp
        raft.h
        utils.cpp
        utils.h
        tracker/progress.cpp
        tracker/progress.h
        tracker/inflights.cpp
        tracker/inflights.h
        quorum/majority.cpp
        quorum/majority.h
        log.cpp
        log.h
        tracker/tracker.cpp
        tracker/tracker.h)

target_include_directories(raft_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${Protobuf_INCLUDE_DIRS})
target_link_libraries(raft_cpp PRIVATE ${Protobuf_LIBRARIES} absl::base)
