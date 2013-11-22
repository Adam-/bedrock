bedrock
=======

Bedrock is a Minecraft Server whose goal is to fully emulate vanilla Minecraft.

## Features

 * Loading existing vanilla generated worlds
 * Multiplayer
 * Creating new players
 * Commands
 * Block mining and placing
 * Inventory
 * Chests
 * Crafting

## TODO
 * Block ticks 
  * Plants, trees
  * Fire
 * Mobs
  * AI
 * Redstone
 * Physics
  * Block physics
  * Liquid physics
  * Projectile physics
 * Lighting
 * Skylight
 * Block light
 * Terrain generation

## Dependencies

  * CMake 2.8 (http://www.cmake.org/)
  * OpenSSL (http://www.openssl.org/)
  * ZLIB (http://www.zlib.net/)
  * Libevent 2 (http://libevent.org/)
  * Libyaml (http://pyyaml.org/wiki/LibYAML)
  * Jansson (http://www.digip.org/jansson/)
  
## Installation

To build bedrock run:

    cmake -DCMAKE_INSTALL_PREFIX=~/bedrock .
    make
    make install

## Configuration

  Copy config.yml.example to config.yml and edit.
