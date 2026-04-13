#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

labels = ['baseline', '1\nbuffer\n64KB', '2\nencode\nLUT', '3\ndecode\nLUT', '4\ndead\ncode', '5\n-Os', '6\nstrip']
x = list(range(len(labels)))

xor_mbps  = [438.5, 515.4, 492.6, 505.0, 490.1, 518.1, 512.8]
enc_mbps  = [13.8,  13.8,  395.2, 392.1, 425.5, 387.5, 392.1]
dec_mbps  = [78.0,  78.8,  78.2,  896.8, 947.8, 803.2, 766.2]
bin_bytes = [73568, 73568, 73592, 73592, 71424, 71424, 67680]

fig, ax1 = plt.subplots(figsize=(11, 6))
fig.patch.set_facecolor('#f8f8f8')
ax1.set_facecolor('#f8f8f8')

ax1.plot(x, xor_mbps,  'o-', color='#2196F3', linewidth=2, markersize=6, label='XOR MB/s')
ax1.plot(x, enc_mbps,  's-', color='#4CAF50', linewidth=2, markersize=6, label='base16encode MB/s')
ax1.plot(x, dec_mbps,  '^-', color='#FF9800', linewidth=2, markersize=6, label='base16decode MB/s')

ax1.set_ylabel('Throughput (MB/s)', fontsize=12)
ax1.set_ylim(0, 1100)
ax1.yaxis.set_minor_locator(ticker.MultipleLocator(50))
ax1.grid(True, which='major', linestyle='--', alpha=0.5)
ax1.grid(True, which='minor', linestyle=':', alpha=0.3)

ax2 = ax1.twinx()
ax2.plot(x, [b/1024 for b in bin_bytes], 'D--', color='#9C27B0', linewidth=1.5,
         markersize=6, label='Binary size (KB)', alpha=0.7)
ax2.set_ylabel('Binary size (KB)', fontsize=12, color='#9C27B0')
ax2.tick_params(axis='y', labelcolor='#9C27B0')
ax2.set_ylim(0, 120)

ax1.set_xticks(x)
ax1.set_xticklabels(labels, fontsize=10)
ax1.set_xlabel('Commit', fontsize=12)

lines1, labs1 = ax1.get_legend_handles_labels()
lines2, labs2 = ax2.get_legend_handles_labels()
ax1.legend(lines1 + lines2, labs1 + labs2, loc='center right', fontsize=10)

plt.title('xor utility — throughput and binary size across commits\n(100 MB input, Raspberry Pi 5, median of 3 runs)', fontsize=12)
plt.tight_layout()
plt.savefig('bench_results.png', dpi=150, bbox_inches='tight')
print("Saved bench_results.png")
