#include <stdint.h>

#define __KERNEL__
#include <uvm_ioctl.h>
#include <uvm_linux_ioctl.h>
#undef __KERNEL__

#include "config.h"
#include "decode_utils.h"
#include "demmt.h"
#include "log.h"
#include "nvrm.h"
#include "nvrm_decode.h"
#include "nvuvm.h"
#include "util.h"

struct nvuvm_ioctl
{
	uint32_t id;
	const char *name;
	int size;
	void *fun;
	void *fun_with_args;
	int disabled;
};

struct nvuvm_client_info
{
	uint32_t handle;
	uint32_t vaSpace;
	struct nvuvm_client_info *next;
};
static struct nvuvm_client_info *client_info = NULL;

static struct nvuvm_client_info *
get_client_info(uint32_t handle)
{
	struct nvuvm_client_info *it = client_info;
	while (it) {
		if (it->handle == handle)
			return it;
		it = it->next;
	}
	return NULL;
}

static void
add_client_info(uint32_t handle, uint32_t vaSpace)
{
	struct nvuvm_client_info *nc = calloc(sizeof(struct nvuvm_client_info), 1);

	nc->handle = handle;
	nc->vaSpace = vaSpace;

	if (client_info)
		nc->next = client_info;
	client_info = nc;
}

static void decode_nvuvm_ioctl_create_range_group(UVM_CREATE_RANGE_GROUP_PARAMS *s)
{
	nvrm_print_x64(s, rangeGroupId);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void decode_nvuvm_ioctl_disable_system_wide_atomics(UVM_DISABLE_SYSTEM_WIDE_ATOMICS_PARAMS *s)
{
	//TODO: gpu_uuid
	nvrm_print_x8(s, gpu_uuid.uuid[0]);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void decode_nvuvm_ioctl_initialize(UVM_INITIALIZE_PARAMS *s)
{
	nvrm_print_x64(s, flags);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void decode_nvuvm_ioctl_is_8_supported(UVM_IS_8_SUPPORTED_PARAMS *s)
{
	nvrm_print_d32(s, is8Supported);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void decode_nvuvm_ioctl_pageable_mem_access(UVM_PAGEABLE_MEM_ACCESS_PARAMS *s)
{
	nvrm_print_d8(s, pageableMemAccess);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void decode_nvuvm_ioctl_map_external_allocation(UVM_MAP_EXTERNAL_ALLOCATION_PARAMS *s)
{
	nvrm_print_x64(s, base);
	nvrm_print_x64(s, length);
	nvrm_print_x64(s, offset);
	// TODO attribs
	nvrm_print_x64(s, gpuAttributesCount);
	nvrm_print_d32(s, rmCtrlFd);
	nvrm_print_x32(s, hClient);
	nvrm_print_x32(s, hMemory);
	nvrm_print_x32(s, hClientFromRm);
	nvrm_print_x32(s, hMemoryFromRm);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void handle_nvuvm_ioctl_map_external_allocation(uint32_t fd, UVM_MAP_EXTERNAL_ALLOCATION_PARAMS *s)
{
	if (s->rmStatus != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->hClientFromRm);

	struct gpu_object *obj = gpu_object_find(s->hClientFromRm, s->hMemoryFromRm);
	if (!obj)
	{
		mmt_error("nvuvm_ioctl_host_map: cannot find object 0x%08x 0x%08x\n", s->hClientFromRm, s->hMemoryFromRm);
		return;
	}

	struct nvuvm_client_info *ci = get_client_info(s->hClientFromRm);
	if (!ci)
	{
		mmt_error("nvuvm_ioctl_host_map: cannot find VA space for object 0x%08x\n", s->hClientFromRm);
		return;
	}

	struct gpu_mapping *mapping = calloc(sizeof(struct gpu_mapping), 1);
	mapping->fd = fd;
	mapping->dev = 0;
	mapping->vspace = ci->vaSpace;
	mapping->address = s->base;
	mapping->object_offset = s->offset;
	mapping->length = s->length;
	if (s->length > obj->length)
	{
		obj->data = realloc(obj->data, s->length);
		memset(obj->data + obj->length, 0, s->length - obj->length);
		obj->length = s->length;
	}
	mapping->object = obj;
	mapping->next = obj->gpu_mappings;
	obj->gpu_mappings = mapping;
}

static void decode_nvuvm_ioctl_register_channel(UVM_REGISTER_CHANNEL_PARAMS *s)
{
	//TODO: gpu_uuid
	nvrm_print_x8(s, gpuUuid.uuid[0]);
	nvrm_print_d32(s, rmCtrlFd);
	nvrm_print_x32(s, hClient);
	nvrm_print_x32(s, hChannel);
	nvrm_print_x64(s, base);
	nvrm_print_x64(s, length);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void decode_nvuvm_register_gpu(UVM_REGISTER_GPU_PARAMS *s)
{
	//TODO: gpu_uuid
	nvrm_print_x8(s, gpu_uuid.uuid[0]);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void decode_nvuvm_register_gpu_vaspace(UVM_REGISTER_GPU_VASPACE_PARAMS *s)
{
	//TODO: gpu_uuid
	nvrm_print_x8(s, gpuUuid.uuid[0]);
	nvrm_print_d32(s, rmCtrlFd);
	nvrm_print_x32(s, hClient);
	nvrm_print_x32(s, hVaSpace);
	nvrm_print_status(s, rmStatus);
	nvrm_print_ln();
}

static void handle_nvuvm_ioctl_register_gpu_vspace(uint32_t fd, UVM_REGISTER_GPU_VASPACE_PARAMS *s)
{
	if (s->rmStatus != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->hClient);

	add_client_info(s->hClient, s->hVaSpace);
}

#define _(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), FUN, NULL, 0 }
#define _a(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), NULL, FUN, 0 }
struct nvuvm_ioctl nvuvm_ioctls[] =
{
	_(UVM_CREATE_RANGE_GROUP, UVM_CREATE_RANGE_GROUP_PARAMS, decode_nvuvm_ioctl_create_range_group),
	_(UVM_DISABLE_SYSTEM_WIDE_ATOMICS, UVM_DISABLE_SYSTEM_WIDE_ATOMICS_PARAMS, decode_nvuvm_ioctl_disable_system_wide_atomics),
	_(UVM_INITIALIZE, UVM_INITIALIZE_PARAMS, decode_nvuvm_ioctl_initialize),
	_(UVM_IS_8_SUPPORTED, UVM_IS_8_SUPPORTED_PARAMS, decode_nvuvm_ioctl_is_8_supported),
	_(UVM_MAP_EXTERNAL_ALLOCATION, UVM_MAP_EXTERNAL_ALLOCATION_PARAMS, decode_nvuvm_ioctl_map_external_allocation),
	_(UVM_PAGEABLE_MEM_ACCESS, UVM_PAGEABLE_MEM_ACCESS_PARAMS, decode_nvuvm_ioctl_pageable_mem_access),
	_(UVM_REGISTER_CHANNEL, UVM_REGISTER_CHANNEL_PARAMS, decode_nvuvm_ioctl_register_channel),
	_(UVM_REGISTER_GPU, UVM_REGISTER_GPU_PARAMS, decode_nvuvm_register_gpu),
	_(UVM_REGISTER_GPU_VASPACE, UVM_REGISTER_GPU_VASPACE_PARAMS, decode_nvuvm_register_gpu_vaspace),
};
#undef _
#undef _a
int nvuvm_ioctls_cnt = ARRAY_SIZE(nvuvm_ioctls);

static int decode_nvuvm_ioctl(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr,
		uint16_t size, struct mmt_buf *buf, uint64_t ret, uint64_t err,
		void *state, struct mmt_memory_dump *args, int argc, const char *name)
{
	int k, found = 0;
	int args_used = 0;
	void (*fun)(void *) = NULL;
	void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

	struct nvuvm_ioctl *ioctl;
	for (k = 0; k < nvuvm_ioctls_cnt; ++k)
	{
		ioctl = &nvuvm_ioctls[k];
		if (ioctl->id == id)
		{
			if (ioctl->size == buf->len)
			{
				if (dump_decoded_ioctl_data && !ioctl->disabled)
				{
					mmt_log("%-26s %-5s fd: %d, ", ioctl->name, name, fd);
					if (ret)
						mmt_log_cont("ret: %" PRId64 ", ", ret);
					if (err)
						mmt_log_cont("%serr: %" PRId64 "%s, ", colors->err, err, colors->reset);

					nvrm_reset_pfx();
					fun = ioctl->fun;
					if (fun)
						fun(buf->data);
					fun_with_args = ioctl->fun_with_args;
					if (fun_with_args)
					{
						fun_with_args(buf->data, args, argc);
						args_used = 1;
					}
				}
				found = 1;
			}
			break;
		}
	}

	if (!found)
		return 1;

	if ((!args_used && dump_decoded_ioctl_data && !ioctl->disabled) || dump_raw_ioctl_data)
		dump_args(args, argc, 0);

	return 0;
}

static int decode_nvuvm_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	return decode_nvuvm_ioctl(fd, id, dir, nr, size, buf, 0, 0, state, args, argc, "pre,");
}

static int decode_nvuvm_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, uint64_t ret, uint64_t err, void *state,
		struct mmt_memory_dump *args, int argc)
{
	return decode_nvuvm_ioctl(fd, id, dir, nr, size, buf, ret, err, state, args, argc, "post,");
}

int nvuvm_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	return decode_nvuvm_ioctl_pre(fd, id, dir, nr, size, buf, state, args, argc);
}

int nvuvm_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, uint64_t ret, uint64_t err, void *state,
		struct mmt_memory_dump *args, int argc)
{
	int r = decode_nvuvm_ioctl_post(fd, id, dir, nr, size, buf, ret, err, state, args, argc);
	void *d = buf->data;

	switch (id) {
	case UVM_MAP_EXTERNAL_ALLOCATION:
		handle_nvuvm_ioctl_map_external_allocation(fd, d);
		break;
	case UVM_REGISTER_GPU_VASPACE:
		handle_nvuvm_ioctl_register_gpu_vspace(fd, d);
		break;
	}

	return r;
}
