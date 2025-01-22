#include "data.hpp"


int main(int argc, char **argv) {
    std::signal(SIGINT, customSignalHandler);

    running = true;

// Create Optional CLI Tool
    CLI::App app{"OmnAI CLI"};

    //Search for devices
    bool search = false;
    app.add_flag("-s,--search", search, "Prints all connected devices, color identical to current LED color");

    std::vector<std::string> startUUID;
    app.add_option("-d,--device, --dev", startUUID, "Start the devices with the given UUIDs");

    std::string filePath;
    app.add_option("-o,--output", filePath, "Add a file you want the data to be saved in");

    app.add_flag("-v,--verbose", verbose, "Add extra for debugging information");

    bool isJson = false;
    app.add_flag("-j,--json", isJson, "Add if you want the file to be in a JSON format");

    bool WS = false;
    app.add_flag("-w,--websocket", WS, "Starts the websocket. To send data a UUID has to be given");

    bool printVersion = false;
    app.add_flag("--version", printVersion, "Prints the current version. Version is set via a git tag.");

    if (argc <= 1) {// if no parameters are given
        std::cout << app.help() << std::endl;
        return 0;
    }
    try {
        CLI11_PARSE(app, argc, argv);
    } catch (const CLI::ParseError &e) {
        // if input is incorrect
        std::cerr << app.help() << std::endl;
        return app.exit(e);
    }

    if(printVersion) {
        std::cout << "Version " << PROJECT_VERSION << std::endl;
        running = false;
    }

    if(search) { // search for devices and print the UUID
        searchDevices();
        printDevices();
    }

    if(WS) {
        websocket = std::thread(WSTest);
        if(!startUUID.empty()) {
            startMeasurementAndWrite(startUUID, filePath, isJson, WS);
        }
    }

    while(running) {
        if(!startUUID.empty()) { // Start the measurment with a set UUID and FilePath, data will be written in the filepath or in the console
            startMeasurementAndWrite(startUUID, filePath, isJson, WS);
        }
    }

    if(!running) {
        crowApp.stop();
        if(verbose) {
            std::cout << "Websocket closed" << std::endl;
        }
        if(websocketActive) {
            while(!websocket.joinable()) {

            }
            if(websocket.joinable()) {
                websocket.join();
                if(verbose) {
                    std::cout << "Websocket thread was joined" << std::endl;
                }
            }
        }
        websocketActive = false;
        if(dataThreadActive) {
            while(!dequeThread.joinable()) {}
            if(dequeThread.joinable()) {
                dequeThread.join();
                dataThreadActive = false;
                if(verbose) {
                    std::cout << "Sending data thread was joined" << std::endl;
                }
            }
        }
        std::cout << "Programm was closed correctly, all threads closed" << std::endl;

    }
}
