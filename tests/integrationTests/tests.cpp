#include <filesystem>
#include "MpiTestFixture.hpp"
#include "gtest/gtest.h"
#include "runners/runners.hpp"

INSTANTIATE_TEST_SUITE_P(
    CompressibleFlow, IntegrationTestsSpecifier,
    testing::Values(
        (MpiTestParameter){
            .testName = "inputs/compressibleFlow/compressibleCouetteFlow.yaml", .nproc = 1, .expectedOutputFile = "outputs/compressibleFlow/compressibleCouetteFlow.txt", .arguments = ""},
        (MpiTestParameter){.testName = "inputs/compressibleFlow/compressibleFlowVortex.yaml",
                           .nproc = 1,
                           .expectedOutputFile = "outputs/compressibleFlow/compressibleFlowVortex.txt",
                           .arguments = "",
                           .expectedFiles{{"outputs/compressibleFlow/compressibleFlowVortex/domain.xmf", "domain.xmf"}}},
        (MpiTestParameter){
            .testName = "inputs/compressibleFlow/customCouetteCompressibleFlow.yaml", .nproc = 1, .expectedOutputFile = "outputs/compressibleFlow/customCouetteCompressibleFlow.txt", .arguments = ""},
        (MpiTestParameter){.testName = "inputs/compressibleFlow/extraVariableTransport.yaml", .nproc = 1, .expectedOutputFile = "outputs/compressibleFlow/extraVariableTransport.txt", .arguments = ""},
        (MpiTestParameter){.testName = "inputs/compressibleFlow/steadyCompressibleFlowLodiTest.yaml",
                           .nproc = 2,
                           .expectedOutputFile = "outputs/compressibleFlow/steadyCompressibleFlowLodiTest.txt",
                           .arguments = ""},
        (MpiTestParameter){
            .testName = "inputs/compressibleFlow/compressibleFlowVortexLodi.yaml", .nproc = 2, .expectedOutputFile = "outputs/compressibleFlow/compressibleFlowVortexLodi.txt", .arguments = ""}),

    [](const testing::TestParamInfo<MpiTestParameter>& info) { return info.param.getTestName(); });

INSTANTIATE_TEST_SUITE_P(FEFlow, IntegrationTestsSpecifier,
                         testing::Values(

                             (MpiTestParameter){.testName = "inputs/feFlow/incompressibleFlow.yaml", .nproc = 1, .expectedOutputFile = "outputs/feFlow/incompressibleFlow.txt", .arguments = ""}),
                         [](const testing::TestParamInfo<MpiTestParameter>& info) { return info.param.getTestName(); });

INSTANTIATE_TEST_SUITE_P(Particles, IntegrationTestsSpecifier,
                         testing::Values(

                             (MpiTestParameter){.testName = "inputs/particles/tracerParticles2DHDF5Monitor.yaml",
                                                .nproc = 2,
                                                .expectedOutputFile = "outputs/particles/tracerParticles2DHDF5Monitor.txt",
                                                .arguments = "",
                                                .expectedFiles{{"outputs/particles/tracerParticles2DHDF5Monitor/flowTracerParticles.xmf", "flowTracerParticles.xmf"},
                                                               {"outputs/particles/tracerParticles2DHDF5Monitor/domain.xmf", "domain.xmf"}}},
                             (MpiTestParameter){.testName = "inputs/particles/tracerParticles3D.yaml", .nproc = 1, .expectedOutputFile = "outputs/particles/tracerParticles3D.txt", .arguments = ""}),
                         [](const testing::TestParamInfo<MpiTestParameter>& info) { return info.param.getTestName(); });

INSTANTIATE_TEST_SUITE_P(
    VolumeOfFluids, IntegrationTestsSpecifier,
    testing::Values(

        (MpiTestParameter){
            .testName = "inputs/volumeOfFluids/twoGasAdvectingDiscontinuity.yaml", .nproc = 1, .expectedOutputFile = "outputs/volumeOfFluids/twoGasAdvectingDiscontinuity.txt", .arguments = ""}),
    [](const testing::TestParamInfo<MpiTestParameter>& info) { return info.param.getTestName(); });

INSTANTIATE_TEST_SUITE_P(ReactingFlow, IntegrationTestsSpecifier,
                         testing::Values(

                             (MpiTestParameter){
                                 .testName = "inputs/reactingFlow/simpleReactingFlow.yaml", .nproc = 1, .expectedOutputFile = "outputs/reactingFlow/simpleReactingFlow.txt", .arguments = ""},
                             (MpiTestParameter){.testName = "inputs/reactingFlow/ignitionDelayGriMech.yaml",
                                                .nproc = 1,
                                                .arguments = "",
                                                .expectedFiles{{"outputs/reactingFlow/ignitionDelayGriMech.PeakYi.txt", "ignitionDelayPeakYi.txt"}}},
                             (MpiTestParameter){.testName = "inputs/reactingFlow/ignitionDelay2S_CH4_CM2.yaml",
                                                .nproc = 1,
                                                .arguments = "",
                                                .expectedFiles{{"outputs/reactingFlow/ignitionDelay2S_CH4_CM2.Temperature.txt", "ignitionDelayTemperature.txt"}}}),
                         [](const testing::TestParamInfo<MpiTestParameter>& info) { return info.param.getTestName(); });

INSTANTIATE_TEST_SUITE_P(
    Machinery, IntegrationTestsSpecifier,
    testing::Values((MpiTestParameter){.testName = "inputs/machinery/dmViewFromOptions.yaml", .nproc = 1, .expectedOutputFile = "outputs/machinery/dmViewFromOptions.txt", .arguments = ""},
                    (MpiTestParameter){.testName = "inputs/machinery/subDomainFVM.yaml",
                                       .nproc = 1,
                                       .expectedOutputFile = "outputs/machinery/subDomainFVM/subDomainFVM.txt",
                                       .arguments = "",
                                       .expectedFiles{{"outputs/machinery/subDomainFVM/fluidField.xmf", "fluidField.xmf"}}}),
    [](const testing::TestParamInfo<MpiTestParameter>& info) { return info.param.getTestName(); });

INSTANTIATE_TEST_SUITE_P(
    ShockTube, IntegrationTestsSpecifier,
    testing::Values(
        (MpiTestParameter){.testName = "inputs/shocktube/shockTubeSODLodiBoundary.yaml", .nproc = 1, .expectedOutputFile = "outputs/shocktube/shockTubeSODLodiBoundary.txt", .arguments = ""},
        (MpiTestParameter){.testName = "inputs/shocktube/shockTube2Gas2D.yaml", .nproc = 1, .expectedOutputFile = "outputs/shocktube/shockTube2Gas2D.txt", .arguments = ""},
        (MpiTestParameter){.testName = "inputs/shocktube/shockTubeRieman.yaml", .nproc = 1, .expectedOutputFile = "outputs/shocktube/shockTubeRieman.txt", .arguments = ""},
        (MpiTestParameter){.testName = "inputs/shocktube/shockTube1DSod_AirWater.yaml", .nproc = 1, .expectedOutputFile = "outputs/shocktube/shockTube1DSod_AirWater.txt", .arguments = ""}),
    [](const testing::TestParamInfo<MpiTestParameter>& info) { return info.param.getTestName(); });

INSTANTIATE_TEST_SUITE_P(CompressibleFlowRestart, IntegrationRestartTestsSpecifier,
                         testing::Values((IntegrationRestartTestsParameters){.mpiTestParameter = {.testName = "inputs/compressibleFlow/compressibleFlowPgsLodi.yaml",
                                                                                                  .nproc = 2,
                                                                                                  .expectedOutputFile = "outputs/compressibleFlow/compressibleFlowPgsLodi.txt",
                                                                                                  .arguments = ""},
                                                                             .restartOverrides = {{"timestepper::arguments::ts_max_steps", "50"}}}),
                         [](const testing::TestParamInfo<IntegrationRestartTestsParameters>& info) {
                             return info.param.mpiTestParameter.getTestName() + "_" + std::to_string(info.param.mpiTestParameter.nproc);
                         });

INSTANTIATE_TEST_SUITE_P(
    RestartFEFlow, IntegrationRestartTestsSpecifier,
    testing::Values(
        (IntegrationRestartTestsParameters){
            .mpiTestParameter = {.testName = "inputs/feFlow/incompressibleFlowRestart.yaml", .nproc = 1, .expectedOutputFile = "outputs/feFlow/incompressibleFlowRestart.txt", .arguments = ""},
            .restartOverrides = {{"timestepper::arguments::ts_max_steps", "30"}}},
        (IntegrationRestartTestsParameters){
            .mpiTestParameter = {.testName = "inputs/feFlow/incompressibleFlowRestart.yaml", .nproc = 2, .expectedOutputFile = "outputs/feFlow/incompressibleFlowRestart.txt", .arguments = ""},
            .restartOverrides = {{"timestepper::arguments::ts_max_steps", "30"}}},
        (IntegrationRestartTestsParameters){.mpiTestParameter = {.testName = "inputs/feFlow/incompressibleFlowIntervalRestart.yaml",
                                                                 .nproc = 1,
                                                                 .expectedOutputFile = "outputs/feFlow/incompressibleFlowIntervalRestart.txt",
                                                                 .arguments = ""},
                                            .restartOverrides = {{"timestepper::arguments::ts_max_steps", "10"}}}),
    [](const testing::TestParamInfo<IntegrationRestartTestsParameters>& info) { return info.param.mpiTestParameter.getTestName() + "_" + std::to_string(info.param.mpiTestParameter.nproc); });

INSTANTIATE_TEST_SUITE_P(
    RestartParticles, IntegrationRestartTestsSpecifier,
    testing::Values((IntegrationRestartTestsParameters){
        .mpiTestParameter = {.testName = "inputs/particles/tracerParticles2DRestart.yaml", .nproc = 1, .expectedOutputFile = "outputs/particles/tracerParticles2DRestart.txt", .arguments = ""},
        .restartOverrides = {{"timestepper::arguments::ts_max_steps", "10"}}}),
    [](const testing::TestParamInfo<IntegrationRestartTestsParameters>& info) { return info.param.mpiTestParameter.getTestName() + "_" + std::to_string(info.param.mpiTestParameter.nproc); });