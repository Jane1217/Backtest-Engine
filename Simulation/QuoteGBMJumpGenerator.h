#pragma once

#include <random>
#include <vector>
#include "TimeFrame.h"
#include "Tick.h"

/**
 * @brief Generates synthetic quote ticks (bid/ask prices) using GBM + Jump model
 * 
 * Similar to GBMJumpGenerator, but generates QuoteTick instead of Tick.
 * This generator creates bid/ask prices with a spread, which is more realistic
 * for strategies that need to see the order book (like SpreadStrategy).
 * 
 * How it works:
 * 1. Generates a mid-price using the same GBM+Jump model as GBMJumpGenerator
 * 2. Generates a random spread around the mid-price
 * 3. Calculates bid = mid - spread/2 and ask = mid + spread/2
 * 
 * The spread is modeled as a random variable to simulate realistic market conditions
 * where spreads vary based on liquidity, volatility, and market conditions.
 * 
 * Model:
 * - Mid-price follows GBM + Jump (same as GBMJumpGenerator)
 * - Spread ~ N(spreadMu, spreadSigma), truncated to minimum 0.001
 * - Bid = mid - spread/2
 * - Ask = mid + spread/2
 */
class QuoteGBMJumpGenerator {
private:
    std::mt19937 rng;              // Random number generator

    size_t nTicks;                 // Number of ticks to generate
    TimeFrame tf;                  // Time frame for calculating time step
    double startPrice;             // Starting mid-price
    double mu;                     // Drift rate / expected return
    double impVol;                 // Implied volatility
    double jumpLambda;             // Jump intensity / probability per period
    double jumpMu;                 // Mean of jump size
    double jumpSigma;              // Standard deviation of jump size

    double spreadMu;               // Mean spread (default: 0.01 = 1% of price)
    double spreadSigma;             // Standard deviation of spread (default: 0.002 = 0.2%)
    
public:
    /**
     * @brief Constructs a QuoteGBM+Jump generator with specified parameters
     * 
     * @param nTicks Number of ticks to generate
     * @param tf Time frame for calculating time step
     * @param startPrice Starting mid-price (default: 100.0)
     * @param mu Drift rate (default: 0.03 = 3% annual)
     * @param impVol Implied volatility (default: 0.2 = 20% annual)
     * @param jumpLambda Jump intensity (default: 0.01 = 1%)
     * @param jumpMu Mean jump size (default: -0.01 = -1%)
     * @param jumpSigma Standard deviation of jump size (default: 0.03 = 3%)
     * @param spreadMu Mean spread as fraction of price (default: 0.01 = 1%)
     * @param spreadSigma Standard deviation of spread (default: 0.002 = 0.2%)
     */
    QuoteGBMJumpGenerator(size_t nTicks,
        TimeFrame tf,
        double startPrice = 100.0,
        double mu = 0.03,
        double impVol = 0.2,
        double jumpLambda = 0.01,
        double jumpMu = -0.01,
        double jumpSigma = 0.03,
        double spreadMu = 0.01,
        double spreadSigma = 0.002);

    /**
     * @brief Generates a vector of synthetic quote ticks
     * 
     * For each tick:
     * 1. Generates mid-price using GBM+Jump model (same as GBMJumpGenerator)
     * 2. Generates random spread ~ N(spreadMu, spreadSigma)
     * 3. Truncates spread to minimum 0.001 to ensure bid < ask
     * 4. Calculates bid = mid - spread/2 and ask = mid + spread/2
     * 5. Generates random volume
     * 6. Creates QuoteTick with timestamp, bid, ask, and volume
     * 
     * @return Vector of generated quote ticks
     */
    std::vector<QuoteTick> generateTicks();
};
