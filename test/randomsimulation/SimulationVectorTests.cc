#include <gtest/gtest.h>

#include <randomsimulation/SimulationVector.h>


namespace randsim {
    TEST(RSSimulationVectorTest, memAlignment) {
        SimulationVectors underTest;
        size_t amount = 10;
        underTest.initialize(amount);
        
        for (size_t i = 0; i < amount; ++i) {
            uintptr_t firstAddr = reinterpret_cast<uintptr_t>(&underTest.get(i));
            EXPECT_TRUE(firstAddr % RANDSIM_ALIGNMENT == 0);
        }
    }
}
