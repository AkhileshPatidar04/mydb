#include <iostream>
#include <cstring>
#include "storage/diskManager.h"
DiskManager::DiskManager(const std::string& db_file) : file_name_(db_file){
    db_io_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);
    if(!db_io_.is_open()){ // may be file does not exist
        db_io_.open(file_name_, std::ios::trunc | std::ios::in | std::ios::out);
    }

    db_io_.open(file_name_, std::ios::binary | std::ios::in | std::ios::out);
    if(!db_io_.is_open()){ // failed to open
        std::cerr << "DiskManager: Failed to open db file: " << file_name_ << std::endl;
        exit(1);
    }

    // figure out next_page_id_
    db_io_.seekg(0, std::ios::end);
    std::streamoff file_size = db_io_.tellg();
    if(file_size > 0){
        next_page_id_ = file_size / PAGE_SIZE;
    }
    else{
        next_page_id_ = 0;
    }
}

DiskManager::~DiskManager(){
    if(db_io_.is_open()){
        db_io_.close();
    }
}

bool DiskManager::readPage(page_id_t page_id, char* out_buf){
    
    std::streamoff offset = page_id * PAGE_SIZE;

    db_io_.clear();
    db_io_.seekg(offset);
    if(db_io_.fail())
    {
        db_io_.clear();
        return false;
    }

    db_io_.read(out_buf, PAGE_SIZE);
    if(db_io_.eof())
    {
        std::streamsize read_count = db_io_.gcount();
        std::memset(out_buf + read_count, 0, PAGE_SIZE - read_count);
        db_io_.clear();
        return true;
    }
    if(db_io_.fail())
    {
        db_io_.clear();
        return false;
    }
    return true;
}

bool DiskManager::writePage(page_id_t page_id, const char* in_buf)
{
  
    std::streamoff offset = page_id * PAGE_SIZE;
    db_io_.seekp(offset);
    if(db_io_.fail())
    {
        db_io_.clear();
        return false;
    }
    db_io_.write(in_buf, PAGE_SIZE);
    if(db_io_.fail()){
        db_io_.clear();
        return false;
    }
    db_io_.flush();
    if(db_io_.fail())
    {
        db_io_.clear();
        return false;
    }
    return true;
}

page_id_t DiskManager::allocatePage()
{
    return next_page_id_++;
}
