#include <petsc.h>
#include <PetscTestFixture.hpp>
#include <finiteVolume/processes/eulerTransport.hpp>
#include <vector>
#include "domain/boxMesh.hpp"
#include "domain/modifiers/ghostBoundaryCells.hpp"
#include "eos/mockEOS.hpp"
#include "eos/perfectGas.hpp"
#include "finiteVolume/compressibleFlowFields.hpp"
#include "finiteVolume/fluxCalculator/ausm.hpp"
#include "finiteVolume/processes/gravity.hpp"
#include "gtest/gtest.h"
#include "mathFunctions/simpleFormula.hpp"
#include "parameters/mapParameters.hpp"

struct GravityTestParameters {
    std::vector<double> gravity;
    std::shared_ptr<ablate::mathFunctions::FieldFunction> initialEuler;
    std::shared_ptr<ablate::mathFunctions::MathFunction> expectedSourceFunction;
};

class GravityTestFixture : public testingResources::PetscTestFixture, public ::testing::WithParamInterface<GravityTestParameters> {};

TEST_P(GravityTestFixture, ShouldComputeCorrectFlux) {
    // arrange
    const auto& params = GetParam();

    // create a mock eos
    auto mockEOS = std::make_shared<ablateTesting::eos::MockEOS>();
    std::vector<std::string> species;
    EXPECT_CALL(*mockEOS, GetSpecies).Times(::testing::Exactly(1)).WillOnce(::testing::ReturnRef(species));

    // Create a box mesh
    auto domain = std::make_shared<ablate::domain::BoxMesh>("testMesh",
                                                            std::vector<std::shared_ptr<ablate::domain::FieldDescriptor>>{std::make_shared<ablate::finiteVolume::CompressibleFlowFields>(mockEOS)},
                                                            std::vector<std::shared_ptr<ablate::domain::modifiers::Modifier>>{std::make_shared<ablate::domain::modifiers::GhostBoundaryCells>()},
                                                            std::vector<int>{5, 7, 10},
                                                            std::vector<double>{.0, .0, .0},
                                                            std::vector<double>{1, 1, 1});

    // Make a finite volume with only a gravity
    auto fvObject = std::make_shared<ablate::finiteVolume::FiniteVolumeSolver>(
        "testFV",
        ablate::domain::Region::ENTIREDOMAIN,
        nullptr /*options*/,
        std::vector<std::shared_ptr<ablate::finiteVolume::processes::Process>>{std::make_shared<ablate::finiteVolume::processes::Gravity>(GetParam().gravity)},
        std::vector<std::shared_ptr<ablate::finiteVolume::boundaryConditions::BoundaryCondition>>{});

    // init
    domain->InitializeSubDomains({fvObject}, std::vector<std::shared_ptr<ablate::mathFunctions::FieldFunction>>{GetParam().initialEuler});
    fvObject->PreStep(nullptr);

    // get the solution vector local
    Vec locX, locF;
    DMGetLocalVector(domain->GetDM(), &locX) >> errorChecker;
    DMGlobalToLocal(domain->GetDM(), domain->GetSolutionVector(), INSERT_VALUES, locX) >> errorChecker;
    DMGetLocalVector(domain->GetDM(), &locF) >> errorChecker;
    VecSet(locF, 0.0);

    // act
    fvObject->ComputeRHSFunction(0.0, locX, locF) >> errorChecker;

    // assert
    // March over each cell
    PetscInt cStart, cEnd;
    const PetscInt* cells = nullptr;
    DMPlexGetSimplexOrBoxCells(fvObject->GetSubDomain().GetDM(), 0, &cStart, &cEnd);
    // get access to the array
    const PetscReal* locFArray;
    VecGetArrayRead(locF, &locFArray) >> errorChecker;

    for (PetscInt c = cStart; c < cEnd; ++c) {
        // if there is a cell array, use it, otherwise it is just c
        PetscBool boundary;
        DMIsBoundaryPoint(fvObject->GetSubDomain().GetDM(), c, &boundary);
        if (boundary) {
            continue;
        }

        // Get the raw data at this point, this check assumes the order the fields
        const PetscScalar* data;
        DMPlexPointLocalRead(fvObject->GetSubDomain().GetDM(), c, locFArray, &data) >> errorChecker;

        // compute the cell centroid
        PetscReal centroid[3];
        DMPlexComputeCellGeometryFVM(fvObject->GetSubDomain().GetDM(), c, nullptr, centroid, nullptr) >> errorChecker;

        // evaluate the expectedSource
        std::vector<double> expectedSource(5);
        GetParam().expectedSourceFunction->Eval(centroid, 3, 0.0, expectedSource);

        for (PetscInt d = 0; d < 2 /*expectedSource.size()*/; d++) {
            ASSERT_NEAR(data[d], expectedSource[d], 1E-8) << "Expected gravity source is incorrect for component " << d << " in cell " << c;
        }
    }
    VecRestoreArrayRead(locF, &locFArray) >> errorChecker;

    // cleanup
    DMRestoreLocalVector(domain->GetDM(), &locX) >> errorChecker;
    DMRestoreLocalVector(domain->GetDM(), &locF) >> errorChecker;
}

INSTANTIATE_TEST_SUITE_P(GravityTests, GravityTestFixture,
                         testing::Values((GravityTestParameters){
                             .gravity = {0.0, 0.0, -9.8},
                             .initialEuler = std::make_shared<ablate::mathFunctions::FieldFunction>(
                                 "euler", std::make_shared<ablate::mathFunctions::SimpleFormula>("1.1 + z, 0.0, (1.1 + z)*10*x, (1.1 + z)*20*y, (1.1 + z)*30*z")),
                             .expectedSourceFunction = std::make_shared<ablate::mathFunctions::SimpleFormula>("0.0,(30*z)*max(((1.1 + z)-1.6)*-9.8, 0.0), 0.0, 0.0, max(((1.1 + z)-1.6)*-9.8, 0.0)")}));
