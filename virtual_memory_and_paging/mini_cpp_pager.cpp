#include <iostream>
#include <vector>
#include <stdexcept>
#include <functional>
#include <iomanip>

using namespace std;

// Define a Page Table Entry (PTE) struct
struct PTE {
    bool present = false;
    uint8_t pfn = 0;
    bool can_write = false;
};

// Define a Translation Lookaside Buffer (TLB) entry struct
struct TLBEntry {
    bool valid = false;
    uint8_t vpn = 0;
    uint8_t pfn = 0;
};

// Constants for virtual memory system
static constexpr int PAGE_SIZE  = 256;
static constexpr int VPN_BITS   = 8;
static constexpr int OFFSET_BIT = 8;
static constexpr int PFN_BITS   = 4;

// Function to tranlate a Virtual Address (VA) into a Physical Address (PA)
uint16_t translateVA(uint16_t va,
                    vector<PTE>& pageTable,
                    vector<TLBEntry>& tlb,
                    bool isWrite,
                    function<uint8_t(uint8_t)> pageFaultHandler
) {
    // Extract VPN (upper 8 bits) and offset (lower 8 bits) from the VA
    uint8_t vpn = (va >> OFFSET_BIT) & 0xFF;
    uint8_t off = va & 0xFF;

    // --- Step 1: TLB lookup ---
    for (auto& e : tlb) {
        if (e.valid && e.vpn == vpn) {
            uint8_t pfn = e.pfn & 0x0F;
            uint16_t pa = (uint16_t(pfn) << OFFSET_BIT) | off;
            return pa;
        }
    }

    // --- Step 2: Page table lookup ---
    PTE &pte = pageTable[vpn];
    if (!pte.present) {
        uint8_t newPFN = pageFaultHandler(vpn);
        pte.present = true;
        pte.pfn = newPFN & 0x0F;
        if (!tlb.empty()) {
            size_t idx = vpn % tlb.size();
            tlb[idx] = {true, vpn, pte.pfn};
        }
    }

    // Check protection for write access
    if (isWrite && !pte.can_write) {
        throw runtime_error("Protection fault: write to read-only page");
    }

    // Compute physical address from PFN + offset
    uint16_t pa = (uint16_t(pte.pfn & 0x0F) << OFFSET_BIT) | off;

    // --- Step 3: Fill TLB on page table hit ---
    if (!tlb.empty()) {
        size_t idx = vpn % tlb.size();
        tlb[idx] = {true, vpn, pte.pfn};
    }
    return pa;
}

int main() {
    // Build a 256-entry page table (virtual pages = 0-255)
    vector<PTE> pt(256);
    pt[0] = {true, 5, false};
    pt[1] = {true, 2, true};
    pt[2] = {true, 8, false};
    // VPN 2 is absent, will trigger page fault

    // Create a tiny 4-entry TLB
    vector<TLBEntry> tlb(4);
    tlb[1] = {true, 1, 2}; // pre-cache: VPN 1 -> PFN 2

    // Define a page fault handler(lambda)
    auto loader = [&](uint8_t vpn) -> uint8_t {
        if (vpn == 2) {
            pt[2].can_write = true;
            return 9;
        }
        throw runtime_error("Page fault on unmapped VPN (no backing file)");
    };

    // Helper lambda to test translations
    auto tryTranslate = [&](uint16_t va, bool isWrite) {
        try {
            uint16_t pa = translateVA(va, pt, tlb, isWrite, loader);
            cout << hex << uppercase
                 << "VA 0x" << setw(4) << setfill('0') << va
                 << (isWrite ? " (W)" : " R)")
                 <<" -> PA 0x" << setw(3) << (pa & 0xFFF) << "\n";
        } catch (const exception& e) {
            cout << "VA 0x" << hex << uppercase << setw(4) << setfill('0') << va
            << (isWrite ? " (W)" : " (R)")
            << " -> fault: " << e.what() << "\n";
        }
    };

    // Test cases
    tryTranslate(0x0137, false);
    tryTranslate(0x00FA, false);
    tryTranslate(0x00F2, true);
    tryTranslate(0x02A0, false);
    tryTranslate(0x02B1, true);

    return 0;
}