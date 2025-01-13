#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include "../ai_omniscope-v2-communication_sw/src/OmniscopeSampler.hpp"
#include "CLI/CLI.hpp"
#include <tuple>
#include <functional>

// Declaration

inline OmniscopeDeviceManager deviceManager{};
inline std::vector<std::shared_ptr<OmniscopeDevice>> devices;
inline std::optional<OmniscopeSampler> sampler{};
inline std::map<Omniscope::Id, std::vector<std::pair<double, double>>> captureData;
std::atomic<bool> running{true};

void waitForExit();
void initDevices();
void writeDatatoFile(std::map<Omniscope::Id, std::vector<std::pair<double, double>>>&, std::string &, std::vector<std::string> &);
void printDevices(std::vector<std::shared_ptr<OmniscopeDevice>> &);
void searchDevices();
void startMeasurementAndWrite(std::vector<std::string> &, std::string &);
void selectDevices();
void printOrWrite(std::string &, std::vector<std::string> &); 
std::tuple<uint8_t, uint8_t, uint8_t> uuidToColor(const std::string& );
std::string rgbToAnsi(const std::tuple<uint8_t, uint8_t, uint8_t>& );

// Initialization

void waitForExit() { // wait until the user closes the programm by pressing ENTER
    std::cout << "OmnAIView is starting. Press enter to stop the programm." << std::endl;
    std::cin.sync();
    std::cin.get();
    running = false;
}

void initDevices() { // Initalize the connected devices
    constexpr int VID = 0x2e8au;
    constexpr int PID = 0x000au;

    devices = deviceManager.getDevices(VID, PID);
    std::cout << "Found " << devices.size() << " devices.\n";
    for(const auto& device : devices){
        auto [r, g, b] = uuidToColor(device->getId()->serial);
        device->send(Omniscope::SetRgb{static_cast<std::uint8_t>(static_cast<int>(r)),
                                   static_cast<std::uint8_t>(static_cast<int>(g)),
                                   static_cast<std::uint8_t>(static_cast<int>(b))});
    }
}

void writeDatatoFile(std::map<Omniscope::Id, std::vector<std::pair<double, double>>> &captureData , std::string &filePath, std::vector<std::string> &UUID) {
     if (captureData.empty()) {
        std::cerr << "No data available to write.\n";
        return;
    }

    // Datei öffnen
    if(filePath.empty()){
        std::string filePath = "data.txt"; 
    }
    std::ofstream outFile(filePath, std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file: " << filePath << "\n";
        return;
    }

    // Überprüfen, ob die Datei leer ist, um die Kopfzeile nur einmal zu schreiben
    outFile.seekp(0, std::ios::end);
    if (outFile.tellp() == 0) { // Datei ist leer
        // Kopfzeile schreiben
        outFile << "Timestamp";
        for (const auto& id : UUID) {
         outFile << id <<" , " ; 
        }
        outFile << "\n";
    }

    // Iteriere durch die erste ID, um die Reihenfolge der x-Werte zu bestimmen
    auto firstDevice = captureData.begin();
    const auto& firstDeviceData = firstDevice->second;

    // Anzahl der Zeilen basierend auf der Größe des ersten Geräts
    size_t numRows = firstDeviceData.size();

    for (size_t i = 0; i < numRows; ++i) {
        // Schreibe x-Wert und y-Wert der ersten ID
        double xValue = firstDeviceData[i].first;
        double yValueFirst = firstDeviceData[i].second;

        outFile << fmt::format("{},{}", xValue, yValueFirst);

        // Schreibe die y-Werte der restlichen IDs für denselben Index
        for (auto it = std::next(captureData.begin()); it != captureData.end(); ++it) {
            const auto& deviceData = it->second;

            if (i < deviceData.size()) {
                double yValue = deviceData[i].second;
                outFile << fmt::format(",{}", yValue);
            } else {
                // Falls keine weiteren Werte für dieses Gerät vorhanden sind
                outFile << ",N/A";
            }
        }
        outFile << "\n"; // Zeilenumbruch nach allen IDs
    }

    outFile.close(); // Datei schließen
    fmt::print("Data successfully written to {}\n", filePath);
}

void printDevices() {

    // get IDs
    if(devices.empty()) {
        std::cout << "No devices are connected. Please connect a device and start again" << std::endl;
    }
    else {
        std::cout << "The following devices are connected:" << std::endl;
        for(const auto& device : devices) {
            std::string deviceId = device -> getId()->serial;
            fmt::print("{}Device: {}\033[0m\n", rgbToAnsi(uuidToColor(deviceId)), deviceId);
        }
        std::cout << "With -p, --play and the UUID you can start a measurement. With -f, --file you can choose a path with the filename to save the data to. Press Enter to stop the measurement." << std::endl;
        devices.clear();
        deviceManager.clearDevices();
        running = false;
    }
}

void searchDevices() {
    std::cout << "Geräte werden gesucht" << std::endl;
    if(!sampler.has_value()) {
        devices.clear();
        deviceManager.clearDevices();
        initDevices(); // check if devices are connected
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Pause between checks
    }
}

void selectDevices(std::vector<std::string> &UUID) {
    if(!devices.empty() && !sampler.has_value()) { // move the device in the sampler
        std::cout <<"Devices where found and are emplaced"<< std::endl;
        for(const auto &device : devices) {
            devices.erase(
                std::remove_if(
                    devices.begin(),
                    devices.end(),
            [&UUID](const std::shared_ptr<OmniscopeDevice>& device) {
                    return std::find(UUID.begin(), UUID.end(), device->getId()->serial) == UUID.end();
            }),
            devices.end()
            );
        }
        if(!devices.empty()) {
            sampler.emplace(deviceManager, std::move(devices));
            if(sampler.has_value()) {
                for (auto &device : sampler->sampleDevices) {
                    device.first->send(Omniscope::Start{});
                }
            }
        }
    }
}

void printOrWrite(std::string &filePath, std::vector<std::string> &UUID) {
    std::cout << "hello" <<std::endl; 
    if(sampler.has_value()) { // write Data into file
        captureData.clear();
        sampler->copyOut(captureData);
        if(filePath.empty()){
        for(const auto& [id, vec] : captureData) {
                std::cout << "filepath empty" << std::endl; 
                fmt::print("dev: {}\n", id);
                for(const auto& [first, second] : vec) {
                    std::cout << "Time:" << first << "," << "Voltage" << second << std::endl;
                    if(!running) {
                        break;
                    }
                }
                if(!running) {
                    break;
                }
         }
        }
        else {
                writeDatatoFile(captureData, filePath, UUID);
            }
        }
    }

void startMeasurementAndWrite(std::vector<std::string> &UUID, std::string &filePath) {
    while(running) {
        searchDevices();   // Init Scopes

        selectDevices(UUID);  // select only chosen devices

        printOrWrite(filePath, UUID); // print the data in the console or save it in the given filepath 
    }
}

std::tuple<uint8_t, uint8_t, uint8_t> uuidToColor(const std::string& uuid) {
    // std::hash anwenden
    size_t hashValue = std::hash<std::string>{}(uuid);

    // Die ersten 3 Bytes extrahieren
    uint8_t r = (hashValue & 0xFF);         // Erste 8 Bits
    uint8_t g = (hashValue >> 8) & 0xFF;   // Zweite 8 Bits
    uint8_t b = (hashValue >> 16) & 0xFF;  // Dritte 8 Bits

    return {r, g, b};
}

std::string rgbToAnsi(const std::tuple<uint8_t, uint8_t, uint8_t>& rgb) {
    auto [r, g, b] = rgb; // Tupel entpacken
    return fmt::format("\033[38;2;{};{};{}m", r, g, b); // ANSI-Farbcode generieren
}


