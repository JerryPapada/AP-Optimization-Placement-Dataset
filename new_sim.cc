#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/log.h"
#include "ns3/netanim-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/nyu-propagation-loss-model.h"
#include "ns3/nyu-channel-condition-model.h"
#include "ns3/building.h"
#include "ns3/buildings-helper.h"
#include "ns3/okumura-hata-propagation-loss-model.h"
#include <sys/stat.h>

using namespace ns3;

static std::ofstream csvFile;
static Ptr<NYUPropagationLossModel> m_propagationLossModel;

NS_LOG_COMPONENT_DEFINE("IntegratedWifiRssiWithMovement");

// Helper function to check if file exists
bool FileExists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

// Helper function to generate a unique filename by appending a number if necessary
std::string GenerateUniqueFilename(const std::string& baseFilename) {
    std::string filename = baseFilename;
    int count = 1;
    while (FileExists(filename)) {
        std::stringstream ss;
        ss << baseFilename << "_" << count << ".csv";
        filename = ss.str();
        count++;
    }
    return filename;
}

std::vector<Vector> ParseVectorList(const std::string& input) {
    std::vector<Vector> positions;
    std::string cleanedInput = input;

    // Remove brackets and spaces
    cleanedInput.erase(std::remove(cleanedInput.begin(), cleanedInput.end(), '['), cleanedInput.end());
    cleanedInput.erase(std::remove(cleanedInput.begin(), cleanedInput.end(), ']'), cleanedInput.end());
    cleanedInput.erase(std::remove(cleanedInput.begin(), cleanedInput.end(), ' '), cleanedInput.end());

    // Split the string by commas into x and y pairs
    std::stringstream ss(cleanedInput);
    std::string token;
    while (std::getline(ss, token, ',')) {
        double x = std::stod(token);
        std::getline(ss, token, ',');
        double y = std::stod(token);

        positions.push_back(Vector(x, y, 1.5)); // Default z-coordinate to 1.5
    }

    return positions;
}

// Structure to hold scenario data
struct ScenarioData {
    std::string layout_name;
    int n_rooms_x, n_rooms_y;
    std::string ap_placement;
    std::vector<Vector> ap_positions;
    std::vector<Vector> mmwave_positions;
    int reduced_ap_count;
    double x_min, x_max, y_min, y_max, z_min, z_max;
};

std::vector<ScenarioData> ParseCsv(const std::string& filename) {
    std::vector<ScenarioData> scenarios;
    std::ifstream file(filename);
    std::string line;

    NS_LOG_UNCOND("Starting to parse CSV file: " << filename);

    // Skip the header line (if there's any, otherwise remove this line)
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        ScenarioData scenario;

        // Parse layout_name, n_rooms_x, and n_rooms_y
        std::getline(ss, scenario.layout_name, ';');
        std::getline(ss, token, ';');
        scenario.n_rooms_x = std::stoi(token);
        std::getline(ss, token, ';');
        scenario.n_rooms_y = std::stoi(token);

        // Parse ap_placement
        std::getline(ss, scenario.ap_placement, ';');

        // Parse AP positions
        std::getline(ss, token, ';');
        scenario.ap_positions = ParseVectorList(token);

        // Parse mmWave positions
        std::getline(ss, token, ';');
        scenario.mmwave_positions = ParseVectorList(token);

        // Parse reduced_ap_count and boundaries
        std::getline(ss, token, ';');
        scenario.reduced_ap_count = std::stoi(token);
        std::getline(ss, token, ';');
        scenario.x_min = std::stod(token);
        std::getline(ss, token, ';');
        scenario.x_max = std::stod(token);
        std::getline(ss, token, ';');
        scenario.y_min = std::stod(token);
        std::getline(ss, token, ';');
        scenario.y_max = std::stod(token);
        std::getline(ss, token, ';');
        scenario.z_min = std::stod(token);
        std::getline(ss, token, ';');
        scenario.z_max = std::stod(token);

        scenarios.push_back(scenario);
    }

    NS_LOG_UNCOND("Finished parsing CSV file.");
    return scenarios;
}


// Function to move the UE to a random position within the specified bounds
void MoveToRandomPosition(Ptr<ConstantPositionMobilityModel> mobility, double xMin, double xMax, double yMin, double yMax) {
    if (mobility == nullptr) {
        NS_LOG_ERROR("mobility pointer is null in MoveToRandomPosition!");
        return;
    }

    double x = xMin + ((double)rand() / RAND_MAX) * (xMax - xMin);
    double y = yMin + ((double)rand() / RAND_MAX) * (yMax - yMin);

    // Ensure the position stays within bounds
    x = std::min(std::max(x, xMin), xMax);
    y = std::min(std::max(y, yMin), yMax);

    mobility->SetPosition(Vector(x, y, 1.5)); // Set z to 1.5 meters for UE
    //NS_LOG_UNCOND("Moved UE to position: (" << x << ", " << y << ")");
}

// Function to schedule periodic UE movements
void ScheduleRandomMovement(Ptr<ConstantPositionMobilityModel> ueMobility, double xMin, double xMax, double yMin, double yMax) {
    if (ueMobility == nullptr) {
        NS_LOG_ERROR("ueMobility pointer is null in ScheduleRandomMovement!");
        return;
    }

    MoveToRandomPosition(ueMobility, xMin, xMax, yMin, yMax);
    Simulator::Schedule(Seconds(0.5), &ScheduleRandomMovement, ueMobility, xMin, xMax, yMin, yMax); // Trigger every second
}

// Function to log RSSI values
void LogRssi(Ptr<Node> ueNode, Ptr<Node> apNode, Ptr<PropagationLossModel> lossModel) {
    if (!ueNode || !apNode || !lossModel) {
        NS_LOG_ERROR("Null pointer detected in LogRssi function!");
        return;
    }

    Ptr<MobilityModel> ueMobility = ueNode->GetObject<MobilityModel>();
    Ptr<MobilityModel> apMobility = apNode->GetObject<MobilityModel>();

    if (!ueMobility || !apMobility) {
        NS_LOG_ERROR("MobilityModel is null for UE or AP!");
        return;
    }

    std::string apType;
    double txPower;

    // Determine AP type and set TxPower
    if (DynamicCast<OkumuraHataPropagationLossModel>(lossModel)) {
        apType = "WiFi";
        txPower = 20.0; // WiFi transmission power
    } else if (DynamicCast<NYUPropagationLossModel>(lossModel)) {
        apType = "mmWave";
        txPower = 30.0; // mmWave transmission power
    } else {
        NS_LOG_ERROR("Unknown AP type for node ID: " << apNode->GetId());
        return;
    }

    // Calculate RSSI
    double rssi = -lossModel->CalcRxPower(txPower, ueMobility, apMobility); // RSSI in dBm
    //NS_LOG_UNCOND(apType << " RSSI to UE: " << rssi << " dBm");
    Vector uePosition = ueMobility->GetPosition();
    Vector apPosition = apMobility->GetPosition();
    double distance = CalculateDistance(uePosition, apPosition);

    // Log to CSV file
    csvFile << Simulator::Now().GetSeconds() << ";";
    csvFile << apNode->GetId() << ";";
    csvFile << "(" << apPosition.x << "," << apPosition.y << "," << apPosition.z << ")" << ";";
    csvFile << ueNode->GetId() << ";";
    csvFile << "(" << uePosition.x << "," << uePosition.y << "," << uePosition.z << ")" << ";";
    csvFile << distance << ";";
    csvFile << rssi << ";";
    csvFile << apType << std::endl;
}

// Function to schedule the RSSI logging at regular intervals
// Function to schedule RSSI logging for multiple APs
void ScheduleRssiLogging(Ptr<Node> ueNode, NodeContainer apNodes, Ptr<PropagationLossModel> lossModel) {
    for (uint32_t i = 0; i < apNodes.GetN(); ++i) {
        LogRssi(ueNode, apNodes.Get(i), lossModel);
    }
    Simulator::Schedule(Seconds(1.0), &ScheduleRssiLogging, ueNode, apNodes, lossModel);
}

// Function to run a single simulation scenario
void RunSimulationForScenario(const ScenarioData& scenario) {
    NodeContainer ueNodes, wifiApNodes, mmWaveApNodes;

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Create nodes for Wi-Fi and mmWave APs based on reduced_ap_count
    ueNodes.Create(1);
    wifiApNodes.Create(scenario.reduced_ap_count);
    mmWaveApNodes.Create(scenario.reduced_ap_count);
    mobility.Install(wifiApNodes);
    mobility.Install(mmWaveApNodes);
    mobility.Install(ueNodes);

    // Set specific positions for the Wi-Fi and mmWave AP nodes
    for (size_t i = 0; i < scenario.ap_positions.size(); ++i) {
        Ptr<ConstantPositionMobilityModel> mobilityModel = wifiApNodes.Get(i)->GetObject<ConstantPositionMobilityModel>();
        mobilityModel->SetPosition(Vector(scenario.ap_positions[i].x,scenario.ap_positions[i].y,1.5));
    }

    for (size_t i = 0; i < scenario.mmwave_positions.size(); ++i) {
        Ptr<ConstantPositionMobilityModel> mobilityModel = mmWaveApNodes.Get(i)->GetObject<ConstantPositionMobilityModel>();
        mobilityModel->SetPosition(Vector(scenario.mmwave_positions[i].x,scenario.mmwave_positions[i].y,1.5));
    }

    // Set building boundaries based on scenario data
    Ptr<Building> building = CreateObject<Building>();
    building->SetBoundaries(Box(scenario.x_min, scenario.x_max, scenario.y_min, scenario.y_max, scenario.z_min, scenario.z_max));
    building->SetBuildingType(Building::Residential);
    building->SetExtWallsType(Building::ConcreteWithWindows);
    building->SetNFloors(1);
    building->SetNRoomsX(scenario.n_rooms_x);
    building->SetNRoomsY(scenario.n_rooms_y);
    
    BuildingsHelper::Install(ueNodes);
    BuildingsHelper::Install(wifiApNodes);
    BuildingsHelper::Install(mmWaveApNodes);

    // Create the propagation loss model for Wi-Fi
    Ptr<OkumuraHataPropagationLossModel> wifiLossModel = CreateObject<OkumuraHataPropagationLossModel>();

    // Create the propagation loss model for mmWave
    ObjectFactory propagationLossModelFactory;
    propagationLossModelFactory.SetTypeId(NYUUmaPropagationLossModel::GetTypeId());
    m_propagationLossModel = propagationLossModelFactory.Create<NYUPropagationLossModel>();

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(ueNodes);
    stack.Install(wifiApNodes);
    stack.Install(mmWaveApNodes);

    // Schedule UE movement and RSSI logging
    ScheduleRandomMovement(ueNodes.Get(0)->GetObject<ConstantPositionMobilityModel>(), 0.0, 20.0, 0.0, 20.0);
    ScheduleRssiLogging(ueNodes.Get(0), wifiApNodes, wifiLossModel);
    ScheduleRssiLogging(ueNodes.Get(0), mmWaveApNodes, m_propagationLossModel);
    // Run simulation
    Simulator::Stop(Seconds(400.0));  // End simulation after 10 seconds
    Simulator::Run();
    Simulator::Destroy();
}

int main(int argc, char* argv[]) {
    LogComponentEnable("IntegratedWifiRssiWithMovement", LOG_LEVEL_INFO);
    LogComponentEnable("Building", LOG_LEVEL_INFO);

    // Parse CSV to load scenarios
    std::vector<ScenarioData> scenarios = ParseCsv("scratch/IWP/transformed_simulations.csv");

    // Run simulation for each scenario
    for (size_t i = 0; i < scenarios.size(); ++i) {
        const ScenarioData& scenario = scenarios[i];

        // Generate a unique CSV filename for this scenario
        std::string outputFilename = "scratch/results/simulation_results_" + scenario.layout_name + ".csv";
        outputFilename = GenerateUniqueFilename(outputFilename);  // Ensure the filename is unique

        // Open CSV file for this scenario's results
        csvFile.open(outputFilename, std::ios::out);
        if (!csvFile.is_open()) {
            NS_LOG_ERROR("Failed to open CSV file: " << outputFilename);
            continue;
        }

        // Write CSV header
        csvFile << "Time(s),AP_ID,AP_Position,UE_ID,UE_Position,Distance,RSSI,AP_Type" << std::endl;

        // Log scenario details
        NS_LOG_UNCOND("Running simulation for scenario " << scenario.layout_name);

        // Run the simulation for the current scenario
        RunSimulationForScenario(scenario);

        // Close the CSV file for this scenario
        csvFile.close();

        // Clean up and prepare for the next scenario
        Simulator::Destroy();
        NS_LOG_UNCOND("Finished simulation for scenario " << scenario.layout_name);
    }

    return 0;
}



