#ifndef DEMMT_NVUVM_H
#define DEMMT_NVUVM_H

#include <stdbool.h>
#include <stdint.h>
#include "buffer.h"

int nvuvm_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc);
int nvuvm_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, uint64_t ret, uint64_t err, void *state,
		struct mmt_memory_dump *args, int argc);

#endif
