#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <chrono>
#include <thread>
#include <sstream>
#include <regex>
#include <filesystem>
#include <fstream>
#include "console.hpp"

void update_check( );
void dump_to_github( );
std::string exec( const char * cmd ) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype( &_pclose )> pipe( _popen( cmd, "r" ), _pclose );
    if ( !pipe ) {
        throw std::runtime_error( "popen() failed!" );
    }
    while ( fgets( buffer.data( ), buffer.size( ), pipe.get( ) ) != nullptr ) {
        result += buffer.data( );
    }
    return result;
}

std::string remote_build;
std::string local_build;
bool updated = false;

constexpr char username[] = "USERNAME";
constexpr char password[] = "PASSWORD";
constexpr char install_dir[] = "..\\csgo_client";

bool update( ) {
    try {
        console::print_success( true, "Update available" );
        console::print( normal, "Local build: ", false );
        console::heck( failure, local_build.c_str( ) );
        console::print( normal, "Remote build: ", false );
        console::heck( success, remote_build.c_str( ) );
        std::stringstream ss;
        ss << "steamcmd\\steamcmd.exe +login " << username << " " << password << " +force_install_dir " << install_dir <<
            " +app_update 730 +quit";
        auto response = exec( ss.str( ).c_str( ) );
        if ( response.find( "Success!" ) != std::string::npos ) {
            updated = true;
            local_build = remote_build;

            std::ofstream outstream( "buildid.txt" );
            outstream << local_build;
            outstream.close( );
        } else {
            updated = false;
            std::this_thread::sleep_for( std::chrono::seconds( 30 ) );
        }
        if ( updated ) {
            dump_to_github( );
        }
        return updated;
    } catch ( std::exception & e ) {
        console::print_failure( true, "Update failed: %s", e.what( ) );
        return false;
    }
}

void update_check( ) {

    try {
        static std::regex regexp( R"(^\s\s\s\"public\"\s+\{\s+\"buildid\"\s+\"(\d+)\")" );

        std::stringstream ss;
        ss << "steamcmd\\steamcmd.exe +login " << username << " " << password << " +app_info_update 1 +app_info_print 730 +quit";
        auto output = exec( ss.str( ).c_str( ) );

        std::smatch match;
        const bool found_match = std::regex_search( output, match, regexp );
        if ( !found_match ) {
            console::print_failure( true, "Update match failed: %s", output.c_str( ) );
            return;
        }

        remote_build = match[ 1 ].str( );

        if ( local_build != remote_build ) {
            console::print_failure( true, "Local build mismatch l: %s r: %s", local_build.c_str( ), remote_build.c_str( ) );
            updated = update( );
            if ( updated ) {
                local_build = remote_build;
                system( "cls" );
                console::print_success( true, "Update was a success" );
            }
        } else {
            updated = true;
            console::set_title_message( "Checked for update, latest build: %s", local_build.c_str( ) );
        }

    } catch ( std::exception & e ) {
        console::print_failure( true, "Exception: %s", e.what( ) );
    }
}

int main( ) {
    std::filesystem::path f{ "buildid.txt" };
    if ( exists( f ) ) {
        std::ifstream filein( f );
        filein >> local_build;
        filein.close( );
    }
    if ( local_build.length( ) )
        console::print( normal, "Local build is: " + local_build, true );
    while ( true ) {
        update_check( );
        if ( !updated ) {
            updated = update( );
            if ( !updated ) {
                std::this_thread::sleep_for( std::chrono::minutes( 1 ) );
                continue;
            }
            local_build = remote_build;
            continue;
        }
        std::this_thread::sleep_for( std::chrono::minutes( 3 ) );
    }
    return 0;
}

void dump_to_github( ) {
    auto output = exec( "blaze\\blazedumper.exe" );
    if ( output.find( "Success!" ) != std::string::npos ) {
        exec( "script.sh" );
    } else {
        console::print_failure( true, "Dumping failed! %s", output.c_str( ) );
    }
}

