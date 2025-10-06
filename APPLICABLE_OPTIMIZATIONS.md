# Applicable Optimizations from SNES9x GX Refactoring

This document identifies optimization opportunities in FCE Ultra GC that match patterns already successfully refactored in SNES9x GX.

## Summary

Based on analysis of the REFACTORING_NOTES from SNES9x GX, **5 out of 8 refactoring patterns are directly applicable** to this codebase:

✅ **Completed** (1):
1. ~~File Operations - Device Mounting~~ ✅ Done (commit 0a480e2)

✅ **Applicable** (4):
1. File Browser - Device Auto-Detection
2. File Browser - File Extension Validation  
3. Menu - Switch Statement Lookups
4. Preferences - XML Save/Load

❌ **Not Applicable** (3):
1. Input - ResetControls (function doesn't exist in this codebase)
2. Filter - Render Filters (SNES9x specific feature)
3. SNES9x - SuperFX/Interpolation (SNES9x specific features)

⚠️ **Partially Applicable** (1):
1. Video - FindVideoMode (exists but with different video modes)

---

## 1. File Browser - Device Auto-Detection ✅ HIGH PRIORITY

**File**: `source/filebrowser.cpp`  
**Functions**: `autoLoadMethod()`, `autoSaveMethod()`  
**Lines**: 56-83, 90-119

### Current Code Pattern
Repetitive if-else chains for device detection:
```cpp
int autoLoadMethod() {
    int device = DEVICE_AUTO;
    
    if(ChangeInterface(DEVICE_SD, SILENT))
        device = DEVICE_SD;
    else if(ChangeInterface(DEVICE_USB, SILENT))
        device = DEVICE_USB;
    else if(ChangeInterface(DEVICE_SD_SLOTA, SILENT))
        device = DEVICE_SD_SLOTA;
    // ... 5 more else-if blocks
    
    return device;
}

int autoSaveMethod(bool silent) {
    // Almost identical 7 else-if blocks
}
```

### Recommended Refactoring
Use priority arrays with loop-based detection (from REFACTORING_NOTES_FILEBROWSER.md):

```cpp
static const int loadDevicePriority[] = {
    DEVICE_SD, DEVICE_USB, DEVICE_SD_SLOTA, 
    DEVICE_SD_SLOTB, DEVICE_SD_PORT2, 
    DEVICE_SD_GCLOADER, DEVICE_DVD, DEVICE_SMB
};

static const int saveDevicePriority[] = {
    DEVICE_SD, DEVICE_USB, DEVICE_SD_SLOTA,
    DEVICE_SD_SLOTB, DEVICE_SD_PORT2,
    DEVICE_SD_GCLOADER, DEVICE_SMB
};

int autoLoadMethod() {
    const int numDevices = sizeof(loadDevicePriority) / sizeof(loadDevicePriority[0]);
    
    for (int i = 0; i < numDevices; i++) {
        if (ChangeInterface(loadDevicePriority[i], SILENT)) {
            return loadDevicePriority[i];
        }
    }
    return DEVICE_AUTO;
}
```

### Benefits
- **Lines reduced**: ~41 lines → ~30 lines
- **Maintainability**: Device priority changes = reorder array only
- **Readability**: Clear priority order at a glance
- **Error reduction**: Single point of maintenance

### Expected Impact
- Net change: ~20 lines added (cleaner structure)
- If-else chains eliminated: 2 (15 total branches)

---

## 2. File Browser - File Extension Validation ✅ MEDIUM PRIORITY

**File**: `source/filebrowser.cpp`  
**Function**: `IsValidROM()`  
**Lines**: 357-400

### Current Code Pattern
Multiple repetitive strcasecmp calls:
```cpp
if (
    strcasecmp(p, ".nes") == 0 ||
    strcasecmp(p, ".fds") == 0 ||
    strcasecmp(p, ".nsf") == 0 ||
    strcasecmp(p, ".unf") == 0 ||
    strcasecmp(p, ".nez") == 0 ||
    strcasecmp(p, ".unif") == 0
)
{
    return true;
}
```

### Recommended Refactoring
Helper function with extension array (from REFACTORING_NOTES_FILEBROWSER.md):

```cpp
static const char* validRomExtensions[] = {
    ".nes", ".fds", ".nsf", ".unf", ".nez", ".unif", ".gba"
};

static bool IsValidExtension(const char* ext) {
    if (!ext) return false;
    
    const int numExtensions = sizeof(validRomExtensions) / sizeof(validRomExtensions[0]);
    for (int i = 0; i < numExtensions; i++) {
        if (strcasecmp(ext, validRomExtensions[i]) == 0)
            return true;
    }
    return false;
}

// Usage in IsValidROM():
if (IsValidExtension(p))
    return true;
```

### Benefits
- **Centralization**: All valid extensions in one array
- **Reusability**: Helper function can be used elsewhere
- **Maintainability**: Add new extension = add to array only

### Expected Impact
- strcasecmp calls reduced: 6+ → 1 loop
- More maintainable structure

---

## 3. File Operations - Device Mounting ✅ COMPLETED

**File**: `source/fileop.cpp`  
**Function**: `MountFAT()`  
**Status**: ✅ Implemented in commit 0a480e2

### Implementation Summary
Replaced large switch statement with device mapping table and lookup function.

**Changes Made**:
- Added `DeviceMapping` struct to map device IDs to names and disc interfaces
- Created platform-specific `deviceMappings[]` arrays (Wii vs GameCube)
- Added `GetDeviceMapping()` helper function for device lookup
- Refactored `MountFAT()` to use mapping table instead of switch statement

### Results Achieved
- **sprintf calls eliminated**: 12 (2 per case × 6 cases)
- **Stack space saved**: 20 bytes (removed 2× char[10] buffers)
- **Platform separation**: Wii/GameCube differences isolated in mapping tables only
- **Type safety**: Compile-time checking of disc interface pointers
- **Maintainability**: Device info centralized in one data structure

### Code Pattern Applied
```cpp
struct DeviceMapping {
    int device;
    const char* name;
    const char* name2;
    DISC_INTERFACE** disc;
};

#ifdef HW_RVL
static DeviceMapping deviceMappings[] = {
    {DEVICE_SD, "sd", "sd:", &sd},
    {DEVICE_USB, "usb", "usb:", &usb}
};
#else
static DeviceMapping deviceMappings[] = {
    {DEVICE_SD_SLOTA, "carda", "carda:", &carda},
    {DEVICE_SD_SLOTB, "cardb", "cardb:", &cardb},
    {DEVICE_SD_PORT2, "port2", "port2:", &port2},
    {DEVICE_SD_GCLOADER, "gcloader", "gcloader:", &gcloader}
};
#endif

static const DeviceMapping* GetDeviceMapping(int device) {
    const int numDevices = sizeof(deviceMappings) / sizeof(deviceMappings[0]);
    for (int i = 0; i < numDevices; i++) {
        if (deviceMappings[i].device == device)
            return &deviceMappings[i];
    }
    return NULL;
}
```

---

## 4. Menu - Switch Statement Lookups ✅ MEDIUM PRIORITY

**File**: `source/menu.cpp`  
**Functions**: Multiple menu configuration functions  
**Lines**: 3515-3557 (and others)

### Current Code Pattern
Switch statements mapping integer values to display strings:
```cpp
switch(GCSettings.TurboModeButton) {
    case 0: sprintf(options.value[1], "Default (Right Stick)"); break;
    case 1: sprintf(options.value[1], "A"); break;
    case 2: sprintf(options.value[1], "B"); break;
    // ... 12 more cases
    case 14: sprintf(options.value[1], "Minus"); break;
}

switch(GCSettings.GamepadMenuToggle) {
    case 0: sprintf(options.value[2], "Default (All Enabled)"); break;
    case 1: sprintf(options.value[2], "Home / Right Stick"); break;
    case 2: sprintf(options.value[2], "L+R+Start / 1+2+Plus"); break;
}
```

### Recommended Refactoring
Lookup tables with helper function (from REFACTORING_NOTES_MENU.md):

```cpp
static const char* turboButtonNames[] = {
    "Default (Right Stick)", "A", "B", "X", "Y",
    "L", "R", "ZL", "ZR", "Z", "C", "1", "2",
    "Plus", "Minus"
};

static const char* gamepadMenuToggleNames[] = {
    "Default (All Enabled)",
    "Home / Right Stick",
    "L+R+Start / 1+2+Plus"
};

static inline const char* GetLookupString(
    const char** table, 
    int index, 
    int maxIndex, 
    const char* defaultStr = "Unknown"
) {
    return (index >= 0 && index < maxIndex) ? table[index] : defaultStr;
}

// Usage:
sprintf(options.value[1], "%s", 
    GetLookupString(turboButtonNames, GCSettings.TurboModeButton, 15));
sprintf(options.value[2], "%s",
    GetLookupString(gamepadMenuToggleNames, GCSettings.GamepadMenuToggle, 3));
```

### Additional Candidates
Other switch statements found in menu.cpp that could benefit:
- `hideoverscan` (lines 3720-3725): 4 cases
- Video render mode (lines 3705-3713): 5 cases  
- Timing modes (various locations)

### Benefits
- **Readability**: All option strings visible in one place
- **Maintainability**: Adding new option = update one array
- **Consistency**: Single helper function handles all lookups
- **Conciseness**: Single line replaces 15+ line switch statements

### Expected Impact
- Lines removed: ~109 (for all applicable switches)
- Lines added: ~86
- Net savings: ~23 lines
- Switch cases eliminated: 44+

---

## 5. Preferences - XML Save/Load ✅ HIGH PRIORITY

**File**: `source/preferences.cpp`  
**Functions**: `preparePrefsData()`, `decodePrefsData()`  
**Lines**: 114-180 (save), 267-350+ (load)

### Current Code Pattern
Repetitive createXMLSetting and loadXMLSetting calls:
```cpp
// Save (preparePrefsData)
createXMLSetting("AutoLoad", "Auto Load", toStr(GCSettings.AutoLoad));
createXMLSetting("AutoSave", "Auto Save", toStr(GCSettings.AutoSave));
createXMLSetting("LoadMethod", "Load Method", toStr(GCSettings.LoadMethod));
// ... 40+ more lines

// Load (decodePrefsData)
loadXMLSetting(&GCSettings.AutoLoad, "AutoLoad");
loadXMLSetting(&GCSettings.AutoSave, "AutoSave");
loadXMLSetting(&GCSettings.LoadMethod, "LoadMethod");
// ... 40+ more lines
```

### Recommended Refactoring
Single configuration table (from REFACTORING_NOTES_PREFS.md):

```cpp
enum SettingType {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING
};

struct SettingInfo {
    const char* name;
    const char* description;
    SettingType type;
    void* ptr;
    int maxSize;
    const char* section;
    const char* sectionDesc;
    bool skipOnPlatform;
};

static const SettingInfo settingsConfig[] = {
    // File Settings
    {"AutoLoad", "Auto Load", TYPE_INT, &GCSettings.AutoLoad, 0, "File", "File Settings", false},
    {"AutoSave", "Auto Save", TYPE_INT, &GCSettings.AutoSave, 0, "File", "File Settings", false},
    {"LoadMethod", "Load Method", TYPE_INT, &GCSettings.LoadMethod, 0, "File", "File Settings", false},
    {"SaveMethod", "Save Method", TYPE_INT, &GCSettings.SaveMethod, 0, "File", "File Settings", false},
    {"LoadFolder", "Load Folder", TYPE_STRING, &GCSettings.LoadFolder, sizeof(GCSettings.LoadFolder), "File", "File Settings", false},
    // ... rest of settings
};

// Both save and load functions iterate through the configuration table
```

### Benefits
- **Single source of truth**: All settings metadata in one place
- **Synchronization**: No more keeping save and load functions in sync manually
- **Type safety**: Explicit type specification (INT, FLOAT, STRING)
- **Maintainability**: Add new setting = add 1 line to table (not 2 separate functions)

### Expected Impact
- Lines removed: ~121
- Lines added: ~131
- Net change: +10 lines (but eliminates duplication)
- Practical improvement: Eliminates need to update two functions for each setting change

---

## 6. Video - FindVideoMode ⚠️ PARTIALLY APPLICABLE

**File**: `source/gcvideo.cpp`  
**Function**: `FindVideoMode()`  
**Lines**: 443-477

### Current Code Pattern
Switch statement for video mode selection:
```cpp
static GXRModeObj * FindVideoMode() {
    GXRModeObj * mode;
    
    switch(GCSettings.videomode) {
        case 1: // NTSC (480i)
            mode = &TVNtsc480IntDf;
            break;
        case 2: // Progressive (480p)
            mode = &TVNtsc480Prog;
            break;
        case 3: // PAL (50Hz)
            mode = &TVPal528IntDf;
            break;
        case 4: // PAL (60Hz)
            mode = &TVEurgb60Hz480IntDf;
            break;
        default:
            mode = VIDEO_GetPreferredMode(NULL);
            // ... special handling
            break;
    }
}
```

### Why Partially Applicable
- Similar pattern to REFACTORING_NOTES_VIDEO.md
- **However**: Only 4 video modes vs 6 in SNES9x GX
- **Note**: Uses `TVPal528IntDf` instead of `TVPal576IntDfScale`
- Default case has special mode substitution logic

### Potential Refactoring
Could use lookup table, but benefits are marginal given smaller size:

```cpp
static GXRModeObj* videoModeTable[] = {
    NULL,                       // case 0: Auto
    &TVNtsc480IntDf,           // case 1: NTSC (480i)
    &TVNtsc480Prog,            // case 2: Progressive (480p)
    &TVPal528IntDf,            // case 3: PAL (50Hz)
    &TVEurgb60Hz480IntDf       // case 4: PAL (60Hz)
};
```

### Recommendation
**OPTIONAL** - Only 4 cases, so the benefit is smaller than in SNES9x GX (which had 6 cases). The switch statement is already quite readable. This refactoring would save only ~6 lines and may not be worth the effort.

---

## Implementation Priority

### Completed ✅
1. ~~**File Operations - Device Mounting**~~ - Eliminates sprintf calls, improves type safety (commit 0a480e2)

### High Priority (Most Impact)
2. **File Browser - Device Auto-Detection** - Eliminates repetitive if-else chains
3. **Preferences - XML Save/Load** - Single source of truth, prevents sync issues

### Medium Priority  
4. **Menu - Switch Statement Lookups** - Multiple switches throughout menu.cpp
5. **File Browser - File Extension Validation** - Cleaner extension management

### Optional
6. **Video - FindVideoMode** - Small benefit given only 4 cases

---

## Testing Recommendations

For each refactoring:

1. **Build Test**: Ensure code compiles for both HW_RVL (Wii) and GameCube
2. **Functional Test**: Verify identical behavior to original code
3. **Platform Test**: Test both Wii and GameCube specific code paths
4. **Edge Cases**: Test with invalid inputs, boundary conditions
5. **CI Validation**: Run through existing GitHub CI if available

---

## Estimated Total Impact

If all applicable refactorings are implemented:

- **Files Modified**: 4 (filebrowser.cpp, fileop.cpp, menu.cpp, preferences.cpp)
- **Net Lines Changed**: ~+50 lines (but much cleaner structure)
- **Switch/If-Else Eliminated**: ~60+ control flow branches
- **sprintf Calls Eliminated**: 12+
- **Maintainability**: Significantly improved - data-driven instead of control-flow
- **Type Safety**: Improved with compile-time checks
- **Code Quality**: Centralized configuration, single source of truth

---

## Notes

All these refactoring patterns have already been:
- ✅ Tested and validated in SNES9x GX
- ✅ Documented in REFACTORING_NOTES
- ✅ Proven to maintain backward compatibility
- ✅ Shown to improve maintainability

The code patterns in FCE Ultra GC are nearly identical to those in SNES9x GX, making these refactorings low-risk and high-value.
