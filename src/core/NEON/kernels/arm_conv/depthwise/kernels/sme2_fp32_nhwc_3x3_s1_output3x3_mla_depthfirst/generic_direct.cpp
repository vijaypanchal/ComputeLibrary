/*
 * Copyright (c) 2022-2024 Arm Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstddef>
#include <cstdint>

#if defined(ARM_COMPUTE_ENABLE_SME2)

namespace arm_conv {
namespace depthwise {

void sme2_fp32_nhwc_3x3_s1_output3x3_mla_depthfirst_direct_impl(
  const unsigned int n_tile_rows,
  const unsigned int n_tile_cols,
  const float *inptr,
  int64_t ld_input_row,
  int64_t ld_input_col,
  float *outptr,
  int64_t ld_output_row,
  int64_t ld_output_col,
  const void *params,
  unsigned int n_channels,
  const float activation_min,
  const float activation_max
)
{
  struct Args
  {
    const uint64_t n_tile_rows, n_tile_cols;
    const float *inptr;
    const uint64_t ld_input_row;
    const uint64_t ld_input_col;
    float *outptr;
    const uint64_t ld_output_row;
    const uint64_t ld_output_col;
    const void *params;
    const float min, max;

    uint64_t tile_i = 0, tile_j = 0;

    Args(
      const unsigned int n_tile_rows,
      const unsigned int n_tile_cols,
      const float *inptr,
      int64_t ld_input_row,
      int64_t ld_input_col,
      float *outptr,
      int64_t ld_output_row,
      int64_t ld_output_col,
      const void *params,
      const float activation_min,
      const float activation_max
    ) : n_tile_rows(n_tile_rows), n_tile_cols(n_tile_cols), inptr(inptr),
        ld_input_row(ld_input_row), ld_input_col(ld_input_col), outptr(outptr),
        ld_output_row(ld_output_row), ld_output_col(ld_output_col),
        params(params), min(activation_min), max(activation_max)
    {
    }
  };

  Args params_struct(
    n_tile_rows, n_tile_cols,
    inptr, ld_input_row, ld_input_col,
    outptr, ld_output_row, ld_output_col,
    params, activation_min, activation_max
  );

  __asm__ __volatile__(
    ".inst 0xd503477f  // SMSTART ZA\n"
    "mov x2, #0x0\n"
    "mov x3, #0x0\n"
    "ptrue p3.b\n"
    ".inst 0x25207810  // ptrue pn8.b\n"
    "1:"  // Tile loop
    "str x2, [%x[params_struct], %[offsetof_args_tile_i]]\n"
    "mov x22, #0x3\n"
    "str x3, [%x[params_struct], %[offsetof_args_tile_j]]\n"
    "ldr x21, [%x[params_struct], %[offsetof_args_ld_input_row]]\n"
    "ldr x4, [%x[params_struct], %[offsetof_args_ld_input_col]]\n"
    "ldr x5, [%x[params_struct], %[offsetof_args_inptr]]\n"
    "mul x20, x2, x21\n"  // offset = tile_i * ld_input_row
    "ldr x6, [%x[params_struct], %[offsetof_args_params]]\n"
    "madd x20, x3, x4, x20\n"  // offset += tile_j * ld_input_col
    "add x7, x4, x4\n"
    "mul x20, x20, x22\n"  // offset *= kernel_stride * output_size
    "add x8, x7, x4\n"
    "add x5, x5, x20, LSL #2\n"  // inptr[0] += offset * sizeof(float)
    "add x17, x8, x4\n"
    "add x16, x5, x21, LSL #2\n"
    "add x15, x16, x21, LSL #2\n"
    "add x14, x15, x21, LSL #2\n"
    "add x13, x14, x21, LSL #2\n"
    "cbnz x3, 2f\n"
    "ldr x20, [%x[params_struct], %[offsetof_args_n_tile_cols]]\n"
    "lsl x12, %x[n_channels], #0x2\n"
    "mov x28, #0xc\n"
    "mul x28, x28, x4\n"
    "add x27, x15, x7, LSL #2\n"
    "add x26, x5, x17, LSL #2\n"
    "add x25, x16, x7, LSL #2\n"
    "sub x20, x20, x3\n"
    "add x24, x13, x17, LSL #2\n"
    "sub x20, x20, #0x1\n"
    "add x23, x15, x4, LSL #2\n"
    "and x20, x20, #0x3fffff\n"
    "add x22, x5, x4, LSL #2\n"
    "orr x12, x12, x20, LSL #22\n"
    "add x21, x5, x8, LSL #2\n"
    "orr x12, x12, x28, LSL #38\n"
    "add x20, x15, x8, LSL #2\n"
    "add x11, x16, x17, LSL #2\n"
    "add x10, x14, x7, LSL #2\n"
    "add x9, x14, x17, LSL #2\n"
    "add x28, x13, x4, LSL #2\n"
    ".inst 0xf8ac4b7a  // rprfm pldonce, x12, [x27]\n"
    "add x27, x16, x4, LSL #2\n"
    ".inst 0xf8ac48ba  // rprfm pldonce, x12, [x5]\n"
    ".inst 0xf8ac4b5a  // rprfm pldonce, x12, [x26]\n"
    "add x26, x16, x8, LSL #2\n"
    ".inst 0xf8ac49ba  // rprfm pldonce, x12, [x13]\n"
    ".inst 0xf8ac4b3a  // rprfm pldonce, x12, [x25]\n"
    "add x25, x13, x8, LSL #2\n"
    ".inst 0xf8ac4b1a  // rprfm pldonce, x12, [x24]\n"
    "add x24, x14, x4, LSL #2\n"
    ".inst 0xf8ac4afa  // rprfm pldonce, x12, [x23]\n"
    "add x23, x5, x7, LSL #2\n"
    ".inst 0xf8ac4ada  // rprfm pldonce, x12, [x22]\n"
    "add x22, x14, x8, LSL #2\n"
    ".inst 0xf8ac4aba  // rprfm pldonce, x12, [x21]\n"
    "add x21, x15, x17, LSL #2\n"
    ".inst 0xf8ac4a9a  // rprfm pldonce, x12, [x20]\n"
    "add x20, x13, x7, LSL #2\n"
    ".inst 0xf8ac4a1a  // rprfm pldonce, x12, [x16]\n"
    ".inst 0xf8ac497a  // rprfm pldonce, x12, [x11]\n"
    ".inst 0xf8ac49da  // rprfm pldonce, x12, [x14]\n"
    ".inst 0xf8ac495a  // rprfm pldonce, x12, [x10]\n"
    ".inst 0xf8ac493a  // rprfm pldonce, x12, [x9]\n"
    ".inst 0xf8ac4b9a  // rprfm pldonce, x12, [x28]\n"
    ".inst 0xf8ac4b7a  // rprfm pldonce, x12, [x27]\n"
    ".inst 0xf8ac4b5a  // rprfm pldonce, x12, [x26]\n"
    ".inst 0xf8ac4b3a  // rprfm pldonce, x12, [x25]\n"
    ".inst 0xf8ac4b1a  // rprfm pldonce, x12, [x24]\n"
    ".inst 0xf8ac4afa  // rprfm pldonce, x12, [x23]\n"
    ".inst 0xf8ac4ada  // rprfm pldonce, x12, [x22]\n"
    ".inst 0xf8ac49fa  // rprfm pldonce, x12, [x15]\n"
    ".inst 0xf8ac4aba  // rprfm pldonce, x12, [x21]\n"
    ".inst 0xf8ac4a9a  // rprfm pldonce, x12, [x20]\n"
    "2:"  // Tile loop: Prefetch input rows: End
    "ldr x23, [%x[params_struct], %[offsetof_args_ld_output_row]]\n"
    "mov x21, #0x3\n"
    "ld1w { z25.s }, p3/Z, [x6]\n"
    "addvl x6, x6, #1\n"
    "ldr x27, [%x[params_struct], %[offsetof_args_ld_output_col]]\n"
    "cntw x22\n"
    ".inst 0xa040c0c0  // ld1w { z0.s-z3.s }, pn8.b/Z, [x6]\n"
    "addvl x6, x6, #4\n"
    "ldr x26, [%x[params_struct], %[offsetof_args_outptr]]\n"
    "whilelt p2.s, XZR, %x[n_channels]\n"
    ".inst 0xa040c0c4  // ld1w { z4.s-z7.s }, pn8.b/Z, [x6]\n"
    "addvl x6, x6, #4\n"
    "mul x20, x2, x23\n"  // offset = tile_i * ld_output_row
    "cmp x22, %x[n_channels]\n"
    "ld1rw { z15.s }, p3/Z, [%x[params_struct], %[offsetof_args_min]]\n"
    "madd x20, x3, x27, x20\n"  // offset += tile_j * ld_output_col
    "add x25, x27, x27\n"
    "ld1rw { z14.s }, p3/Z, [%x[params_struct], %[offsetof_args_max]]\n"
    "mul x20, x20, x21\n"  // offset *= output_tile_size
    "mov x21, #0x0\n"
    "ld1w { z8.s }, p3/Z, [x6]\n"
    "add x26, x26, x20, LSL #2\n"  // outptrs[0] += offset * sizeof(float)
    "sub x20, XZR, x22\n"
    "ld1w { z9.s }, p2/Z, [x15, x7, LSL #2]\n"
    "add x24, x26, x23, LSL #2\n"
    "ld1w { z10.s }, p2/Z, [x5]\n"
    "addvl x6, x6, #1\n"
    "add x23, x24, x23, LSL #2\n"
    "ld1w { z11.s }, p2/Z, [x5, x17, LSL #2]\n"
    "ld1w { z12.s }, p2/Z, [x13]\n"
    "ld1w { z13.s }, p2/Z, [x16, x7, LSL #2]\n"
    "bge 4f\n"
    "3:"  // Tile loop: Channel loop
    "movprfx z28, z25\n fmla z28.s, p3/M, z7.s, z9.s\n"
    "movprfx z23, z25\n fmla z23.s, p3/M, z8.s, z9.s\n"
    "whilelt p1.s, x22, %x[n_channels]\n"
    "incw x21\n"
    "movprfx z29, z25\n fmla z29.s, p3/M, z6.s, z9.s\n"
    "movprfx z30, z25\n fmla z30.s, p3/M, z5.s, z9.s\n"
    "incw x22\n"
    "mov p0.b, p2.b\n"
    "movprfx z31, z25\n fmla z31.s, p3/M, z4.s, z9.s\n"
    "movprfx z16, z25\n fmla z16.s, p3/M, z3.s, z9.s\n"
    "incw x20\n"
    "movprfx z17, z25\n fmla z17.s, p3/M, z2.s, z9.s\n"
    "movprfx z19, z25\n fmla z19.s, p3/M, z0.s, z9.s\n"
    "fmla z28.s, p3/M, z4.s, z13.s\n"
    "fmla z23.s, p3/M, z0.s, z10.s\n"
    "ld1w { z10.s }, p2/Z, [x15, x8, LSL #2]\n"
    "fmla z29.s, p3/M, z2.s, z11.s\n"
    "ld1w { z20.s }, p2/Z, [x15, x4, LSL #2]\n"
    "fmla z30.s, p3/M, z2.s, z13.s\n"
    "fmla z31.s, p3/M, z1.s, z13.s\n"
    "fmla z16.s, p3/M, z0.s, z13.s\n"
    "fmla z17.s, p3/M, z6.s, z12.s\n"
    "ld1w { z21.s }, p2/Z, [x13, x17, LSL #2]\n"
    "movprfx z18, z25\n fmla z18.s, p3/M, z1.s, z9.s\n"
    "fmla z28.s, p3/M, z6.s, z20.s\n"
    "fmla z23.s, p3/M, z5.s, z13.s\n"
    "ld1w { z25.s }, p3/Z, [x6]\n"
    "addvl x6, x6, #1\n"
    "fmla z29.s, p3/M, z3.s, z13.s\n"
    "ld1w { z27.s }, p2/Z, [x5, x4, LSL #2]\n"
    "fmla z30.s, p3/M, z4.s, z20.s\n"
    "fmla z19.s, p3/M, z8.s, z21.s\n"
    "ld1w { z24.s }, p2/Z, [x5, x8, LSL #2]\n"
    "fmla z31.s, p3/M, z3.s, z20.s\n"
    "fmla z18.s, p3/M, z0.s, z20.s\n"
    "fmla z17.s, p3/M, z1.s, z20.s\n"
    "fmla z28.s, p3/M, z0.s, z27.s\n"
    "fmla z23.s, p3/M, z7.s, z20.s\n"
    "ld1w { z21.s }, p2/Z, [x16]\n"
    "fmla z29.s, p3/M, z1.s, z24.s\n"
    "fmla z16.s, p3/M, z4.s, z10.s\n"
    "fmla z19.s, p3/M, z1.s, z10.s\n"
    "fmla z31.s, p3/M, z5.s, z10.s\n"
    "fmla z18.s, p3/M, z2.s, z10.s\n"
    "fmla z30.s, p3/M, z0.s, z21.s\n"
    "fmla z28.s, p3/M, z2.s, z24.s\n"
    "fmla z23.s, p3/M, z1.s, z27.s\n"
    "ld1w { z13.s }, p2/Z, [x16, x17, LSL #2]\n"
    "ld1w { z20.s }, p2/Z, [x14]\n"
    "fmla z29.s, p3/M, z7.s, z10.s\n"
    "fmla z16.s, p3/M, z2.s, z13.s\n"
    "fmla z28.s, p3/M, z8.s, z10.s\n"
    "fmla z17.s, p3/M, z3.s, z20.s\n"
    "ld1w { z27.s }, p2/Z, [x14, x7, LSL #2]\n"
    "fmla z23.s, p3/M, z3.s, z21.s\n"
    "fmla z29.s, p3/M, z5.s, z13.s\n"
    "ld1w { z22.s }, p2/Z, [x14, x17, LSL #2]\n"
    "fmla z30.s, p3/M, z6.s, z20.s\n"
    "ld1w { z20.s }, p2/Z, [x13, x4, LSL #2]\n"
    "fmla z18.s, p3/M, z4.s, z27.s\n"
    "fmla z19.s, p3/M, z3.s, z27.s\n"
    "ld1w { z21.s }, p2/Z, [x16, x4, LSL #2]\n"
    "fmla z31.s, p3/M, z7.s, z27.s\n"
    "fmla z16.s, p3/M, z6.s, z27.s\n"
    "fmla z17.s, p3/M, z5.s, z27.s\n"
    "fmla z30.s, p3/M, z8.s, z27.s\n"
    "fmla z28.s, p3/M, z3.s, z21.s\n"
    "fmla z19.s, p3/M, z5.s, z22.s\n"
    "fmla z18.s, p3/M, z6.s, z20.s\n"
    "fmla z16.s, p3/M, z8.s, z22.s\n"
    "fmla z31.s, p3/M, z0.s, z21.s\n"
    "ld1w { z9.s }, p2/Z, [x16, x8, LSL #2]\n"
    "addvl x16, x16, #1\n"
    "fmla z17.s, p3/M, z7.s, z20.s\n"
    "ld1w { z20.s }, p2/Z, [x13, x8, LSL #2]\n"
    "fmla z23.s, p3/M, z4.s, z21.s\n"
    "fmla z30.s, p3/M, z1.s, z21.s\n"
    "ld1w { z21.s }, p2/Z, [x14, x4, LSL #2]\n"
    "fmla z28.s, p3/M, z5.s, z9.s\n"
    "fmla z29.s, p3/M, z4.s, z9.s\n"
    "fmla z18.s, p3/M, z8.s, z20.s\n"
    "fmla z19.s, p3/M, z7.s, z20.s\n"
    "ld1w { z12.s }, p2/Z, [x14, x8, LSL #2]\n"
    "addvl x14, x14, #1\n"
    "fmla z31.s, p3/M, z2.s, z9.s\n"
    "fmla z16.s, p3/M, z1.s, z9.s\n"
    "ld1w { z20.s }, p2/Z, [x5, x7, LSL #2]\n"
    "addvl x5, x5, #1\n"
    "fmla z17.s, p3/M, z4.s, z21.s\n"
    "fmla z30.s, p3/M, z7.s, z21.s\n"
    "ld1w { z10.s }, p1/Z, [x5]\n"
    "fmla z18.s, p3/M, z3.s, z21.s\n"
    "fmla z23.s, p3/M, z2.s, z20.s\n"
    "fmla z19.s, p3/M, z4.s, z12.s\n"
    "fmla z31.s, p3/M, z6.s, z21.s\n"
    "ld1w { z11.s }, p2/Z, [x15]\n"
    "fmla z28.s, p3/M, z1.s, z20.s\n"
    "fmla z29.s, p3/M, z0.s, z20.s\n"
    "ld1w { z20.s }, p2/Z, [x15, x17, LSL #2]\n"
    "addvl x15, x15, #1\n"
    "fmla z16.s, p3/M, z7.s, z12.s\n"
    "ld1w { z9.s }, p1/Z, [x15, x7, LSL #2]\n"
    "fmla z18.s, p3/M, z5.s, z12.s\n"
    "fmla z23.s, p3/M, z6.s, z11.s\n"
    "fmla z17.s, p3/M, z0.s, z11.s\n"
    "fmla z19.s, p3/M, z2.s, z20.s\n"
    "fmla z31.s, p3/M, z8.s, z12.s\n"
    "ld1w { z13.s }, p2/Z, [x13, x7, LSL #2]\n"
    "fmla z30.s, p3/M, z3.s, z11.s\n"
    "whilelt p2.s, x21, %x[n_channels]\n"
    "fmla z29.s, p3/M, z8.s, z20.s\n"
    "fmla z16.s, p3/M, z5.s, z20.s\n"
    ".inst 0xa040c0c0  // ld1w { z0.s-z3.s }, pn8.b/Z, [x6]\n"
    "addvl x6, x6, #4\n"
    "fmax z23.s, p3/M, z23.s, z15.s\n"
    "addvl x13, x13, #1\n"
    "cmp x22, %x[n_channels]\n"
    "ld1w { z11.s }, p1/Z, [x5, x17, LSL #2]\n"
    "fmla z17.s, p3/M, z8.s, z13.s\n"
    "fmla z18.s, p3/M, z7.s, z13.s\n"
    "ld1w { z12.s }, p1/Z, [x13]\n"
    "fmla z19.s, p3/M, z6.s, z13.s\n"
    ".inst 0xa040c0c4  // ld1w { z4.s-z7.s }, pn8.b/Z, [x6]\n"
    "addvl x6, x6, #4\n"
    ".inst 0xc1aec9fc  // fclamp { z28.s-z31.s }, z15.s, z14.s\n"
    "ld1w { z13.s }, p1/Z, [x16, x7, LSL #2]\n"
    "fmin z23.s, p3/M, z23.s, z14.s\n"
    "ld1w { z8.s }, p3/Z, [x6]\n"
    "addvl x6, x6, #1\n"
    ".inst 0xc1aec9f0  // fclamp { z16.s-z19.s }, z15.s, z14.s\n"
    "st1w { z30.s }, p0, [x24]\n"
    "st1w { z23.s }, p0, [x26]\n"
    "st1w { z28.s }, p0, [x26, x27, LSL #2]\n"
    "st1w { z29.s }, p0, [x26, x25, LSL #2]\n"
    "addvl x26, x26, #1\n"
    "st1w { z31.s }, p0, [x24, x27, LSL #2]\n"
    "st1w { z16.s }, p0, [x24, x25, LSL #2]\n"
    "addvl x24, x24, #1\n"
    "st1w { z17.s }, p0, [x23]\n"
    "st1w { z18.s }, p0, [x23, x27, LSL #2]\n"
    "st1w { z19.s }, p0, [x23, x25, LSL #2]\n"
    "addvl x23, x23, #1\n"
    "blt 3b\n"
    "4:"  // Tile loop: Channel tail
    "movprfx z20, z25\n fmla z20.s, p3/M, z7.s, z9.s\n"
    "movprfx z24, z25\n fmla z24.s, p3/M, z8.s, z9.s\n"
    "ldr x3, [%x[params_struct], %[offsetof_args_tile_j]]\n"
    "mov p0.b, p2.b\n"
    "movprfx z21, z25\n fmla z21.s, p3/M, z6.s, z9.s\n"
    "movprfx z22, z25\n fmla z22.s, p3/M, z5.s, z9.s\n"
    "ldr x2, [%x[params_struct], %[offsetof_args_tile_i]]\n"
    "movprfx z23, z25\n fmla z23.s, p3/M, z4.s, z9.s\n"
    "movprfx z28, z25\n fmla z28.s, p3/M, z3.s, z9.s\n"
    "ldr x22, [%x[params_struct], %[offsetof_args_n_tile_cols]]\n"
    "movprfx z29, z25\n fmla z29.s, p3/M, z2.s, z9.s\n"
    "movprfx z31, z25\n fmla z31.s, p3/M, z0.s, z9.s\n"
    "ldr x21, [%x[params_struct], %[offsetof_args_n_tile_rows]]\n"
    "add x3, x3, #0x1\n"
    "fmla z20.s, p3/M, z4.s, z13.s\n"
    "fmla z24.s, p3/M, z0.s, z10.s\n"
    "ld1w { z19.s }, p2/Z, [x15, x8, LSL #2]\n"
    "add x20, x2, #0x1\n"
    "fmla z21.s, p3/M, z2.s, z11.s\n"
    "ld1w { z18.s }, p2/Z, [x15, x4, LSL #2]\n"
    "fmla z22.s, p3/M, z2.s, z13.s\n"
    "cmp x3, x22\n"
    "fmla z23.s, p3/M, z1.s, z13.s\n"
    "fmla z28.s, p3/M, z0.s, z13.s\n"
    "csel x2, x2, x20, LT\n"
    "csel x3, x3, XZR, LT\n"
    "fmla z29.s, p3/M, z6.s, z12.s\n"
    "ld1w { z16.s }, p2/Z, [x13, x17, LSL #2]\n"
    "movprfx z30, z25\n fmla z30.s, p3/M, z1.s, z9.s\n"
    "cmp x2, x21\n"
    "fmla z20.s, p3/M, z6.s, z18.s\n"
    "fmla z24.s, p3/M, z5.s, z13.s\n"
    "fmla z21.s, p3/M, z3.s, z13.s\n"
    "ld1w { z17.s }, p2/Z, [x5, x4, LSL #2]\n"
    "fmla z22.s, p3/M, z4.s, z18.s\n"
    "fmla z31.s, p3/M, z8.s, z16.s\n"
    "ld1w { z16.s }, p2/Z, [x5, x8, LSL #2]\n"
    "fmla z23.s, p3/M, z3.s, z18.s\n"
    "fmla z30.s, p3/M, z0.s, z18.s\n"
    "fmla z29.s, p3/M, z1.s, z18.s\n"
    "fmla z20.s, p3/M, z0.s, z17.s\n"
    "fmla z24.s, p3/M, z7.s, z18.s\n"
    "ld1w { z18.s }, p2/Z, [x16]\n"
    "fmla z21.s, p3/M, z1.s, z16.s\n"
    "fmla z28.s, p3/M, z4.s, z19.s\n"
    "fmla z31.s, p3/M, z1.s, z19.s\n"
    "fmla z23.s, p3/M, z5.s, z19.s\n"
    "fmla z30.s, p3/M, z2.s, z19.s\n"
    "fmla z22.s, p3/M, z0.s, z18.s\n"
    "fmla z20.s, p3/M, z2.s, z16.s\n"
    "fmla z24.s, p3/M, z1.s, z17.s\n"
    "ld1w { z17.s }, p2/Z, [x16, x17, LSL #2]\n"
    "ld1w { z16.s }, p2/Z, [x14]\n"
    "fmla z21.s, p3/M, z7.s, z19.s\n"
    "fmla z28.s, p3/M, z2.s, z17.s\n"
    "fmla z20.s, p3/M, z8.s, z19.s\n"
    "fmla z29.s, p3/M, z3.s, z16.s\n"
    "ld1w { z19.s }, p2/Z, [x14, x7, LSL #2]\n"
    "fmla z24.s, p3/M, z3.s, z18.s\n"
    "fmla z21.s, p3/M, z5.s, z17.s\n"
    "ld1w { z18.s }, p2/Z, [x14, x17, LSL #2]\n"
    "fmla z22.s, p3/M, z6.s, z16.s\n"
    "ld1w { z16.s }, p2/Z, [x13, x4, LSL #2]\n"
    "fmla z30.s, p3/M, z4.s, z19.s\n"
    "fmla z31.s, p3/M, z3.s, z19.s\n"
    "ld1w { z17.s }, p2/Z, [x16, x4, LSL #2]\n"
    "fmla z23.s, p3/M, z7.s, z19.s\n"
    "fmla z28.s, p3/M, z6.s, z19.s\n"
    "fmla z29.s, p3/M, z5.s, z19.s\n"
    "fmla z22.s, p3/M, z8.s, z19.s\n"
    "fmla z20.s, p3/M, z3.s, z17.s\n"
    "fmla z31.s, p3/M, z5.s, z18.s\n"
    "fmla z30.s, p3/M, z6.s, z16.s\n"
    "fmla z28.s, p3/M, z8.s, z18.s\n"
    "fmla z23.s, p3/M, z0.s, z17.s\n"
    "ld1w { z18.s }, p2/Z, [x16, x8, LSL #2]\n"
    "fmla z29.s, p3/M, z7.s, z16.s\n"
    "ld1w { z16.s }, p2/Z, [x13, x8, LSL #2]\n"
    "fmla z24.s, p3/M, z4.s, z17.s\n"
    "fmla z22.s, p3/M, z1.s, z17.s\n"
    "ld1w { z17.s }, p2/Z, [x14, x4, LSL #2]\n"
    "fmla z20.s, p3/M, z5.s, z18.s\n"
    "fmla z21.s, p3/M, z4.s, z18.s\n"
    "fmla z30.s, p3/M, z8.s, z16.s\n"
    "fmla z31.s, p3/M, z7.s, z16.s\n"
    "ld1w { z19.s }, p2/Z, [x14, x8, LSL #2]\n"
    "fmla z23.s, p3/M, z2.s, z18.s\n"
    "fmla z28.s, p3/M, z1.s, z18.s\n"
    "ld1w { z16.s }, p2/Z, [x5, x7, LSL #2]\n"
    "fmla z29.s, p3/M, z4.s, z17.s\n"
    "fmla z22.s, p3/M, z7.s, z17.s\n"
    "fmla z30.s, p3/M, z3.s, z17.s\n"
    "fmla z24.s, p3/M, z2.s, z16.s\n"
    "fmla z31.s, p3/M, z4.s, z19.s\n"
    "fmla z23.s, p3/M, z6.s, z17.s\n"
    "ld1w { z18.s }, p2/Z, [x15]\n"
    "fmla z20.s, p3/M, z1.s, z16.s\n"
    "fmla z21.s, p3/M, z0.s, z16.s\n"
    "ld1w { z17.s }, p2/Z, [x15, x17, LSL #2]\n"
    "fmla z28.s, p3/M, z7.s, z19.s\n"
    "fmla z30.s, p3/M, z5.s, z19.s\n"
    "fmla z24.s, p3/M, z6.s, z18.s\n"
    "fmla z29.s, p3/M, z0.s, z18.s\n"
    "fmla z31.s, p3/M, z2.s, z17.s\n"
    "fmla z23.s, p3/M, z8.s, z19.s\n"
    "ld1w { z16.s }, p2/Z, [x13, x7, LSL #2]\n"
    "fmla z22.s, p3/M, z3.s, z18.s\n"
    "fmla z21.s, p3/M, z8.s, z17.s\n"
    "fmla z28.s, p3/M, z5.s, z17.s\n"
    "fmax z24.s, p3/M, z24.s, z15.s\n"
    "fmla z29.s, p3/M, z8.s, z16.s\n"
    "fmla z30.s, p3/M, z7.s, z16.s\n"
    "fmla z31.s, p3/M, z6.s, z16.s\n"
    ".inst 0xc1aec9f4  // fclamp { z20.s-z23.s }, z15.s, z14.s\n"
    "fmin z24.s, p3/M, z24.s, z14.s\n"
    ".inst 0xc1aec9fc  // fclamp { z28.s-z31.s }, z15.s, z14.s\n"
    "st1w { z22.s }, p0, [x24]\n"
    "st1w { z24.s }, p0, [x26]\n"
    "st1w { z20.s }, p0, [x26, x27, LSL #2]\n"
    "st1w { z21.s }, p0, [x26, x25, LSL #2]\n"
    "st1w { z23.s }, p0, [x24, x27, LSL #2]\n"
    "st1w { z28.s }, p0, [x24, x25, LSL #2]\n"
    "st1w { z29.s }, p0, [x23]\n"
    "st1w { z30.s }, p0, [x23, x27, LSL #2]\n"
    "st1w { z31.s }, p0, [x23, x25, LSL #2]\n"
    "blt 1b\n"
    ".inst 0xd503467f  // SMSTOP\n"
    :
    : [n_channels] "r" ((unsigned long) n_channels), [offsetof_args_inptr] "I" (offsetof(Args, inptr)), [offsetof_args_ld_input_col] "I" (offsetof(Args, ld_input_col)), [offsetof_args_ld_input_row] "I" (offsetof(Args, ld_input_row)), [offsetof_args_ld_output_col] "I" (offsetof(Args, ld_output_col)), [offsetof_args_ld_output_row] "I" (offsetof(Args, ld_output_row)), [offsetof_args_max] "I" (offsetof(Args, max)), [offsetof_args_min] "I" (offsetof(Args, min)), [offsetof_args_n_tile_cols] "I" (offsetof(Args, n_tile_cols)), [offsetof_args_n_tile_rows] "I" (offsetof(Args, n_tile_rows)), [offsetof_args_outptr] "I" (offsetof(Args, outptr)), [offsetof_args_params] "I" (offsetof(Args, params)), [offsetof_args_tile_i] "I" (offsetof(Args, tile_i)), [offsetof_args_tile_j] "I" (offsetof(Args, tile_j)), [params_struct] "r" (&params_struct)
    : "cc", "memory", "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8", "p9", "p10", "p11", "p12", "p13", "p14", "p15", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z8", "z9", "z10", "z11", "z12", "z13", "z14", "z15", "z16", "z17", "z18", "z19", "z20", "z21", "z22", "z23", "z24", "z25", "z26", "z27", "z28", "z29", "z30", "z31"
  );
}

}  // namespace depthwise
}  // namespace arm_conv

#endif  // defined(ARM_COMPUTE_ENABLE_SME2)
