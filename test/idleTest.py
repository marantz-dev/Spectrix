import os
import sys

import matplotlib.pyplot as plt
import numpy as np
import scipy.signal
import soundfile as sf

FFT_SIZE = 4096
OVERLAP = 4


def compute_stft_analysis(data, samplerate):
    window = scipy.signal.windows.blackmanharris(FFT_SIZE)
    hop_size = FFT_SIZE // OVERLAP

    f, t, Zxx = scipy.signal.stft(
        data,
        fs=samplerate,
        window=window,
        nperseg=FFT_SIZE,
        noverlap=FFT_SIZE - hop_size,
        return_onesided=True,
    )

    mag_linear = np.mean(np.abs(Zxx), axis=1)
    mag_db = 20 * np.log10(mag_linear + 1e-12)

    mean_vector = np.mean(Zxx, axis=1)
    phase_rad = np.unwrap(np.angle(mean_vector))

    return f, mag_db, phase_rad, Zxx


def smooth_curve(y, window_size=7):
    if window_size < 3:
        return y
    return np.convolve(y, np.ones(window_size) / window_size, mode="same")


def generate_spectrix_report(dry_file, proc1_file, output_file):
    print(f"--- Analisi Spettrale Spectrix ---")

    for f in [dry_file, proc1_file]:
        if not os.path.exists(f):
            print(f"ERRORE: File non trovato: {f}")
            return

    # ---- LOAD ----
    data_dry, sr = sf.read(dry_file)
    data_p1, _ = sf.read(proc1_file)

    data_dry = np.mean(data_dry, axis=1)
    data_p1 = np.mean(data_p1, axis=1)

    # ---- ANALYSIS ----
    f, mag_dry, ph_dry, Z_dry = compute_stft_analysis(data_dry, sr)
    _, mag_p1, ph_p1, Z_p1 = compute_stft_analysis(data_p1, sr)

    # ---- MAGNITUDE DELTAS ----
    delta1 = mag_p1 - mag_dry

    # ---- PHASE DELTAS ----
    def compute_phase_diff(Zx, Zy):
        ratio = np.divide(Zx, Zy, out=np.zeros_like(Zx), where=(np.abs(Zy) > 1e-12))
        ph = np.angle(ratio)
        weights = np.abs(Zy)
        return np.average(ph, axis=1, weights=weights)

    phase1 = compute_phase_diff(Z_p1, Z_dry)

    # ---- SMOOTH ----
    mag_dry_s = smooth_curve(mag_dry)
    mag_p1_s = smooth_curve(mag_p1)

    delta1_s = smooth_curve(delta1)

    phase1_s = smooth_curve(phase1, 11)

    # -------------------------------
    #   PEAK DETECTION
    # -------------------------------
    # Delta 1
    peak1_pos_idx = np.argmax(delta1_s)
    peak1_neg_idx = np.argmin(delta1_s)
    peak1_pos_freq = f[peak1_pos_idx]
    peak1_neg_freq = f[peak1_neg_idx]
    peak1_pos_val = delta1_s[peak1_pos_idx]
    peak1_neg_val = delta1_s[peak1_neg_idx]

    # Phase 1
    phase1_pos_idx = np.argmax(phase1_s)
    phase1_neg_idx = np.argmin(phase1_s)
    phase1_pos_freq = f[phase1_pos_idx]
    phase1_neg_freq = f[phase1_neg_idx]
    phase1_pos_val = phase1_s[phase1_pos_idx]
    phase1_neg_val = phase1_s[phase1_neg_idx]

    print("   -> Generazione grafici...")

    # --- Styling da tesi / scientifico ---
    plt.style.use("default")
    plt.rcParams["axes.edgecolor"] = "#000000"
    plt.rcParams["axes.linewidth"] = 1.1
    plt.rcParams["grid.color"] = "#999999"
    plt.rcParams["grid.linewidth"] = 0.8
    plt.rcParams["grid.alpha"] = 0.6
    plt.rcParams["font.size"] = 11

    fig, axs = plt.subplots(3, 1, figsize=(11, 15), sharex=True)
    fig.suptitle("Analisi Spettrale – IDLE Test", fontsize=16)

    freq_min, freq_max = 20, sr / 2
    freq_ticks = np.array([20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000])
    freq_tick_labels = ["20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"]

    # -----------------------------
    #          PLOT 1
    # -----------------------------
    ax1 = axs[0]
    ax1.semilogx(
        f, mag_dry_s, label="Input", color="#444444", linewidth=1.6, alpha=0.7, zorder=2
    )

    ax1.semilogx(
        f,
        mag_p1_s,
        label="Output",
        color="#1f77b4",
        linewidth=2.0,
        alpha=0.75,
        zorder=3,
    )

    ax1.set_title("1. Risposta in Frequenza (Magnitudine)", fontweight="bold")
    ax1.set_ylabel("Ampiezza (dB)")
    ax1.grid(True, which="both")  # griglia identica alla tua originale
    ax1.legend(loc="lower left")
    ax1.set_ylim([-100, 5])

    # -----------------------------
    #          PLOT 2
    # -----------------------------
    ax2 = axs[1]
    ax2.semilogx(
        f,
        delta1_s,
        color="#1f77b4",
        linewidth=2,
        alpha=0.9,
        label="Delta",
        zorder=2,
    )
    # peak markers Delta1
    ax2.plot(
        peak1_pos_freq, peak1_pos_val, "o", color="#c40000", markersize=6, zorder=5
    )
    ax2.plot(
        peak1_neg_freq, peak1_neg_val, "o", color="#c40000", markersize=6, zorder=5
    )
    ax2.annotate(
        f"{peak1_pos_val:.1f} dB",
        (peak1_pos_freq, peak1_pos_val),
        textcoords="offset points",
        xytext=(8, 8),
        fontsize=9,
        zorder=10,
    )
    ax2.annotate(
        f"{peak1_neg_val:.1f} dB",
        (peak1_neg_freq, peak1_neg_val),
        textcoords="offset points",
        xytext=(8, -12),
        fontsize=9,
        zorder=10,
    )

    ax2.fill_between(f, delta1_s, 0, where=(delta1_s <= 0), color="#c4000033")
    ax2.set_title("2. Differenza di Magnitudine (Delta Gain)", fontweight="bold")
    ax2.set_ylabel("Attenuazione (dB)")
    ax2.grid(True, which="both")
    ax2.axhline(0, color="black", linewidth=1)
    ax2.set_ylim([-60, 20])
    ax2.legend()

    # -----------------------------
    #          PLOT 3
    # -----------------------------
    ax3 = axs[2]
    ax3.semilogx(
        f,
        phase1_s,
        color="#1f77b4",
        linewidth=2.2,
        alpha=0.9,
        label="Phase Delta 1",
        zorder=2,
    )

    # phase1 markers
    ax3.plot(
        phase1_pos_freq, phase1_pos_val, "o", color="#228833", markersize=6, zorder=5
    )
    ax3.plot(
        phase1_neg_freq, phase1_neg_val, "o", color="#228833", markersize=6, zorder=5
    )
    ax3.annotate(
        f"{phase1_pos_val:.2f} rad",
        (phase1_pos_freq, phase1_pos_val),
        textcoords="offset points",
        xytext=(8, 8),
        fontsize=9,
        zorder=10,
    )
    ax3.annotate(
        f"{phase1_neg_val:.2f} rad",
        (phase1_neg_freq, phase1_neg_val),
        textcoords="offset points",
        xytext=(8, -12),
        fontsize=9,
        zorder=10,
    )

    ax3.set_title("3. Errore di Fase (Delta Phase)", fontweight="bold")
    ax3.set_ylabel("Sfasamento (rad)")
    ax3.set_xlabel("Frequenza (Hz)")
    ax3.grid(True, which="both")
    ax3.axhline(0, color="black", linewidth=1)
    ax3.set_ylim([-np.pi, np.pi])
    ax3.set_yticks([-np.pi, -np.pi / 2, 0, np.pi / 2, np.pi])
    ax3.set_yticklabels(
        [r"$-\pi$", r"$-\frac{\pi}{2}$", "0", r"$\frac{\pi}{2}$", r"$\pi$"]
    )
    ax3.legend()

    # -----------------------------
    #     TICKS SU TUTTI I PLOT
    # -----------------------------
    for ax in axs[:-1]:
        ax.tick_params(labelbottom=True)

    for ax in axs:
        ax.set_xlim([freq_min, freq_max])
        ax.set_xticks(freq_ticks)
        ax.set_xticklabels(freq_tick_labels)
        ax.minorticks_on()
        ax.grid(which="minor", linestyle=":", linewidth=0.6, alpha=0.5)

    plt.tight_layout(rect=[0, 0, 1, 0.97])
    plt.savefig(
        output_file,
        format="pdf",
        bbox_inches="tight",
    )
    # plt.show()


if __name__ == "__main__":
    generate_spectrix_report(sys.argv[1], sys.argv[2], sys.argv[3])
