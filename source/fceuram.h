/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * fceuram.h
 *
 * Memory Based Load/Save Game Save File Manager
 ****************************************************************************/

bool SaveRAM (char * filepath, bool silent);
bool SaveRAMAuto (bool silent);
bool LoadRAM (char * filepath, bool silent);
bool LoadRAMAuto (bool silent);

// Size in bytes of the Game Save File this game saves: 0 when it has none (no battery)
int GetRAMSaveSize ();

// Deferred auto-save: SnapshotRAMAuto() copies the Game Save File and its destination right
// now (call it from the thread that owns the emulator, while the game is still
// loaded); WriteRAMSnapshot() does the device I/O later, on any thread.
// SnapshotRAMAuto() returns nullptr if there is nothing to save. The snapshot
// must be freed with FreeRAMSnapshot(), which accepts nullptr.
struct RAMSnapshot;
RAMSnapshot * SnapshotRAMAuto ();
bool WriteRAMSnapshot (RAMSnapshot * snapshot, bool silent);
void FreeRAMSnapshot (RAMSnapshot * snapshot);