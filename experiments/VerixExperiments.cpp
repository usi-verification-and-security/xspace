//
// Created by labbaf on 29.05.2024.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include "VerixExperiments.h"
#include "algorithms/BasicVerix.h"
#include "verifiers/opensmt/OpenSMTVerifier.h"
#include "verifiers/marabou/MarabouVerifier.h"

void
VerixExperiments::experiment_on_dataset(std::string modelPath, std::string datasetPath, std::string verifier,
                                        std::string outputPath, int featureSize) {
    xai::algo::BasicVerix algo(modelPath);
    if (verifier == "OpenSMT")
        algo.setVerifier(std::make_unique<xai::verifiers::OpenSMTVerifier>());
    else if (verifier == "Marabou")
        algo.setVerifier(std::make_unique<xai::verifiers::MarabouVerifier>());

    // Open the data CSV file
    std::ifstream file(datasetPath);
    if (!file.is_open()) {
        std::cout << "Could not open file " << datasetPath << std::endl;
        return;
    }

    //open the output CSV file
    std::ofstream outputFile(outputPath);
    if (!outputFile.is_open()) {
        std::cout << "Could not open file " << outputPath << std::endl;
        return;
    }

    std::vector<std::vector<float>> data;
    std::string line;
    std::string header;
    // Read the first line to skip the header

    getline(file, header);
    outputFile << "datapoint" << "," << header;
    // Iterate over each line in the file
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        std::vector<float> row;
        std::vector<float> datapoint;
        std::cout << "row: ";
        std::cout << line;
        while (std::getline(ss, field, ',')) {
            // Convert the field to float and add it to the row vector
            row.push_back(std::stof(field));
        }
        float output = row[featureSize];
        datapoint = std::vector<float>(row.begin(), row.begin() + featureSize);
        data.push_back(datapoint);

        auto res = algo.computeExplanation(datapoint, 0.5);
        std::cout <<"explanation: ";
        std::vector<int> explanation(featureSize, 0);
        for (auto val : res.explanation) {
            std::cout << val << " ";
            if (val >= 0 && val < featureSize) {
                explanation[val] = 1;
            }
        }
        std::cout << std::endl;

        // add results to output file
        for (int i = 0; i < datapoint.size(); ++i) {
            outputFile << datapoint[i] << " ";
        }
        outputFile << ",";
        for (int i = 0; i < explanation.size(); ++i) {
            outputFile << explanation[i];
            outputFile << ",";
        }
        outputFile << output << "," << "\n";
    }

    // Close the file
    file.close();
}
