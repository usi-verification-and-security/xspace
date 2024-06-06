//
// Created by labbaf on 06.06.2024.
//

#ifndef XAI_SMT_VERIXHIDDENLAYEREXPERIMENTS_H
#define XAI_SMT_VERIXHIDDENLAYEREXPERIMENTS_H

#include <string>
class VerixHiddenLayerExperiments {
public:
    static void experiment_on_dataset(std::string modelPath, std::string datasetPath, std::string verifier,
                                      std::string outputPath, int featureSize, float freedom_factor);
};


#endif //XAI_SMT_VERIXHIDDENLAYEREXPERIMENTS_H
