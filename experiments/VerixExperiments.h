//
// Created by labbaf on 29.05.2024.
//
#include <iostream>
#include "algorithms/BasicVerix.h"

#ifndef XAI_SMT_VERIXEXPERIMENTS_H
#define XAI_SMT_VERIXEXPERIMENTS_H


class VerixExperiments {
public:
    static void experiment_on_dataset(std::string modelPath, std::string datasetPath, std::string verifier,
                                      std::string outputPath, int featureSize=13);

};


#endif //XAI_SMT_VERIXEXPERIMENTS_H
