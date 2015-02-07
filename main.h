#ifndef main_h
#define main_h

#include <string>
#include <vector>
#include <sstream>

extern std::stringstream ss;

extern unsigned int config_backups_per_world;

enum{
    YEAR,
    MONTH,
    DAY,
    HOUR,
    MINUTE,
    SECOND,
    FOLDER_NUMBER
};

struct date_and_time{
    unsigned int data[FOLDER_NUMBER+1];
};

std::string directory;

bool load_config();
bool save_config();

std::vector<date_and_time> quick_sort(std::vector<date_and_time> dates,short sort_by);

void write_to_log(std::string message);

std::string determine_world_name();

void create_top_level_directories(std::string world_name);

bool create_backup(std::string world_name);

void check_for_oldest_backup(std::string world_name);

void delete_oldest_backup(int target_folder_number,std::string world_directory);

int main(int argc,char* args[]);

#endif
