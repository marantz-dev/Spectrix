#!/opt/homebrew/bin/bash

DIR="./abletonProject/Samples/Recorded"

# Funzione che prende una keyword e ritorna i 3 file più recenti che la contengono
get_top3() {
    local keyword="$1"

    files=$(while IFS= read -r f; do
        ts=$(stat -f "%m" "$f")
        echo "$ts|$f"
    done < <(find "$DIR" -type f -name "*${keyword}*.wav") |
        sort -nr | head -n 3 | cut -d'|' -f2-)

    IFS=$'\n' read -rd '' -a arr <<<"$files" 2>/dev/null

    if [ ${#arr[@]} -lt 2 ]; then
        echo "Non ho trovato abbastanza file per '${keyword}'" >&2
        exit 1
    fi

    for f in "${arr[@]}"; do
        echo "$f"
    done
}
# ----- 1) Recupera i 3 file con "Saw" -----
mapfile -t saw_files < <(get_top3 "Saw")

echo "File Saw:"
printf '%s\n' "${saw_files[@]}"

uv run analyzer.py \
    "${saw_files[1]}" \
    "${saw_files[2]}" \
    "${saw_files[0]}" \
    "/Users/riccardomarantonio/dev/university/Tesi_Triennale/immagini/saw_report.pdf" \
    "Compressed" "Gated"

# ----- 2) Recupera i 3 file con "Noise" -----
mapfile -t noise_files < <(get_top3 "Noise")

echo "File Noise:"
printf '%s\n' "${noise_files[@]}"

uv run analyzer.py \
    "${noise_files[1]}" \
    "${noise_files[2]}" \
    "${noise_files[0]}" \
    "/Users/riccardomarantonio/dev/university/Tesi_Triennale/immagini/noise_report.pdf" \
    "Compressed" "Gated"

mapfile -t noise_files < <(get_top3 "IDLE")

echo "File IDLE:"
printf '%s\n' "${noise_files[@]}"

uv run idleTest.py \
    "${noise_files[1]}" \
    "${noise_files[0]}" \
    "/Users/riccardomarantonio/dev/university/Tesi_Triennale/immagini/idle_report.pdf"

open "/Users/riccardomarantonio/dev/university/Tesi_Triennale/immagini"
