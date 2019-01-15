/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#ifndef X_E8029478_2FCD_4B07_9B47_58F51AA1C2FE_RANDOMSIMULATOR_H
#define X_E8029478_2FCD_4B07_9B47_58F51AA1C2FE_RANDOMSIMULATOR_H

#include <memory>
#include <chrono>

#include "Conjectures.h"
#include "SimulationVector.h"

namespace Candy {
    class GateAnalyzer;
}

namespace Candy {
    class ClauseOrder;
    class Partition;
    class Randomization;
    class Propagation;
    class GateFilter;
    
    /**
     * \defgroup RandomSimulation
     */
    
    /**
     * \class RandomSimulator
     *
     * \ingroup RandomSimulation
     *
     * \brief The main random simulator interface.
     *
     * This class is the "main" random simulator interface. A random simuator
     * detects possible equivalences and backbone literals in a given gate structure
     * by repeatedly applying random values to the gate structure's input variables
     * and then propagating the values through the gate structure, while observing
     * correspondences between literals rsp. literals and constants.
     *
     * Instances of this class are meant to be created using a RandomSimulatorBuilder
     * or a factory method.
     */
    class BitparallelRandomSimulator {
    public:
        BitparallelRandomSimulator(std::unique_ptr<ClauseOrder> clauseOrderStrat,
            std::unique_ptr<Partition> partitionStrat,
            std::unique_ptr<Randomization> randomizationStrat,
            std::unique_ptr<Propagation> propagationStrat,
            const GateAnalyzer &gateAnalyzer,
            float reductionRateAbortThreshold); 
        
        /**
         * Performs random simulation using a maximum amount of variable assignment rounds.
         * 
         * \param nSteps The nonzero maximum amount of variable assignment rounds. Note that depending
         *   on the random simulator's configuration, fewer than the maximum amount of
         *   rounds may actually be performed due to heuristic decisions. (This heuristic is
         *   disabled by default.)
         *   Implementations of RandomSimulator may (e.g. due to alignment requirements) actually
         *   perform more (up to a compile-time defined constant) rounds than given by nRounds.
         */
        Conjectures run(unsigned int nRounds);
        
        /**
         * Performs random simulation using a maximum amount of variable assignment rounds, aborting
         * random simulation if the given time limit has been exceeded.
         *
         * This method throws an OutOfTimeException if the time limit has been exceeded.
         *
         * \param nSteps The nonzero maximum amount of variable assignment rounds. Note that depending
         *   on the random simulator's configuration, fewer than the maximum amount of
         *   rounds may actually be performed due to heuristic decisions. (This heuristic is
         *   disabled by default.)
         *   Implementations of RandomSimulator may (e.g. due to alignment requirements) actually
         *   perform more (up to a compile-time defined constant) rounds than given by nRounds.
         *
         * \param timeLimit CPU time limit in seconds. If timeLimit == -1, no time limit is used.
         */
        Conjectures run(unsigned int nRounds, std::chrono::milliseconds timeLimit);
        
        /**
         * Performs random simulation until the random simulator heuristically determines
         * that further simulation is not worthwile. Since this heuristic is disabled by
         * default, you need to create the RandomSimulator object with a rsp. heuristic
         * configuration. Otherwise, it is an error to call this method.
         */
        Conjectures run();

        void ensureInitialized();
        
        ~BitparallelRandomSimulator();
        BitparallelRandomSimulator(const BitparallelRandomSimulator& other) = delete;
        BitparallelRandomSimulator& operator=(const BitparallelRandomSimulator &other) = delete;
        
    private:
        bool isFurtherSimulationWorthwile();
        
        Conjectures runImpl(bool boundedRun, unsigned int nSteps, std::chrono::milliseconds timeLimit);
        
        std::unique_ptr<ClauseOrder> m_clauseOrderStrat;
        std::unique_ptr<Partition> m_partitionStrat;
        std::unique_ptr<Randomization> m_randomizationStrat;
        std::unique_ptr<Propagation> m_propagationStrat;
        
        const GateAnalyzer& m_gateAnalyzer;
        bool m_isInitialized;
        
        SimulationVectors m_simulationVectors;
        float m_abortThreshold;

    };
    
    
    /**
     * \class RandomSimulatorBuilder
     *
     * \ingroup RandomSimulation
     *
     * \brief Builder class for random simulators.
     *
     * RandomSimulatorBuilder objects are responsible for configuring and creating
     * implementations of the RandomSimulator interface. If a withX method is not
     * invoked before build, the implementation uses a default implementation of X
     * instead (see the implementation's documentation).
     *
     */
    class BitparallelRandomSimulatorBuilder {
    public:
        /**
         * Configures the builder to create random simulators with the given clause order strategy.
         */
        BitparallelRandomSimulatorBuilder& withClauseOrderStrategy(std::unique_ptr<ClauseOrder> clauseOrderStrat);
        
        /**
         * Configures the builder to create random simulators with the given partition strategy.
         */
        BitparallelRandomSimulatorBuilder& withPartitionStrategy(std::unique_ptr<Partition> partitionStrat);
        
        /**
         * Configures the builder to create random simulators with the given randomization strategy.
         */
        BitparallelRandomSimulatorBuilder& withRandomizationStrategy(std::unique_ptr<Randomization> randomizationStrat);
        
        /**
         * Configures the builder to create random simulators with the given variable assignment propagation strategy.
         */
        BitparallelRandomSimulatorBuilder& withPropagationStrategy(std::unique_ptr<Propagation> propagationStrat);
        
        /**
         * Configures the builder to create random simulators with the given reduction rate abort threshold (RRAT).
         * TODO: document the RRAT, and that negative values of threshold disable it
         */
        BitparallelRandomSimulatorBuilder& withReductionRateAbortThreshold(float threshold);
        
        /**
         * Configures the builder to create random simulators with the given gate filter. Random simulators
         * only consider gates with outputs marked enabled by this filter. If no such filter is configured,
         * all output variables are considered to be enabled.
         */
        BitparallelRandomSimulatorBuilder& withGateFilter(std::unique_ptr<GateFilter> filter);

        /**
         * Configures the builder to create random simualtors with the given gate analyzer. Note that
         * the random simulation system does not invoke gateAnalyzer.analyze(), and does not configure
         * the gate analyzer; this has to be performed by the user before starting random simulation
         * by invoking the random simulator's run() method.
         */
        BitparallelRandomSimulatorBuilder& withGateAnalyzer(const GateAnalyzer& gateAnalyzer);

        /**
         * Builds the random simulator.
         */
        std::unique_ptr<BitparallelRandomSimulator> build();
        
        BitparallelRandomSimulatorBuilder(const GateAnalyzer& analyzer);
        ~BitparallelRandomSimulatorBuilder();
        BitparallelRandomSimulatorBuilder(const BitparallelRandomSimulatorBuilder& other) = delete;
        BitparallelRandomSimulatorBuilder& operator=(const BitparallelRandomSimulatorBuilder& other) = delete;
        
    private:
        std::unique_ptr<ClauseOrder> m_clauseOrderStrat;
        std::unique_ptr<Partition> m_partitionStrat;
        std::unique_ptr<Randomization> m_randomizationStrat;
        std::unique_ptr<Propagation> m_propagationStrat;
        std::vector<std::unique_ptr<GateFilter>> m_gateFilters;
        const GateAnalyzer& m_gateAnalyzer;
        float m_reductionRateAbortThreshold;
    };

}
#endif
