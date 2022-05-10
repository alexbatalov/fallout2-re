# Fallout 2

In this repository you'll find reverse engineered source code for Fallout 2.

## Goal

The goal of this project is to restore original source code as close possible with all it's imperfections. This means Windows/x86/640x480 among many other things.

## Status

The game is playable and you can complete the [speedrun](https://fallout.fandom.com/wiki/Forum:Speedrun#Fallout_2). I don't know if a normal walkthrough is possible (i.e. finishing majority of quests), likely it's not. Function-wise about 4% (150 of 3800 functions) of the code is not yet decompiled. Notably dialog, widget system, and interpreter lib. Most of them are not essential to gameplay (probably leftovers from Fallout 1), while others (especially marked as `incomplete`) will lead to crashes when used.

## Installation

You must own the game to play. Purchase your copy on [GoG](https://www.gog.com/game/fallout_2) or [Steam](https://store.steampowered.com/app/38410). Download latest build or build from source. The `fallout2-re.exe` serves as a drop-in replacement for `fallout2.exe`. Copy it to your Fallout 2 directory and run.

## Contributing

If you'd like to contribute make sure you understand the goal of this project. Here is how you can help.

### Playing

This is the most important, time consuming, but also the most fun part, which does not require programming skills. Install latest version, play, report anything unsual. It would be great if you can verify and describe expected behaviour from the original game. Attach zipped save game if needed.

### Improving code readability

> There are only two hard things in Computer Science: cache invalidation and naming things.

There are hundreds of unnamed variables and functions, which makes it really hard to understand some parts of the code. It would be great if you can find good names for some of them. Renaming things is fine.

### Improving code accuracy

You need to have reverse engineering experience and appropriate tools. Notable stuff that requires attention are graph lib (LZSS implementation), movie lib, sound, 16bpp support, MMX blitting, world map.

Some things to consider (keep in mind the goal):
- Do not create new functions (which are not present in the original binary, some one-liners are OK).
- Do not change algorithms (even if they are bad).
- Do not fix bugs in the original code (use `FIXME` annotation).
- Document anything interesting.
- Extract constants into defines or enums (especially UI).

### Improving build system

I don't have much experience with `cmake` so I'm open to any improvements.

### No new features

Please do not submit new features at this time. Once reference implementation is completed, the development will be continued in the new repository. This repository will be left intact for historical reasons.

## Legal

The source code in this repository is produced by reverse engineering the original binary. There are couple of exceptions for reverse engineering under DMCA - documentation, interoperability, fair use. Documentation is needed to achieve interoperability. Running your legally purchased copy on modern Mac M1 for example (interoperability in action) constitutes fair use. Publishing this stuff to wide audience is questionable. Eventually it's up to Bethesda/Microsoft to takedown the project or leave it be. See [#29](https://github.com/alexbatalov/fallout2-re/issues/29) for discussion.

## License

The source code in this repository is for education and research purposes only. No commercial use of any kind. That's all I know.
