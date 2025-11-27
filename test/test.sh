#!/usr/bin/env bash

DIR="./abletonProject/Samples/Recorded"

# Usa find + stat di macOS (BSD) per ottenere timestamp + file
files=$(while IFS= read -r f; do
    ts=$(stat -f "%m" "$f") # timestamp modifica (BSD)
    echo "$ts|$f"
done < <(find "$DIR" -type f -name "*.wav") |
    sort -nr | head -n 3 | cut -d'|' -f2-)

# Converti l'output in array senza rompere gli spazi
IFS=$'\n' read -rd '' -a arr <<<"$files"

# Controlla che ci siano almeno due file
if [ ${#arr[@]} -lt 3 ]; then
    echo "Non ci sono almeno due file .wav nella cartella."
    exit 1
fi

echo "File da analizzare:"
echo "${arr[0]}"
echo "${arr[1]}"
echo "${arr[2]}"

# Passa i due file allo script Python
uv run analyzer.py "${arr[2]}" "${arr[1]}" "${arr[0]}"
