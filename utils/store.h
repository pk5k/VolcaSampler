#ifndef UTILS_STORE_H_INC
#define UTILS_STORE_H_INC

bool load_sample_from_slot(const char* export_path, const char* target_name, int slot);
bool store_sample_to_slot(const char* export_path, const char* sample_file, int slot);

#endif