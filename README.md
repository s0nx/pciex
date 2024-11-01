![screenshot2](https://github.com/user-attachments/assets/77084ede-db82-49ee-8f02-c19f53404eb3)

# pciex
terminal-based PCI topology explorer for Linux

## Features
 * whole topology overview in compact or verbose mode
 * visual representation of the device configuration space layout
 * detailed information about each register within header/capability
 * ability to display only needed register information
 * virtual-to-physical address mapping info for `BARs`
 * additional information decoding for `VirtIO` devices
 * quick navigation with keyboard & mouse
 * ... more to come :)

## Requirements
 * compiler supporting `C++20`
 * `cmake`
 * `hwdata` (for device IDs)
 * [fmt](https://github.com/fmtlib/fmt) library

## Installation
### fmt packages
 * Ubuntu 24.04: `libfmt-dev`,`libfmt9`
 * Arch Linux: `fmt`
 * Fedora 40: `fmt`,`fmt-devel`

### Building
```
git clone https://github.com/s0nx/pciex.git
cd pciex && mkdir build
cmake -B build -S .
make -C build -j
```

## Usage
`sudo ./build/pciex`  
(note: the tool requires root privileges in order to read the whole 4k of configuration space and parse `vmalloced` areas)  

In order to be able to get meaningful v2p mapping info, `kptr_restrict` kernel parameter should set to `1`:   
`echo 1 | sudo tee /proc/sys/kernel/kptr_restrict`, otherwise the addresses would be hashed.  
More information: [kptr_restrict](https://docs.kernel.org/admin-guide/sysctl/kernel.html#kptr-restrict)

Help window can be accessed at any time by pressing `?` key.

## References
The following libraries are used by this tool:
 * UI is built using [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
 * [fmt](https://github.com/fmtlib/fmt) is used for text formatting

## Misc
### Generating compilation database
Add `-DCMAKE_EXPORT_COMPILE_COMMANDS=1` during `cmake` invocation to generate `compile_commands.json`
### Logging
By default, logs are written to `/var/log/pciex/pciex.log`
### Project state
This project is in early development phase. Some features are still being worked on.  
Several PCI capabilities have not been implemented yet.
