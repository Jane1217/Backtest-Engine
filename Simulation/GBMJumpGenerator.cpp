#include "GBMJumpGenerator.h"

/**
 * @brief Constructs the generator and initializes random number generator
 * 
 * The random number generator is seeded with a random device to ensure
 * different sequences on each run.
 */
GBMJumpGenerator::GBMJumpGenerator(size_t nTicks,
    TimeFrame tf,
    double startPrice,
    double mu,
    double impVol,
    double jumpLambda,
    double jumpMu,
    double jumpSigma) : rng(std::random_device{}()),  // Seed RNG with random device
                               nTicks(nTicks),
                               tf(tf),
                               startPrice(startPrice),
                               mu(mu),
                               impVol(impVol),
                               jumpLambda(jumpLambda),
                               jumpMu(jumpMu),
                               jumpSigma(jumpSigma) {}

/**
 * @brief Generates synthetic ticks using GBM + Jump model
 * 
 * Algorithm:
 * 1. Calculate time step (dt) based on time frame
 *    - dt = 1 / (252 trading days * ticks per day)
 *    - Example: MINUTE time frame = 1 / (252 * 390) ≈ 0.0000102 years per tick
 * 
 * 2. For each tick:
 *    a. Generate standard normal random variable Z ~ N(0,1)
 *    b. Calculate GBM price change: dS = (mu - 0.5*sigma^2)*dt + sigma*Z*sqrt(dt)
 *       - The -0.5*sigma^2 term is the Ito correction for log-normal processes
 *    c. Check if jump occurs: random(0,1) < jumpLambda
 *    d. If jump: generate jump size ~ N(jumpMu, jumpSigma), apply exp(jump)
 *    e. Update price: price = price * exp(dS) * jumpFactor
 *    f. Generate random volume (uniform between 0.5 and 1.5)
 *    g. Create Tick with index as timestamp, current price, and volume
 * 
 * The timestamp is simply the tick index (0, 1, 2, ...) for simplicity.
 * In a real system, you'd use actual timestamps based on the time frame.
 */
std::vector<Tick> GBMJumpGenerator::generateTicks() {
    std::vector<Tick> genTicks;
    genTicks.reserve(nTicks);  // Pre-allocate for efficiency

    // Random distributions for generating random numbers
    std::normal_distribution<double> norm(0.0, 1.0);           // Standard normal: Z ~ N(0,1)
    std::uniform_real_distribution<double> volGen(0.5, 1.5);   // Volume: uniform between 0.5 and 1.5
    std::uniform_real_distribution<double> jumpProb(0.0, 1.0);  // Jump probability: uniform [0,1]
    std::normal_distribution<double> jumpDist(jumpMu, jumpSigma); // Jump size: N(jumpMu, jumpSigma)

    // Calculate time step (dt) in years
    // 252 = trading days per year
    // getTicksPerDay(tf) = number of ticks per trading day
    // Example: MINUTE time frame = 390 ticks/day, so dt = 1/(252*390) years
    double dt = 1.0 / (252.0 * getTicksPerDay(tf));
    
    double currentPrice = startPrice;  // Start at initial price
    double jumpFactor = 1.0;            // Jump multiplier (1.0 = no jump)

    // Generate each tick
    for (size_t i = 0; i < nTicks; i++) {
        // Step 1: Generate standard normal random variable
        double Z = norm(rng);
        
        // Step 2: Calculate GBM price change
        // Formula: dS = (mu - 0.5*sigma^2)*dt + sigma*Z*sqrt(dt)
        // The -0.5*sigma^2 term is the Ito correction for geometric Brownian motion
        double dS = (mu - 0.5 * impVol * impVol) * dt + impVol * Z * std::sqrt(dt);
        
        // Step 3: Check for jump
        jumpFactor = 1.0;  // Default: no jump
        if (jumpProb(rng) < jumpLambda) {
            // Jump occurs: generate jump size and apply it
            double jump = jumpDist(rng);
            jumpFactor = std::exp(jump);  // Convert to multiplicative factor
        }

        // Step 4: Generate random volume for this tick
        double newVolume = volGen(rng);

        // Step 5: Update price
        // Price follows: S(t+dt) = S(t) * exp(dS) * jumpFactor
        // This ensures prices are always positive (log-normal distribution)
        currentPrice *= std::exp(dS) * jumpFactor;

        // Step 6: Create tick
        // Using index as timestamp for simplicity (in real system, use actual timestamps)
        genTicks.push_back({ static_cast<uint64_t>(i), currentPrice, newVolume });
    }

    return genTicks;
}
