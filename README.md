# Benchmark d'implémentations C — Traitement d'images PGM

## Présentation

Ce projet vise à développer et comparer différentes implémentations en C d'un algorithme d'amélioration du contraste sur des images au format PGM (*Portable Gray Map*).

L'objectif est de mesurer les performances des différentes implémentations, en commençant par une version séquentielle, puis en étudiant les gains apportés par les futures versions parallèles.

## Structure du projet

```text
projet/
├── implementations/
│   ├── code_sequentiel.c
│   ├── code_openmp.c
│   ├── code_avx_short.c
│   ├── code_avx_char.c
│   └── code_avx_int.c
├── scripts/
│   ├── benchmark.py
│   └── global_benchmarking.py
├── img/
│   └── *.pgm
├── results/
├── build/
├── Makefile
├── requirements.txt
└── README.md
```

| Dossier / fichier | Description |
|---|---|
| `implementations/` | Contient les différentes implémentations en C. |
| `scripts/` | Contient les scripts Python de benchmark. |
| `img/` | Contient les images PGM utilisées pour les tests. |
| `results/` | Contient les résultats des benchmarks. |
| `build/` | Contient les exécutables compilés. |
| `Makefile` | Automatise la compilation et l'exécution. |
| `requirements.txt` | Liste les dépendances Python. |

## Prérequis

- GCC
- GNU Make
- Python 3
- Matplotlib

Installation des dépendances Python :

```bash
pip install -r requirements.txt
```

## Compilation et exécution

### Compiler une implémentation

Par défaut, `make` compile `implementations/code_sequentiel.c`.

```bash
make
```

Pour compiler une autre implémentation :

```bash
make SRC=implementations/nom_implementation.c
```

Les exécutables sont générés dans le dossier `build/`.

La version AVX traite huit pixels à la fois et nécessite un processeur x86
avec AVX. Le Makefile ajoute automatiquement l'option `-mavx` pour cette version.
La version `code_avx_short.c` stocke les pixels sur 16 bits et
`code_avx_int.c` conserve la représentation en `int`.

`code_avx_char.c` stocke chaque pixel sur un octet (`uint8_t`). Cette variante
accepte les fichiers PGM ASCII (P2) dont le maximum déclaré est compris entre 1
et 255. Elle écrit une image étalée sur une échelle de 0 à 255.

```bash
make SRC=implementations/code_avx_short.c
make run SRC=implementations/code_avx_short.c IMAGE=img/image1.pgm
python3 scripts/global_benchmarking.py img/image1.pgm -c code_sequentiel code_avx_int code_avx_short code_avx_char
```

### Version OpenMP

Le Makefile ajoute `-fopenmp` pour cette version. Une même équipe répartit
les lignes de l’image sur les deux passes. La première combine les extrema
locaux avec `reduction`, puis la seconde applique l’étalement.
Par défaut, l’équipe utilise un thread par processeur logique disponible,
selon `omp_get_num_procs()`. On peut fixer une autre taille pour le benchmark
avec `OMP_NUM_THREADS`.
`omp_get_wtime()` mesure la durée écoulée du traitement, hors lecture et
écriture du fichier PGM.

```bash
make run SRC=implementations/code_openmp.c IMAGE=img/image1.pgm
```

Sur macOS, voici la commande vérifiée avec GCC 12 et le SDK des outils
en ligne de commande d’Apple.

```bash
sdk="$(xcrun --sdk macosx --show-sdk-path)"
make run CC=gcc-12 \
  CPPFLAGS="-D_POSIX_C_SOURCE=200809L -isysroot $sdk -isystem $sdk/usr/include" \
  SRC=implementations/code_openmp.c IMAGE=img/image1.pgm
```

### Exécuter une implémentation

```bash
make run IMAGE=img/image.pgm
```

Pour exécuter une autre implémentation :

```bash
make run SRC=implementations/nom_implementation.c IMAGE=img/image.pgm
```

### Nettoyer le projet

```bash
make clean
```

Cette commande supprime les exécutables générés ainsi que les images de sortie correspondant au motif `*_new.pgm`.

## Benchmarks

### Lancer la suite avec `bench.sh`

Depuis le dossier `projet`, préparez l’environnement Python une première fois.

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

Sur une machine x86 avec AVX, le script compare les trois variantes AVX
au séquentiel, puis à OpenMP pour chaque nombre de threads demandé.
Sur Mac, il mesure uniquement OpenMP avec GCC 12.

```bash
bash bench.sh x86 2 4 8
bash bench.sh mac 2 4 8
```

Les nombres après `x86` ou `mac` sont les tailles d’équipe OpenMP testées
séparément. Le script mesure les quatre images d’origine et effectue
30 répétitions par image et par version. Pour un essai rapide à 4 threads,
lancez par exemple `ITERATIONS=2 bash bench.sh mac 4`.

Pour le compte rendu, conservez les résultats dans `results/benchmark_x86`
et `results/benchmark_mac`.

Deux scripts permettent de mesurer les performances des implémentations.

### 1. Benchmark individuel

Le script `scripts/benchmark.py` mesure les performances d'une seule implémentation sur une ou plusieurs images.

Par défaut, il utilise l'exécutable `build/code_sequentiel`.

```bash
python3 scripts/benchmark.py img/image1.pgm img/image2.pgm
```

Options disponibles :

| Option | Description | Valeur par défaut |
|---|---|---|
| `-n`, `--iterations` | Nombre d'exécutions par image | `30` |
| `-e`, `--executable` | Exécutable à benchmarker | `./build/code_sequentiel` |
| `-o`, `--output` | Dossier de sortie | `results` |

Exemples :

```bash
# 30 exécutions par image
python3 scripts/benchmark.py img/image1.pgm

# 100 exécutions par image
python3 scripts/benchmark.py -n 100 img/image1.pgm

# Tester une autre implémentation
python3 scripts/benchmark.py -e ./build/code_sequentiel img/image1.pgm

# Choisir un dossier de sortie
python3 scripts/benchmark.py -o results/code_sequentiel img/image1.pgm
```

### 2. Benchmark comparatif global

Le script `scripts/global_benchmarking.py` permet de comparer plusieurs implémentations C sur un ensemble d'images.

Il détecte automatiquement les fichiers `.c` présents dans `implementations/`, les compile individuellement et exécute chaque programme sur les images sélectionnées.

Par défaut :
- Toutes les implémentations sont testées.
- Toutes les images `.pgm` du dossier `img/` sont utilisées.
- Chaque combinaison implémentation/image est exécutée 30 fois.

```bash
python3 scripts/global_benchmarking.py
```

Options disponibles :

| Option | Description | Valeur par défaut |
|---|---|---|
| `-c`, `--compare` | Implémentations à comparer (sans extension `.c`) | Toutes |
| `-n`, `--iterations` | Nombre d'exécutions par image | `30` |
| `-o`, `--output` | Dossier de sortie | `results/comparaison` |

Exemples :

```bash
# Comparer toutes les implémentations
python3 scripts/global_benchmarking.py

# Comparer uniquement deux implémentations
python3 scripts/global_benchmarking.py -c code_sequentiel code_openmp

# Comparer trois implémentations avec 100 itérations
python3 scripts/global_benchmarking.py -c code_sequentiel code_openmp -n 100

# Utiliser une image spécifique
python3 scripts/global_benchmarking.py img/image1.pgm -c code_sequentiel code_openmp

# Modifier le dossier de sortie
python3 scripts/global_benchmarking.py -o results/test
```

Sur la machine x86 avec AVX, comparez les variantes sur une même image en
fixant la taille de l’équipe OpenMP. Choisissez une valeur de `n` adaptée
à la machine et conservez un dossier de résultats par valeur.

```bash
n=4
OMP_DYNAMIC=FALSE OMP_NUM_THREADS="$n" \
  python3 scripts/global_benchmarking.py img/stavrovouni.pgm -n 30 \
  -c code_sequentiel code_avx_int code_avx_short code_avx_char code_openmp \
  -o "results/avx_openmp_${n}_threads"
```

### Résultats générés

Le benchmark individuel génère :
- Un graphique des temps d'exécution.
- Un fichier CSV contenant les mesures.
- Un fichier `system.txt` contenant les informations sur l'environnement de test.

Le benchmark comparatif global génère :

```text
results/
└── comparaison/
    ├── benchmark_comparatif.png
    ├── results.csv
    ├── summary.csv
    └── system.txt
```

Le graphique comparatif affiche les mesures individuelles de chaque implémentation avec une couleur distincte.
Sa légende, le terminal et `summary.csv` affichent la moyenne de toutes
les mesures sélectionnées pour chaque implémentation.

Le fichier CSV contient les colonnes suivantes :

| Colonne | Description |
|---|---|
| `implementation` | Nom de l'implémentation testée |
| `occurrence` | Numéro de l'exécution |
| `image` | Image utilisée |
| `time_ms` | Temps d'exécution en millisecondes |

Le fichier `system.txt` conserve les informations matérielles et logicielles de la machine afin de faciliter la reproductibilité des expériences.

## Mesure des performances

Les versions mesurent la durée écoulée du calcul, hors lecture et écriture
du PGM. OpenMP inclut le lancement et la synchronisation de l’équipe.
Les scripts convertissent les secondes affichées en millisecondes.
