#include "pressureGradientScaling.hpp"
#include <petscviewerhdf5.h>
#include <utility>
#include "finiteVolume/compressibleFlowFields.hpp"
#include "finiteVolume/processes/flowProcess.hpp"
#include "utilities/mpiError.hpp"

ablate::finiteVolume::processes::PressureGradientScaling::PressureGradientScaling(std::shared_ptr<eos::EOS> eos, double alphaInit, double domainLength, double maxAlphaAllowedIn,
                                                                                  double maxDeltaPressureFacIn, std::shared_ptr<ablate::monitors::logs::Log> log)
    : eos(std::move(eos)),
      maxAlphaAllowed(maxAlphaAllowedIn > 1.0 ? maxAlphaAllowedIn : 100.0),
      maxDeltaPressureFac(maxDeltaPressureFacIn > 0.0 ? maxDeltaPressureFacIn : 0.05),
      domainLength(domainLength),
      log(std::move(log)),
      alpha(alphaInit) {}

PetscErrorCode ablate::finiteVolume::processes::PressureGradientScaling::UpdatePreconditioner(TS flowTs, ablate::solver::Solver &flow) {
    PetscFunctionBeginUser;

    // Compute global maximum valuesfluxCalculator
    PetscReal pAvgLocal = 0.e+0;
    PetscReal pMinLocal = PETSC_MAX_REAL;
    PetscReal pMaxLocal = 0.e+0;
    PetscReal cMaxLocal = 0.e+0;
    PetscReal machMaxLocal = 0.e+0;
    PetscReal alphaMaxLocal = PETSC_MAX_REAL;

    PetscInt countLocal = 0;

    // get access to the underlying data for the flow
    PetscInt flowEulerId = flow.GetSubDomain().GetField(finiteVolume::CompressibleFlowFields::EULER_FIELD).id;
    PetscInt flowDensityYiId = -1;
    if (flow.GetSubDomain().ContainsField(finiteVolume::CompressibleFlowFields::DENSITY_YI_FIELD)) {
        flowDensityYiId = flow.GetSubDomain().GetField(finiteVolume::CompressibleFlowFields::DENSITY_YI_FIELD).id;
    }
    PetscInt dim = flow.GetSubDomain().GetDimensions();

    // get the flowSolution from the ts
    Vec globFlowVec = flow.GetSubDomain().GetSolutionVector();
    const PetscScalar *flowArray;
    PetscErrorCode ierr = VecGetArrayRead(globFlowVec, &flowArray);
    CHKERRQ(ierr);

    // Get the valid cell range over this region
    IS cellIS;
    PetscInt cStart, cEnd;
    const PetscInt *cells;
    flow.GetCellRange(cellIS, cStart, cEnd, cells);

    // get decode state function/context
    auto decodeStateFunction = eos->GetDecodeStateFunction();
    auto decodeStateContext = eos->GetDecodeStateContext();

    // check for ghost nodes
    // check to see if there is a ghost label
    auto dm = flow.GetSubDomain().GetDM();
    DMLabel ghostLabel;
    DMGetLabel(dm, "ghost", &ghostLabel) >> checkError;

    // March over each cell
    for (PetscInt c = cStart; c < cEnd; ++c) {
        // if there is a cell array, use it, otherwise it is just c
        const PetscInt cell = cells ? cells[c] : c;

        PetscBool boundary;
        PetscInt ghost = -1;
        if (ghostLabel) {
            DMLabelGetValue(ghostLabel, cell, &ghost);
        }
        DMIsBoundaryPoint(dm, cell, &boundary);
        PetscInt numChildren;
        DMPlexGetTreeChildren(dm, cell, &numChildren, nullptr);
        if (ghost >= 0 || boundary || numChildren) {
            continue;
        }

        // Get the current state variables for this cell
        const PetscScalar *euler = nullptr;
        ierr = DMPlexPointGlobalFieldRead(flow.GetSubDomain().GetDM(), cell, flowEulerId, flowArray, &euler);
        CHKERRQ(ierr);

        // If valid cell
        if (euler) {
            const PetscScalar *densityYi = nullptr;
            if (flowDensityYiId >= 0) {
                ierr = DMPlexPointGlobalFieldRead(flow.GetSubDomain().GetDM(), cell, flowDensityYiId, flowArray, &densityYi);
                CHKERRQ(ierr);
            }

            // Extract values
            PetscReal density;
            PetscReal velocity[3];
            PetscReal internalEnergy;
            PetscReal a;
            PetscReal mach;
            PetscReal p;

            // Decode the state to compute
            finiteVolume::processes::FlowProcess::DecodeEulerState(decodeStateFunction, decodeStateContext, dim, euler, densityYi, &density, velocity, &internalEnergy, &a, &mach, &p);

            // Store the max/min values
            countLocal++;
            pAvgLocal += p;
            pMinLocal = PetscMin(pMinLocal, p);
            pMaxLocal = PetscMax(pMaxLocal, p);
            cMaxLocal = PetscMax(cMaxLocal, a);
            machMaxLocal = PetscMax(machMaxLocal, mach);
            alphaMaxLocal = PetscMin(alphaMaxLocal, maxMachAllowed / mach);
        }
    }

    // return the cell range
    flow.RestoreRange(cellIS, cStart, cEnd, cells);

    // Take the global values
    auto comm = flow.GetSubDomain().GetComm();
    PetscReal sumValues[2] = {pAvgLocal, (PetscReal)countLocal};
    ierr = MPIU_Allreduce(MPI_IN_PLACE, sumValues, 2, MPIU_REAL, MPIU_SUM, comm);
    CHKERRMPI(ierr);
    PetscReal maxValues[3] = {pMaxLocal, cMaxLocal, machMaxLocal};
    ierr = MPIU_Allreduce(MPI_IN_PLACE, maxValues, 3, MPIU_REAL, MPIU_MAX, comm);
    CHKERRMPI(ierr);
    PetscReal minValues[2] = {pMinLocal, alphaMaxLocal};
    ierr = MPIU_Allreduce(MPI_IN_PLACE, minValues, 2, MPIU_REAL, MPIU_MIN, comm);
    CHKERRMPI(ierr);

    PetscReal alphaMax = minValues[1];
    PetscReal cMax = maxValues[1];
    PetscReal pRef = sumValues[0] / (sumValues[1]);
    PetscReal maxDeltaP = PetscMax(PetscAbsReal(maxValues[0] - pRef), PetscAbsReal(minValues[0] - pRef));
    maxMach = maxValues[2];

    // Get the current timeStep from TS
    PetscReal dt;
    ierr = TSGetTimeStep(flowTs, &dt);
    CHKERRQ(ierr);

    // Update alpha
    PetscReal alphaOld = alpha;
    PetscReal term1 = 0.5e+0 * dt * cMax / domainLength;
    alpha = (alpha + term1 * (PetscSqrtReal(maxDeltaPressureFac * pRef / (maxDeltaP + 1E-30)) - 1.0));
    alpha = PetscMin(alpha, (1. + maxAlphaChange) * alphaOld);  // avoid alpha jumping up to quickly if maxdelPfac=0
    alpha = PetscMin(alpha, alphaMax);
    alpha = PetscMin(alpha, maxAlphaAllowed);
    alpha = PetscMax(alpha, 1.e+0);

    // Update log
    if (log) {
        log->Printf("PGS: %g (alpha), %g (maxMach),  %g (maxMach'), %g (maxDeltaP)\n", alpha, maxMach, alpha * maxMach, maxDeltaP);
    }
    PetscFunctionReturn(0);
}

void ablate::finiteVolume::processes::PressureGradientScaling::Initialize(ablate::finiteVolume::FiniteVolumeSolver &fv) {
    auto preStep = std::bind(&ablate::finiteVolume::processes::PressureGradientScaling::UpdatePreconditioner, this, std::placeholders::_1, std::placeholders::_2);
    fv.RegisterPreStep(preStep);

    // initialize the log if provided
    if (log) {
        log->Initialize(fv.GetSubDomain().GetComm());
    }
}

void ablate::finiteVolume::processes::PressureGradientScaling::Save(PetscViewer viewer, PetscInt sequenceNumber, PetscReal time) {
    // Use time stepping.
    Vec pgsAlphaVec;
    VecCreateMPI(PetscObjectComm((PetscObject)viewer), PETSC_DECIDE, 1, &pgsAlphaVec) >> checkError;
    PetscObjectSetName((PetscObject)pgsAlphaVec, "pressureGradientScalingAlpha") >> checkError;
    VecSetValue(pgsAlphaVec, 0, alpha, INSERT_VALUES) >> checkError;
    VecAssemblyBegin(pgsAlphaVec) >> checkError;
    VecAssemblyEnd(pgsAlphaVec) >> checkError;
    VecView(pgsAlphaVec, viewer);
    VecDestroy(&pgsAlphaVec) >> checkError;
}

void ablate::finiteVolume::processes::PressureGradientScaling::Restore(PetscViewer viewer, PetscInt sequenceNumber, PetscReal time) {
    int rank;
    MPI_Comm_rank(PetscObjectComm((PetscObject)viewer), &rank) >> checkMpiError;

    // load in the old alpha
    Vec pgsAlphaVec;
    VecCreateMPI(PetscObjectComm((PetscObject)viewer), rank == 0 ? 1 : 0, 1, &pgsAlphaVec) >> checkError;
    PetscObjectSetName((PetscObject)pgsAlphaVec, "pressureGradientScalingAlpha") >> checkError;
    VecLoad(pgsAlphaVec, viewer) >> checkError;

    // Load in alpha
    if (rank == 0) {
        PetscScalar alphaScalar;
        PetscInt index[1] = {0};
        VecGetValues(pgsAlphaVec, 1, index, &alphaScalar) >> checkError;
        alpha = (PetscReal)alphaScalar;
    }

    // Broadcast everywhere
    MPI_Bcast(&alpha, 1, MPIU_REAL, 0, PetscObjectComm((PetscObject)viewer)) >> checkMpiError;
    VecDestroy(&pgsAlphaVec) >> checkError;
}

#include "registrar.hpp"
REGISTER_DEFAULT(ablate::finiteVolume::processes::PressureGradientScaling, ablate::finiteVolume::processes::PressureGradientScaling,
                 "Rescales the thermodynamic pressure gradient scaling the acoustic propagation speeds to allow for a larger time step.",
                 ARG(ablate::eos::EOS, "eos", "the equation of state used for the flow"), ARG(double, "alphaInit", "the initial alpha"),
                 ARG(double, "domainLength", "the reference length of the domain"), OPT(double, "maxAlphaAllowed", "the maximum allowed alpha during the simulation (default 100)"),
                 OPT(double, "maxDeltaPressureFac", "max variation from mean pressure (default 0.05)"), OPT(ablate::monitors::logs::Log, "log", "where to record log (default is stdout)"));

REGISTER(ablate::finiteVolume::processes::Process, ablate::finiteVolume::processes::PressureGradientScaling,
         "Rescales the thermodynamic pressure gradient scaling the acoustic propagation speeds to allow for a larger time step.",
         ARG(ablate::eos::EOS, "eos", "the equation of state used for the flow"), ARG(double, "alphaInit", "the initial alpha"), ARG(double, "domainLength", "the reference length of the domain"),
         OPT(double, "maxAlphaAllowed", "the maximum allowed alpha during the simulation (default 100)"), OPT(double, "maxDeltaPressureFac", "max variation from mean pressure (default 0.05)"),
         OPT(ablate::monitors::logs::Log, "log", "where to record log (default is stdout)"));