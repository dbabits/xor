#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

ops = ['XOR\n(encrypt/decrypt)', 'base16encode', 'base16decode']
c_mbps   = [510.2, 452.4, 775.1]
asm_mbps = [520.8, 393.7, 719.4]

x = np.arange(len(ops))
w = 0.35

fig, ax = plt.subplots(figsize=(9, 6))
fig.patch.set_facecolor('#f8f8f8')
ax.set_facecolor('#f8f8f8')

bars_c   = ax.bar(x - w/2, c_mbps,   w, label='C (gcc -Os)',    color='#90CAF9', edgecolor='#1565C0')
bars_asm = ax.bar(x + w/2, asm_mbps, w, label='ARM64 assembly', color='#FFCC80', edgecolor='#E65100')

for bar, val in zip(bars_c, c_mbps):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 8,
            f'{val:.0f}', ha='center', va='bottom', fontsize=10, color='#1565C0')

for bar, val, cval in zip(bars_asm, asm_mbps, c_mbps):
    pct = (val - cval) / cval * 100
    sign = '+' if pct >= 0 else ''
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 8,
            f'{val:.0f}\n({sign}{pct:.0f}%)', ha='center', va='bottom', fontsize=10, color='#E65100')

ax.set_xticks(x)
ax.set_xticklabels(ops, fontsize=12)
ax.set_ylabel('Throughput (MB/s)', fontsize=12)
ax.set_ylim(0, 950)
ax.set_title('C vs ARM64 assembly — throughput comparison\n'
             '(100 MB input, Raspberry Pi 5, median of 3 runs)', fontsize=12)
ax.legend(fontsize=11)
ax.grid(axis='y', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig('bench_asm_comparison.png', dpi=150, bbox_inches='tight')
print("Saved bench_asm_comparison.png")
