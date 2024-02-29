#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <iomanip>
#include <thread>

#include "file_utils.h"
#include "server.h"

int main(int argc, char**argv) {
    CmdOpts cmdOpts{};
    parse_cmd_arg(argc, argv, &cmdOpts);

    change_work_dir(cmdOpts.work_dir);

    std::set<std::string> diff_file;
    DbUtils dbUtils(cmdOpts.db_name);
    if (cmdOpts.just_so_diff) {
        fetch_new_diff_so(dbUtils, cmdOpts, diff_file);
        for(const auto& file:diff_file) {
            std::cout << file << std::endl;
        }
        return 0;
    }

    if (cmdOpts.just_deploy_once) {
        deploy(cmdOpts, dbUtils);
        return 0;
    }


    if (cmdOpts.daemon) {
        std::thread deploy_worker([&cmdOpts, &dbUtils](){
            while(true) {
                deploy_by_notify(cmdOpts, dbUtils);
            }
        });

        std::thread event_worker([&cmdOpts](){
            event_service(cmdOpts.port, cmdOpts.file_path.c_str());
        });

        deploy_worker.join();
        event_worker.join();
    }

    return 0;
}
