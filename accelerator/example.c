#include "types.h"
#include "memory_map.h"
#include "ascii.h"
#include "uart.h"

#define BUF_LEN 128
#define DATA_SIZE 20

typedef void (*entry_t)(void);

// Accelerator Address Mapping
volatile uint32_t* addr_din = (volatile uint32_t*)0x80010000;
volatile uint32_t* addr_dout = (volatile uint32_t*)0x80010004;
volatile uint32_t* addr_clear = (volatile uint32_t*)0x80010008;
volatile uint32_t* addr_w0 = (volatile uint32_t*)0x8001000C;
volatile uint32_t* addr_w1 = (volatile uint32_t*)0x80010010;
volatile uint32_t* addr_w2 = (volatile uint32_t*)0x80010014;

int main(void)
{
    // Input Data (x[n])
    uint32_t data[DATA_SIZE] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        10, 9, 8, 7, 6, 5, 4, 3, 2, 1
    };

    // Filter Weights (w0, w1, w2) -> y[n] = 1*x[n] + 2*x[n-1] + 1*x[n-2]
    uint32_t w[3] = { 1, 2, 1 };

    uint32_t hw_result[DATA_SIZE];
    uint32_t sw_result[DATA_SIZE];
    int8_t buffer[BUF_LEN];
    uint32_t i;
    volatile uint32_t counter_start, counter_end;
    uint32_t hw_cycles, sw_cycles;

    uwrite_int8s("\r\n=== 1D Filter Convolution Test ===\r\n");

    // ---------------------------------------------------------
    // 1. HW Accelerator Execution
    // ---------------------------------------------------------
    COUNTER_RST = 1;
    counter_start = CYCLE_COUNTER;

    *addr_clear = 1;      // Reset Internal Registers
    *addr_w0 = w[0];      // Set Weights
    *addr_w1 = w[1];
    *addr_w2 = w[2];

    for (i = 0; i < DATA_SIZE; i++)
    {
        *addr_din = data[i];     // Write Input -> Triggers Compute
        hw_result[i] = *addr_dout; // Read Result
    }

    counter_end = CYCLE_COUNTER;
    hw_cycles = counter_end - counter_start;

    // ---------------------------------------------------------
    // 2. SW Execution (CPU Calculation)
    // ---------------------------------------------------------
    COUNTER_RST = 1;
    counter_start = CYCLE_COUNTER;

    uint32_t x0, x1, x2;
    x1 = 0; x2 = 0;      // Initialize history (x[n-1], x[n-2])

    for (i = 0; i < DATA_SIZE; i++)
    {
        x0 = data[i];
        // Convolution Formula
        sw_result[i] = (x0 * w[0]) + (x1 * w[1]) + (x2 * w[2]);

        // Update history
        x2 = x1;
        x1 = x0;
    }

    counter_end = CYCLE_COUNTER;
    sw_cycles = counter_end - counter_start;

    // ---------------------------------------------------------
    // 3. Print Results & Verification
    // ---------------------------------------------------------
    uwrite_int8s("\r\n[Index]  [Input]   [HW Result]  [SW Result]   [Check]\r\n");

    int error_count = 0;
    for (i = 0; i < DATA_SIZE; i++)
    {
        // Print Index
        uwrite_int8s("  ");
        uwrite_int8s(uint32_to_ascii_hex(i, buffer, BUF_LEN));

        // Print Input Data
        uwrite_int8s("       ");
        uwrite_int8s(uint32_to_ascii_hex(data[i], buffer, BUF_LEN));

        // Print HW Result
        uwrite_int8s("     ");
        uwrite_int8s(uint32_to_ascii_hex(hw_result[i], buffer, BUF_LEN));

        // Print SW Result
        uwrite_int8s("     ");
        uwrite_int8s(uint32_to_ascii_hex(sw_result[i], buffer, BUF_LEN));

        // Check Match
        if (hw_result[i] == sw_result[i]) {
            uwrite_int8s("      OK\r\n");
        }
        else {
            uwrite_int8s("      FAIL\r\n");
            error_count++;
        }
    }

    uwrite_int8s("\r\nPerformance Comparison (Cycles):\r\n");
    uwrite_int8s("HW Accelerator: ");
    uwrite_int8s(uint32_to_ascii_hex(hw_cycles, buffer, BUF_LEN));
    uwrite_int8s("\r\n");
    uwrite_int8s("Software (CPU): ");
    uwrite_int8s(uint32_to_ascii_hex(sw_cycles, buffer, BUF_LEN));
    uwrite_int8s("\r\n");

    if (error_count == 0) {
        uwrite_int8s("\r\nFINAL RESULT: SUCCESS\r\n");
    }
    else {
        uwrite_int8s("\r\nFINAL RESULT: FAIL\r\n");
    }

    // Return to Monitor
    uint32_t bios = ascii_hex_to_uint32("40000000");
    entry_t start = (entry_t)(bios);
    start();
    return 0;
}




