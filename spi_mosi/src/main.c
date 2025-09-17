#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree.h>
#include <stdio.h>

// Get SPI device from devicetree (epaper@0 under spi21)
static const struct spi_dt_spec epaper_spi = SPI_DT_SPEC_GET(DT_NODELABEL(epaper), SPI_WORD_SET(8), 0);

// Get control pins from devicetree
static const struct gpio_dt_spec dc    = GPIO_DT_SPEC_GET(DT_NODELABEL(epaper), dc_gpios);
static const struct gpio_dt_spec reset = GPIO_DT_SPEC_GET(DT_NODELABEL(epaper), reset_gpios);
static const struct gpio_dt_spec busy  = GPIO_DT_SPEC_GET(DT_NODELABEL(epaper), busy_gpios);

void epaper_reset_sequence(void)
{
    gpio_pin_configure_dt(&reset, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&busy, GPIO_INPUT);

    gpio_pin_set_dt(&reset, 0); k_msleep(20);
    gpio_pin_set_dt(&reset, 1); k_msleep(10);
    gpio_pin_set_dt(&reset, 0); k_msleep(20);
    gpio_pin_set_dt(&reset, 1); k_msleep(10);

    // Wait for BUSY to go HIGH
    printk("Waiting for BUSY to go HIGH...\n");
    while (gpio_pin_get_dt(&busy) == 0) {
        k_msleep(1);
    }
    printk("BUSY is HIGH\n");
    k_msleep(10);
}

void epaper_write_command(uint8_t cmd)
{
    gpio_pin_set_dt(&dc, 0); // Command mode
    struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
    struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
    spi_write_dt(&epaper_spi, &tx_set);
}

void epaper_write_data(uint8_t data)
{
    gpio_pin_set_dt(&dc, 1); // Data mode
    struct spi_buf tx_buf = { .buf = &data, .len = 1 };
    struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
    spi_write_dt(&epaper_spi, &tx_set);
}

int main(void)
{
    // Configure control pins
    gpio_pin_configure_dt(&dc, GPIO_OUTPUT);
    gpio_pin_configure_dt(&reset, GPIO_OUTPUT);
    gpio_pin_configure_dt(&busy, GPIO_INPUT);

    printf("Hello Epaper! %s\n", CONFIG_BOARD_TARGET);

    epaper_reset_sequence();

    // Example: Write a command and data (no read possible)
    epaper_write_command(0x01); // Example command
    epaper_write_data(0xAA);    // Example data

    // Do not attempt to read from the display, as MISO is not connected.

    return 0;
}