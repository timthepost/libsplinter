// types for splinter snapshots

/*
typedef struct splinter_header_snapshot {
    @brief Magic number (SPLINTER_MAGIC) to verify integrity.
    uint32_t magic;
    @brief Data layout version (SPLINTER_VER).
    uint32_t version;
    @brief Total number of available key-value slots.
    uint32_t slots;
    @brief Maximum size for any single value.
    uint32_t max_val_sz;
    @brief Global epoch, incremented on any write. Used for change detection.
    uint64_t epoch;
    @brief toggle for zeroing out the value region prior to writing there.
    uint32_t auto_vacuum;

    @brief Diagnostics: counts of parse failures reported by clients / harnesses
    uint64_t parse_failures;
    uint64_t last_failure_epoch;
} splinter_header_snapshot_t;
*/

export type SplinterHeaderSnapshot = {
    magic: number,  
    version: number, 
    slots: number, 
    max_val_sz: number, 
    epoch: bigint,  
    auto_vacuum: number,
    parse_failures: bigint,
    last_failure_epoch: bigint
};

/*
typedef struct splinter_slot_snapshot {
    @brief The FNV-1a hash of the key. 0 indicates an empty slot. 
    uint64_t hash;
    @brief Per-slot epoch, incremented on write to this slot. Used for polling. 
    uint64_t epoch;
    @brief Offset into the VALUES region where the value data is stored. 
    uint32_t val_off;
    @brief The actual length of the stored value data (atomic). 
    uint32_t val_len;
    @brief The null-terminated key string. 
    char key[KEY_MAX];
} splinter_slot_snapshot_t;
*/

export type SplinterSlotSnapshot = {
    hash: bigint,
    epoch: bigint,
    val_off: number,
    val_len: number,
    key: string
};