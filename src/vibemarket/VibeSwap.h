#pragma once

#include "MachinePalCommon.h"
#include "crypto/EthAddress.h"
#include "crypto/EIP3009Value.h"

// VibeSwap is a thin, in-memory Uniswap V2-style model. It tracks wallet balances for tokens
// and LP balances for each pair, then applies the same constant-product (x*y=k) math used by
// Uniswap V2 when adding liquidity and swapping:
// - Pairs are identified by sorted token order (token0/token1). Reserves are stored in that order.
// - Adding liquidity mints LP tokens proportional to contributions; the first mint locks
//   MINIMUM_LIQUIDITY to the burn address, matching Uniswap V2 behavior.
// - Removing liquidity burns LP tokens and returns proportional reserves, respecting minimums.
// - Swaps apply the 0.3% fee (997/1000), compute hop-by-hop amounts for a path, and update reserves.
// - The model only moves balances within this instance; it does not touch any on-chain state.
class VibeSwap {
public:
    // Ethereum-style token symbol/identifier used by this in-memory model.
    using Token = std::string;
    using Amount = EIP3009Value;

    // Result of addLiquidity, matching Uniswap V2 behavior (actual amounts deposited + LP minted).
    struct AddLiquidityResult {
        Amount amountA;
        Amount amountB;
        Amount liquidity;
    };

    // Result of removeLiquidity, matching Uniswap V2 behavior (amounts returned).
    struct RemoveLiquidityResult {
        Amount amountA;
        Amount amountB;
    };

    // Snapshot of a pair's state (sorted tokens, reserves, and total LP supply).
    struct PairSnapshot {
        Token token0;
        Token token1;
        EthAddress contractAddress;
        Amount reserve0;
        Amount reserve1;
        Amount totalSupply;
    };

    // Construct an empty swap model with no pairs and no balances.
    VibeSwap();

    // Mint fungible tokens into a wallet (testing/funding helper; not a Uniswap V2 factory action).
    void mintToken(const EthAddress &to, const Token &token, const Amount &amount);
    // Return ERC20-like balance for a wallet and token.
    Amount balanceOf(const EthAddress &owner, const Token &token) const;
    // Return LP token balance for a wallet for the (tokenA, tokenB) pair.
    Amount lpBalanceOf(const EthAddress &owner, const Token &tokenA, const Token &tokenB) const;

    // Read-only pair state, with tokens in sorted order.
    PairSnapshot getPairSnapshot(const Token &tokenA, const Token &tokenB) const;
    // Convenience accessor for reserves in sorted order (token0, token1).
    std::pair<Amount, Amount> getReserves(const Token &tokenA, const Token &tokenB) const;

    // Add liquidity following Uniswap V2 math: initial mint uses sqrt(amountA*amountB) - MINIMUM_LIQUIDITY,
    // subsequent mints are proportional to reserves. Reverts if min constraints are not met.
    AddLiquidityResult addLiquidity(const EthAddress &provider,
                                   const Token &tokenA,
                                   const Token &tokenB,
                                   const Amount &amountADesired,
                                   const Amount &amountBDesired,
                                   const Amount &amountAMin,
                                   const Amount &amountBMin);

    // Remove liquidity, burning LP tokens and returning underlying tokens subject to min constraints.
    RemoveLiquidityResult removeLiquidity(const EthAddress &provider,
                                          const Token &tokenA,
                                          const Token &tokenB,
                                          const Amount &liquidity,
                                          const Amount &amountAMin,
                                          const Amount &amountBMin);

    // Swap an exact input amount across the path; returns all hop amounts and credits `to`.
    std::vector<Amount> swapExactTokensForTokens(const EthAddress &trader,
                                               const Amount &amountIn,
                                               const Amount &amountOutMin,
                                               const std::vector<Token> &path,
                                               const EthAddress &to);

    // Swap to receive an exact output amount across the path; debits `trader` up to amountInMax.
    std::vector<Amount> swapTokensForExactTokens(const EthAddress &trader,
                                               const Amount &amountOut,
                                               const Amount &amountInMax,
                                               const std::vector<Token> &path,
                                               const EthAddress &to);

    // Uniswap V2 quote() helper: amountB = amountA * reserveB / reserveA.
    static Amount quote(const Amount &amountA, const Amount &reserveA, const Amount &reserveB);
    // Uniswap V2 getAmountOut with 0.3% fee (997/1000): amountOut = ...
    // Fee-adjusted invariant: (reserveIn * reserveOut) <= (reserveIn + amountInWithFee) * (reserveOut - amountOut),
    // where amountInWithFee = amountIn * 997 / 1000. This preserves x*y=k after fees are applied.
    Amount getAmountOut(const Amount &amountIn, const Amount &reserveIn, const Amount &reserveOut) const;
    // Uniswap V2 getAmountIn with 0.3% fee (997/1000): amountIn = ...
    Amount getAmountIn(const Amount &amountOut, const Amount &reserveIn, const Amount &reserveOut) const;

    // Per-hop outputs for a path given an exact input.
    std::vector<Amount> getAmountsOut(const Amount &amountIn, const std::vector<Token> &path) const;
    // Per-hop inputs for a path to achieve an exact output.
    std::vector<Amount> getAmountsIn(const Amount &amountOut, const std::vector<Token> &path) const;

private:
    // Pair data stored in sorted token order with LP balances for providers.
    // Math notes (Uniswap V2 style): reserves are (reserve0, reserve1) for (token0, token1) and
    // define the constant-product invariant k = reserve0 * reserve1. Liquidity (LP) supply tracks
    // pro-rata ownership of reserves. On first mint, liquidity = sqrt(amount0 * amount1)
    // minus MINIMUM_LIQUIDITY, which is locked to the burn address. On subsequent mints,
    // liquidity = min(amount0 * totalSupply / reserve0, amount1 * totalSupply / reserve1).
    // On burn, amounts returned are amount0 = liquidity * reserve0 / totalSupply and
    // amount1 = liquidity * reserve1 / totalSupply (after which totalSupply decreases).
    struct Pair {
        Token token0;
        Token token1;
        EthAddress contractAddress;
        Amount reserve0{};
        Amount reserve1{};
        Amount totalSupply{};
        std::map<EthAddress, Amount> lpBalances;
    };

    // Uniswap V2 fee parameters (0.3%) and minimum locked liquidity.
    static constexpr uint32_t kFeeNumerator = 997;
    static constexpr uint32_t kFeeDenominator = 1000;
    static constexpr uint32_t kMinimumLiquidity = 1000;
    // Burn address for the permanently locked MINIMUM_LIQUIDITY.
    static EthAddress burnAddress();

    // Sort tokens deterministically (mirrors Uniswap V2 token0/token1).
    static std::pair<Token, Token> sortTokens(const Token &tokenA, const Token &tokenB);
    // Integer square-root used for initial liquidity minting.
    static Amount integerSqrt(const Amount &value);

    static EthAddress makePairAddress(const Token &token0, const Token &token1);

    // Deterministic key for pair map (token0, token1).
    std::string pairKey(const Token &token0, const Token &token1) const;
    // Get or create a pair in sorted order.
    Pair &getOrCreatePair(const Token &tokenA, const Token &tokenB);
    // Get an existing pair or throw if not present.
    Pair &getPairChecked(const Token &tokenA, const Token &tokenB);
    const Pair &getPairChecked(const Token &tokenA, const Token &tokenB) const;

    // Validate wallet balance before debiting.
    void ensureBalance(const EthAddress &owner, const Token &token, const Amount &amount) const;
    // Move tokens from wallet to pool.
    void debit(const EthAddress &owner, const Token &token, const Amount &amount);
    // Move tokens from pool to wallet.
    void credit(const EthAddress &owner, const Token &token, const Amount &amount);

    // Execute swap given precomputed hop amounts (path size N yields amounts size N).
    std::vector<Amount> swapWithKnownAmounts(const EthAddress &trader,
                                           const std::vector<Amount> &amounts,
                                           const std::vector<Token> &path,
                                           const EthAddress &to);

    // Map of pairKey -> Pair state, and wallet balances per token.
    std::map<std::string, Pair> pairs_;
    std::map<EthAddress, std::map<Token, Amount>> balances_;
};
