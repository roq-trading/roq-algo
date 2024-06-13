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
  --symbols ETH-PERPETUAL,BTC-PERPETUAL \
  --params 100,0.05 \
  --quantity 20 \
  --side buy \
  $@
