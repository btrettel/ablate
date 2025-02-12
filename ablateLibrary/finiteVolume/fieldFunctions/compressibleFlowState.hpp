#ifndef ABLATELIBRARY_COMPRESSIBLEFLOWSTATE_HPP
#define ABLATELIBRARY_COMPRESSIBLEFLOWSTATE_HPP

#include <eos/eos.hpp>
#include <mathFunctions/fieldFunction.hpp>
#include <memory>
#include <vector>
namespace ablate::finiteVolume::fieldFunctions {

class CompressibleFlowState {
   private:
    const std::shared_ptr<ablate::eos::EOS> eos;
    const std::shared_ptr<mathFunctions::MathFunction> temperatureFunction;
    const std::shared_ptr<mathFunctions::MathFunction> pressureFunction;
    const std::shared_ptr<mathFunctions::MathFunction> velocityFunction;

    const std::shared_ptr<mathFunctions::FieldFunction> massFractionFunction;

   public:
    CompressibleFlowState(std::shared_ptr<ablate::eos::EOS> eos, std::shared_ptr<mathFunctions::MathFunction> temperatureFunction, std::shared_ptr<mathFunctions::MathFunction> pressureFunction,
                          std::shared_ptr<mathFunctions::MathFunction> velocityFunction, std::shared_ptr<mathFunctions::FieldFunction> massFractionFunction = {});

    static PetscErrorCode ComputeEulerFromState(PetscInt dim, PetscReal time, const PetscReal x[], PetscInt Nf, PetscScalar* u, void* ctx);
    static PetscErrorCode ComputeDensityYiFromState(PetscInt dim, PetscReal time, const PetscReal x[], PetscInt Nf, PetscScalar* u, void* ctx);

    /**
     * Helper function for other flowStates
     */
    PetscReal ComputeDensityFromState(PetscInt dim, PetscReal time, const PetscReal x[]);
};

}  // namespace ablate::finiteVolume::fieldFunctions
#endif  // ABLATELIBRARY_COMPRESSIBLEFLOWSTATE_HPP
