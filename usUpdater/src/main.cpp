#include <string>
#include <memory>
#include <filesystem>
#include <forward_list>
#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

namespace fs = std::filesystem;
namespace opt=boost::program_options;

std::shared_ptr<spdlog::logger> logger_ptr_ {nullptr};

//part for windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
SERVICE_STATUS        status_   {NULL};
SERVICE_STATUS_HANDLE h_status_ {NULL};

VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler (DWORD);
int start_installer_win(DWORD argc, LPTSTR *argv);
#endif

void init_spdlog(const std::string& app_dir_path);
int start_installer_linux(int argc, char *argv[]);

//part for windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv)
{
    h_status_ = RegisterServiceCtrlHandler (L"USystemUpdater", ServiceCtrlHandler);
    if (h_status_ == NULL){
        const std::string& msg {"USystemUpdater: ServiceMain: RegisterServiceCtrlHandler returned error"};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory (&status_, sizeof (status_));
    status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status_.dwControlsAccepted = 0;
    status_.dwCurrentState = SERVICE_START_PENDING;
    status_.dwWin32ExitCode = 0;
    status_.dwServiceSpecificExitCode = 0;
    status_.dwCheckPoint = 0;

    if (SetServiceStatus (h_status_, &status_) == FALSE){
        const std::string& msg {"USystemUpdater: ServiceMain: SetServiceStatus returned error"};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
    }

    //Perform tasks neccesary to start the service here
    {
        const std::string& msg {"USystemUpdater: ServiceMain: Performing Service Start Operations"};
        if(logger_ptr_){
            logger_ptr_->info(msg);
        }
    }

    // Tell the service controller we are started
    status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    status_.dwCurrentState = SERVICE_RUNNING;
    status_.dwWin32ExitCode = 0;
    status_.dwCheckPoint = 0;

    if (SetServiceStatus (h_status_, &status_) == FALSE){
        const std::string& msg {"USystemUpdater: ServiceMain: SetServiceStatus returned error"};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
    }

    // Start new process for installer
    {
        const int& result {start_installer_win(argc,argv)};
        const std::string& msg {(boost::format("USystemUpdater: start_inslaller: result: '%1%'")
                    % result).str()};
        if(logger_ptr_){
            logger_ptr_->info(msg);
        }
    }

    //Perform any cleanup tasks
    {
        const std::string& msg {"USystemUpdater: ServiceMain: Performing Cleanup Operations"};
        if(logger_ptr_){
            logger_ptr_->info(msg);
        }
    }

    status_.dwControlsAccepted = 0;
    status_.dwCurrentState = SERVICE_STOPPED;
    status_.dwWin32ExitCode = 0;
    status_.dwCheckPoint = 3;

    if (SetServiceStatus (h_status_, &status_) == FALSE){
        const std::string& msg {"USystemUpdater: ServiceMain: SetServiceStatus returned error"};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
    }
    return;
}

VOID WINAPI ServiceCtrlHandler (DWORD CtrlCode)
{
    switch(CtrlCode){
    case SERVICE_CONTROL_STOP :
    {
        if (status_.dwCurrentState != SERVICE_RUNNING){
            break;
        }
        status_.dwControlsAccepted = 0;
        status_.dwCurrentState = SERVICE_STOP_PENDING;
        status_.dwWin32ExitCode = 0;
        status_.dwCheckPoint = 4;
        if(SetServiceStatus (h_status_, &status_)==FALSE){
            const std::string& msg {"Fail to stop service, SetServiceStatus failed"};
            if(logger_ptr_){
                logger_ptr_->warn(msg);
            }
        }
    }
        break;
    default:
        break;
    }
}

int start_installer_win(DWORD argc, LPTSTR *argv)
{
    try{
        fs::path self_path { argv[0] };
        self_path = self_path.remove_filename();
        init_spdlog(self_path.string());

        opt::options_description start_options("Start options");
        start_options.add_options()
                ("execute_path", opt::value<std::string>(), "path to program for execute")
                ("execute_args", opt::value<std::string>(), "args for program to execute");

        opt::variables_map vm;
        opt::store(opt::parse_command_line(argc,argv,start_options),vm);
        opt::notify(vm);
        if(vm.count("execute_path") && vm.count("execute_args")){
            const std::string& installer_path {vm.at("execute_path").as<std::string>()};
            const std::string& installer_arg {vm.at("execute_args").as<std::string>()};
            if(logger_ptr_){
                const std::string& msg {(boost::format("USystemUpdater: start_installer: execute_path: '%1%', execute_args: '%2%'")
                            % installer_path
                            % installer_arg).str()};
                logger_ptr_->info(msg);
            }

            const int& install_result {boost::process::system(installer_path.data(), installer_arg)};

            if (0 != install_result) {
                const std::string& msg {(boost::format("USystemUpdater: start_installer: process '%1% %2%' was completed with code '%3%'!")
                            % installer_path
                            % installer_arg
                            % install_result).str()};
                if(logger_ptr_){
                    logger_ptr_->warn(msg);
                }
            }
            return EXIT_SUCCESS;
        }
    }
    catch(std::exception& ex){
        const std::string& msg {"Operation failed, execption: " + std::string{ex.what()}};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
        return EXIT_FAILURE;
    }
    catch(...){
        const std::string& msg {"Operation failed, unknown execption"};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
#endif

int start_installer_linux(int argc, char *argv[])
{
    try{
        fs::path self_path { argv[0] };
        self_path = self_path.remove_filename();
        init_spdlog(self_path.string());

        opt::options_description start_options("Start options");
        start_options.add_options()
                ("execute_path", opt::value<std::string>(), "path to program for execute")
                ("mode", opt::value<std::string>(), "mode for program to execute");

        opt::variables_map vm;
        opt::store(opt::parse_command_line(argc,argv,start_options),vm);
        opt::notify(vm);
        if(vm.count("execute_path") && vm.count("mode")){
            const std::string& installer_path {vm.at("execute_path").as<std::string>()};
            const std::string& mode {vm.at("mode").as<std::string>()};
            int install_result {};

            if(logger_ptr_){
                const std::string& msg {(boost::format("USystemUpdater: start_installer: execute_path: '%1%', mode: '%2%'")
                            % installer_path
                            % mode).str()};
                logger_ptr_->info(msg);
            }

            //install 'deb' package
            if(mode=="deb"){
                const auto& command { std::string("apt -y --allow-downgrades install ") + installer_path };
                install_result=boost::process::system(command);
            }
            //install 'rpm' package
            else if(mode=="rpm"){
                const auto& command { std::string("yum -y install ") + installer_path };
                install_result=boost::process::system(command);
            }
            else{
                const std::string& msg{"Mode not set or not correct"};
                if(logger_ptr_){
                    logger_ptr_->warn(msg);
                    return EXIT_FAILURE;
                }
            }

            if (0 != install_result) {
                const std::string& msg {(boost::format("USystemUpdater: start_installer: process '%1% %2%' was completed with code '%3%'!")
                            % installer_path
                            % mode
                            % install_result).str()};
                if(logger_ptr_){
                    logger_ptr_->warn(msg);
                }
                throw std::runtime_error(msg);
            }

            //restart 'usagent' service
            const std::string& restart_command {std::string("systemctl restart usagent")};
            const int& restart_result {boost::process::system(restart_command)};
            return EXIT_SUCCESS;
        }
    }
    catch(std::exception& ex){
        const std::string& msg {"Operation failed, execption: " + std::string{ex.what()}};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
        return EXIT_FAILURE;
    }
    catch(...){
        const std::string& msg {"Operation failed, unknown execption"};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void init_spdlog(const std::string& app_dir_path)
{
    const boost::posix_time::ptime pt {boost::posix_time::second_clock::universal_time()};
    const std::string& datetime_str {boost::posix_time::to_iso_string(pt)};
    const std::string& log_name {"USystemUpdater"};
    const std::string& log_path {app_dir_path + "USystemUpdater_" + datetime_str + ".txt"};

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    //sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(path_to_log_file, 0, 0));
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path, 1024 * 1024, 3));

    logger_ptr_=spdlog::get(log_name);
    if(!logger_ptr_){
        logger_ptr_.reset(new spdlog::logger(log_name, sinks.begin(),sinks.end()));
        spdlog::register_logger(logger_ptr_);
        //logger_ptr_->set_pattern("[%H:%M:%S:%e.%f %z] [%o] [thread %t] [%n] %^[%L] %v%$");
        logger_ptr_->set_level(spdlog::level::info);
        logger_ptr_->flush_on(spdlog::level::info);
    }
}

int main(int argc, char *argv[])
{
    try{
        fs::path self_path {argv[0]};
        self_path = self_path.remove_filename();
        init_spdlog(self_path.string());

//part for windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        //create 'SERVICE_TABLE_ENTRY'
        SERVICE_TABLE_ENTRY service_table[]={
            {L"USystemUpdater", (LPSERVICE_MAIN_FUNCTION) ServiceMain},
            {NULL, NULL}
        };

        //try to run 'StartServiceCtrlDispatcher'
        if (StartServiceCtrlDispatcher (service_table) == FALSE){
            const DWORD& err {GetLastError()};
            const std::string& msg {(boost::format("USystemUpdater:main: fail to run 'StartServiceCtrlDispatcher', error: '%1%'")
                        % err).str()};
            if(logger_ptr_){
                logger_ptr_->warn(msg);
            }
            return EXIT_FAILURE;
        }
#endif

//part for linux
#if defined (__linux__) || defined(__linux) || defined(__gnu_linux__)
        return start_installer_linux(argc,argv);
#endif
    }
    catch(std::exception& ex){
        const std::string& msg {(boost::format("USystemUpdater:main: exception: '%1%'") % ex.what()).str()};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
        return EXIT_FAILURE;
    }
    catch(...){
        const std::string& msg {(boost::format("USystemUpdater:main: unknown exception")).str()};
        if(logger_ptr_){
            logger_ptr_->warn(msg);
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
