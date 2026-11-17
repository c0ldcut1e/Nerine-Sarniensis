# NerineRL

External Rocket League overlay for Linux. I made this for fun to mess around with game reversing, kernel modules and some Linux input/overlay stuff. The code is pretty messy and a bunch of things don't really work, so don't expect much.

## What's in it

Overlay stuff (GTK4 layer-shell + ImGui):

- ball prediction (uses RocketSim)
- boost pad timers
- boost amounts above cars
- car hitboxes
- demo markers
- off-screen player indicators
- radar
- config profiles saved as TOML

Things that probably don't work:

- **Match display**: is supposed to show player ranks/MMR through a parse.bot scraper API (needs an API key). Might be broken.
- **Nexto bot**: runs the Nexto model with libtorch and controls the car through a virtual gamepad. Doesn't work.
- **Controller stuff**: the virtual uinput gamepad and the hidraw gate that hides your real controller from the game. Might not work.

## How it works

The client reads game memory through a small kernel module (`kernel/`) that it talks to over netlink. The module reads the game's memory by walking its page tables. The request/response structs are in `include/nerine/nerine_protocol.hpp`. If you don't want to use the module, uncomment `NO_KERNEL_MODULE` in `client/src/core/nerine_device.cpp` and it'll use `process_vm_readv` instead.

As long as Rocket League and Easy Anti-Cheat are running through Proton, the module should be undetectable, since EAC can't see the Linux kernel from inside Proton. I wouldn't count on that though.

Game objects are found by walking the UE3 `GNames`/`GObjects` tables. Offsets get resolved on startup in `reflection/offset_resolver.cpp`, using the SDK dump in `client/third_party/rlsdk`.

## Building

You need CMake 3.20+, a C++23 compiler, pkg-config, gtk4, gtk4-layer-shell, epoxy, libcurl, fmt and kernel headers. The overlay needs a Wayland compositor that supports layer-shell.

```sh
./client/scripts/fetch-libtorch.sh   # downloads libtorch 2.7.1 (CPU)

cmake -S client -B build/client -DCMAKE_BUILD_TYPE=Release
cmake --build build/client -j

make -C kernel                       # module ends up in build/kernel
```

## Running

1. `sudo insmod build/kernel/nerine.ko`
2. If you want the controller stuff, run `nerine_controller_setup` once
3. Start `build/client/sarniensis` **before** opening Rocket League. It waits for the game, resolves offsets and then opens the overlay
4. Right Shift toggles focus on the menu, Escape closes the overlay

Offsets are tied to whatever game version the SDK was dumped from, so a game update will probably break things.

## Credits

- [RocketSim](https://github.com/ZealanL/RocketSim)
- [Nexto](https://github.com/RLBot/RLBotPack/tree/master/RLBotPack/Necto/Nexto)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [nlohmann/json](https://github.com/nlohmann/json)
- [toml++](https://github.com/marzer/tomlplusplus)
- [libtorch](https://pytorch.org/)

Third-party code keeps its own license, see `client/third_party`.
