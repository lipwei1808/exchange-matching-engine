# Concurrent Matching Exchange

## Introduction

This is a concurrent matching engine project built in C++. This project aims to maximise the concurrent execution of different orders.

## Build

1. After cloning the project, run the following line to build

```
make all
```

2. The engine executable is found in `./build/engine`

## Overview

The concurrent matching exchange consists of three main components:

1. **Engine**: The matching engine responsible for handling connections and orders.
2. **OrderBook**: Manages the overall order book (buy and sell side), handling incoming orders and sending orders to the correct side.
3. **Book**: Represents either the buy (bids) or sell (asks) side, maintaining an ordered map of prices to a queue of orders.
