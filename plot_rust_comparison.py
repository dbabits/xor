#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

ops = ['XOR\n(encrypt/decrypt)', 'base16encode', 'base16decode']

# Fresh measurements — Raspberry Pi 5, 100 MB input, median of 3 runs
c_mbps          = [503,  452,  797]
neon_asm_mbps   = [2778, 1351, 2128]
rust_scalar_mbps= [467,  485,  1156]
rust_neon_mbps  = [4167, 1282, 1653]

x = np.arange(len(ops))
w = 0.20

fig, ax = plt.subplots(figsize=(13, 7))
fig.patch.set_facecolor('#f8f8f8')
ax.set_facecolor('#f8f8f8')

bars_c  = ax.bar(x - 1.5*w, c_mbps,           w, label='C (gcc -Os)',    color='#90CAF9', edgecolor='#1565C0')
bars_na = ax.bar(x - 0.5*w, neon_asm_mbps,     w, label='NEON ASM',       color='#A5D6A7', edgecolor='#2E7D32')
bars_rs = ax.bar(x + 0.5*w, rust_scalar_mbps,  w, label='Rust (scalar)',  color='#FFCCBC', edgecolor='#BF360C')
bars_rn = ax.bar(x + 1.5*w, rust_neon_mbps,    w, label='Rust (NEON)',    color='#CE93D8', edgecolor='#6A1B9A')

def annotate(bars, vals, ref_vals, color):
    for bar, val, ref in zip(bars, vals, ref_vals):
        mult = val / ref
        label = f'{val}\n({mult:.1f}x)' if abs(mult - 1.0) > 0.04 else f'{val}'
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 30,
                label, ha='center', va='bottom', fontsize=8, color=color)

annotate(bars_c,  c_mbps,          c_mbps,  '#1565C0')
annotate(bars_na, neon_asm_mbps,   c_mbps,  '#2E7D32')
annotate(bars_rs, rust_scalar_mbps,c_mbps,  '#BF360C')
annotate(bars_rn, rust_neon_mbps,  c_mbps,  '#6A1B9A')

ax.set_xticks(x)
ax.set_xticklabels(ops, fontsize=12)
ax.set_ylabel('Throughput (MB/s)', fontsize=12)
ax.set_ylim(0, 5200)
ax.set_title('C vs NEON ASM vs Rust (scalar) vs Rust (NEON) — throughput comparison\n'
             '(100 MB input, Raspberry Pi 5, median of 3 runs)', fontsize=12)
ax.legend(fontsize=11)
ax.grid(axis='y', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig('bench_rust_comparison.png', dpi=150, bbox_inches='tight')
print("Saved bench_rust_comparison.png")
