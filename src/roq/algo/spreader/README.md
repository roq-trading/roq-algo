This will trade a portfolio of N instruments

We will name the first instrument (index 0) the "base" instrument

The side and quantity applies to the first instrument (index 0)

The (N-1) weights are relative to the base instrument and defined from the second parameter (index 1...N)

The target "spread" is indicated by the first parameter (index 0)

The model will compute the impact price for each instrument

An "impact price" is the worst price we want to pay if we are to aggress resting orders of an instrument

From the impact price of (N-1) instruments, and the target spread, we can work out the price where we want to place a limit order for the last instrument

If a working order is filled, we must proportionally aggress the other (N-1) instrument

TODO Risk assumptions may be required to manage the "residual" risk due to the minimum trading size for each instrument

TODO Hung orders (when aggressing)