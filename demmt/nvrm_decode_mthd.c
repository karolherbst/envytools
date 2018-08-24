/*
 * Copyright (C) 2014 Marcin Ślusarz <marcin.slusarz@gmail.com>.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "decode_utils.h"
#include "log.h"
#include "nvrm.h"
#include "nvrm_decode.h"
#include "nvrm_mthd.h"
#include "util.h"

static void decode_nvrm_mthd_context_list_devices(struct nvrm_mthd_context_list_devices *m)
{
	int j;
	for (j = 0; j < 32; ++j)
		if (m->gpu_id[j] != 0 && m->gpu_id[j] != 0xffffffff)
			mmt_log_cont("gpu_id[%d]: 0x%08x ", j, m->gpu_id[j]);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_context_unk0201(struct nvrm_mthd_context_unk0201 *m)
{
	int j;
	for (j = 0; j < 32; ++j)
		if (m->gpu_id[j] != 0 && m->gpu_id[j] != 0xffffffff)
			mmt_log_cont("gpu_id[%d]: 0x%08x ", j, m->gpu_id[j]);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_context_enable_device(struct nvrm_mthd_context_enable_device *m)
{
	int j;
	nvrm_print_x32(m, gpu_id);
	for (j = 0; j < 32; ++j)
		if (m->unk04[j] != 0)
			mmt_log_cont(", unk04[%d]: 0x%08x ", j, m->unk04[j]);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_context_unk0202(struct nvrm_mthd_context_unk0202 *m)
{
	nvrm_print_x32(m, gpu_id);
	nvrm_print_x32(m, unk04);
	nvrm_print_x32(m, unk08);
	nvrm_print_x32(m, unk0c);
	nvrm_print_x32(m, unk10);
	nvrm_print_x32(m, unk14);
	nvrm_print_x32(m, unk18);
	nvrm_print_x32(m, unk1c_gpu_id);
	nvrm_print_x32(m, unk20);
	nvrm_print_x32(m, unk24);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_device_unk1401(struct nvrm_mthd_device_unk1401 *m,
		struct mmt_memory_dump *args, int argc)
{
	int j;

	nvrm_print_x32(m, cnt);
	nvrm_print_handle(m, cid, cid);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	if (!data)
	{
		nvrm_print_ln();
		return;
	}

	mmt_log_cont(" ->%s", "");
	for (j = 0; j < data->len; ++j)
		mmt_log_cont(" 0x%02x", data->data[j]);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_device_get_classes(struct nvrm_mthd_device_get_classes *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, cnt);
	nvrm_print_pad_x32(m, _pad);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_words_desc(data, nvrm_get_class_name);
}

static void decode_nvrm_mthd_device_unk0280(struct nvrm_mthd_device_unk0280 *m)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_bus_id(struct nvrm_mthd_subdevice_get_bus_id *m)
{
	nvrm_print_x32(m, main_id);
	nvrm_print_x32(m, subsystem_id);
	nvrm_print_x32(m, stepping);
	nvrm_print_x32(m, real_product_id);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_chipset(struct nvrm_mthd_subdevice_get_chipset *m)
{
	nvrm_print_x32(m, major);
	nvrm_print_x32(m, minor);
	nvrm_print_x32(m, stepping);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_chipset16(struct nvrm_mthd_subdevice_get_chipset16 *m)
{
	nvrm_print_x32(m, major);
	nvrm_print_x32(m, minor);
	nvrm_print_x32(m, stepping);
	nvrm_print_x32(m, unk0c);
	nvrm_print_ln();
}

#define _(V) { V, #V }
static struct
{
	uint32_t val;
	const char *name;
}
fb_params [] =
{
	_(NVRM_PARAM_SUBDEVICE_FB_BUS_WIDTH),
	_(NVRM_PARAM_SUBDEVICE_FB_UNK13),
	_(NVRM_PARAM_SUBDEVICE_FB_UNK23),
	_(NVRM_PARAM_SUBDEVICE_FB_UNK24),
	_(NVRM_PARAM_SUBDEVICE_FB_PART_COUNT),
	_(NVRM_PARAM_SUBDEVICE_FB_L2_CACHE_SIZE),
};
#undef _

static const char *fb_param_name(uint32_t v)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(fb_params); ++i)
		if (fb_params[i].val == v)
			return fb_params[i].name;
	return "";
}

static void decode_nvrm_mthd_subdevice_fb_get_params(struct nvrm_mthd_subdevice_fb_get_params *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, cnt);
	nvrm_print_x32(m, unk04);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_word_pairs(data, fb_param_name);
}

static void decode_nvrm_mthd_device_unk1701(struct nvrm_mthd_device_unk1701 *m,
		struct mmt_memory_dump *args, int argc)
{
	int j;
	nvrm_print_x32(m, cnt);
	nvrm_print_x32(m, unk04);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	if (!data)
	{
		nvrm_print_ln();
		return;
	}

	mmt_log_cont(" ->%s", "");
	for (j = 0; j < data->len; ++j)
		mmt_log_cont(" 0x%02x", data->data[j]);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_bus_info(struct nvrm_mthd_subdevice_get_bus_info *m)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_x32(m, unk04);
	nvrm_print_pad_x32(m, _pad1);
	nvrm_print_d32_align(m, regs_size_mb, 4);
	nvrm_print_x64(m, regs_base);
	nvrm_print_pad_x32(m, _pad2);
	nvrm_print_d32_align(m, fb_size_mb, 4);
	nvrm_print_x64(m, fb_base);
	nvrm_print_pad_x32(m, _pad3);
	nvrm_print_d32_align(m, ramin_size_mb, 4);
	nvrm_print_x64(m, ramin_base);
	nvrm_print_x32(m, unk38);
	nvrm_print_x32(m, unk3c);
	nvrm_print_x64(m, unk40);
	nvrm_print_x64(m, unk48);
	nvrm_print_x64(m, unk50);
	nvrm_print_x64(m, unk58);
	nvrm_print_x64(m, unk60);
	nvrm_print_x64(m, unk68);
	nvrm_print_x64(m, unk70);
	nvrm_print_x64(m, unk78);
	nvrm_print_x64(m, unk80);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_fifo_engines(struct nvrm_mthd_subdevice_get_fifo_engines *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, cnt);
	nvrm_print_pad_x32(m, _pad);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

#define _(V) { V, #V }
static struct
{
	uint32_t val;
	const char *name;
}
bus_params [] =
{
	_(NVRM_PARAM_SUBDEVICE_BUS_EXP_LNK_CAP),
	_(NVRM_PARAM_SUBDEVICE_BUS_BUS_ID),
	_(NVRM_PARAM_SUBDEVICE_BUS_DEV_ID),
	_(NVRM_PARAM_SUBDEVICE_BUS_DOMAIN_ID),
};
#undef _

static const char *bus_param_name(uint32_t v)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(bus_params); ++i)
		if (bus_params[i].val == v)
			return bus_params[i].name;
	return "";
}

static void decode_nvrm_mthd_subdevice_bus_get_params(struct nvrm_mthd_subdevice_bus_get_params *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, cnt);
	nvrm_print_pad_x32(m, _pad);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_word_pairs(data, bus_param_name);
}

static void decode_nvrm_mthd_device_unk1102(struct nvrm_mthd_device_unk1102 *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, cnt);
	nvrm_print_x32(m, unk04);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_unk0101(struct nvrm_mthd_subdevice_unk0101 *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, cnt);
	nvrm_print_pad_x32(m, _pad);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_word_pairs(data, NULL);
}

static void decode_nvrm_mthd_subdevice_unk0119(struct nvrm_mthd_subdevice_unk0119 *m)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_unk1201(struct nvrm_mthd_subdevice_unk1201 *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, cnt);
	nvrm_print_pad_x32(m, _pad);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_word_pairs(data, NULL);
}

static void decode_nvrm_mthd_subdevice_get_gpc_mask(struct nvrm_mthd_subdevice_get_gpc_mask *m)
{
	nvrm_print_x32(m, gpc_mask);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_gpc_tp_mask(struct nvrm_mthd_subdevice_get_gpc_tp_mask *m)
{
	nvrm_print_x32(m, gpc_id);
	nvrm_print_x32(m, tp_mask);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_fifo_joinable_engines(struct nvrm_mthd_subdevice_get_fifo_joinable_engines *m)
{
	int j;

	nvrm_print_x32(m, eng);
	nvrm_print_class(m, cls);
	nvrm_print_x32(m, cnt);
	nvrm_print_ln();

	for (j = 0; j < 0x20; ++j)
		if (j < m->cnt || m->res[j])
			mmt_log("     [%2d]: 0x%08x\n", j, m->res[j]);
}

static void decode_nvrm_mthd_subdevice_get_fifo_classes(struct nvrm_mthd_subdevice_get_fifo_classes *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, eng);
	nvrm_print_x32(m, cnt);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_get_name(struct nvrm_mthd_subdevice_get_name *m)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_str(m, name);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_uuid(struct nvrm_mthd_subdevice_get_uuid *m)
{
	int j;
	nvrm_print_x32(m, unk00);
	nvrm_print_x32(m, unk04);
	nvrm_print_d32(m, uuid_len);
	mmt_log_cont(", uuid: %s", "");

	for (j = 0; j < m->uuid_len; ++j)
		mmt_log_cont("%02x", (unsigned char)m->uuid[j]);

	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_compute_mode(struct nvrm_mthd_subdevice_get_compute_mode *m)
{
	nvrm_print_x32(m, mode);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_fifo_ib_activate(struct nvrm_mthd_fifo_ib_activate *m)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_context_unk0301(struct nvrm_mthd_context_unk0301 *m)
{
	int j;
	nvrm_print_ln();

	for (j = 0; j < 12; ++j)
		mmt_log("        0x%08x\n", m->unk00[j]);
}

static void decode_nvrm_mthd_context_disable_device(struct nvrm_mthd_context_disable_device *m)
{
	int j;
	nvrm_print_x32(m, gpu_id);
	nvrm_print_ln();

	for (j = 0; j < 31; ++j)
		mmt_log("        0x%08x\n", m->unk04[j]);
}

static void decode_nvrm_mthd_device_unk170d(struct nvrm_mthd_device_unk170d *m,
		struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data1, *data2;

	nvrm_print_x32(m, cnt);
	nvrm_print_pad_x32(m, _pad);
	data1 = nvrm_print_ptr(m, ptr1, args, argc);
	data2 = nvrm_print_ptr(m, ptr2, args, argc);
	nvrm_print_ln();

	if (data1)
		dump_mmt_buf_as_words(data1);

	if (data2)
	{
		mmt_log("%s\n", "");

		dump_mmt_buf_as_words(data2);
	}
}

static void decode_nvrm_mthd_subdevice_bar0(struct nvrm_mthd_subdevice_bar0 *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_handle(m, cid, cid);
	nvrm_print_handle(m, handle, cid);
	nvrm_print_x32(m, unk08);
	nvrm_print_x32(m, unk0c);
	nvrm_print_x32(m, unk10);
	nvrm_print_d32(m, cnt);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
	{
		int i;
		for (i = 0; i < m->cnt; ++i)
		{
			struct nvrm_mthd_subdevice_bar0_op *d = &((struct nvrm_mthd_subdevice_bar0_op *)data->data)[i];

			mmt_log("        %d: ", i);
			nvrm_pfx = "";
			nvrm_print_x32(d, dir);
			nvrm_print_x32(d, unk04);
			nvrm_print_x32(d, unk08);
			nvrm_print_x32(d, mmio);
			nvrm_print_x32(d, unk10);
			nvrm_print_x32(d, value);
			nvrm_print_x32(d, unk18);
			nvrm_print_x32(d, mask);
			nvrm_print_ln();
		}
	}
}

static void decode_nvrm_mthd_subdevice_get_gpu_id(struct nvrm_mthd_subdevice_get_gpu_id *m)
{
	nvrm_print_x32(m, gpu_id);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_get_time(struct nvrm_mthd_subdevice_get_time *m)
{
	nvrm_print_x64(m, time);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_subdevice_unk0512(struct nvrm_mthd_subdevice_unk0512 *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_x32(m, unk04);
	nvrm_print_x32(m, size);
	nvrm_print_x32(m, unk0c);
	nvrm_print_x32(m, unk10);
	nvrm_print_x32(m, unk14);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_unk0522(struct nvrm_mthd_subdevice_unk0522 *m,
		struct mmt_memory_dump *args, int argc)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_x32(m, unk04);
	nvrm_print_x32(m, size);
	nvrm_print_x32(m, unk0c);
	nvrm_print_x32(m, unk10);
	nvrm_print_x32(m, unk14);
	struct mmt_buf *data = nvrm_print_ptr(m, ptr, args, argc);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_unk200a(struct nvrm_mthd_subdevice_unk200a *m)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_x32(m, unk04);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_fifo_ib_object_info(struct nvrm_mthd_fifo_ib_object_info *m)
{
	nvrm_print_x32(m, handle);
	nvrm_print_x32(m, name);
	nvrm_pfx = ", hw"; nvrm_print_class(m, hwcls);
	nvrm_print_x32(m, eng);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_context_unk021b(struct nvrm_mthd_context_unk021b *m)
{
	nvrm_print_x32(m, gpu_id);
	nvrm_print_x32(m, unk04);
	nvrm_print_x32(m, unk08);
	nvrm_print_ln();
}

static void decode_nvrm_mthd_context_get_cpu_info(struct nvrm_mthd_context_get_cpu_info *m)
{
	nvrm_print_x32(m, unk00);
	nvrm_print_x32(m, unk04);
	nvrm_print_d32_align(m, cur_freq, 4);
	nvrm_print_d32_align(m, unk0c, 2);
	nvrm_print_d32_align(m, cache_size, 5);
	nvrm_print_d32_align(m, unk14, 2);
	nvrm_print_d32(m, threads);
	nvrm_print_d32(m, cores);
	nvrm_print_str(m, cpu_desc);
	nvrm_print_pad_x32(m, _pad50);
	nvrm_print_d32(m, cpus);
	nvrm_print_ln();
}

#define _(MTHD, STR, FUN) { MTHD, #MTHD , sizeof(STR), FUN, NULL, 0 }
#define _a(MTHD, STR, FUN) { MTHD, #MTHD , sizeof(STR), NULL, FUN, 0 }

struct nvrm_mthd nvrm_mthds[] =
{
	_(NVRM_MTHD_CONTEXT_LIST_DEVICES, struct nvrm_mthd_context_list_devices, decode_nvrm_mthd_context_list_devices),
	_(NVRM_MTHD_CONTEXT_ENABLE_DEVICE, struct nvrm_mthd_context_enable_device, decode_nvrm_mthd_context_enable_device),
	_(NVRM_MTHD_CONTEXT_UNK0202, struct nvrm_mthd_context_unk0202, decode_nvrm_mthd_context_unk0202),
	_(NVRM_MTHD_CONTEXT_UNK0201, struct nvrm_mthd_context_unk0201, decode_nvrm_mthd_context_unk0201),
	_a(NVRM_MTHD_DEVICE_UNK1401, struct nvrm_mthd_device_unk1401, decode_nvrm_mthd_device_unk1401),
	_a(NVRM_MTHD_DEVICE_GET_CLASSES, struct nvrm_mthd_device_get_classes, decode_nvrm_mthd_device_get_classes),
	_(NVRM_MTHD_DEVICE_UNK0280, struct nvrm_mthd_device_unk0280, decode_nvrm_mthd_device_unk0280),
	_(NVRM_MTHD_SUBDEVICE_GET_BUS_ID, struct nvrm_mthd_subdevice_get_bus_id, decode_nvrm_mthd_subdevice_get_bus_id),
	_(NVRM_MTHD_SUBDEVICE_GET_CHIPSET, struct nvrm_mthd_subdevice_get_chipset, decode_nvrm_mthd_subdevice_get_chipset),
	_(NVRM_MTHD_SUBDEVICE_GET_CHIPSET, struct nvrm_mthd_subdevice_get_chipset16, decode_nvrm_mthd_subdevice_get_chipset16),
	_a(NVRM_MTHD_SUBDEVICE_FB_GET_PARAMS, struct nvrm_mthd_subdevice_fb_get_params, decode_nvrm_mthd_subdevice_fb_get_params),
	_a(NVRM_MTHD_DEVICE_UNK1701, struct nvrm_mthd_device_unk1701, decode_nvrm_mthd_device_unk1701),
	_(NVRM_MTHD_SUBDEVICE_GET_BUS_INFO, struct nvrm_mthd_subdevice_get_bus_info, decode_nvrm_mthd_subdevice_get_bus_info),
	_a(NVRM_MTHD_SUBDEVICE_GET_FIFO_ENGINES, struct nvrm_mthd_subdevice_get_fifo_engines, decode_nvrm_mthd_subdevice_get_fifo_engines),
	_a(NVRM_MTHD_SUBDEVICE_BUS_GET_PARAMS, struct nvrm_mthd_subdevice_bus_get_params, decode_nvrm_mthd_subdevice_bus_get_params),
	_a(NVRM_MTHD_DEVICE_UNK1102, struct nvrm_mthd_device_unk1102, decode_nvrm_mthd_device_unk1102),
	_a(NVRM_MTHD_SUBDEVICE_UNK0101, struct nvrm_mthd_subdevice_unk0101, decode_nvrm_mthd_subdevice_unk0101),
	_(NVRM_MTHD_SUBDEVICE_UNK0119, struct nvrm_mthd_subdevice_unk0119, decode_nvrm_mthd_subdevice_unk0119),
	_a(NVRM_MTHD_SUBDEVICE_UNK1201, struct nvrm_mthd_subdevice_unk1201, decode_nvrm_mthd_subdevice_unk1201),
	_(NVRM_MTHD_SUBDEVICE_GET_GPC_MASK, struct nvrm_mthd_subdevice_get_gpc_mask, decode_nvrm_mthd_subdevice_get_gpc_mask),
	_(NVRM_MTHD_SUBDEVICE_GET_GPC_TP_MASK, struct nvrm_mthd_subdevice_get_gpc_tp_mask, decode_nvrm_mthd_subdevice_get_gpc_tp_mask),
	_(NVRM_MTHD_SUBDEVICE_GET_FIFO_JOINABLE_ENGINES, struct nvrm_mthd_subdevice_get_fifo_joinable_engines, decode_nvrm_mthd_subdevice_get_fifo_joinable_engines),
	_a(NVRM_MTHD_SUBDEVICE_GET_FIFO_CLASSES, struct nvrm_mthd_subdevice_get_fifo_classes, decode_nvrm_mthd_subdevice_get_fifo_classes),
	_(NVRM_MTHD_SUBDEVICE_GET_NAME, struct nvrm_mthd_subdevice_get_name, decode_nvrm_mthd_subdevice_get_name),
	_(NVRM_MTHD_SUBDEVICE_GET_UUID, struct nvrm_mthd_subdevice_get_uuid, decode_nvrm_mthd_subdevice_get_uuid),
	_(NVRM_MTHD_SUBDEVICE_GET_COMPUTE_MODE, struct nvrm_mthd_subdevice_get_compute_mode, decode_nvrm_mthd_subdevice_get_compute_mode),
	_(NVRM_MTHD_FIFO_IB_ACTIVATE, struct nvrm_mthd_fifo_ib_activate, decode_nvrm_mthd_fifo_ib_activate),
	_(NVRM_MTHD_CONTEXT_UNK0301, struct nvrm_mthd_context_unk0301, decode_nvrm_mthd_context_unk0301),
	_(NVRM_MTHD_CONTEXT_DISABLE_DEVICE, struct nvrm_mthd_context_disable_device, decode_nvrm_mthd_context_disable_device),
	_a(NVRM_MTHD_DEVICE_UNK170D, struct nvrm_mthd_device_unk170d, decode_nvrm_mthd_device_unk170d),
	_a(NVRM_MTHD_SUBDEVICE_BAR0, struct nvrm_mthd_subdevice_bar0, decode_nvrm_mthd_subdevice_bar0),
	_(NVRM_MTHD_SUBDEVICE_GET_GPU_ID, struct nvrm_mthd_subdevice_get_gpu_id, decode_nvrm_mthd_subdevice_get_gpu_id),
	_(NVRM_MTHD_SUBDEVICE_GET_TIME, struct nvrm_mthd_subdevice_get_time, decode_nvrm_mthd_subdevice_get_time),
	_a(NVRM_MTHD_SUBDEVICE_UNK0512, struct nvrm_mthd_subdevice_unk0512, decode_nvrm_mthd_subdevice_unk0512),
	_a(NVRM_MTHD_SUBDEVICE_UNK0522, struct nvrm_mthd_subdevice_unk0522, decode_nvrm_mthd_subdevice_unk0522),
	_(NVRM_MTHD_SUBDEVICE_UNK200A, struct nvrm_mthd_subdevice_unk200a, decode_nvrm_mthd_subdevice_unk200a),
	_(NVRM_MTHD_FIFO_IB_OBJECT_INFO, struct nvrm_mthd_fifo_ib_object_info, decode_nvrm_mthd_fifo_ib_object_info),
	_(NVRM_MTHD_FIFO_IB_OBJECT_INFO2, struct nvrm_mthd_fifo_ib_object_info, decode_nvrm_mthd_fifo_ib_object_info),
	_(NVRM_MTHD_FIFO_IB_OBJECT_INFO3, struct nvrm_mthd_fifo_ib_object_info, decode_nvrm_mthd_fifo_ib_object_info),
	_(NVRM_MTHD_FIFO_IB_OBJECT_INFO4, struct nvrm_mthd_fifo_ib_object_info, decode_nvrm_mthd_fifo_ib_object_info),
	_(NVRM_MTHD_CONTEXT_UNK021B, struct nvrm_mthd_context_unk021b, decode_nvrm_mthd_context_unk021b),
	_(NVRM_MTHD_CONTEXT_GET_CPU_INFO, struct nvrm_mthd_context_get_cpu_info, decode_nvrm_mthd_context_get_cpu_info),
};
#undef _
#undef _a
int nvrm_mthds_cnt = ARRAY_SIZE(nvrm_mthds);

void decode_nvrm_ioctl_call(struct nvrm_ioctl_call *s, struct mmt_memory_dump *args, int argc)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_x32(s, mthd);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_ptr(s, ptr, args, argc);
	nvrm_print_x32(s, size);
	nvrm_print_status(s, status);
	nvrm_print_ln();

	struct mmt_buf *data = find_ptr(s->ptr, args, argc);
	if (!data)
	{
		if (!dump_raw_ioctl_data && dump_decoded_ioctl_data)
			dump_args(args, argc, 0);
		return;
	}

	int k, found = 0;
	void (*fun)(void *) = NULL;
	void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

	struct nvrm_mthd *mthd;
	for (k = 0; k < nvrm_mthds_cnt; ++k)
	{
		mthd = &nvrm_mthds[k];
		if (mthd->mthd == s->mthd && mthd->argsize == data->len)
		{
			if (dump_decoded_ioctl_data && !mthd->disabled)
			{
				mmt_log("    %s: ", mthd->name);
				nvrm_pfx = "";
				fun = mthd->fun;
				if (fun)
					fun(data->data);
				fun_with_args = mthd->fun_with_args;
				if (fun_with_args)
					fun_with_args(data->data, args, argc);
			}
			found = 1;
		}
	}
	if (!found)
		mthd = NULL;

	if (!dump_raw_ioctl_data && dump_decoded_ioctl_data && (mthd == NULL || !mthd->disabled))
	{
		if (!found)
			dump_args(args, argc, 0);
		else if (argc != 1 && fun_with_args == NULL)
			dump_args(args, argc, s->ptr);
	}
}
