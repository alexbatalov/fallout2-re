# Fallout 2 Reference Edition

In this repository you'll find reverse engineered source code for Fallout 2.

As a player/gamer you're most likely interested in [Fallout 2 Community Edition](https://github.com/alexbatalov/fallout2-ce) which is based on this project.

If you're a developer you might also want to check [Fallout Reference Edition](https://github.com/alexbatalov/fallout1-re) to see evolution of the engine.

## Goal

The goal of this project is to restore original source code as close possible with all it's imperfections. In very many respects this goal can be considered achieved.

## Status

There is a small number of functions which are not yet decompiled. These functions are not essential to gameplay, most of them are leftovers from Fallout 1, others are a part of larger APIs that were not fully utilized. Aside from these missing functions there is ongoing effort to update the codebase to C89 to make sure the game can be compiled with Watcom C compiler (which might be handy to achieve binary identical results). This tasks are low priority and probably will never be completed.

## Installation

You must own the game to play. Purchase your copy on [GOG](https://www.gog.com/game/fallout_2) or [Steam](https://store.steampowered.com/app/38410). Download latest build or build from source. The `fallout2-re.exe` serves as a drop-in replacement for `fallout2.exe`. Copy it to your Fallout 2 directory and run.

## Legal

The source code in this repository is produced by reverse engineering the original binary. There are couple of exceptions for reverse engineering under DMCA - documentation, interoperability, fair use. Documentation is needed to achieve interoperability. Running your legally purchased copy on modern Mac M1 for example (interoperability in action) constitutes fair use. Publishing this stuff to wide audience is questionable. Eventually it's up to Bethesda/Microsoft to takedown the project or leave it be. See [#29](https://github.com/alexbatalov/fallout2-re/issues/29) for discussion.

## License

The source code is this repository is available under the [Sustainable Use License](LICENSE.md).
