import argparse
import subprocess
import re
import statistics
import platform
from pathlib import Path

import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()

parser.add_argument("images", nargs="+")
parser.add_argument("-n", "--iterations", type=int, default=30)
parser.add_argument("-e", "--executable", default="./build/code_sequentiel")
parser.add_argument("-o", "--output", default="results")

args = parser.parse_args()

cpu = platform.processor()

if platform.system() == "Darwin":
    cpu = subprocess.getoutput("sysctl -n machdep.cpu.brand_string")
    cores = subprocess.getoutput("sysctl -n hw.physicalcpu")
    ram = subprocess.getoutput("sysctl -n hw.memsize")
    ram = f"{int(ram) / (1024 ** 3):.2f} Go"

elif platform.system() == "Linux":
    cpu = subprocess.getoutput(
        "grep -m1 'model name' /proc/cpuinfo"
    ).split(":", 1)[-1].strip()
    cores = subprocess.getoutput(
        "lscpu | grep '^Core(s) per socket:' | awk '{print $4}'"
    )
    ram = subprocess.getoutput(
        "free -h | awk '/^Mem:/ {print $2}'"
    )
else:
    cores = "Non disponible"
    ram = "Non disponible"

os_name = f"{platform.system()} {platform.release()}"
program = Path(args.executable).stem.replace("_", " ").title()

results = []

for image in args.images:
    for i in range(args.iterations):
        process = subprocess.run(
            [args.executable, image],
            capture_output=True,
            text=True,
            check=True
        )

        match = re.search(
            r"Temps de calcul\s+([\d.,]+)\s*s",
            process.stdout
        )

        if not match:
            raise RuntimeError("Temps de calcul introuvable.")

        # Conversion des secondes en millisecondes
        execution_time = float(match.group(1).replace(",", ".")) * 1000

        results.append({
            "image": Path(image).name,
            "time": execution_time
        })

output = Path(args.output)
output.mkdir(parents=True, exist_ok=True)

times = [result["time"] for result in results]
occurrences = list(range(1, len(times) + 1))
mean = statistics.mean(times)

# Graphique
plt.figure(figsize=(12, 7))

plt.plot(
    occurrences,
    times,
    color="steelblue",
    alpha=0.6,
    linewidth=1,
    marker="o",
    markersize=4,
    label="Temps mesurés"
)

plt.axhline(
    mean,
    color="red",
    linestyle="-",
    linewidth=2,
    label=f"Moyenne : {mean:.3f} ms"
)

plt.xlabel("Numéro de l'occurrence")
plt.ylabel("Temps d'exécution (ms)")

plt.suptitle(
    f"Benchmark - {len(times)} exécutions - {program}\n"
    f"CPU : {cpu} | OS : {os_name}",
    fontsize=12
)

plt.legend()
plt.grid(alpha=0.3)
plt.tight_layout(rect=[0, 0, 1, 0.92])
plt.savefig(output / "benchmark.png", dpi=200)
plt.close()

# Export CSV
with open(output / "results.csv", "w", encoding="utf-8") as file:
    file.write("occurrence,image,time_ms\n")

    for i, result in enumerate(results, start=1):
        file.write(
            f"{i},{result['image']},{result['time']:.3f}\n"
        )

# Informations système
gcc = subprocess.getoutput("gcc --version").splitlines()[0]

specs = [
    f"OS : {os_name}",
    f"Version OS : {platform.version()}",
    f"Architecture : {platform.machine()}",
    f"Processeur : {cpu}",
    f"Cœurs physiques : {cores}",
    f"Cœurs logiques : {platform.os.cpu_count() if False else __import__('os').cpu_count()}",
    f"RAM : {ram}",
    f"Compilateur : {gcc}",
    f"Python : {platform.python_version()}",
    f"Exécutable : {args.executable}",
    f"Images : {', '.join(args.images)}",
    f"Itérations par image : {args.iterations}",
    f"Nombre total d'exécutions : {len(results)}",
    f"Temps moyen : {mean:.3f} ms",
    "Options de compilation : -Wall -Wextra -O2 -std=c11",
]

(output / "system.txt").write_text(
    "\n".join(specs),
    encoding="utf-8"
)