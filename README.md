# Traitement d'images PGM

## Présentation

Ce projet porte sur l'amélioration du contraste d'images au format PGM.

L'objectif est d'implémenter un algorithme de traitement d'image, puis d'évaluer ses performances à travers des mesures de temps d'exécution.

Le projet commence par une implémentation séquentielle, qui servira de référence pour les futures évolutions.

## Structure du projet

```text
.
├── implementations/
│   └── code_sequentiel.c
├── scripts/
│   ├── benchmark.py
│   └── global_bencharmking.py
├── img/
├── results/
├── build/
├── Makefile
├── requirements.txt
└── README.md
```

| Répertoire / fichier | Description |
|---|---|
| `implementations/` | Contient les implémentations en C |
| `scripts/` | Contient les scripts Python de benchmark |
| `img/` | Contient les images utilisées pour les tests |
| `results/` | Contient les résultats des benchmarks |
| `build/` | Contient les exécutables générés |
| `Makefile` | Automatise la compilation et l'exécution |
| `requirements.txt` | Liste les dépendances Python |

## Installation

### Prérequis

- GCC
- Make
- Python 3

Installer les dépendances Python :

```bash
pip install -r requirements.txt
```

## Compilation

Compiler l'implémentation par défaut :

```bash
make
```

Par défaut, le Makefile utilise :

```text
implementations/code_sequentiel.c
```

L'exécutable est généré dans :

```text
build/code_sequentiel
```

### Choisir un fichier source

Le fichier source peut être modifié directement depuis la commande `make` :

```bash
make SRC=implementations/code_sequentiel.c
```

Le nom de l'exécutable est automatiquement déterminé à partir du nom du fichier source.

## Exécution

Pour exécuter le programme sur une image :

```bash
make run IMAGE=img/image1.pgm
```

Il est également possible de préciser le fichier source :

```bash
make run SRC=implementations/code_sequentiel.c IMAGE=img/image1.pgm
```

## Benchmark individuel

Le script `scripts/benchmark.py` permet de mesurer les temps d'exécution d'une implémentation sur une ou plusieurs images.

### Utilisation

```bash
python3 scripts/benchmark.py img/image1.pgm
```

Par défaut :
- 30 exécutions par image ;
- exécutable : `./build/code_sequentiel` ;
- dossier de sortie : `results/`.

### Options

| Option | Description | Valeur par défaut |
|---|---|---|
| `-n`, `--iterations` | Nombre d'exécutions par image | `30` |
| `-e`, `--executable` | Chemin vers l'exécutable | `./build/code_sequentiel` |
| `-o`, `--output` | Dossier de sortie | `results` |

### Exemples

```bash
# Benchmark sur une image
python3 scripts/benchmark.py img/image1.pgm

# 50 exécutions sur plusieurs images
python3 scripts/benchmark.py -n 50 img/image1.pgm img/image2.pgm

# Choisir un dossier de sortie
python3 scripts/benchmark.py -o results/sequentiel img/image1.pgm
```

## Benchmark comparatif global

Le script `scripts/global_bencharmking.py` permet de comparer automatiquement toutes les implémentations présentes dans le dossier `implementations/`.

Il détecte les fichiers `.c`, les compile avec le Makefile, puis exécute chaque programme sur les images sélectionnées.

### Fonctionnement

- Détection automatique de toutes les implémentations `.c`.
- Compilation automatique de chaque implémentation.
- Utilisation de toutes les images `.pgm` du dossier `img/` par défaut.
- Possibilité de sélectionner uniquement certaines images.
- Nombre d'itérations configurable.
- Une couleur différente pour chaque implémentation.
- Affichage des mesures individuelles sur un graphique commun.
- Export des résultats au format CSV et des informations système.

Les temps sont exprimés en millisecondes.

### Utilisation

Lancer le benchmark sur toutes les implémentations et toutes les images :

```bash
python3 scripts/global_bencharmking.py
```

Par défaut :
- 30 exécutions par image et par implémentation ;
- toutes les images `.pgm` du dossier `img/` ;
- toutes les implémentations `.c` du dossier `implementations/` ;
- dossier de sortie : `results/comparaison`.

### Options

| Option | Description | Valeur par défaut |
|---|---|---|
| `-n`, `--iterations` | Nombre d'exécutions par image | `30` |
| `-o`, `--output` | Dossier de sortie | `results/comparaison` |
| `images` | Images à utiliser | Toutes les images du dossier `img/` |

### Exemples

```bash
# Toutes les implémentations et toutes les images
python3 scripts/global_bencharmking.py

# 50 exécutions par image
python3 scripts/global_bencharmking.py -n 50

# Sélectionner certaines images
python3 scripts/global_bencharmking.py img/image1.pgm img/image2.pgm

# Personnaliser le nombre d'itérations et le dossier de sortie
python3 scripts/global_bencharmking.py -n 50 -o results/comparaison_50
```

### Résultats générés

Le script génère trois fichiers dans le dossier de sortie :

| Fichier | Description |
|---|---|
| `benchmark_comparatif.png` | Graphique comparant les temps d'exécution de chaque implémentation |
| `results.csv` | Ensemble des mesures individuelles, avec l'implémentation et l'image correspondantes |
| `system.txt` | Informations sur la machine et l'environnement de benchmark |

## Nettoyage

Pour supprimer les exécutables générés et les images temporaires :

```bash
make clean
```

## Dépendances Python

Les dépendances nécessaires aux benchmarks sont listées dans `requirements.txt`.