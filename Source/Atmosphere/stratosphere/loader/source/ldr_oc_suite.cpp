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

//#define EXPERIMENTAL
#include <stratosphere.hpp>
#include "ldr_oc_suite.hpp"

namespace ams::ldr::oc {
    /* Allocate CustomizeTable in loader.kip (could be customized by user without recompiling) */
    static const volatile CustomizeTable C = {
    /******************** C P U ********************/
    // Max Clock in kHz:
    //   >= 2193000 will enable overvolting (> 1120 mV)
    .cpuMaxClock = 2397000,

    // Max Voltage in mV:
    //   Default voltage: 1120
    //   Haven't tested anything higher than 1220.
    .cpuMaxVolt = 1220,

    /******************** G P U ********************/
    // Max Clock in kHz:
    .gpuMaxClock = 1305600,

    /****************** RAM / EMC ******************/
    // RAM(EMC) Clock in kHz:
    //   1862400, 1894400, 1932800, 1996800, 2064000, 2099200, 2131200
    //   (Other values might work as well)
    // [Warning] RAM overclock could be UNSTABLE and cause severe problems:
    //     - Graphical glitches
    //     - System instabilities
    //     - NAND corruption
    //   1862400/1996800 has been tested stable for all DRAM chips
    .emcMaxClock = 1996800,
    };

    static const bool isMariko = (spl::GetSocType() == spl::SocType_Mariko);

    namespace pcv {
        constexpr u32 CommonDvfsEntryCnt = 32;
        /* CPU */
        constexpr u32 OldCpuDvfsEntryCnt = 18;
        constexpr cpu_freq_cvb_table_t NewCpuTables[] = {
         // OldCpuTables
         // {  204000, {  721589, -12695, 27 }, {} },
         // {  306000, {  747134, -14195, 27 }, {} },
         // {  408000, {  776324, -15705, 27 }, {} },
         // {  510000, {  809160, -17205, 27 }, {} },
         // {  612000, {  845641, -18715, 27 }, {} },
         // {  714000, {  885768, -20215, 27 }, {} },
         // {  816000, {  929540, -21725, 27 }, {} },
         // {  918000, {  976958, -23225, 27 }, {} },
         // { 1020000, { 1028021, -24725, 27 }, { 1120000 } },
         // { 1122000, { 1082730, -26235, 27 }, { 1120000 } },
         // { 1224000, { 1141084, -27735, 27 }, { 1120000 } },
         // { 1326000, { 1203084, -29245, 27 }, { 1120000 } },
         // { 1428000, { 1268729, -30745, 27 }, { 1120000 } },
         // { 1581000, { 1374032, -33005, 27 }, { 1120000 } },
         // { 1683000, { 1448791, -34505, 27 }, { 1120000 } },
         // { 1785000, { 1527196, -36015, 27 }, { 1120000 } },
         // { 1887000, { 1609246, -37515, 27 }, { 1120000 } },
         // { 1963500, { 1675751, -38635, 27 }, { 1120000 } },
            { 2091000, { 1785520, -40523, 27 }, { 1120000 } },
            { 2193000, { 1878755, -42027, 27 }, { 1120000 } },
            { 2295000, { 1975655, -43531, 27 }, { 1120000 } },
            { 2397000, { 2076220, -45036, 27 }, { 1120000 } },
        };
        static_assert(sizeof(NewCpuTables) <= sizeof(cpu_freq_cvb_table_t) * (CommonDvfsEntryCnt - OldCpuDvfsEntryCnt));

        /* GPU */
        constexpr u32 OldGpuDvfsEntryCnt = 17;
        constexpr gpu_cvb_pll_table_t NewGpuTables[] = {
         // OldGpuTables
         // {   76800, {}, {  610000,                                 } },
         // {  153600, {}, {  610000,                                 } },
         // {  230400, {}, {  610000,                                 } },
         // {  307200, {}, {  610000,                                 } },
         // {  384000, {}, {  610000,                                 } },
         // {  460800, {}, {  610000,                                 } },
         // {  537600, {}, {  801688, -10900, -163,  298, -10599, 162 } },
         // {  614400, {}, {  824214,  -5743, -452,  238,  -6325,  81 } },
         // {  691200, {}, {  848830,  -3903, -552,  119,  -4030,  -2 } },
         // {  768000, {}, {  891575,  -4409, -584,    0,  -2849,  39 } },
         // {  844800, {}, {  940071,  -5367, -602,  -60,    -63, -93 } },
         // {  921600, {}, {  986765,  -6637, -614, -179,   1905, -13 } },
         // {  998400, {}, { 1098475, -13529, -497, -179,   3626,   9 } },
         // { 1075200, {}, { 1163644, -12688, -648,    0,   1077,  40 } },
         // { 1152000, {}, { 1204812,  -9908, -830,    0,   1469, 110 } },
         // { 1228800, {}, { 1277303, -11675, -859,    0,   3722, 313 } },
         // { 1267200, {}, { 1335531, -12567, -867,    0,   3681, 559 } },
            { 1305600, {}, { 1374130, -13725, -859,    0,   4442, 576 } },
        };
        static_assert(sizeof(NewGpuTables) <= sizeof(gpu_cvb_pll_table_t) * (CommonDvfsEntryCnt - OldGpuDvfsEntryCnt));

        /* GPU Max Clock asm Pattern:
         *
         * MOV W11, #0x1000 MOV (wide immediate)                0x1000                              0xB (11)
         *  sf | opc |                 | hw  |                   imm16                        |      Rd
         * #31 |30 29|28 27 26 25 24 23|22 21|20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5 |4  3  2  1  0
         *   0 | 1 0 | 1  0  0  1  0  1| 0  0| 0  0  0  1  0  0  0  0  0  0  0  0  0  0  0  0 |0  1  0  1  1
         *
         * MOVK W11, #0xE, LSL#16     <shift>16                    0xE                              0xB (11)
         *  sf | opc |                 | hw  |                   imm16                        |      Rd
         * #31 |30 29|28 27 26 25 24 23|22 21|20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5 |4  3  2  1  0
         *   0 | 1 1 | 1  0  0  1  0  1| 0  1| 0  0  0  0  0  0  0  0  0  0  0  0  1  1  1  0 |0  1  0  1  1
         */
        constexpr u32 gpuOfficialMarikoPattern[2] = {
            0x52820000,
            0x72A001C0
        };
        const     u32 gpuMaxClockMarikoPattern[2] = {
            (gpuOfficialMarikoPattern[0] & 0xFFE00000) | ((C.gpuMaxClock & 0xFFFF) << 5),
            (gpuOfficialMarikoPattern[1] & 0xFFE00000) | (((C.gpuMaxClock >> 16) & 0xFFFF) << 5)
        };
        #define COMPARE_HIGH(val1, val2, bit_div) (((val1 ^ val2) >> bit_div) == 0)

        /* EMC */
        // DvbTable is all about frequency scaling along with CPU core voltage, no need to care about this for now.

        // constexpr emc_dvb_dvfs_table_t EmcDvbTable[6] =
        // {
        //     {  204000, {  637,  637,  637, } },
        //     {  408000, {  637,  637,  637, } },
        //     {  800000, {  637,  637,  637, } },
        //     { 1065600, {  637,  637,  637, } },
        //     { 1331200, {  650,  637,  637, } },
        //     { 1600000, {  675,  650,  637, } },
        // };

        static void MtcTableAutoAdjustMariko(MarikoMtcTable* table, const MarikoMtcTable* ref)
        {
            /* Official Tegra X1 TRM, sign up for nvidia developer program (free) to download:
             *     https://developer.nvidia.com/embedded/dlc/tegra-x1-technical-reference-manual
             *     Section 18.11: MC Registers
             *
             * Retail Mariko: 200FBGA 16Gb DDP LPDDR4X SDRAM x 2
             * x16/Ch, 1Ch/die, Double-die, 2Ch, 1CS(rank), 8Gb density per die
             * 64Mb x 16DQ x 8banks x 2channels = 2048MB (x32DQ) per package
             *
             * Devkit Mariko: 200FBGA 32Gb DDP LPDDR4X SDRAM x 2
             * x16/Ch, 1Ch/die, Quad-die,   2Ch, 2CS(rank), 8Gb density per die
             * X1+ EMC can R/W to both ranks at the same time, resulting in doubled DQ
             * 64Mb x 32DQ x 8banks x 2channels = 4096MB (x64DQ) per package
             *
             * If you have access to LPDDR4(X) specs or datasheets (from manufacturers or Google),
             * you'd better calculate timings yourself rather than relying on following algorithm.
             */

            #define ADJUST_PROP(TARGET, REF)                                                             \
                (u32)(std::ceil(REF + ((C.emcMaxClock-MemClkOSAlt)*(TARGET-REF))/(MemClkOSLimit-MemClkOSAlt)))

            #define ADJUST_PARAM(TARGET, REF)     \
                TARGET = ADJUST_PROP(TARGET, REF);

            #define ADJUST_PARAM_TABLE(TABLE, PARAM, REF)    ADJUST_PARAM(TABLE->PARAM, REF->PARAM)

            #define ADJUST_PARAM_ALL_REG(TABLE, PARAM, REF)                 \
                ADJUST_PARAM_TABLE(TABLE, burst_regs.PARAM, REF)            \
                ADJUST_PARAM_TABLE(TABLE, shadow_regs_ca_train.PARAM, REF)  \
                ADJUST_PARAM_TABLE(TABLE, shadow_regs_rdwr_train.PARAM, REF)

            #define WRITE_PARAM_ALL_REG(TABLE, PARAM, VALUE)\
                TABLE->burst_regs.PARAM = VALUE;            \
                TABLE->shadow_regs_ca_train.PARAM = VALUE;  \
                TABLE->shadow_regs_rdwr_train.PARAM = VALUE;

            ADJUST_PARAM_ALL_REG(table, emc_r2w, ref);
            ADJUST_PARAM_ALL_REG(table, emc_w2r, ref);
            ADJUST_PARAM_ALL_REG(table, emc_r2p, ref);
            ADJUST_PARAM_ALL_REG(table, emc_w2p, ref);
            ADJUST_PARAM_ALL_REG(table, emc_trtm, ref);
            ADJUST_PARAM_ALL_REG(table, emc_twtm, ref);
            ADJUST_PARAM_ALL_REG(table, emc_tratm, ref);
            ADJUST_PARAM_ALL_REG(table, emc_twatm, ref);

            ADJUST_PARAM_ALL_REG(table, emc_rw2pden, ref);

            ADJUST_PARAM_ALL_REG(table, emc_tclkstop, ref);

            ADJUST_PARAM_ALL_REG(table, emc_pmacro_dll_cfg_2, ref); // EMC_DLL_CFG_2_0: level select for VDDA?

            // ADJUST_PARAM_TABLE(table, dram_timings.rl); // not used on Mariko

            ADJUST_PARAM_TABLE(table, la_scale_regs.mc_mll_mpcorer_ptsa_rate, ref);
            ADJUST_PARAM_TABLE(table, la_scale_regs.mc_ptsa_grant_decrement, ref);

            // ADJUST_PARAM_TABLE(table, min_mrs_wait); // not used on LPDDR4X
            // ADJUST_PARAM_TABLE(table, latency); // not used

            /* Patch PLLMB divisors */
            {
                // Calculate DIVM and DIVN (clock divisors)
                // Common PLL oscillator is 38.4 MHz
                // PLLMB_OUT = 38.4 MHz / PLLLMB_DIVM * PLLMB_DIVN
                typedef struct {
                    u8 numerator   : 4;
                    u8 denominator : 4;
                } pllmb_div;

                constexpr pllmb_div div[] = {
                    {3, 4}, {2, 3}, {1, 2}, {1, 3}, {1, 4}, {0, 1}
                };

                constexpr u32 pll_osc_in = 38400;

                u32 divm {}, divn {};
                const u32 remainder = C.emcMaxClock % pll_osc_in;
                for (const auto &index : div) {
                    if (remainder >= pll_osc_in * index.numerator / index.denominator) {
                        divm = index.denominator;
                        divn = C.emcMaxClock / pll_osc_in * divm + index.numerator;
                        break;
                    }
                }

                table->pllmb_divm = divm;
                table->pllmb_divn = divn;
            }

            /* Timings that are available in or can be derived from LPDDR4X datasheet or TRM */
            {
                const bool use_4266_spec = C.mtcConf == AUTO_ADJ_MARIKO_4266;
                // tCK_avg (average clock period) in ns
                const double tCK_avg = 1000'000. / C.emcMaxClock;
                // tRPpb (row precharge time per bank) in ns
                const u32 tRPpb = 18;
                // tRPab (row precharge time all banks) in ns
                const u32 tRPab = 21;
                // tRAS (row active time) in ns
                const u32 tRAS = 42;
                // tRC (ACTIVATE-ACTIVATE command period same bank) in ns
                const u32 tRC = tRPpb + tRAS;
                // tRFCab (refresh cycle time all banks) in ns for 8Gb density
                const u32 tRFCab = 280;
                // tRFCpb (refresh cycle time per bank) in ns for 8Gb density
                const u32 tRFCpb = 140;
                // tRCD (RAS-CAS delay) in ns
                const u32 tRCD = 18;
                // tRRD (Active bank-A to Active bank-B) in ns
                const double tRRD = use_4266_spec ? 7.5 : 10.;
                // tREFpb (average refresh interval per bank) in ns for 8Gb density
                const u32 tREFpb = 488;
                // tREFab (average refresh interval all 8 banks) in ns for 8Gb density
                // const u32 tREFab = tREFpb * 8;
                // #_of_rows per die for 8Gb density
                const u32 numOfRows = 65536;
                // {REFRESH, REFRESH_LO} = max[(tREF/#_of_rows) / (emc_clk_period) - 64, (tREF/#_of_rows) / (emc_clk_period) * 97%]
                // emc_clk_period = dram_clk / 2;
                // 1600 MHz: 5894, but N' set to 6176 (~4.8% margin)
                const u32 REFRESH = u32(std::ceil((double(tREFpb) * C.emcMaxClock / numOfRows * 1.048 / 2 - 64))) / 4 * 4;
                // tPDEX2WR, tPDEX2RD (timing delay from exiting powerdown mode to a write/read command) in ns
                const u32 tPDEX2 = 10;
                // [Guessed] tACT2PDEN (timing delay from an activate, MRS or EMRS command to power-down entry) in ns
                const u32 tACT2PDEN = 14;
                // [Guessed] tPDEX2MRR (timing delay from exiting powerdown mode to MRR command) in ns
                const double tPDEX2MRR = 28.75;
                // [Guessed] tCKE2PDEN (timing delay from turning off CKE to power-down entry) in ns
                const double tCKE2PDEN = 8.5;
                // tXSR (SELF REFRESH exit to next valid command delay) in ns
                const double tXSR = tRFCab + 7.5;
                // tCKE (minimum CKE high pulse width) in ns
                const u32 tCKE = 8;
                // tCKELPD (minimum CKE low pulse width in SELF REFRESH) in ns
                const u32 tCKELPD = 15;
                // [Guessed] tPD (minimum CKE low pulse width in power-down mode) in ns
                const double tPD = 7.5;
                // tFAW (Four-bank Activate Window) in ns
                const u32 tFAW = use_4266_spec ? 30 : 40;

                #define GET_CYCLE_CEIL(PARAM) u32(std::ceil(double(PARAM) / tCK_avg))

                WRITE_PARAM_ALL_REG(table, emc_rc,      GET_CYCLE_CEIL(tRC));
                WRITE_PARAM_ALL_REG(table, emc_rfc,     GET_CYCLE_CEIL(tRFCab));
                WRITE_PARAM_ALL_REG(table, emc_rfcpb,   GET_CYCLE_CEIL(tRFCpb));
                WRITE_PARAM_ALL_REG(table, emc_ras,     GET_CYCLE_CEIL(tRAS));
                WRITE_PARAM_ALL_REG(table, emc_rp,      GET_CYCLE_CEIL(tRPpb));
                WRITE_PARAM_ALL_REG(table, emc_rd_rcd,  GET_CYCLE_CEIL(tRCD));
                WRITE_PARAM_ALL_REG(table, emc_wr_rcd,  GET_CYCLE_CEIL(tRCD));
                WRITE_PARAM_ALL_REG(table, emc_rrd,     GET_CYCLE_CEIL(tRRD));
                WRITE_PARAM_ALL_REG(table, emc_refresh, REFRESH);
                WRITE_PARAM_ALL_REG(table, emc_pre_refresh_req_cnt, REFRESH / 4);
                WRITE_PARAM_ALL_REG(table, emc_pdex2wr, GET_CYCLE_CEIL(tPDEX2));
                WRITE_PARAM_ALL_REG(table, emc_pdex2rd, GET_CYCLE_CEIL(tPDEX2));
                WRITE_PARAM_ALL_REG(table, emc_act2pden,GET_CYCLE_CEIL(tACT2PDEN));
                WRITE_PARAM_ALL_REG(table, emc_cke2pden,GET_CYCLE_CEIL(tCKE2PDEN));
                WRITE_PARAM_ALL_REG(table, emc_pdex2mrr,GET_CYCLE_CEIL(tPDEX2MRR));
                WRITE_PARAM_ALL_REG(table, emc_txsr,    GET_CYCLE_CEIL(tXSR));
                WRITE_PARAM_ALL_REG(table, emc_txsrdll, GET_CYCLE_CEIL(tXSR));
                WRITE_PARAM_ALL_REG(table, emc_tcke,    GET_CYCLE_CEIL(tCKE));
                WRITE_PARAM_ALL_REG(table, emc_tckesr,  GET_CYCLE_CEIL(tCKELPD));
                WRITE_PARAM_ALL_REG(table, emc_tpd,     GET_CYCLE_CEIL(tPD));
                WRITE_PARAM_ALL_REG(table, emc_tfaw,    GET_CYCLE_CEIL(tFAW));
                WRITE_PARAM_ALL_REG(table, emc_trpab,   GET_CYCLE_CEIL(tRPab));
                WRITE_PARAM_ALL_REG(table, emc_trefbw,  REFRESH + 64);

                constexpr u32 MC_ARB_DIV = 4; // ?
                table->burst_mc_regs.mc_emem_arb_timing_rcd     = std::ceil(GET_CYCLE_CEIL(tRCD) / MC_ARB_DIV - 2);
                table->burst_mc_regs.mc_emem_arb_timing_rp      = std::ceil(GET_CYCLE_CEIL(tRPpb) / MC_ARB_DIV - 1);
                table->burst_mc_regs.mc_emem_arb_timing_rc      = std::ceil(std::max(GET_CYCLE_CEIL(tRC), GET_CYCLE_CEIL(tRAS)+GET_CYCLE_CEIL(tRPpb)) / MC_ARB_DIV);
                table->burst_mc_regs.mc_emem_arb_timing_ras     = std::ceil(GET_CYCLE_CEIL(tRAS) / MC_ARB_DIV - 2);
                table->burst_mc_regs.mc_emem_arb_timing_faw     = std::ceil(GET_CYCLE_CEIL(tFAW) / MC_ARB_DIV - 1);
                table->burst_mc_regs.mc_emem_arb_timing_rrd     = std::ceil(GET_CYCLE_CEIL(tRRD) / MC_ARB_DIV - 1);
                table->burst_mc_regs.mc_emem_arb_timing_rap2pre = std::ceil(table->burst_regs.emc_r2p / MC_ARB_DIV);
                table->burst_mc_regs.mc_emem_arb_timing_wap2pre = std::ceil(table->burst_regs.emc_w2p / MC_ARB_DIV);
                table->burst_mc_regs.mc_emem_arb_timing_r2w     = std::ceil(table->burst_regs.emc_r2w / MC_ARB_DIV + 1);
                table->burst_mc_regs.mc_emem_arb_timing_w2r     = std::ceil(table->burst_regs.emc_w2r / MC_ARB_DIV + 1);
                table->burst_mc_regs.mc_emem_arb_timing_rfcpb   = std::ceil(GET_CYCLE_CEIL(tRFCpb) / MC_ARB_DIV + 1); // ?
            }

            #ifdef EXPERIMENTAL
            {
                #define ADJUST_PARAM_ROUND2_ALL_REG(TARGET_TABLE, REF_TABLE, PARAM)                                                        \
                    TARGET_TABLE->burst_regs.PARAM =                                                                                       \
                        ((ADJUST_PROP(TARGET_TABLE->burst_regs.PARAM, REF_TABLE->burst_regs.PARAM) + 1) >> 1) << 1;                        \
                    TARGET_TABLE->shadow_regs_ca_train.PARAM =                                                                             \
                        ((ADJUST_PROP(TARGET_TABLE->shadow_regs_ca_train.PARAM, REF_TABLE->shadow_regs_ca_train.PARAM) + 1) >> 1) << 1;    \
                    TARGET_TABLE->shadow_regs_rdwr_train.PARAM =                                                                           \
                        ((ADJUST_PROP(TARGET_TABLE->shadow_regs_rdwr_train.PARAM, REF_TABLE->shadow_regs_rdwr_train.PARAM) + 1) >> 1) << 1;

                #define TRIM_BIT(IN_BITS, HIGH, LOW)                       \
                    ((IN_BITS >> LOW) & ( (1u << (HIGH - LOW + 1u)) - 1u ))

                #define ADJUST_BIT(TARGET_PARAM, REF_PARAM, HIGH, LOW)                            \
                    ADJUST_PROP(TRIM_BIT(TARGET_PARAM, HIGH, LOW), TRIM_BIT(REF_PARAM, HIGH, LOW))

                #define CLEAR_BIT(BITS, HIGH, LOW)                        \
                    BITS = BITS & ~( ((1u << HIGH) << 1u) - (1u << LOW) );

                #define ADJUST_BIT_ALL_REG_SINGLE_OP(TARGET_TABLE, REF_TABLE, PARAM, HIGH, LOW, OPERATION)                                            \
                    TARGET_TABLE->burst_regs.PARAM =                                                                                                  \
                        (ADJUST_BIT(TARGET_TABLE->burst_regs.PARAM, REF_TABLE->burst_regs.PARAM, HIGH, LOW) << LOW) OPERATION;                        \
                    TARGET_TABLE->shadow_regs_ca_train.PARAM =                                                                                        \
                        (ADJUST_BIT(TARGET_TABLE->shadow_regs_ca_train.PARAM, REF_TABLE->shadow_regs_ca_train.PARAM, HIGH, LOW)) << LOW OPERATION;    \
                    TARGET_TABLE->shadow_regs_rdwr_train.PARAM =                                                                                      \
                        (ADJUST_BIT(TARGET_TABLE->shadow_regs_rdwr_train.PARAM, REF_TABLE->shadow_regs_rdwr_train.PARAM, HIGH, LOW)) << LOW OPERATION;

                #define ADJUST_BIT_ALL_REG_PAIR(TARGET_TABLE, REF_TABLE, PARAM, HIGH1, LOW1, HIGH2, LOW2)                                      \
                    TARGET_TABLE->burst_regs.PARAM =                                                                                           \
                        ADJUST_BIT(TARGET_TABLE->burst_regs.PARAM, REF_TABLE->burst_regs.PARAM, HIGH1, LOW1) << LOW1                           \
                        | ADJUST_BIT(TARGET_TABLE->burst_regs.PARAM, REF_TABLE->burst_regs.PARAM, HIGH2, LOW2) << LOW2;                        \
                    TARGET_TABLE->shadow_regs_ca_train.PARAM =                                                                                 \
                        ADJUST_BIT(TARGET_TABLE->shadow_regs_ca_train.PARAM, REF_TABLE->shadow_regs_ca_train.PARAM, HIGH1, LOW1) << LOW1       \
                        | ADJUST_BIT(TARGET_TABLE->shadow_regs_ca_train.PARAM, REF_TABLE->shadow_regs_ca_train.PARAM, HIGH2, LOW2) << LOW2;    \
                    TARGET_TABLE->shadow_regs_rdwr_train.PARAM =                                                                               \
                        ADJUST_BIT(TARGET_TABLE->shadow_regs_rdwr_train.PARAM, REF_TABLE->shadow_regs_rdwr_train.PARAM, HIGH1, LOW1) << LOW1   \
                        | ADJUST_BIT(TARGET_TABLE->shadow_regs_rdwr_train.PARAM, REF_TABLE->shadow_regs_rdwr_train.PARAM, HIGH2, LOW2) << LOW2;

                /* For latency allowance */
                #define ADJUST_INVERSE(TARGET) (TARGET * (MemClkOSLimit / 1000) / (C.emcMaxClock / 1000))

                /* emc_wdv, emc_wsv, emc_wev, emc_wdv_mask,
                   emc_quse, emc_quse_width, emc_ibdly, emc_obdly,
                   emc_einput, emc_einput_duration, emc_qrst, emc_qsafe,
                   emc_rdv, emc_rdv_mask, emc_rdv_early, emc_rdv_early_mask */
                ADJUST_PARAM_ROUND2_ALL_REG(target_table, ref_table, emc_wdv);
                ADJUST_PARAM_ROUND2_ALL_REG(target_table, ref_table, emc_wsv);
                ADJUST_PARAM_ROUND2_ALL_REG(target_table, ref_table, emc_wev);
                ADJUST_PARAM_ROUND2_ALL_REG(target_table, ref_table, emc_wdv_mask);

                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_quse);
                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_quse_width);

                ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_ibdly, 6,0, | (1 << 28));
                ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_obdly, 5,0, | (1 << 28));

                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_einput);
                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_einput_duration);

                ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_qrst, 6,0, | (6 << 16));
                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_qsafe);

                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_rdv);
                target_table->burst_regs.emc_rdv_mask = target_table->burst_regs.emc_rdv + 2;
                target_table->shadow_regs_ca_train.emc_rdv_mask = target_table->shadow_regs_ca_train.emc_rdv + 2;
                target_table->shadow_regs_rdwr_train.emc_rdv_mask = target_table->shadow_regs_rdwr_train.emc_rdv + 2;

                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_rdv_early);
                target_table->burst_regs.emc_rdv_early_mask = target_table->burst_regs.emc_rdv_early + 2;
                target_table->shadow_regs_ca_train.emc_rdv_early_mask = target_table->shadow_regs_ca_train.emc_rdv_early + 2;
                target_table->shadow_regs_rdwr_train.emc_rdv_early_mask = target_table->shadow_regs_rdwr_train.emc_rdv_early + 2;

                /* emc_pmacro_...,
                   emc_zcal_wait_cnt, emc_mrs_wait_cnt(2),
                   emc_pmacro_autocal_cfg_common, emc_dyn_self_ref_control, emc_qpop, emc_pmacro_cmd_pad_tx_ctrl,
                   emc_tr_timing_0, emc_tr_rdv, emc_tr_qpop, emc_tr_rdv_mask, emc_tr_qsafe, emc_tr_qrst,
                   emc_training_vref_settle */
                /* DDLL values */
                {
                    #define OFFSET_ALL_REG(PARAM)                               \
                        offsetof(MarikoMtcTable, burst_regs.PARAM),             \
                        offsetof(MarikoMtcTable, shadow_regs_ca_train.PARAM),   \
                        offsetof(MarikoMtcTable, shadow_regs_rdwr_train.PARAM)  \

                    /* Section 1: adjust HI bits: BIT 26:16 */
                    const std::vector<uintptr_t> ddll_high = {
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dq_rank1_4),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dq_rank1_5),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_4),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_5),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_4),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_5),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_0),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_1),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_2),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_3),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_4),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3),
                    };
                    for (const auto &offset : ddll_high)
                    {
                        u32 *ddll     = reinterpret_cast<u32 *>(reinterpret_cast<uintptr_t>(table) + offset);
                        u32 *ddll_ref = reinterpret_cast<u32 *>(reinterpret_cast<uintptr_t>(ref) + offset);
                        u16 adjusted_ddll = ADJUST_BIT(*ddll, *ddll_ref, 26,16) & ((1 << (26-16)) - 1);
                        CLEAR_BIT(*ddll, 26,16)
                        *ddll |= adjusted_ddll << 16;
                    }

                    /* Section 2: adjust LOW bits: BIT 10:0 */
                    const std::vector<uintptr_t> ddll_low = {
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dq_rank1_4),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dq_rank1_5),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_0),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_1),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_3),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_4),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_0),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_1),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_3),
                        OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_4),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_0),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_1),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_2),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_3),
                        OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_4),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2),
                        offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3),
                    };
                    for (const auto &offset : ddll_low)
                    {
                        u32 *ddll     = reinterpret_cast<u32 *>(reinterpret_cast<uintptr_t>(table) + offset);
                        u32 *ddll_ref = reinterpret_cast<u32 *>(reinterpret_cast<uintptr_t>(ref) + offset);
                        u16 adjusted_ddll = ADJUST_BIT(*ddll, *ddll_ref, 10,0) & ((1 << 10) - 1);
                        CLEAR_BIT(*ddll, 10,0)
                        *ddll |= adjusted_ddll;
                    }
                }

                ADJUST_BIT_ALL_REG_PAIR(target_table, ref_table, emc_zcal_wait_cnt, 21,16, 10,0)
                ADJUST_BIT_ALL_REG_PAIR(target_table, ref_table, emc_mrs_wait_cnt, 21,16, 10,0)
                ADJUST_BIT_ALL_REG_PAIR(target_table, ref_table, emc_mrs_wait_cnt2, 21,16, 10,0)

                ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_auto_cal_channel, 5,0, | 0xC1E00300)
                ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_pmacro_autocal_cfg_common, 5,0, | 8 << 8)

                ADJUST_BIT_ALL_REG_PAIR(target_table, ref_table, emc_dyn_self_ref_control, 31,31, 15,0)

                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_qpop);

                ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_tr_timing_0, 9,0, | 0x1186100)

                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_tr_rdv);
                target_table->burst_regs.emc_tr_rdv_mask = target_table->burst_regs.emc_tr_rdv + 2;
                target_table->shadow_regs_ca_train.emc_tr_rdv_mask = target_table->shadow_regs_ca_train.emc_tr_rdv + 2;
                target_table->shadow_regs_rdwr_train.emc_tr_rdv_mask = target_table->shadow_regs_rdwr_train.emc_tr_rdv + 2;

                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_tr_qpop);
                ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_tr_qsafe);
                ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_tr_qrst, 6,0, | (6 << 16));

                ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_training_vref_settle, 15,0, | (4 << 16));

                /* External Memory Arbitration Configuration */
                /* BIT 20:16 - EXTRA_TICKS_PER_UPDATE: 0 */
                /* BIT 8:0   - CYCLES_PER_UPDATE: 12(1600MHz), 10(1331.2MHz) */
                ADJUST_PARAM_TABLE(table, burst_mc_regs.mc_emem_arb_cfg, ref);

                /* External Memory Arbitration Configuration: Direction Arbiter: Turns */
                /* BIT 31:24 - W2R_TURN: approx. mc_emem_arb_timing_w2r */
                /* BIT 23:16 - R2W_TURN: approx. mc_emem_arb_timing_r2w */
                /* BIT 15:8  - W2W_TURN: 0 */
                /* BIT 7:0   - R2R_TURN: 0 */
                {
                    u8 w2r_turn = table->burst_mc_regs.mc_emem_arb_timing_w2r;
                    u8 r2w_turn = table->burst_mc_regs.mc_emem_arb_timing_r2w;
                    table->burst_mc_regs.mc_emem_arb_da_turns = w2r_turn << 24 | r2w_turn << 16;
                }

                /* External Memory Arbitration Configuration: Direction Arbiter: Covers */
                /* BIT 23:16 - RCD_W_COVER: 13(1600MHz), 11(1331.2MHz) */
                /* BIT 15:8  - RCD_R_COVER: 8(1600MHz), 7(1331.2MHz) */
                /* BIT 7:0   - RC_COVER: approx. mc_emem_arb_timing_rc, 12(1600MHz), 9(1331.2MHz) */
                {
                    u32 param_max = table->burst_mc_regs.mc_emem_arb_da_covers;
                    u32 param_ref = ref->burst_mc_regs.mc_emem_arb_da_covers;
                    u8 rcd_w_cover = ADJUST_BIT(param_max, param_ref, 23,16);
                    u8 rcd_r_cover = (ADJUST_BIT(param_max, param_ref, 23,16) + 3) / 2;
                    u8 rc_cover    = table->burst_mc_regs.mc_emem_arb_timing_rc;
                    table->burst_mc_regs.mc_emem_arb_da_covers = rcd_w_cover << 16 | rcd_r_cover << 8 | rc_cover;
                }

                /* External Memory Arbitration Configuration: Miscellaneous Thresholds (0) */
                /* BIT 20:16 - PRIORITY_INVERSION_ISO_THRESHOLD: 12(1600MHz), 10(1331.2MHz) */
                /* BIT 14:8  - PRIORITY_INVERSION_THRESHOLD: 36(1600MHz), 30(1331.2MHz) */
                /* BIT 7:0   - BC2AA_HOLDOFF_THRESHOLD: set to mc_emem_arb_timing_rc */
                {
                    u32 param_max = table->burst_mc_regs.mc_emem_arb_misc0;
                    u32 param_ref = ref->burst_mc_regs.mc_emem_arb_misc0;
                    u8 priority_inversion_iso_threshold = ADJUST_BIT(param_max, param_ref, 20,16);
                    u8 priority_inversion_threshold     = 3 * ADJUST_BIT(param_max, param_ref, 20,16);
                    u8 bc2aa_holdoff_threshold          = table->burst_mc_regs.mc_emem_arb_timing_rc;
                    CLEAR_BIT(table->burst_mc_regs.mc_emem_arb_misc0, 20,16)
                    CLEAR_BIT(table->burst_mc_regs.mc_emem_arb_misc0, 14,8)
                    CLEAR_BIT(table->burst_mc_regs.mc_emem_arb_misc0, 7,0)
                    table->burst_mc_regs.mc_emem_arb_misc0 |=
                        (priority_inversion_iso_threshold << 16 | priority_inversion_threshold << 8 | bc2aa_holdoff_threshold);
                }

                /* Latency allowance settings */
                {
                    /* Section 1: adjust write latency */
                    /* BIT 23:16 - ALLOWANCE_WRITE: 128(1600MHz), 153(1331.2MHz) */
                    const uint32_t latency_write_offset[] = {
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_xusb_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_xusb_1),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_tsec_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_sdmmca_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_sdmmcaa_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_sdmmc_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_sdmmcab_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_ppcs_1),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_mpcore_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_avpc_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_gpu_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_gpu2_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_nvenc_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_nvdec_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_vic_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_isp2_1),
                    };
                    for (uint32_t i = 0; i < sizeof(latency_write_offset)/sizeof(uint32_t); i++)
                    {
                        uint32_t *latency = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(target_table) + latency_write_offset[i]);
                        CLEAR_BIT(*latency, 23,16)
                        *latency |= ADJUST_INVERSE(128) << 16;
                    }

                    /* Section 2: adjust read latency */
                    /* BIT 7:0   - ALLOWANCE_READ */
                    const uint32_t latency_read_offset[] = {
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_hc_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_hc_1),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_gpu_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_gpu2_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_vic_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_vi2_0),
                        offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_isp2_1),
                    };
                    for (uint32_t i = 0; i < sizeof(latency_read_offset)/sizeof(uint32_t); i++)
                    {
                        uint32_t *latency = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(target_table) + latency_read_offset[i]);
                        uint8_t adjusted_latency = ADJUST_INVERSE(TRIM_BIT(*latency, 7,0));
                        CLEAR_BIT(*latency, 7,0)
                        *latency |= adjusted_latency;
                    }
                }

                /* PLLM and PLLMB control */
                {
                    /*
                     * CLK_RST_CONTROLLER_PLLM_SS_CTRL1:
                     * BIT 31:16 : PLLM_SDM_SSC_MAX
                     * BIT 15:0  : PLLM_SDM_SSC_MIN
                     *
                     * CLK_RST_CONTROLLER_PLLM_SS_CTRL2:
                     * BIT 31:16 : PLLM_SDM_SSC_STEP
                     * BIT 15:0  : PLLM_SDM_DIN
                     *
                     * pllm(b)_ss_ctrl1:
                     *   1365,    342 (1600MHz)
                     * 0xFAAB, 0xF404 (1331MHz)
                     *
                     * pllm(b)_ss_ctrl2:
                     *      2,   1365 (1600MHz)
                     *      6, 0xFAAB (1331MHz)
                     *
                     * No need to care about this if Spread Spectrum (SS) is disabled
                     */
                    // Disable PLL Spread Spectrum Control (degrades performance)
                    table->pll_en_ssc = 0;
                    table->pllm_ss_cfg = 1 << 30;
                }

                /* EMC misc. configuration */
                {
                    /* ? Command Trigger: MRW, MRW2: MRW_OP - [PMC] data to be written ?
                     *
                     *  EMC_MRW: MRW_OP
                     * 1600 MHz: 0x54
                     * 1331 MHz: 0x44
                     * 1065 MHz: 0x34
                     *  800 MHz: 0x34
                     *  665 MHz: 0x14
                     *  408 MHz: 0x04
                     *  204 MHz: 0x04
                     *
                     * EMC_MRW2: MRW2_OP
                     * 1600 MHz: 0x2D 45 5*9
                     * 1331 MHz: 0x24 36 4*9
                     * 1065 MHz: 0x1B 27 3*9
                     *  800 MHz: 0x12 18 2*9
                     *  665 MHz: 0x09  9 1*9
                     *  408 MHz: 0x00
                     *  204 MHz: 0x00
                     */
                    {

                    }

                    /* EMC_CFG_2 */
                    /* BIT 5:3  - ZQ_EXTRA_DELAY: 6(1600MHz), 5(1331.2MHz), max possible value: 7 */
                    {
                        CLEAR_BIT(target_table->emc_cfg_2, 5,3)
                        target_table->emc_cfg_2 |= 7 << 3;
                    }
                }
            }
            #endif
        }

        static Result CpuClockVddHandler(u32* ptr) {
            u32 value_next2 = *(ptr + 2);
            constexpr u32 cpuClockVddCpuPatternNext = 0;
            if (value_next2 != cpuClockVddCpuPatternNext)
            {
                return ResultFailure();
            }

            PatchOffset(ptr, C.cpuMaxClock);
            return ResultSuccess();
        }

        static Result CpuDvfsHandler(cpu_freq_cvb_table_t* entry_1963, uintptr_t nso_end_offset) {
            cpu_freq_cvb_table_t* entry_free = entry_1963 + 1;
            cpu_freq_cvb_table_t* entry_204  = entry_free - 18;
            uintptr_t entry_end_offset = reinterpret_cast<uintptr_t>(entry_free) + sizeof(NewCpuTables) - sizeof(u32);

            if (   entry_end_offset >= nso_end_offset
                || *(reinterpret_cast<u32 *>(entry_free))       != 0
                || *(reinterpret_cast<u32 *>(entry_204))        != 204'000
                || *(reinterpret_cast<u32 *>(entry_end_offset)) != 0 )
            {
                return ResultFailure();
            }

            std::memcpy(reinterpret_cast<void *>(entry_free), NewCpuTables, sizeof(NewCpuTables));

            // Patch CPU max volt in CPU dvfs table
            cpu_freq_cvb_table_t* entry_current = entry_1963 + sizeof(NewCpuTables) / sizeof(cpu_freq_cvb_table_t);
            if (entry_current->cvb_pll_param.c0 != CpuVoltOfficial * 1000)
                return ResultFailure();

            while (entry_current->cvb_pll_param.c0 == CpuVoltOfficial * 1000)
            {
                PatchOffset(reinterpret_cast<u32 *>(std::addressof(entry_current->cvb_pll_param)), C.cpuMaxVolt * 1000);
                entry_current--;
            }

            return ResultSuccess();
        }

        static Result GpuDvfsHandler(gpu_cvb_pll_table_t* entry_1267, uintptr_t nso_end_offset) {
            gpu_cvb_pll_table_t* entry_free = entry_1267 + 1;
            gpu_cvb_pll_table_t* entry_76_8 = entry_free - 17;
            uintptr_t entry_end_offset = reinterpret_cast<uintptr_t>(entry_free) + sizeof(NewGpuTables) - sizeof(u32);

            if (   entry_end_offset >= nso_end_offset
                || *(reinterpret_cast<u32 *>(entry_free))       != 0
                || *(reinterpret_cast<u32 *>(entry_76_8))       != 76'800
                || *(reinterpret_cast<u32 *>(entry_end_offset)) != 0 )
            {
                return ResultFailure();
            }

            std::memcpy(reinterpret_cast<void *>(entry_free), NewGpuTables, sizeof(NewGpuTables));
            return ResultSuccess();
        }

        static Result CpuVoltRangeHandler(u32* ptr) {
            u32 value_cpu_min_volt = *(ptr - 1);

            switch (value_cpu_min_volt) {
                case 800:
                case 637:
                case 620:
                case 610:
                    PatchOffset(ptr, C.cpuMaxVolt);
                    return ResultSuccess();
                default:
                    return ResultFailure();
            }
        }

        static Result GpuMaxClockHandler(u32* ptr) {
            u32  value      = *(ptr);
            u32* ptr_next   = ptr + 1;
            u32  value_next = *(ptr_next);
            if (COMPARE_HIGH(value_next, gpuOfficialMarikoPattern[1], 5))
            {
                u32 reg_id      = value      & ((1 << 5) - 1);
                u32 reg_id_next = value_next & ((1 << 5) - 1);
                if (reg_id == reg_id_next)
                {
                    PatchOffset(ptr     , gpuMaxClockMarikoPattern[0] | reg_id);
                    PatchOffset(ptr_next, gpuMaxClockMarikoPattern[1] | reg_id);

                    return ResultSuccess();
                }
            }
            return ResultFailure();
        }

        static Result MtcTableHandler(u32* ptr) {
            bool replace_entire_table = (C.mtcConf == ENTIRE_TABLE_ERISTA) || (C.mtcConf == ENTIRE_TABLE_MARIKO);
            if (isMariko) {
                MarikoMtcTable* const mtc_table_max = reinterpret_cast<MarikoMtcTable *>(ptr - offsetof(MarikoMtcTable, rate_khz) / sizeof(u32));
                MarikoMtcTable* const mtc_table_alt = mtc_table_max - 1;
                constexpr u32 mtc_mariko_rev = 3;
                if (   mtc_table_max->rev != mtc_mariko_rev
                    || mtc_table_alt->rev != mtc_mariko_rev
                    || mtc_table_alt->rate_khz != MemClkOSAlt )
                    return ResultFailure();

                MarikoMtcTable* const table = const_cast<MarikoMtcTable *>(std::addressof(C.mtcMariko));

                if (replace_entire_table) {
                    std::memcpy(reinterpret_cast<void *>(mtc_table_alt), reinterpret_cast<void *>(mtc_table_max), sizeof(MarikoMtcTable));
                    std::memcpy(reinterpret_cast<void *>(mtc_table_max), reinterpret_cast<void *>(table), sizeof(MarikoMtcTable));
                    return ResultSuccess();
                }

                std::memcpy(reinterpret_cast<void *>(table), reinterpret_cast<void *>(mtc_table_max), sizeof(MarikoMtcTable));
                MtcTableAutoAdjustMariko(mtc_table_max, mtc_table_alt);
                std::memcpy(reinterpret_cast<void *>(mtc_table_alt), reinterpret_cast<void *>(table), sizeof(MarikoMtcTable));
            } else {
                if (replace_entire_table) {
                    EristaMtcTable* const mtc_table_max = reinterpret_cast<EristaMtcTable *>(ptr - offsetof(EristaMtcTable, rate_khz) / sizeof(u32));
                    EristaMtcTable* const table         = const_cast<EristaMtcTable *>(std::addressof(C.mtcErista));
                    std::memcpy(reinterpret_cast<void *>(mtc_table_max), reinterpret_cast<void *>(table), sizeof(EristaMtcTable));
                }
            }

            return ResultSuccess();
        }

        static Result MemMaxClockHandler(u32* ptr) {
            u32 value_next  = *(ptr + 1);
            u32 value_next2 = *(ptr + 2);

            // Mariko have 3 mtc tables (204/1331/1600 MHz), only these 3 frequencies could be set.
            // Replace 1331 MHz with 1600 MHz as perf @ 1331 MHz is crap.
            constexpr u32 mtc_mariko_min_volt = 1100;
            constexpr u32 mtc_erista_min_volt = 887;
            constexpr u32 dvb_entry_volt      = 675;

            Result rc = ResultSuccess();

            if ((isMariko && value_next == mtc_mariko_min_volt) || (!isMariko && value_next == mtc_erista_min_volt)) {
                rc = MtcTableHandler(ptr);
            } else if (isMariko && value_next2 == dvb_entry_volt) {
                emc_dvb_dvfs_table_t* dvb_max_entry  = reinterpret_cast<emc_dvb_dvfs_table_t *>(ptr);
                emc_dvb_dvfs_table_t* dvb_1331_entry = dvb_max_entry - 1;

                u32* dvb_1331_offset = reinterpret_cast<u32 *>(dvb_1331_entry);
                if (*(dvb_1331_offset) != MemClkOSAlt)
                    return ResultFailure();

                PatchOffset(dvb_1331_offset, MemClkOSLimit);
            }

            PatchOffset(ptr, C.emcMaxClock);
            return rc;
        }

        void Patch(uintptr_t mapped_nso, size_t nso_size) {
            /* Abort immediately once something goes wrong */
            enum PatchSuccessCnt {
                COMMON_MEM_CLOCK,
                MARIKO_CPU_CLOCK_VDD,
                MARIKO_CPU_TABLE,
                MARIKO_GPU_TABLE,
                MARIKO_CPU_MAX_VOLT,
                MARIKO_GPU_MAX_CLOCK,
                PATCH_SUCCESS_CNT_MAX,
            };

            u8 cnt[PATCH_SUCCESS_CNT_MAX] = {};

            uintptr_t ptr = mapped_nso;
            while (ptr <= mapped_nso + nso_size - std::max(sizeof(MarikoMtcTable), sizeof(EristaMtcTable)))
            {
                u32 value = *(reinterpret_cast<u32 *>(ptr));

                if (isMariko)
                {
                    if (value == CpuClkOSLimit)
                        if (R_SUCCEEDED(CpuClockVddHandler(reinterpret_cast<u32 *>(ptr))))
                            cnt[MARIKO_CPU_CLOCK_VDD]++;

                    if (value == CpuClkOfficial)
                        if (R_SUCCEEDED(CpuDvfsHandler(reinterpret_cast<cpu_freq_cvb_table_t *>(ptr), mapped_nso + nso_size)))
                            cnt[MARIKO_CPU_TABLE]++;

                    if (value == GpuClkOfficial)
                        if (R_SUCCEEDED(GpuDvfsHandler(reinterpret_cast<gpu_cvb_pll_table_t *>(ptr), mapped_nso + nso_size)))
                            cnt[MARIKO_GPU_TABLE]++;

                    if (value == CpuVoltOfficial)
                        if (R_SUCCEEDED(CpuVoltRangeHandler(reinterpret_cast<u32 *>(ptr))))
                            cnt[MARIKO_CPU_MAX_VOLT]++;

                    if (COMPARE_HIGH(value, gpuOfficialMarikoPattern[0], 5))
                        if (R_SUCCEEDED(GpuMaxClockHandler(reinterpret_cast<u32 *>(ptr))))
                            cnt[MARIKO_GPU_MAX_CLOCK]++;
                }

                if (value == MemClkOSLimit)
                    if (R_SUCCEEDED(MemMaxClockHandler(reinterpret_cast<u32 *>(ptr))))
                        cnt[COMMON_MEM_CLOCK]++;

                ptr += sizeof(u32);
            }

            bool marikoCheck = cnt[MARIKO_CPU_CLOCK_VDD] != 1
                            || cnt[MARIKO_CPU_TABLE]     != 1
                            || cnt[MARIKO_GPU_TABLE]     != 1
                            || cnt[MARIKO_CPU_MAX_VOLT]   > 13 || !cnt[MARIKO_CPU_MAX_VOLT]
                            || cnt[MARIKO_GPU_MAX_CLOCK] != 2;

            if (!cnt[COMMON_MEM_CLOCK] || (isMariko && marikoCheck)) {
                AMS_ABORT();
                __builtin_unreachable();
            }
        }
    }

    namespace ptm {
        void Patch(uintptr_t mapped_nso, size_t nso_size) {
            /* No abort here as ptm is not that critical */
            if (!isMariko)
                return;

            perf_conf_entry* confTable = 0;
            constexpr u32 entryCnt     = 16;
            constexpr u32 memPtmLimit  = MemClkOSLimit * 1000;
            constexpr u32 memPtmAlt    = MemClkOSAlt * 1000;
            constexpr u32 memPtmClamp  = MemClkOSClampDn * 1000;
            const     u32 memPtmMax    = C.emcMaxClock * 1000;

            uintptr_t ptr = mapped_nso;
            while (ptr <= mapped_nso + nso_size - sizeof(perf_conf_entry) * entryCnt)
            {
                u32 value = *(reinterpret_cast<u32 *>(ptr));

                if (value == memPtmLimit)
                {
                    confTable = reinterpret_cast<perf_conf_entry *>(ptr - offsetof(perf_conf_entry, emc_freq_1));
                    break;
                }

                ptr += sizeof(u32);
            }

            if (!confTable)
                return;

            for (u32 i = 0; i < entryCnt; i++)
            {
                perf_conf_entry* entry_current = confTable + i;

                if (entry_current->emc_freq_1 != entry_current->emc_freq_2)
                    return;

                switch (entry_current->emc_freq_1)
                {
                    case memPtmLimit:
                        PatchOffset(std::addressof(entry_current->emc_freq_1), memPtmMax);
                        PatchOffset(std::addressof(entry_current->emc_freq_2), memPtmMax);
                        break;
                    case memPtmAlt:
                    case memPtmClamp:
                        PatchOffset(std::addressof(entry_current->emc_freq_1), memPtmLimit);
                        PatchOffset(std::addressof(entry_current->emc_freq_2), memPtmLimit);
                        break;
                    default:
                        return;
                }
            }
        }
    }

}
