#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

ops = ['XOR\n(encrypt/decrypt)', 'base16encode', 'base16decode']

# Previous (scalar) assembly result, kept for reference
scalar_mbps = [521,   394,   719]
# C compiler result
c_mbps      = [521,   435,   775]
# NEON assembly result
neon_mbps   = [3704, 1333,  1429]

x = np.arange(len(ops))
w = 0.26

fig, ax = plt.subplots(figsize=(11, 7))
fig.patch.set_facecolor('#f8f8f8')
ax.set_facecolor('#f8f8f8')

bars_s = ax.bar(x - w,   scalar_mbps, w, label='Scalar ASM',   color='#FFE0B2', edgecolor='#E65100')
bars_c = ax.bar(x,       c_mbps,      w, label='C (gcc -Os)',   color='#90CAF9', edgecolor='#1565C0')
bars_n = ax.bar(x + w,   neon_mbps,   w, label='NEON ASM',      color='#A5D6A7', edgecolor='#2E7D32')

for bar, val in zip(bars_s, scalar_mbps):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 20,
            f'{val}', ha='center', va='bottom', fontsize=9, color='#E65100')

for bar, val in zip(bars_c, c_mbps):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 20,
            f'{val}', ha='center', va='bottom', fontsize=9, color='#1565C0')

for bar, val, cval in zip(bars_n, neon_mbps, c_mbps):
    mult = val / cval
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 20,
            f'{val}\n({mult:.1f}x)', ha='center', va='bottom', fontsize=9, color='#2E7D32')

ax.set_xticks(x)
ax.set_xticklabels(ops, fontsize=12)
ax.set_ylabel('Throughput (MB/s)', fontsize=12)
ax.set_ylim(0, 4600)
ax.set_title('Scalar ASM vs C vs NEON ASM — throughput comparison\n'
             '(100 MB input, Raspberry Pi 5, median of 3 runs)', fontsize=12)
ax.legend(fontsize=11)
ax.grid(axis='y', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig('bench_asm_comparison.png', dpi=150, bbox_inches='tight')
print("Saved bench_asm_comparison.png")
