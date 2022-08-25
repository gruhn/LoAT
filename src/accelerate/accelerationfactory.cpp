#include "accelerationfactory.hpp"
#include "accelerationviacalculus.hpp"
#include "accelerationviaqe.hpp"

std::unique_ptr<AccelerationTechnique> AccelerationFactory::get(const LinearRule &rule, option<Recurrence::Result> closed, ITSProblem &its) {
    if (closed && rule.getGuard()->isPolynomial() && closed->update.isPoly()) {
        return std::unique_ptr<AccelerationTechnique>(new AccelerationViaQE(rule, *closed, its));
    } else {
        return std::unique_ptr<AccelerationTechnique>(new AccelerationViaCalculus(rule, closed, its));
    }
}
