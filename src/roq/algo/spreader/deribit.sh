#!/usr/bin/env bash

if [ "$1" == "debug" ]; then
  KERNEL="$(uname -a)"
  case "$KERNEL" in
    Linux*)
      PREFIX="gdb --args"
      ;;
    Darwin*)
      PREFIX="lldb --"
      ;;
  esac
  shift 1
else
  PREFIX=
fi

$PREFIX ./roq-algo-spreader \
  $HOME/run/deribit.sock \
  --name trader \
  --account A1 \
  --exchange deribit \
  --side buy \
  --quantity 5 \
  --symbols BTC-21JUN24,BTC-PERPETUAL \
  --params 0,1 \
  --threshold_quantity_multiplier 2 \
  --threshold_number_of_orders 3 \
  $@
