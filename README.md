![pciex_demo](https://github.com/user-attachments/assets/2bb17a1d-37d1-4113-ae43-81a93e59dd29)


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
 * topology snapshots
 * ... more to come :)

## Requirements
 * compiler supporting `C++23`
 * `cmake`
 * `hwdata` (for device IDs)

## Building
```
git clone https://github.com/s0nx/pciex.git
cd pciex && mkdir build
cmake -B build -S .
make -C build -j
```

## Usage
There are 3 operation modes:
1. Live mode: display PCI device topology information of the current system.  
   `sudo ./build/pciex -l`

2. Snapshot capture mode: obtain PCI device topology information of the current system  
   and save it to file.  
   `sudo ./build/pciex -c < path/to/snapshot >`

2. Snapshot view mode: parse previously captured snapshot and display PCI device topology  
   `./build/pciex -s < path/to/snapshot >`

(note: modes 1 and 2 require root privileges in order to read the whole configuration space and parse `vmalloced` areas)  

In order to be able to get meaningful v2p mapping info, `kptr_restrict` kernel parameter should set to `1`:   
`echo 1 | sudo tee /proc/sys/kernel/kptr_restrict`, otherwise the addresses would be hashed.  
More information: [kptr_restrict](https://docs.kernel.org/admin-guide/sysctl/kernel.html#kptr-restrict)

Help window can be accessed at any time by pressing `?` key.

## Configuration
_pciex_ can be configured by editing _/etc/pciex/config.json_ file.  
An example configuration file is located in __cfg/__ folder.  
Options are not documented yet, but there are some comments in __src/config.h__

## References
The following libraries are used by this tool:
 * UI is built using [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
 * [CLI11](https://github.com/CLIUtils/CLI11) - command line parsing
 * [glaze](https://github.com/stephenberry/glaze) - json parsing/reflection

## Misc
### Generating compilation database
Add `-DCMAKE_EXPORT_COMPILE_COMMANDS=1` during `cmake` invocation to generate `compile_commands.json`
### Logging
Logging is disabled by default. It can be enabled by modifying configuration json.  
Logs are written to `/tmp/pciex/logs/`
### Examples
An example topology snapshot ( __examples/test_snapshot__ ) can be used to explore the tool.

### Project state
This project is in early development phase. Some features are still being worked on.  
Several PCI capabilities have not been implemented yet.
