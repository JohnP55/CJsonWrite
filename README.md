
# CJsonWrite
A JSON library for embedded systems in pure C99 that allows writing and exporting data to JSON.

## How To Use

CJsonWrite abstracts the JSON format into a concept called a JSONNode. Key-value pairs are JSONNodes, array elements are JSONNodes, arrays themselves are JSONNodes, even the root of the tree is a JSONNode.

A JSONNode can have multiple types: Null, Bool, Int, Float, String, Obj, and Array.
JSONNodes such as objects or arrays hold a doubly-linked list pointing to their children nodes (or elements, in the case of an array.)

**An example program + makefile was provided in the /example/ folder, which you can build by running `make` in that folder.**

## Setup

First, you need to setup the library's assert macro and int/float types in `include/CJSONWrite_config.h`. It's really straightforward; there are messages there to guide you, and I even put a default configuration that should be decent for most people (assert.h's `assert` macro and `int32_t/float` for int/float types).

But if your fancy codebase already has an assert macro system, that place is where you can set it up. And if you're using this on a tiny architecture, that's the place where you can choose to use `int16_t` or `int8_t`.
