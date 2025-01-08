#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include "ai_omniscope-v2-communication_sw/src/OmniscopeSampler.hpp"

inline OmniscopeDeviceManager deviceManager{};
inline std::vector<std::shared_ptr<OmniscopeDevice>> devices;
// inline std::map<Omniscope::Id, std::array<float, 3>> colorMap;
inline std::optional<OmniscopeSampler> sampler{}; 
inline std::map<Omniscope::Id, std::vector<std::pair<double, double>>> captureData; 


void waitForExit(); 
void initDevices(); 

std::atomic<bool> running{true}; 
