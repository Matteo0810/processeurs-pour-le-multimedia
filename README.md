# Benchmark d'implémentations C — Traitement d'images PGM

## Présentation

Ce projet vise à développer et comparer différentes implémentations en C d'un algorithme d'amélioration du contraste sur des images au format PGM (*Portable Gray Map*).

L'objectif est de mesurer les performances des différentes implémentations, en commençant par une version séquentielle, puis en étudiant les gains apportés par les futures versions parallèles.

## Structure du projet

```text
projet/
├── implementations/
│   ├── code_sequentiel.c
│   ├── code_avx_short.c
│   ├── code_avx_char.c
│   └── code_avx_int.c
├── scripts/
│   ├── benchmark.py
│   └── global_bencharmking.py
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

Le script `scripts/global_bencharmking.py` permet de comparer plusieurs implémentations C sur un ensemble d'images.

Il détecte automatiquement les fichiers `.c` présents dans `implementations/`, les compile individuellement et exécute chaque programme sur les images sélectionnées.

Par défaut :
- Toutes les implémentations sont testées.
- Toutes les images `.pgm` du dossier `img/` sont utilisées.
- Chaque combinaison implémentation/image est exécutée 30 fois.

```bash
python3 scripts/global_bencharmking.py
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
python3 scripts/global_bencharmking.py

# Comparer uniquement deux implémentations
python3 scripts/global_bencharmking.py -c code_sequentiel code_openmp

# Comparer trois implémentations avec 100 itérations
python3 scripts/global_bencharmking.py -c code_sequentiel code_openmp code_pthread -n 100

# Utiliser une image spécifique
python3 scripts/global_bencharmking.py img/image1.pgm -c code_sequentiel code_openmp

# Modifier le dossier de sortie
python3 scripts/global_bencharmking.py -o results/test
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

Les temps mesurés correspondent au temps consacré au calcul de l'algorithme, exprimé en secondes par le programme C puis converti en millisecondes par les scripts Python.
