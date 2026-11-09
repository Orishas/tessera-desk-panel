#!/bin/sh
# Host tests of the model (no LVGL, no Tessera): sh tests/run.sh
set -e
cd "$(dirname "$0")/.."
c++ -std=c++17 -Wall -Werror -I components/desk_panel tests/test_model.cpp components/desk_panel/model.cpp \
  components/desk_panel/logo.cpp -o tests/run
tests/run
