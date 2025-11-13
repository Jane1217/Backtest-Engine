#pragma once

/**
 * @brief Enumeration of available time frames for bar aggregation
 * 
 * Time frames define how ticks are grouped into bars. Each time frame
 * represents a different granularity level for market data analysis.
 * Strategies can operate on different time frames depending on their needs.
 */
enum class TimeFrame {
    MINUTE,      // 1-minute bars (most granular for intraday trading)
    FIVEMINTUES, // 5-minute bars (common for short-term strategies)
    HOUR,        // 1-hour bars (for medium-term analysis)
    DAY          // Daily bars (for long-term strategies)
};

/**
 * @brief Calculates how many bars of a given time frame fit in one trading day
 * 
 * This function is used for normalization and performance calculations.
 * Assumes a standard trading day of 6.5 hours (390 minutes).
 * 
 * @param tf The time frame to calculate for
 * @return Number of bars per trading day
 * 
 * Examples:
 * - MINUTE: 390 bars/day (6.5 hours * 60 minutes)
 * - FIVEMINTUES: 78 bars/day (390 minutes / 5)
 * - HOUR: 6.5 bars/day
 * - DAY: 1 bar/day
 */
inline double getTicksPerDay(TimeFrame tf) {
    switch (tf) {
        case TimeFrame::MINUTE: return 390.0;      // 390 minutes in a trading day
        case TimeFrame::FIVEMINTUES: return 78.0;   // 390 / 5 = 78 five-minute periods
        case TimeFrame::HOUR: return 6.5;           // 6.5 hours in a trading day
        case TimeFrame::DAY: return 1.0;            // 1 day per day
        default: return 1.0;
    }
}