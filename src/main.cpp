/*
 * This file is part of the trojan project.
 * Trojan is an unidentifiable mechanism that helps you bypass GFW.
 * Copyright (C) 2018  GreaterFire
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <csignal>
#include <iostream>
#include <boost/program_options.hpp>
#include "service.h"
#include "version.h"
using namespace std;
namespace po = boost::program_options;

Service *service;
bool restart;

void handleTermination(int) {
    service->stop();
}

void restartService(int) {
    restart = true;
    service->stop();
}

int main(int argc, const char *argv[]) {
    try {
        Log::log("Welcome to trojan " + Version::get_version(), Log::FATAL);
        string config_file;
        string log_file;
        bool test;
        po::options_description desc("options");
        desc.add_options()
#ifdef _WIN32
            ("config,c", po::value<string>(&config_file)->default_value("config.json")->value_name("CONFIG"), "specify config file")
#else // _WIN32
            ("config,c", po::value<string>(&config_file)->default_value("/etc/trojan/config.json")->value_name("CONFIG"), "specify config file")
#endif // _WIN32
            ("help,h", "print help message")
            ("log,l", po::value<string>(&log_file)->value_name("LOG"), "specify log file location")
            ("test,t", po::bool_switch(&test), "test config file")
            ("version,v", "print version and build info")
        ;
        po::positional_options_description pd;
        pd.add("config", 1);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pd).run(), vm);
        po::notify(vm);
        if (vm.count("help")) {
            Log::log(string("usage: ") + argv[0] + " [-htv] [-l LOG] [[-c] CONFIG]", Log::FATAL);
            cout << desc;
            return 0;
        }
        if (vm.count("version")) {
#ifdef ENABLE_MYSQL
            Log::log(" [Enabled] MySQL Support", Log::FATAL);
#else // ENABLE_MYSQL
            Log::log("[Disabled] MySQL Support", Log::FATAL);
#endif // ENABLE_MYSQL
#ifdef TCP_FASTOPEN
            Log::log(" [Enabled] TCP_FASTOPEN Support", Log::FATAL);
#else // TCP_FASTOPEN
            Log::log("[Disabled] TCP_FASTOPEN Support", Log::FATAL);
#endif // TCP_FASTOPEN
#ifdef TCP_FASTOPEN_CONNECT
            Log::log(" [Enabled] TCP_FASTOPEN_CONNECT Support", Log::FATAL);
#else // TCP_FASTOPEN_CONNECT
            Log::log("[Disabled] TCP_FASTOPEN_CONNECT Support", Log::FATAL);
#endif // TCP_FASTOPEN_CONNECT
            return 0;
        }
        if (vm.count("log")) {
            Log::redirect(log_file);
        }
        Config config;
        do {
            restart = false;
            config.load(config_file);
            service = new Service(config, test);
            if (test) {
                Log::log("The config file looks good.", Log::OFF);
                return 0;
            }
            signal(SIGINT, handleTermination);
            signal(SIGTERM, handleTermination);
#ifndef _WIN32
            signal(SIGHUP, restartService);
#endif // _WIN32
            service->run();
            delete service;
            if (restart) {
                Log::log_with_date_time("trojan service restarting. . . ", Log::FATAL);
            }
        } while (restart);
        return 0;
    } catch (const exception &e) {
        Log::log_with_date_time(string("fatal: ") + e.what(), Log::FATAL);
        Log::log_with_date_time("exiting. . . ", Log::FATAL);
        return 1;
    }
}
