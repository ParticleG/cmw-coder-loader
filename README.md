# cmw-coder-loader

This is a program that uses Microsoft's [Detours](https://github.com/microsoft/Detours) library to inject a DLL into a
target executable and force process to load the DLL.

## Overview

The format of a typical Windows PE binary file (normally a `.exe` file) look like this:

|            **DOS Header**             |
|:-------------------------------------:|
|            **PE (W/COFF)**            |
|   **.text Section**<br>Program Code   |
| **.data Section**<br>Initialized Data |
|  **.idata Section**<br>Import Table   |
|  **.edata Section**<br>Export Table   |
|           **Debug Symbols**           |

Detours creates a new .detours section between the export table and the debug symbols. The new section contains a
detours header record and a copy of the original PE header. If modifying the import table, Detours creates the new
import table, appends it to the copied PE header, then modifies the original PE header to point to the new import table.
Finally, Detours writes any user payloads at the end of the .detours section. The modified PE file may look like this:

|                                          **DOS Header**                                          |
|:------------------------------------------------------------------------------------------------:|
|                       **PE (W/COFF)**<br>_Modified_ Using .detour Section                        |
|                                **.text Section**<br>Program Code                                 |
|                              **.data Section**<br>Initialized Data                               |
|                           **.idata Section**<br>_Unused_ Import Table                            |
|                                **.edata Section**<br>Export Table                                |
| **.detours Section**<br>detour header<br>original PE header<br>new import table<br>user payloads |
|                                        **Debug Symbols**                                         |

Detours can reverse modifications to the Windows binary by restoring the original PE header from the .detours section
and removing the .detours section.

## Flowchart

```mermaid
flowchart TB
    programStart([Program Start]) --> findExe[Find desired executable] --> checkDetourSection{{Check for .detours section}}
    checkDetourSection -- No --> injectDetour[Inject .detours section] --> loadDll[Load DLL]
    checkDetourSection -- Yes --> removeDetour[Remove .detours section] --> restorePE[Restore PE header]
```