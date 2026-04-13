#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

# Baseline vs final (all changes applied)
ops = ['XOR\n(encrypt/decrypt)', 'base16encode', 'base16decode']
baseline = [438.5, 13.8, 78.0]
final    = [512.8, 392.1, 766.2]

x = np.arange(len(ops))
w = 0.35

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 6))
fig.patch.set_facecolor('#f8f8f8')

# --- Left: throughput before/after ---
ax1.set_facecolor('#f8f8f8')
bars_b = ax1.bar(x - w/2, baseline, w, label='Baseline', color='#90CAF9', edgecolor='#1565C0')
bars_f = ax1.bar(x + w/2, final,    w, label='Final (all changes)', color='#A5D6A7', edgecolor='#2E7D32')

for bar, val in zip(bars_b, baseline):
    ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 10,
             f'{val:.0f}', ha='center', va='bottom', fontsize=9, color='#1565C0')
for bar, val, base in zip(bars_f, final, baseline):
    mult = val / base
    ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 10,
             f'{val:.0f}\n({mult:.0f}x)', ha='center', va='bottom', fontsize=9, color='#2E7D32')

ax1.set_xticks(x)
ax1.set_xticklabels(ops, fontsize=11)
ax1.set_ylabel('Throughput (MB/s)', fontsize=12)
ax1.set_ylim(0, 1050)
ax1.set_title('Throughput: baseline vs final', fontsize=12)
ax1.legend(fontsize=10)
ax1.grid(axis='y', linestyle='--', alpha=0.5)

# --- Right: cumulative progression (the original lines, for reference) ---
ax2.set_facecolor('#f8f8f8')
labels_short = ['base', '+buf\n64K', '+enc\nLUT', '+dec\nLUT', '+dead\ncode', '+-Os', '+strip']
x2 = list(range(7))
xor_mbps  = [438.5, 515.4, 492.6, 505.0, 490.1, 518.1, 512.8]
enc_mbps  = [13.8,  13.8,  395.2, 392.1, 425.5, 387.5, 392.1]
dec_mbps  = [78.0,  78.8,  78.2,  896.8, 947.8, 803.2, 766.2]

ax2.plot(x2, xor_mbps, 'o-', color='#2196F3', lw=2, ms=6, label='XOR')
ax2.plot(x2, enc_mbps, 's-', color='#4CAF50', lw=2, ms=6, label='base16encode')
ax2.plot(x2, dec_mbps, '^-', color='#FF9800', lw=2, ms=6, label='base16decode')

# shade the "final" column
ax2.axvspan(5.5, 6.5, color='#A5D6A7', alpha=0.3, label='final config')

ax2.set_xticks(x2)
ax2.set_xticklabels(labels_short, fontsize=9)
ax2.set_ylabel('Throughput (MB/s)', fontsize=12)
ax2.set_ylim(0, 1100)
ax2.set_title('Cumulative progression', fontsize=12)
ax2.legend(fontsize=10)
ax2.grid(True, linestyle='--', alpha=0.5)

plt.suptitle('xor utility — performance improvements\n(100 MB input, Raspberry Pi 5, median of 3 runs)', fontsize=12)
plt.tight_layout()
plt.savefig('bench_combined.png', dpi=150, bbox_inches='tight')
print("Saved bench_combined.png")
