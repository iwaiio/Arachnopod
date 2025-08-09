#pragma once

#include <chrono>
#include <thread>

// ===================================================================
// TIMING CONTROL SYSTEM
// ===================================================================

namespace Timing {
    
    // Global timing constants (can be adjusted for debugging)
    static constexpr double SIMULATION_FREQUENCY_HZ = 100.0;  // 100 Hz = 10ms period
    static constexpr double CVS_FREQUENCY_HZ = 50.0;          // 50 Hz = 20ms period  
    static constexpr double SES_FREQUENCY_HZ = 20.0;          // 20 Hz = 50ms period
    static constexpr double DS_FREQUENCY_HZ = 100.0;          // 100 Hz = 10ms period (fast for motors)
    static constexpr double BUS_FREQUENCY_HZ = 1000.0;        // 1000 Hz = 1ms period (very fast bus)
    
    // Convert frequency to period in milliseconds
    constexpr double frequencyToPeriodMs(double freq_hz) {
        return 1000.0 / freq_hz;
    }
    
    // High precision timer class
    class PrecisionTimer {
    private:
        std::chrono::high_resolution_clock::time_point m_start_time;
        std::chrono::microseconds m_target_period;
        
    public:
        explicit PrecisionTimer(double frequency_hz) {
            m_target_period = std::chrono::microseconds(
                static_cast<long long>(1000000.0 / frequency_hz));
            reset();
        }
        
        void reset() {
            m_start_time = std::chrono::high_resolution_clock::now();
        }
        
        void waitForNextCycle() {
            auto target_time = m_start_time + m_target_period;
            std::this_thread::sleep_until(target_time);
            m_start_time = target_time;
        }
        
        // Get elapsed time since last reset in microseconds
        long long getElapsedMicros() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(
                now - m_start_time).count();
        }
        
        // Check if we're running behind schedule
        bool isOverrun() const {
            return getElapsedMicros() > m_target_period.count();
        }
    };
    
    // Simple delay function
    inline void delay_ms(uint32_t milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    
    inline void delay_us(uint32_t microseconds) {
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    }
    
    // Get current time in milliseconds since epoch
    inline uint64_t getCurrentTimeMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    
    // Get current time in microseconds since epoch
    inline uint64_t getCurrentTimeUs() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    
} // namespace Timing