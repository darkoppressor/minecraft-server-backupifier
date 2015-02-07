#include "main.h"

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

stringstream ss;

unsigned int config_backups_per_world=7;

bool save_config(){
    ofstream save("minecraft-server-backup.cfg");

    if(save.is_open()){
        save<<"//The number of backups that will be saved for any given world.\n";
        save<<"//If the number of backups exceeds this limit, the oldest backup will be deleted.\n";
        save<<"//A value of 0 means no limit.\n";
        save<<"//Default: 7\n\n";
        save<<"backups per world:"<<config_backups_per_world<<"\n\n";

        save.close();
        save.clear();
    }
    else{
        return false;
    }

    return true;
}

bool load_config(){
    ifstream load("minecraft-server-backup.cfg");

    if(load.is_open()){
        write_to_log("Loading minecraft-server-backup.cfg...");

        //As long as we haven't reached the end of the file.
        while(!load.eof()){
            string line="";

            //The data name strings used in the file.

            string backups_per_world="backups per world:";

            //Grab the next line of the file.
            getline(load,line);

            //Clear initial whitespace from the line.
            boost::algorithm::trim(line);

            //If the line is a comment.
            if(boost::algorithm::istarts_with(line,"//")){
                //Ignore this line.
            }

            //Backups per world
            else if(boost::algorithm::icontains(line,backups_per_world)){
                //Clear the data name.
                line.erase(0,backups_per_world.length());

                config_backups_per_world=atoi(line.c_str());
            }
        }

        load.close();
        load.clear();
    }
    else{
        load.close();
        load.clear();
        write_to_log("minecraft-server-backup.cfg not found. Creating one...");
        if(!save_config()){
            write_to_log("Unable to save minecraft-server-backup.cfg!");
            write_to_log("Aborting...\n");
            return false;
        }
    }

    return true;
}

vector<date_and_time> quick_sort(vector<date_and_time> dates,short sort_by){
    vector<date_and_time> dates_less;
    vector<date_and_time> dates_greater;

    if(dates.size()<=1){
        return dates;
    }

    date_and_time pivot=dates[0];

    for(int i=1;i<dates.size();i++){
        if(dates[i].data[sort_by]<=pivot.data[sort_by]){
            dates_less.push_back(dates[i]);
        }
        else{
            dates_greater.push_back(dates[i]);
        }
    }

    vector<date_and_time> result=quick_sort(dates_less,sort_by);
    result.push_back(pivot);
    vector<date_and_time> dates_greater_sorted=quick_sort(dates_greater,sort_by);
    result.insert(result.end(),dates_greater_sorted.begin(),dates_greater_sorted.end());

    return result;
}

void write_to_log(string message){
    string save_str="log-minecraft-server-backupifier";

    #ifdef GAME_OS_WINDOWS
        save_str+=".txt";
    #endif

    ofstream save_log(save_str,ofstream::app);

    if(save_log.is_open()){
        time_t now;
        struct tm *tm_now;
        char buff[BUFSIZ];
        now=time(NULL);
        tm_now=localtime(&now);
        strftime(buff,sizeof buff,"[%Y-%m-%d %H:%M:%S] ",tm_now);

        save_log<<buff<<"["<<directory<<"] "<<message<<"\n";

        save_log.close();
        save_log.clear();
    }
}

string determine_world_name(){
    string world_name="";

    string load_str=directory+"server.properties";
    ifstream load(load_str.c_str(),ifstream::in);

    if(load.is_open()){
        write_to_log("server.properties successfully loaded!");

        //As long as we haven't reached the end of the file.
        while(!load.eof()){
            string line="";

            //Grab the next line of the file.
            getline(load,line);

            //Clear initial whitespace from the line.
            boost::algorithm::trim(line);

            if(boost::algorithm::istarts_with(line,"level-name=")){
                boost::algorithm::ierase_first(line,"level-name=");
                boost::algorithm::trim(line);
                world_name=line;
                string msg="World name=";
                msg+=world_name;
                write_to_log(msg);
                break;
            }
        }

        load.close();
        load.clear();
    }
    else{
        load.close();
        load.clear();

        write_to_log("Failed to load server.properties!");
        write_to_log("Is the Minecraft Server Backupifier in the Minecraft server's directory?");
        write_to_log("Or, did you pass the Minecraft server's correct directory to the Backupifier?");
        write_to_log("Has the server been properly setup?");
        write_to_log("Aborting backup...\n");
    }

    return world_name;
}

void create_top_level_directories(string world_name){
    boost::filesystem::create_directory(directory+"backups");

    boost::filesystem::create_directories(directory+"backups/"+world_name);
}

bool create_backup(string world_name){
    write_to_log("Starting backup...");

    time_t now;
    struct tm *tm_now;
    char buff[BUFSIZ];
    now=time(NULL);
    tm_now=localtime(&now);
    strftime(buff,sizeof buff,"%Y-%m-%d_%H.%M.%S",tm_now);

    string backup_directory=directory+"backups/";
    backup_directory+=world_name;
    backup_directory+="/";
    backup_directory+=buff;

    if(boost::filesystem::exists(backup_directory)){
        write_to_log("Backup with this exact name already exists!\nAborting backup...\n");

        return false;
    }

    if(!boost::filesystem::exists(world_name)){
        write_to_log("The world specified in server.properties does not exist!\nAborting backup...\n");

        return false;
    }

    string world_backup=directory+"backups/";
    world_backup+=world_name;
    boost::filesystem::create_directory(world_backup);
    boost::filesystem::create_directory(backup_directory);

    for(boost::filesystem::recursive_directory_iterator end_of_files,dir(world_name);dir!=end_of_files;++dir){
        boost::filesystem::path temp_path(*dir);
        string destination_file=temp_path.string();
        boost::algorithm::replace_first(destination_file,world_name,backup_directory);
        boost::filesystem::copy(*dir,destination_file);
    }

    write_to_log("Success! Backup created.");

    return true;
}

void check_for_oldest_backup(string world_name){
    write_to_log("Number of backups exceeds the limit.");
    write_to_log("Deleting oldest backup...");

    string world_directory=directory+"backups/";
    world_directory+=world_name;

    vector<date_and_time> dates;

    int folder_number=0;

    //Look through all of the directories in the directory.
    for(boost::filesystem::directory_iterator it(world_directory);it!=boost::filesystem::directory_iterator();it++){
        if(boost::filesystem::is_directory(it->path())){
            //Create a new date/time entry.
            dates.push_back(date_and_time());

            dates[dates.size()-1].data[FOLDER_NUMBER]=folder_number;

            //Create a string that holds the folder's name.
            string new_date=it->path().filename().string();

            string convert="";
            for(int i=0;i<4;i++){
                convert+=new_date[i];
            }
            dates[dates.size()-1].data[YEAR]=atoi(convert.c_str());

            convert="";
            for(int i=5;i<7;i++){
                convert+=new_date[i];
            }
            dates[dates.size()-1].data[MONTH]=atoi(convert.c_str());

            convert="";
            for(int i=8;i<10;i++){
                convert+=new_date[i];
            }
            dates[dates.size()-1].data[DAY]=atoi(convert.c_str());

            convert="";
            for(int i=11;i<13;i++){
                convert+=new_date[i];
            }
            dates[dates.size()-1].data[HOUR]=atoi(convert.c_str());

            convert="";
            for(int i=14;i<16;i++){
                convert+=new_date[i];
            }
            dates[dates.size()-1].data[MINUTE]=atoi(convert.c_str());

            convert="";
            for(int i=17;i<19;i++){
                convert+=new_date[i];
            }
            dates[dates.size()-1].data[SECOND]=atoi(convert.c_str());

            folder_number++;
        }
    }

    vector<date_and_time> dates_temp;

    string msg="";

    ///This was used for debugging.
    /**for(int i=0;i<dates.size();i++){
        ss.clear();ss.str("");
        ss<<"[";ss<<dates[i].data[FOLDER_NUMBER];ss<<"]";
        ss<<"Year: ";ss<<dates[i].data[YEAR];ss<<"\n";
        ss<<"Month: ";ss<<dates[i].data[MONTH];ss<<"\n";
        ss<<"Day: ";ss<<dates[i].data[DAY];ss<<"\n";
        ss<<"Hour: ";ss<<dates[i].data[HOUR];ss<<"\n";
        ss<<"Minute: ";ss<<dates[i].data[MINUTE];ss<<"\n";
        ss<<"Second: ";ss<<dates[i].data[SECOND];ss<<"\n";
        msg+=ss.str();
    }
    write_to_log(msg);*/

    dates=quick_sort(dates,YEAR);
    dates_temp.clear();
    for(int i=0,n=dates[0].data[YEAR];i<dates.size();i++){
        if(dates[i].data[YEAR]>n){
            break;
        }
        dates_temp.push_back(dates[i]);
    }
    dates=dates_temp;
    if(dates.size()==1){
        delete_oldest_backup(dates[0].data[FOLDER_NUMBER],world_directory);
        return;
    }

    dates=quick_sort(dates,MONTH);
    dates_temp.clear();
    for(int i=0,n=dates[0].data[MONTH];i<dates.size();i++){
        if(dates[i].data[MONTH]>n){
            break;
        }
        dates_temp.push_back(dates[i]);
    }
    dates=dates_temp;
    if(dates.size()==1){
        delete_oldest_backup(dates[0].data[FOLDER_NUMBER],world_directory);
        return;
    }

    dates=quick_sort(dates,DAY);
    dates_temp.clear();
    for(int i=0,n=dates[0].data[DAY];i<dates.size();i++){
        if(dates[i].data[DAY]>n){
            break;
        }
        dates_temp.push_back(dates[i]);
    }
    dates=dates_temp;
    if(dates.size()==1){
        delete_oldest_backup(dates[0].data[FOLDER_NUMBER],world_directory);
        return;
    }

    dates=quick_sort(dates,HOUR);
    dates_temp.clear();
    for(int i=0,n=dates[0].data[HOUR];i<dates.size();i++){
        if(dates[i].data[HOUR]>n){
            break;
        }
        dates_temp.push_back(dates[i]);
    }
    dates=dates_temp;
    if(dates.size()==1){
        delete_oldest_backup(dates[0].data[FOLDER_NUMBER],world_directory);
        return;
    }

    dates=quick_sort(dates,MINUTE);
    dates_temp.clear();
    for(int i=0,n=dates[0].data[MINUTE];i<dates.size();i++){
        if(dates[i].data[MINUTE]>n){
            break;
        }
        dates_temp.push_back(dates[i]);
    }
    dates=dates_temp;
    if(dates.size()==1){
        delete_oldest_backup(dates[0].data[FOLDER_NUMBER],world_directory);
        return;
    }

    dates=quick_sort(dates,SECOND);
    dates_temp.clear();
    for(int i=0,n=dates[0].data[SECOND];i<dates.size();i++){
        if(dates[i].data[SECOND]>n){
            break;
        }
        dates_temp.push_back(dates[i]);
    }
    dates=dates_temp;
    if(dates.size()==1){
        delete_oldest_backup(dates[0].data[FOLDER_NUMBER],world_directory);
        return;
    }

    //If all folder sorting is complete and we still have more than one folder.
    if(dates.size()>1){
        msg="";
        ss.clear();ss.str("");ss<<"More than one backup folder (";ss<<dates.size();ss<<") with the same date/time!";msg=ss.str();
        write_to_log(msg);
        write_to_log("Aborting old backup deletion...\n");
    }
}

void delete_oldest_backup(int target_folder_number,string world_directory){
    int folder_number=0;

    //Look through all of the directories in the directory.
    for(boost::filesystem::directory_iterator it(world_directory);it!=boost::filesystem::directory_iterator();it++){
        if(boost::filesystem::is_directory(it->path())){
            if(folder_number==target_folder_number){
                boost::filesystem::remove_all(it->path());
            }

            folder_number++;
        }
    }

    write_to_log("Done!\n");
}

int main(int argc,char* args[]){
    directory="./";

    //Can this even happen?
    if(argc<1){
        cout<<"Minecraft Server Backupifier error: Did not receive the program name";

        return 1;
    }

    if(argc>=2){
        directory=args[1];
    }

    boost::algorithm::replace_all(directory,"\\","/");

    if(directory[directory.size()-1]!='/'){
        directory+="/";
    }

    if(!boost::filesystem::exists(directory)){
        cout<<"Minecraft Server Backupifier error: The passed directory does not exist";

        return 1;
    }

    write_to_log("Beginning Minecraft server backup...");

    string world_name=determine_world_name();

    if(world_name.length()==0){
        return 1;
    }

    create_top_level_directories(world_name);

    if(!load_config()){
        return 1;
    }

    if(create_backup(world_name)){
        string world_directory=directory+"backups/";
        world_directory+=world_name;

        int folder_number=0;

        //Look through all of the directories in the directory.
        for(boost::filesystem::directory_iterator it(world_directory);it!=boost::filesystem::directory_iterator();it++){
            if(boost::filesystem::is_directory(it->path())){
                folder_number++;
            }
        }

        if(config_backups_per_world>0 && folder_number>config_backups_per_world){
            check_for_oldest_backup(world_name);
        }
        else if(config_backups_per_world<=0){
            write_to_log("No backup limit set, so no need to delete any.");
            write_to_log("Done!\n");
        }
        else{
            write_to_log("Number of backups is below the limit, so no need to delete any.");
            write_to_log("Done!\n");
        }
    }
    else{
        return 1;
    }

    return 0;
}
