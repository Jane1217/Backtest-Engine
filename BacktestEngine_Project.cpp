#include <vector>
#include <mutex>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <cstring>

#include "Tick.h"
#include "TimeFrame.h"
#include "GBMJumpGenerator.h"
#include "QuoteGBMJumpGenerator.h"
#include "BacktestEngine.h"
#include "MeanReversionSimpleStrategy.h"
#include "BreakoutStrategy.h"
#include "SpreadStrategy.h"

/**
 * @brief Global mutex for thread-safe console output
 * 
 * Since strategies can run in parallel threads, we need to synchronize
 * console output to prevent garbled text when multiple threads print simultaneously.
 * Note: Console output is now disabled when running from web interface.
 */
std::mutex globalPrintMutex;

/**
 * @brief Parses command line arguments or environment variables
 * 
 * Priority: Command line arguments > Environment variables > Default values
 * 
 * @param argc Number of command line arguments
 * @param argv Command line arguments array
 * @param numTicks Output: Number of ticks to generate
 * @param initialCapital Output: Initial capital for each strategy
 * @return true if parsing successful, false otherwise
 */
bool parseArguments(int argc, char* argv[], size_t& numTicks, double& initialCapital) {
    // Default values
    numTicks = 1000;
    initialCapital = 10000.0;
    
    // Try to read from environment variables (set by web interface)
    const char* envTicks = std::getenv("NUM_TICKS");
    const char* envCapital = std::getenv("INITIAL_CAPITAL");
    
    if (envTicks) {
        numTicks = std::stoul(envTicks);
    }
    if (envCapital) {
        initialCapital = std::stod(envCapital);
    }
    
    // Override with command line arguments if provided
    // Usage: ./BacktestEngine [num_ticks] [initial_capital]
    if (argc >= 2) {
        numTicks = std::stoul(argv[1]);
    }
    if (argc >= 3) {
        initialCapital = std::stod(argv[2]);
    }
    
    // Validate inputs
    if (numTicks < 10 || numTicks > 100000) {
        std::cerr << "Error: num_ticks must be between 10 and 100000\n";
        return false;
    }
    if (initialCapital <= 0 || initialCapital > 100000000) {
        std::cerr << "Error: initial_capital must be between 0 and 100000000\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Main entry point of the backtesting engine
 * 
 * This program demonstrates the complete backtesting workflow:
 * 1. Parse command line arguments or environment variables for configuration
 * 2. Generate synthetic market data (ticks) using GBM + Jump model
 * 3. Create a backtest engine and load the market data
 * 4. Register multiple trading strategies to test
 * 5. Run all strategies in parallel on the same market data
 * 6. Collect and report performance statistics
 * 
 * The engine supports running multiple strategies simultaneously, each with
 * its own initial capital and time frame, allowing for easy strategy comparison.
 * 
 * Command line usage:
 *   ./BacktestEngine [num_ticks] [initial_capital]
 * 
 * Environment variables (used by web interface):
 *   NUM_TICKS: Number of ticks to generate
 *   INITIAL_CAPITAL: Starting capital for each strategy
 * 
 * @param argc Number of command line arguments
 * @param argv Command line arguments array
 * @return 0 on success, 1 on error
 */
int main(int argc, char* argv[])
{
    // Parse configuration from command line or environment variables
    size_t numTicks;
    double initialCapital;
    if (!parseArguments(argc, argv, numTicks, initialCapital)) {
        return 1;
    }
    
    // Check if running from web interface (disable verbose output)
    bool isWebInterface = std::getenv("WEB_INTERFACE") != nullptr;
    
    // Start timing to measure total execution time
    auto start = std::chrono::high_resolution_clock::now();

    // ========================================================================
    // STEP 1: Generate synthetic market data (ticks)
    // ========================================================================
    
    // Create a generator for regular trade ticks using Geometric Brownian Motion + Jump model
    // Parameters: numTicks (from command line/env), 1-minute time frame
    // This simulates realistic price movements with random jumps
    std::unique_ptr<GBMJumpGenerator> jumpGenerator = 
        std::make_unique<GBMJumpGenerator>(numTicks, TimeFrame::MINUTE);
    std::vector<Tick> ticks = jumpGenerator->generateTicks();

    // Create a generator for quote ticks (bid/ask prices)
    // Some strategies need to see the bid-ask spread, not just trade prices
    std::unique_ptr<QuoteGBMJumpGenerator> quoteJumpGenerator = 
        std::make_unique<QuoteGBMJumpGenerator>(numTicks, TimeFrame::MINUTE);
    std::vector<QuoteTick> quoteTicks = quoteJumpGenerator->generateTicks();
    
    // ========================================================================
    // STEP 2: Initialize the backtest engine and load market data
    // ========================================================================
    
    BacktestEngine engine;
    // Load regular trade ticks (for strategies that use Tick)
    engine.setTickData(std::move(ticks));
    // Load quote ticks (for strategies that use QuoteTick, like SpreadStrategy)
    engine.setTickData(std::move(quoteTicks));
    
    // ========================================================================
    // STEP 3: Register trading strategies to test
    // ========================================================================
    
    // Strategy 1: Mean Reversion - buys on price drops, sells on price rises
    // Parameters: name, strategy instance, time frame, initial capital
    engine.addStrategy("Mean_Reversion", 
                       std::make_unique<MeanReversionSimple>(), 
                       TimeFrame::MINUTE, 
                       initialCapital);
    
    // Strategy 2: Breakout - enters positions when price breaks through a window
    // The template parameter <20> is the window size for the breakout detection
    engine.addStrategy("Breakout_Win20", 
                       std::make_unique<BreakoutStrategy<20>>(), 
                       TimeFrame::MINUTE, 
                       initialCapital);
    
    // Strategy 3: Spread - profits from bid-ask spread (uses QuoteTick data)
    engine.addStrategy("Spread", 
                       std::make_unique<SpreadStrategy>(), 
                       TimeFrame::MINUTE, 
                       initialCapital);
    
    // ========================================================================
    // STEP 4: Run all strategies and collect results
    // ========================================================================
    
    // Run all strategies in parallel threads
    // Parameters:
    //   - saveToCSV = true: Generate CSV files with results
    //   - verbose = !isWebInterface: Only print to console if not running from web
    engine.runAll(true, !isWebInterface);
    
    // ========================================================================
    // STEP 5: Report execution time (only if not running from web interface)
    // ========================================================================
    
    if (!isWebInterface) {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        // Thread-safe output of total execution time
        {
            std::unique_lock<std::mutex> lock(globalPrintMutex);
            std::cout << "Total execution elapsed time: " << elapsed.count() << " seconds\n";
        }
    }

    return 0;
}