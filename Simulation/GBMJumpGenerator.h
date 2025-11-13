#pragma once

#include <random>

#include "Tick.h"
#include "TimeFrame.h"

/**
 * @brief Generates synthetic market data using Geometric Brownian Motion (GBM) + Jump model
 * 
 * This generator creates realistic price movements by combining:
 * 1. Geometric Brownian Motion: Continuous price movements with drift and volatility
 * 2. Jump component: Random price jumps to simulate market shocks/events
 * 
 * The GBM model is widely used in finance to model stock prices. It assumes:
 * - Prices follow a log-normal distribution
 * - Returns are normally distributed
 * - Price changes have both a drift component (trend) and a random component (volatility)
 * 
 * The jump component adds realism by occasionally introducing sudden price movements,
 * which are common in real markets due to news, events, or large trades.
 * 
 * Model formula:
 *   dS = S * (mu * dt + sigma * dW) * jumpFactor
 * 
 * Where:
 * - S: Current price
 * - mu: Drift rate (expected return)
 * - sigma: Volatility
 * - dW: Random Wiener process (Brownian motion)
 * - jumpFactor: Random jump multiplier (1.0 if no jump, exp(jump) if jump occurs)
 */
class GBMJumpGenerator {
private:
	std::mt19937 rng;              // Random number generator (Mersenne Twister)

	size_t nTicks;                 // Number of ticks to generate
	TimeFrame tf;                  // Time frame for calculating time step
	double startPrice;             // Starting price (default: 100.0)
	double mu;                     // Drift rate / expected return (default: 0.03 = 3% annual)
	double impVol;                 // Implied volatility (default: 0.2 = 20% annual)
	double jumpLambda;             // Jump intensity / probability per period (default: 0.01 = 1%)
	double jumpMu;                 // Mean of jump size (default: -0.01 = -1%)
	double jumpSigma;              // Standard deviation of jump size (default: 0.03 = 3%)
	
public:
	/**
	 * @brief Constructs a GBM+Jump generator with specified parameters
	 * 
	 * @param nTicks Number of ticks to generate
	 * @param tf Time frame for calculating time step (affects dt in GBM formula)
	 * @param startPrice Starting price (default: 100.0)
	 * @param mu Drift rate / expected annual return (default: 0.03 = 3%)
	 * @param impVol Implied volatility / annual volatility (default: 0.2 = 20%)
	 * @param jumpLambda Jump intensity - probability of jump per period (default: 0.01 = 1%)
	 * @param jumpMu Mean jump size (default: -0.01 = -1%, negative means jumps tend to be downward)
	 * @param jumpSigma Standard deviation of jump size (default: 0.03 = 3%)
	 */
	GBMJumpGenerator(size_t nTicks,
		TimeFrame tf,
		double startPrice = 100.0,
		double mu = 0.03,
		double impVol = 0.2,
		double jumpLambda = 0.01,
		double jumpMu = -0.01,
		double jumpSigma = 0.03);

	/**
	 * @brief Generates a vector of synthetic ticks
	 * 
	 * For each tick:
	 * 1. Generates a random normal variable Z (standard normal distribution)
	 * 2. Calculates price change using GBM formula: dS = (mu - 0.5*sigma^2)*dt + sigma*Z*sqrt(dt)
	 * 3. Randomly decides if a jump occurs (based on jumpLambda probability)
	 * 4. If jump occurs, applies jump factor: exp(jump) where jump ~ N(jumpMu, jumpSigma)
	 * 5. Updates price: newPrice = oldPrice * exp(dS) * jumpFactor
	 * 6. Generates random volume
	 * 7. Creates Tick with timestamp, price, and volume
	 * 
	 * @return Vector of generated ticks
	 */
	std::vector<Tick> generateTicks();
};