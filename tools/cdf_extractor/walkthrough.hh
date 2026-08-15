#ifndef WALKTHROUGH_HH__
#define WALKTHROUGH_HH__



#include <filesystem>



bool walkthrough(std::filesystem::path a_path, void (*f_cb)(void *, std::filesystem::path), void *a_data);



#endif
