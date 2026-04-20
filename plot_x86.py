#!/usr/bin/env python3
# Compares C-build throughput on Raspberry Pi 5 vs x86_64 (VM).
# x86 measurements taken after adding the test suite (commit 7_tests).
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

ops = ['XOR\n(encrypt/decrypt)', 'base16encode', 'base16decode']

pi5_mbps  = [512.8, 392.1, 766.2]   # 6_strip / 7_tests on Pi5
x86_mbps  = [409.8, 127.3, 687.2]   # 7_tests on x86_64

x = np.arange(len(ops))
w = 0.35

fig, ax = plt.subplots(figsize=(10, 6))
fig.patch.set_facecolor('#f8f8f8')
ax.set_facecolor('#f8f8f8')

bars_p = ax.bar(x - w/2, pi5_mbps, w, label='Raspberry Pi 5 (C, gcc -Os)',
                color='#90CAF9', edgecolor='#1565C0')
bars_x = ax.bar(x + w/2, x86_mbps, w, label='x86_64 Linux VM (C, gcc -Os)',
                color='#FFCC80', edgecolor='#E65100')

for bar, val in zip(bars_p, pi5_mbps):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 8,
            f'{val:.0f}', ha='center', va='bottom', fontsize=10, color='#1565C0')
for bar, val in zip(bars_x, x86_mbps):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 8,
            f'{val:.0f}', ha='center', va='bottom', fontsize=10, color='#E65100')

ax.set_xticks(x)
ax.set_xticklabels(ops, fontsize=12)
ax.set_ylabel('Throughput (MB/s)', fontsize=12)
ax.set_ylim(0, 950)
ax.set_title('xor utility — C build throughput: Raspberry Pi 5 vs x86_64\n'
             '(100 MB input, median of 3 runs; same source, gcc -Os)', fontsize=12)
ax.legend(fontsize=11)
ax.grid(axis='y', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig('bench_x86.png', dpi=150, bbox_inches='tight')
print("Saved bench_x86.png")
