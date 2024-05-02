//
// Created by labbaf on 24.04.2024.
//

#include <vector>

#ifndef XAI_SMT_NNMODEL_H
#define XAI_SMT_NNMODEL_H


class NNModel {
public:
    std::vector<float> predict(std::vector<float> x) {
        /*
         * This is a dummy neural network model with 3 input nodes, 2 hidden nodes and 2 output nodes.
            h1 = 5x1 − x2 − x3
            h2 = −x1 + x2 + x3
            h′1 = ReLU (h1)
            h′2 = ReLU (h2)
            o1 = h′1 − h′2
            o2 = −0.5h′1 + h′2
         */
        float h1 = 5*x[0] - x[1] - x[2];
        float h2 = -x[0] + x[1] + x[2];
        float h1_ = h1 > 0 ? h1 : 0;
        float h2_ = h2 > 0 ? h2 : 0;
        float o1 = h1_ - h2_;
        float o2 = -0.5*h1_ + h2_;
        std::vector<float> result;
        result.push_back(o1);
        result.push_back(o2);
        return result;
    }
};


#endif //XAI_SMT_NNMODEL_H
