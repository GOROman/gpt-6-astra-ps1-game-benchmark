#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p test-build
${CC:-cc} -std=c99 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g -Isrc src/game.c tests/test_game.c -o test-build/test_game
./test-build/test_game

${CC:-cc} -std=c99 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g -Isrc src/sound_synth.c tests/test_sound.c -o test-build/test_sound
./test-build/test_sound
