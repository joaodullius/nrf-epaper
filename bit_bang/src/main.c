#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <stdio.h>

// Get pin specs from devicetree
static const struct gpio_dt_spec sclk  = GPIO_DT_SPEC_GET(DT_NODELABEL(sclk_pin), gpios);
static const struct gpio_dt_spec sdin  = GPIO_DT_SPEC_GET(DT_NODELABEL(sdin_pin), gpios);
static const struct gpio_dt_spec cs    = GPIO_DT_SPEC_GET(DT_NODELABEL(cs_pin), gpios);
static const struct gpio_dt_spec dc    = GPIO_DT_SPEC_GET(DT_NODELABEL(dc_pin), gpios);
static const struct gpio_dt_spec busy  = GPIO_DT_SPEC_GET(DT_NODELABEL(busy_pin), gpios);
static const struct gpio_dt_spec reset = GPIO_DT_SPEC_GET(DT_NODELABEL(reset_pin), gpios);

// Helper to set SDIN direction
static void set_sdin_direction(bool output)
{
    gpio_pin_configure_dt(&sdin, output ? GPIO_OUTPUT : GPIO_INPUT);
}

// Helper to set a pin
static void set_pin(const struct gpio_dt_spec *spec, int value)
{
    gpio_pin_set_dt(spec, value);
}

// Helper to get a pin
static int get_pin(const struct gpio_dt_spec *spec)
{
    return gpio_pin_get_dt(spec);
}

// Bit-bang SPI write (8 bits)
static void spi_write_byte(uint8_t data)
{
    set_sdin_direction(true); // SDIN as output
    for (int i = 0; i < 8; i++) {
        set_pin(&sclk, 0);
        set_pin(&sdin, (data & 0x80) ? 1 : 0);
        k_busy_wait(1); // Short delay
        set_pin(&sclk, 1);
        k_busy_wait(1);
        data <<= 1;
    }
    set_pin(&sclk, 0);
}

// Bit-bang SPI read (8 bits)
static uint8_t spi_read_byte(void)
{
    uint8_t data = 0;
    set_sdin_direction(false); // SDIN as input
    for (int i = 0; i < 8; i++) {
        set_pin(&sclk, 0);
        k_busy_wait(1);
        set_pin(&sclk, 1);
        data <<= 1;
        if (get_pin(&sdin)) {
            data |= 0x01;
        }
        k_busy_wait(1);
    }
    set_pin(&sclk, 0);
    return data;
}

// Example: Read Chip ID (0x70)
void epaper_read_chip_id(uint8_t *id)
{
    set_pin(&cs, 0);
    set_pin(&dc, 0); // Command mode
    spi_write_byte(0x70); // Send command
    set_pin(&dc, 1); // Data mode
    id[0] = spi_read_byte();
    id[1] = spi_read_byte();
    set_pin(&cs, 1);
}

void epaper_reset_sequence(void)
{
    gpio_pin_configure_dt(&reset, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&busy, GPIO_INPUT);

    set_pin(&reset, 0); k_msleep(20);
    set_pin(&reset, 1); k_msleep(10);
    set_pin(&reset, 0); k_msleep(20);
    set_pin(&reset, 1); k_msleep(10);

    // Wait for BUSY to go HIGH
    while (get_pin(&busy) == 0) {
        k_msleep(1);
    }
    k_msleep(10);
}

int main(void)
{
    // Configure all pins
    gpio_pin_configure_dt(&sclk, GPIO_OUTPUT);
    gpio_pin_configure_dt(&sdin, GPIO_OUTPUT); // Start as output
    gpio_pin_configure_dt(&cs, GPIO_OUTPUT);
    gpio_pin_configure_dt(&dc, GPIO_OUTPUT);
    gpio_pin_configure_dt(&busy, GPIO_INPUT);
    gpio_pin_configure_dt(&reset, GPIO_OUTPUT);

    printf("Hello Epaper! %s\n", CONFIG_BOARD_TARGET);

    epaper_reset_sequence();

    // Read Chip ID
    uint8_t chip_id[2];
    epaper_read_chip_id(chip_id);

    printf("Chip ID: 0x%02X 0x%02X\n", chip_id[0], chip_id[1]);

    return 0;
}