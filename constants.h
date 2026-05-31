#pragma once

#include<cstdint>    
#include<cstddef>


namespace Config{
    constexpr std::size_t PAGE_SIZE     = 4096;  //4 KB 
    constexpr std::size_t PAGE_HEADER_SIZE = 24;
    constexpr std::size_t SLOT_SIZE = 8;

}

constexpr uint32_t INVALID_PAGE_ID = 0xFFFFFFFF;
constexpr uint16_t INVALID_SLOT_ID = 0xFFFF;


constexpr uint32_t DIRECTORY_PAGE_ID = 0;

constexpr std::size_t BUFFER_POOL_SIZE = 32;