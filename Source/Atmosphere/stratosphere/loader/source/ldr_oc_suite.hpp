/*
 * Copyright (C) Switch-OC-Suite
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define MTC_CONF_REV 1

namespace ams::ldr::oc {
    #include "mtc_timing_table.hpp"

    constexpr u32 CpuClkOSLimit   = 1785'000;
    constexpr u32 CpuClkOfficial  = 1963'500;
    constexpr u32 CpuVoltOfficial = 1120;
    constexpr u32 GpuClkOfficial  = 1267'200;
    constexpr u32 MemClkOSLimit   = 1600'000;
    constexpr u32 MemClkOSAlt     = 1331'200;
    constexpr u32 MemClkOSClampDn = 1065'600;

    enum MtcConfig {
        AUTO_ADJ_MARIKO_SAFE = 0x0, // Use timings for LPDDR4 ≤3733 Mbps specs, 8Gb density
        AUTO_ADJ_MARIKO_4266 = 0x1, // Use timings for LPDDR4X 4266 Mbps specs, 8Gb density
        ENTIRE_TABLE_ERISTA  = 0x10,
        ENTIRE_TABLE_MARIKO  = 0x11,
    };

    typedef struct {
        u8  magic[4] = {'C', 'U', 'S', 'T'};
        u16 mtcRev   = MTC_CONF_REV;
        u16 mtcConf  = AUTO_ADJ_MARIKO_SAFE;
        u32 cpuMaxClock;
        u32 cpuMaxVolt;
        u32 gpuMaxClock;
        u32 emcMaxClock;
        union {
            EristaMtcTable mtcErista;
            MarikoMtcTable mtcMariko;
        };
    } CustomizeTable;

    inline void PatchOffset(u32* offset, u32 value) { *(offset) = value; }

    inline Result ResultFailure() { return -1; }

    namespace pcv {
        typedef struct {
            s32 c0 = 0;
            s32 c1 = 0;
            s32 c2 = 0;
            s32 c3 = 0;
            s32 c4 = 0;
            s32 c5 = 0;
        } cvb_coefficients;

        typedef struct {
            u64 freq;
            cvb_coefficients cvb_dfll_param;
            cvb_coefficients cvb_pll_param;  // only c0 is reserved
        } cpu_freq_cvb_table_t;

        typedef struct {
            u64 freq;
            cvb_coefficients cvb_dfll_param; // empty, dfll clock source not selected
            cvb_coefficients cvb_pll_param;
        } gpu_cvb_pll_table_t;

        typedef struct {
            u64 freq;
            s32 volt[4] = {0};
        } emc_dvb_dvfs_table_t;

        void Patch(uintptr_t mapped_nso, size_t nso_size);
    }

    namespace ptm {
        typedef struct {
            u32 conf_id;
            u32 cpu_freq_1;
            u32 cpu_freq_2;
            u32 gpu_freq_1;
            u32 gpu_freq_2;
            u32 emc_freq_1;
            u32 emc_freq_2;
            u32 padding;
        } perf_conf_entry;

        void Patch(uintptr_t mapped_nso, size_t nso_size);
    }
}