add_library(frontend OBJECT
    CLIOptions.cc
    CLIOptions.h
    CandyBuilder.h
    SolverFactory.h
    SolverFactory.cc
    RSILSolverBuilder.h
    RSILSolverBuilder.cc
    Exceptions.h
    Exceptions.cc
    RandomSimulationFrontend.h
    RandomSimulationFrontend.cc
    GateAnalyzerFrontend.h
    GateAnalyzerFrontend.cc
    RSARFrontend.h
    RSARFrontend.cc
    CandyCommandLineParser.h
    CandyCommandLineParser.cc)

