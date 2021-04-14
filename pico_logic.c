/**
 * Pico Logic analyzer
 * Code largely based off of various SDK examples including the logic-analyzer example
 * This is different because it performs run-length compression and
 * Continuously streams USB
 **/

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/stdio/driver.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "logic.pio.h"

// Some logic to analyse:
#include "hardware/structs/pwm.h"

// #define DEBUG

extern stdio_driver_t stdio_usb;

const uint pin_start = 8;
#define buffer_pow 15
#define buffer_size (1 << buffer_pow)
const uint buffer_size_words = buffer_size / 4;

typedef uint32_t dma_buffer_t[buffer_size] __attribute__ ((aligned (buffer_size)));
dma_buffer_t capture_buffer;

struct {uint32_t trans_count;} dma_control_block;

dma_channel_hw_t *logic_engine_go(PIO pio, uint sm) {
    /**
     * This creates continuous looping DMA from PIO by:
     * 1. Using a control channel which triggers data channel with length
     * 2. Data channel is chained back to control channel
     * 3. Data channel uses a ring buffer, it needs to be aligned correctly - see dma_buffer_t
     *    - This makes it so that new write address does not need to be configured
     * 
     * After the DMA is going, we can monitor the data channel registers like transfer_count
     * to output this over USB
    */

    pio_sm_set_enabled(pio, sm, false);
    // Need to clear _input shift counter_, as well as FIFO, because there may be
    // partial ISR contents left over from a previous run. sm_restart does this.
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);

    dma_control_block.trans_count = buffer_size_words;

    int ctrl_chan = dma_claim_unused_channel(true);
    int data_chan = dma_claim_unused_channel(true);    

    dma_channel_config c = dma_channel_get_default_config(ctrl_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_ring(&c, true, 2); // 1 << 2 byte boundary on write ptr

    dma_channel_configure(
        ctrl_chan,
        &c,
        &dma_hw->ch[data_chan].al1_transfer_count_trig, // Initial write address
        &dma_control_block,                        // Initial read address
        1,                                         // Halt after each control block
        false                                      // Don't start yet
    );


    c = dma_channel_get_default_config(data_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
    channel_config_set_chain_to(&c, ctrl_chan);
    channel_config_set_ring(&c, true, buffer_pow); // 1 << 2 byte boundary on write ptr

    // Raise the IRQ flag when 0 is written to a trigger register (end of chain):
    // TG: Not sure this is needed
    channel_config_set_irq_quiet(&c, true);

    dma_channel_configure(data_chan, &c,
        capture_buffer,        // Destination pointer
        &pio->rxf[sm],      // Source pointer
        0, // Transfer count unimportant - the control channel will reprogram it each time.
        false               // Don't Start immediately
    );

    dma_start_channel_mask(1u << ctrl_chan);

    // pio_sm_exec(pio, sm, pio_encode_wait_gpio(trigger_level, trigger_pin));
    pio_sm_set_enabled(pio, sm, true);

    return &dma_hw->ch[data_chan];
}




int main() {
    stdio_init_all();
    
    // GPIO 0: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 1: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 2: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 3: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 4: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 5: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 6: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 7: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 8: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 9: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 10: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 11: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 12: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 13: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 14: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 15: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 16: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 17: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 18: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 19: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 20: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 21: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 22: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 23: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 24: Pull Up 0 - Pull Down 1 - Dir 0 Val 1
    // GPIO 25: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 26: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 27: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 28: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 29: Pull Up 0 - Pull Down 1 - Dir 0 Val 0
    // GPIO 30: Pull Up 1 - Pull Down 0 - Dir 0 Val 0
    // GPIO 31: Pull Up 1 - Pull Down 0 - Dir 0 Val 0
    /*
    while (true) {
        for (int i=0; i<32;i++) {
            printf("GPIO %d: ", i);
            printf("Pull Up %d - ", gpio_is_pulled_up(i));
            printf("Pull Down %d - ", gpio_is_pulled_down(i));
            printf("Dir %d ", gpio_get_dir(i));
            printf("Val %d", gpio_get(i));
            printf("\n");
        }
        sleep_ms(3000);
    }
    */
    if (0) {
        // TODO: Create config option for this
        // By default, GPIO pull-downs are enabled  
        for (int i=0; i<8; i++) {
            gpio_disable_pulls(pin_start+i);
        }
    }
    // sleep_ms(5000);
    // printf("Start\n");
    // printf("Capture buffer at 0x%08x\n", capture_buffer);
    PIO pio = pio0;
    
    uint offset = pio_add_program(pio, &logic_program);
    uint sm = pio_claim_unused_sm(pio, true);

    // printf("PIO logic program loaded at %d\n", offset);
    // sleep_ms(2000);
    // Configure state machines, set bit rate at 5 Mbps
    
    if (1) {
        float sampling_msps = 1.6;
        logic_program_init(pio, sm, offset, pin_start, 125.f / (8 * sampling_msps));
    } else {
        // Full speed - 15.625 Msamples/second
        logic_program_init(pio, sm, offset, pin_start, 1.f);
    }

    hard_assert(capture_buffer);

    dma_channel_hw_t *dma_data_ch = logic_engine_go(pio, sm);

    int last_transfer_count = buffer_size_words;
    int bytes_to_read = 0;
    
    uint8_t *read_address = (uint8_t *) capture_buffer;
    uint8_t *read_stop = read_address + buffer_size;
#ifdef DEBUG    
    int debug_counter = 0;
#endif
    
    while (true) {
        if (bytes_to_read>=64) {
#ifdef DEBUG  
            if (!(counter & 0xf)) {
                printf("Bytes to read: %d - reading 64\n", bytes_to_read);
                printf("Count: %d\n", dma_data_ch->transfer_count);
                printf("Buffer address: %d\n", capture_buffer);
                printf("Read address: %d\n", dma_data_ch->read_addr);
                printf("Write address: %d\n", dma_data_ch->write_addr);        
            }
#endif
            stdio_usb.out_chars(read_address, 64);
            read_address += 64;
            bytes_to_read -= 64;                
            if (read_address == read_stop) {
#ifdef DEBUG                
                if (!(counter & 0xf))
                    printf("Read wrapping\n");
#endif                    
                read_address = (uint8_t *) capture_buffer;
            }
#ifdef DEBUG            
            counter ++;
#endif            
        }
        int current_transfer_count = dma_data_ch->transfer_count;
        if (current_transfer_count > last_transfer_count) {
#ifdef DEBUG            
            printf("DMA Wrap\n");
#endif            
            bytes_to_read += last_transfer_count*4;
            last_transfer_count = buffer_size_words;
        }
        bytes_to_read += (last_transfer_count - current_transfer_count)*4;
        last_transfer_count = current_transfer_count;
    }
}
