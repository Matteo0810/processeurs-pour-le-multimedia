
import argparse
import subprocess
import re
import platform
from pathlib import Path

import matplotlib.pyplot as plt


# Configuration
PROJECT_DIR = Path(__file__).resolve().parent.parent
IMPLEMENTATIONS_DIR = PROJECT_DIR / "implementations"
IMAGES_DIR = PROJECT_DIR / "img"
BUILD_DIR = PROJECT_DIR / "build"


# Arguments
parser = argparse.ArgumentParser(
    description="Benchmark comparatif de toutes les implémentations C."
)

parser.add_argument(
    "images",
    nargs="*",
    help="Images PGM à utiliser. Par défaut, toutes les images du dossier img/."
)

parser.add_argument(
    "-n", "--iterations",
    type=int,
    default=30,
    help="Nombre d'exécutions par image (défaut : 30)."
)

parser.add_argument(
    "-o", "--output",
    default="results/comparaison",
    help="Dossier de sortie des résultats."
)

args = parser.parse_args()

if args.iterations < 1:
    parser.error("Le nombre d'itérations doit être supérieur à 0.")


# Détection des implémentations
implementations = sorted(IMPLEMENTATIONS_DIR.glob("*.c"))

if not implementations:
    raise RuntimeError(
        f"Aucun fichier C trouvé dans {IMPLEMENTATIONS_DIR}"
    )


# Détection des images
if args.images:
    images = [Path(image).resolve() for image in args.images]
else:
    images = sorted(IMAGES_DIR.glob("*.pgm"))

if not images:
    raise RuntimeError("Aucune image PGM trouvée.")

for image in images:
    if not image.is_file():
        raise FileNotFoundError(f"Image introuvable : {image}")


# Informations système
cpu = platform.processor()

if platform.system() == "Darwin":
    cpu = subprocess.getoutput(
        "sysctl -n machdep.cpu.brand_string"
    )
    cores = subprocess.getoutput(
        "sysctl -n hw.physicalcpu"
    )
    ram = subprocess.getoutput(
        "sysctl -n hw.memsize"
    )
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


# Préparation des résultats
results = {
    source.stem: []
    for source in implementations
}

total_runs = len(implementations) * len(images) * args.iterations
current_run = 0


# Benchmark
for source in implementations:
    program = source.stem
    executable = BUILD_DIR / program

    print(f"\n=== {program} ===")

    # Compilation
    subprocess.run(
        [
            "make",
            "-C", str(PROJECT_DIR),
            f"SRC=implementations/{source.name}"
        ],
        check=True
    )

    # Exécution sur chaque image
    for image in images:
        print(f"\nImage : {image.name}")

        for iteration in range(args.iterations):
            current_run += 1

            process = subprocess.run(
                [str(executable), str(image)],
                capture_output=True,
                text=True,
                check=True
            )

            match = re.search(
                r"Temps de calcul\s+([\d.,]+)\s*s",
                process.stdout
            )

            if not match:
                raise RuntimeError(
                    f"Temps de calcul introuvable pour {program} "
                    f"sur {image.name}."
                )

            # Conversion secondes vers millisecondes
            execution_time = (
                float(match.group(1).replace(",", ".")) * 1000
            )

            results[program].append({
                "image": image.name,
                "time": execution_time
            })

            print(
                f"\rExécution {current_run}/{total_runs}",
                end="",
                flush=True
            )

        print()


# Préparation du dossier de sortie
output = Path(args.output)
output.mkdir(parents=True, exist_ok=True)


# Graphique comparatif
plt.figure(figsize=(14, 8))

colors = plt.get_cmap("tab20").colors

for index, (program, measurements) in enumerate(results.items()):
    color = colors[index % len(colors)]

    times = [
        measurement["time"]
        for measurement in measurements
    ]

    occurrences = list(range(1, len(times) + 1))

    # Mesures individuelles
    plt.plot(
        occurrences,
        times,
        color=color,
        alpha=0.65,
        linewidth=1,
        marker="o",
        markersize=4,
        label=program
    )

plt.xlabel("Numéro de l'occurrence")
plt.ylabel("Temps d'exécution (ms)")

plt.title(
    f"Benchmark comparatif - {total_runs} exécutions\n"
    f"CPU : {cpu} | OS : {os_name}"
)

plt.legend()
plt.grid(alpha=0.3)
plt.tight_layout()

plt.savefig(output / "benchmark_comparatif.png", dpi=200)
plt.close()


# Export CSV
with open(
    output / "results.csv",
    "w",
    encoding="utf-8"
) as file:
    file.write("implementation,occurrence,image,time_ms\n")

    for program, measurements in results.items():
        for occurrence, measurement in enumerate(
            measurements,
            start=1
        ):
            file.write(
                f"{program},"
                f"{occurrence},"
                f"{measurement['image']},"
                f"{measurement['time']:.3f}\n"
            )


# Informations système
gcc = subprocess.getoutput(
    "gcc --version"
).splitlines()[0]

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
    f"Images : {', '.join(image.name for image in images)}",
    f"Itérations par image : {args.iterations}",
    f"Nombre d'implémentations : {len(implementations)}",
    f"Nombre total d'exécutions : {total_runs}",
    "Options de compilation : -Wall -Wextra -O2 -std=c11",
]

(output / "system.txt").write_text(
    "\n".join(specs),
    encoding="utf-8"
)

print("\n=== Benchmark terminé ===")
print(f"Implémentations testées : {len(implementations)}")
print(f"Images utilisées : {len(images)}")
print(f"Nombre total d'exécutions : {total_runs}")
print(f"Résultats enregistrés dans : {output}")
