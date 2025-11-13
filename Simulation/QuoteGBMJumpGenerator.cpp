#include "QuoteGBMJumpGenerator.h"

/**
 * @brief Constructs the generator and initializes random number generator
 */
QuoteGBMJumpGenerator::QuoteGBMJumpGenerator(size_t nTicks,
    TimeFrame tf,
    double startPrice,
    double mu,
    double impVol,
    double jumpLambda,
    double jumpMu,
    double jumpSigma,
    double spreadMu,
    double spreadSigma)
    : rng(std::random_device{}()),  // Seed RNG with random device
    nTicks(nTicks),
    tf(tf),
    startPrice(startPrice),
    mu(mu),
    impVol(impVol),
    jumpLambda(jumpLambda),
    jumpMu(jumpMu),
    jumpSigma(jumpSigma),
    spreadMu(spreadMu),
    spreadSigma(spreadSigma) {
}

/**
 * @brief Generates synthetic quote ticks with bid/ask prices
 * 
 * Algorithm:
 * 1. Calculate time step (dt) based on time frame (same as GBMJumpGenerator)
 * 
 * 2. For each tick:
 *    a. Generate mid-price using GBM+Jump model (same process as GBMJumpGenerator)
 *       - Generate Z ~ N(0,1)
 *       - Calculate dS = (mu - 0.5*sigma^2)*dt + sigma*Z*sqrt(dt)
 *       - Check for jump and apply jump factor
 *       - Update mid-price: mid = mid * exp(dS) * jumpFactor
 * 
 *    b. Generate spread ~ N(spreadMu, spreadSigma)
 *       - Truncate to minimum 0.001 to ensure bid < ask
 *       - This prevents negative spreads or bid >= ask
 * 
 *    c. Calculate bid and ask prices:
 *       - bid = mid - spread/2
 *       - ask = mid + spread/2
 * 
 *    d. Generate random volume
 * 
 *    e. Create QuoteTick with timestamp, bid, ask, and volume
 * 
 * The spread is modeled as a random variable to simulate realistic market conditions
 * where spreads vary based on liquidity and volatility.
 */
std::vector<QuoteTick> QuoteGBMJumpGenerator::generateTicks() {
    std::vector<QuoteTick> genTicks;
    genTicks.reserve(nTicks);  // Pre-allocate for efficiency

    // Random distributions
    std::normal_distribution<double> norm(0.0, 1.0);           // Standard normal for GBM
    std::uniform_real_distribution<double> volGen(0.5, 1.5);   // Volume generation
    std::uniform_real_distribution<double> jumpProb(0.0, 1.0);  // Jump probability
    std::normal_distribution<double> jumpDist(jumpMu, jumpSigma); // Jump size
    std::normal_distribution<double> spreadGen(spreadMu, spreadSigma); // Spread generation

    // Calculate time step (same as GBMJumpGenerator)
    double dt = 1.0 / (252.0 * getTicksPerDay(tf));
    double currentPrice = startPrice;  // Current mid-price

    // Generate each quote tick
    for (size_t i = 0; i < nTicks; ++i) {
        // Step 1: Generate mid-price using GBM+Jump model (same as GBMJumpGenerator)
        double Z = norm(rng);
        double dS = (mu - 0.5 * impVol * impVol) * dt + impVol * Z * std::sqrt(dt);

        // Check for jump
        double jumpFactor = 1.0;
        if (jumpProb(rng) < jumpLambda) {
            double jump = jumpDist(rng);
            jumpFactor = std::exp(jump);
        }

        // Update mid-price
        currentPrice *= std::exp(dS) * jumpFactor;
        
        // Step 2: Generate volume
        double volume = volGen(rng);

        // Step 3: Generate spread and calculate bid/ask
        // Spread is modeled as normal distribution, truncated to minimum 0.001
        // This ensures bid < ask (spread must be positive)
        double spread = std::max(0.001, spreadGen(rng));
        
        // Calculate bid and ask prices centered around mid-price
        double bid = currentPrice - spread / 2.0;  // Bid is below mid
        double ask = currentPrice + spread / 2.0;  // Ask is above mid

        // Step 4: Create quote tick
        genTicks.push_back({ static_cast<uint64_t>(i), bid, ask, volume });
    }

    return genTicks;
}
