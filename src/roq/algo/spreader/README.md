# Roq Spreader

## Design

This will trade a portfolio of N instruments

We will name the first instrument (index 0) the "base" instrument

The side and quantity applies to the base instrument

The (N-1) weights are relative to the base instrument and defined from the second parameter (index 1...N)

The target "spread" is indicated by the first parameter (index 0)

The model will compute the impact price for each instrument

An "impact price" is the worst price we want to accept if we are to aggress resting orders of an instrument

There are controls to manage risk-aversion when computing this impact price, e.g. quantity multiplier and minimum number of orders on the exchange

From the impact price of (N-1) instruments, and the target spread, we can work out the target price where we want to place a limit order for any other instrument

Due to model precision and the instrument's tick size, the target price must be rounded away from best to at least achieve the target spread

If a working order is (partially) filled, we must proportionally aggress the other (N-1) instruments


## TODO

Risk assumptions are required to manage the "residual" risk due to the minimum trading size for each instrument

Hung orders (when aggressing) -- stop loss?

Iceberg style for larger quantities?

How to manage various error conditions?

How to avoid spoofing (will result in working orders updating too often, we need some way to slowly update working orders when the price is improving)
