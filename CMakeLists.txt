cmake_minimum_required(VERSION 2.6)

find_package(Boost REQUIRED COMPONENTS serialization)

add_library(bare_chest_msgpack msgpack_archiver.cc)

