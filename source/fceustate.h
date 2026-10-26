/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * fceustate.h
 *
 * Memory Based Load/Save Snapshot Manager
 ****************************************************************************/

bool SaveState (char * filepath, bool silent);
bool LoadState (char * filepath, bool silent);
// \return 1 if loaded, 0 if there is no Auto Snapshot, -1 if it exists but couldn't be loaded
int LoadStateAuto (bool silent);
bool SavePreviewImg (char * filepath, bool silent);

// The Snapshot this session's game was started from, or last saved to or loaded from:
// the saves screen points at it, and offers to update it. Empty when there is none.
// ClearPlayingFrom() is for when another game is loaded.
const char * GetPlayingFrom ();
void ClearPlayingFrom ();

// Deferred auto-save: SnapshotStateAuto() captures the Snapshot (and screenshot) and
// its destination right now (call it from the thread that owns the emulator, while
// the game is still loaded); WriteStateSnapshot() does the device I/O later, on any
// thread. SnapshotStateAuto() returns nullptr if nothing could be captured. The
// snapshot must be freed with FreeStateSnapshot(), which accepts nullptr.
struct StateSnapshot;
StateSnapshot * SnapshotStateAuto (bool withScreenshot = true);
bool WriteStateSnapshot (StateSnapshot * snapshot, bool silent);
void FreeStateSnapshot (StateSnapshot * snapshot);
