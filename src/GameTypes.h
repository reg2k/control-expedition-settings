#pragma once
#include <cstdint>
#include <cstddef>

//===============================
// Type Defintions
//===============================
namespace GameTypes {
    // size = 0x50
    struct ExpeditionTierDescription {
        void*    vecModEntities;        // 00
        uint64_t unk08;                 // 08 - probably array size and capacity
        uint64_t unk10;                 // 10
        uint32_t activationTimeLimit;   // 18
        uint32_t expeditionTimeLimit;   // 1C
        uint32_t startingPlayerProgressionLevel; // 20
        uint32_t numTicketsToStart;     // 24
        uint64_t rewards;               // 28
        char     unk30[0x18];           // 30
        bool     isUnlockedAtStart;     // 48
        char     pad49[7];              // 49
    };
    static_assert(sizeof(ExpeditionTierDescription) == 0x50, "size error");
    static_assert(offsetof(ExpeditionTierDescription, isUnlockedAtStart) == 0x48, "offset error");

    struct ExpeditionManager {
        void* vtbl;
        uint64_t unk08;
        ExpeditionTierDescription* expeditionTierDescriptionArr; // 10
        //...
    };
    static_assert(offsetof(ExpeditionManager, expeditionTierDescriptionArr) == 0x10, "offset error");
}