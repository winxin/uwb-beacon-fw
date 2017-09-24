#ifndef MPU9250_H
#define MPU9250_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    SPIDriver *spi;
} mpu9250_t;

/** Initializes the given MPU9250 driver instance, but does not configure the
 * actual chip. */
void mpu9250_init(mpu9250_t *dev, SPIDriver *spi_dev);

/** Returns true if the MPU9250 is correctly detected, false otherwise. */
bool mpu9250_ping(mpu9250_t *dev);

/** Sends the configuration to the MPU9250. */
void mpu9250_configure(mpu9250_t *dev);

#ifdef __cplusplus
}
#endif
#endif // MPU9250_H
