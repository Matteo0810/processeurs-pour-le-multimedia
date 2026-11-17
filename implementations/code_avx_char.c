/*==============================================================================*/
/* Programme : code_avx_char.c                                                  */
/* Auteur    : Daniel CHILLET                                                   */
/* Version   : 2025 10                                                          */
/* Objet     : Rehaussement de contraste                                        */
/*==============================================================================*/


#include <immintrin.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/*==============================================================================*/
/* Macros utiles pour la mesure du temps                                        */
/*==============================================================================*/

#define InitClock    struct timespec start, stop
#define ClockStart   clock_gettime(CLOCK_MONOTONIC, &start)
#define ClockEnd   clock_gettime(CLOCK_MONOTONIC, &stop)
#define BILLION  1000000000L
#define ClockMesure_Sec "Temps de calcul %2.9f s\n",(( stop.tv_sec - start.tv_sec )+ (stop.tv_nsec - start.tv_nsec )/(double)BILLION)

#define ClockMesure_m_Sec "Temps de calcul %2.6f ms\n",(( stop.tv_sec - start.tv_sec )+ (stop.tv_nsec - start.tv_nsec )/(double)BILLION*1000)

#define ClockMesure_u_Sec "Temps de calcul %2.3f us\n",(( stop.tv_sec - start.tv_sec )+ (stop.tv_nsec - start.tv_nsec )/(double)BILLION*1000000)

/*==============================================================================*/
/* Constantes et macros utiles                                                  */
/*==============================================================================*/
#define MAX_CHAINE 256
#define MAX_VALEUR 255
#define NBPOINTSPARLIGNES 15

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define CHECK_ALLOC(ptr) if (!(ptr)) { fprintf(stderr, "Erreur d'allocation mémoire\n"); exit(EXIT_FAILURE); }

/*==============================================================================*/
/* Structures                                                                   */
/*==============================================================================*/
typedef struct {
    int width;
    int height;
    int maxval;
    uint8_t **data;
} ImagePGM;

/*==============================================================================*/
/* Allocation et libération d'image                                             */
/*==============================================================================*/
uint8_t **alloc_image(int height, int width) {
    uint8_t **img = malloc(height * sizeof(*img));
    CHECK_ALLOC(img);
    for (int i = 0; i < height; i++) {
        img[i] = calloc(width, sizeof(*img[i]));
        CHECK_ALLOC(img[i]);
    }
    return img;
}

void free_image(ImagePGM img) {
    for (int i = 0; i < img.height; i++)
        free(img.data[i]);
    free(img.data);
}

/*==============================================================================*/
/* Lecture d'un fichier PGM ASCII (format P2)                                   */
/*==============================================================================*/
// Lire les nombres P2, avec commentaires et espaces entre les champs.
static int lire_entier_pgm(FILE *f, int limite) {
    int c = fgetc(f);
    while (c != EOF) {
        if (c == '#') {
            do {
                c = fgetc(f);
            } while (c != '\n' && c != EOF);
        } else if (!isspace(c)) {
            break;
        } else {
            c = fgetc(f);
        }
    }

    if (c < '0' || c > '9')
        return -1;

    int valeur = 0;
    do {
        int chiffre = c - '0';
        if (valeur > limite / 10 ||
            (valeur == limite / 10 && chiffre > limite % 10))
            return -1;
        valeur = valeur * 10 + chiffre;
        c = fgetc(f);
    } while (c >= '0' && c <= '9');

    if (c == '#')
        ungetc(c, f);
    else if (c != EOF && !isspace(c))
        return -1;

    return valeur;
}

ImagePGM read_pgm(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Erreur ouverture fichier source");
        exit(EXIT_FAILURE);
    }

    if (fgetc(f) != 'P' || fgetc(f) != '2' || !isspace(fgetc(f))) {
        fprintf(stderr, "Erreur : format non supporté (P2 attendu)\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    ImagePGM img;
    img.width = lire_entier_pgm(f, INT_MAX);
    img.height = lire_entier_pgm(f, INT_MAX);
    img.maxval = lire_entier_pgm(f, UINT8_MAX);
    if (img.width <= 0 || img.height <= 0 || img.maxval <= 0 ||
        (size_t)img.height > SIZE_MAX / sizeof(*img.data)) {
        fprintf(stderr, "Erreur : en-tête PGM invalide (dimensions positives, maxval entre 1 et 255)\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    img.data = alloc_image(img.height, img.width);
    for (int i = 0; i < img.height; i++) {
        for (int j = 0; j < img.width; j++) {
            int pixel = lire_entier_pgm(f, img.maxval);
            if (pixel < 0) {
                fprintf(stderr, "Erreur : pixel absent ou invalide à la ligne %d, colonne %d\n",
                        i + 1, j + 1);
                free_image(img);
                fclose(f);
                exit(EXIT_FAILURE);
            }
            img.data[i][j] = (uint8_t)pixel;
        }
    }

    fclose(f);
    return img;
}

/*==============================================================================*/
/* Écriture d'un fichier PGM ASCII (format P2)                                  */
/*==============================================================================*/
void write_pgm(const char *filename, ImagePGM img) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Erreur ouverture fichier destination");
        exit(EXIT_FAILURE);
    }

    fprintf(f, "P2\n");
    fprintf(f, "# Image étalée automatiquement\n");
    fprintf(f, "%d %d\n%d\n", img.width, img.height, img.maxval);

    int n = 0;
    for (int i = 0; i < img.height; i++) {
        for (int j = 0; j < img.width; j++) {
            fprintf(f, "%3u ", (unsigned)img.data[i][j]);
            n++;
            if (n == NBPOINTSPARLIGNES) {
                n = 0;
                fprintf(f, "\n");
            }
        }
    }
    fprintf(f, "\n");
    fclose(f);
}

/*==============================================================================*/
/* Etalement du contraste                                                       */
/*==============================================================================*/
// Lire quatre octets sans exiger d'alignement, puis élargir chaque pixel à 32 bits.
static inline __m128i lire_quatre_pixels(const uint8_t *pixels) {
    int octets;
    memcpy(&octets, pixels, sizeof(octets));
    return _mm_cvtepu8_epi32(_mm_cvtsi32_si128(octets));
}

// Étaler huit pixels et ramener les résultats à huit entiers de 16 bits.
static inline __m128i etaler_huit_pixels(const uint8_t *pixels, __m256 minimum, __m256 facteurs) {
    __m128i bas = lire_quatre_pixels(pixels);
    __m128i haut = lire_quatre_pixels(pixels + 4);
    __m256 valeurs = _mm256_cvtepi32_ps(_mm256_set_m128i(haut, bas));

    __m256 ecarts = _mm256_sub_ps(valeurs, minimum);
    __m256 etales = _mm256_mul_ps(ecarts, facteurs);
    __m256i entiers = _mm256_cvttps_epi32(etales); // Tronquer comme le cast en C.
    __m128i moities[2];
    _mm256_storeu_si256((__m256i *)moities, entiers);
    return _mm_packs_epi32(moities[0], moities[1]);
}

void etalement_lineaire(ImagePGM in, ImagePGM out) {
    __m128i min_octets = _mm_set1_epi32(-1); // 0xFF dans chaque octet.
    __m128i max_octets = _mm_setzero_si128();
    int min = MAX_VALEUR, max = 0;
    const int fin_blocs = in.width - in.width % 16;

    for (int i = 0; i < in.height; i++) {
        // Comparer directement 16 pixels non signés, sans conversion en flottants.
        for (int j = 0; j < fin_blocs; j += 16) {
            __m128i pixels = _mm_loadu_si128((const __m128i *)&in.data[i][j]);
            min_octets = _mm_min_epu8(min_octets, pixels);
            max_octets = _mm_max_epu8(max_octets, pixels);
        }

        for (int j = fin_blocs; j < in.width; j++) {
            min = MIN(min, in.data[i][j]);
            max = MAX(max, in.data[i][j]);
        }
    }

    uint8_t mins[16], maxs[16];
    _mm_storeu_si128((__m128i *)mins, min_octets);
    _mm_storeu_si128((__m128i *)maxs, max_octets);
    for (int k = 0; k < 16; k++) {
        min = MIN(min, mins[k]);
        max = MAX(max, maxs[k]);
    }

    // Si l'image est uniforme (0 à 255), la sortie reste à zéro (calloc).
    if (max == min)
        return;

    float facteur = (float)MAX_VALEUR / (float)(max - min);
    __m256 minimum = _mm256_set1_ps((float)min);
    __m256 facteurs = _mm256_set1_ps(facteur);

    for (int i = 0; i < in.height; i++) {
        int j = 0;
        for (; j < fin_blocs; j += 16) {
            __m128i bas = etaler_huit_pixels(&in.data[i][j], minimum, facteurs);
            __m128i haut = etaler_huit_pixels(&in.data[i][j + 8], minimum, facteurs);
            // Rassembler les deux groupes en 16 octets et les écrire ensemble.
            __m128i octets = _mm_packus_epi16(bas, haut);
            _mm_storeu_si128((__m128i *)&out.data[i][j], octets);
        }

        // Traiter encore huit pixels s'il reste un bloc complet.
        if (in.width - j >= 8) {
            __m128i valeurs = etaler_huit_pixels(&in.data[i][j], minimum, facteurs);
            __m128i octets = _mm_packus_epi16(valeurs, _mm_setzero_si128());
            _mm_storel_epi64((__m128i *)&out.data[i][j], octets);
            j += 8;
        }

        for (; j < in.width; j++)
            out.data[i][j] = (int)((in.data[i][j] - min) * facteur);
    }
}

/*==============================================================================*/
/* Programme principal                                                          */
/*==============================================================================*/
int main(int argc, char **argv) {

    InitClock;


    if (argc != 2) {
        fprintf(stderr, "Usage : %s image.pgm\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *srcFile = argv[1];
    char dstFile[MAX_CHAINE];

    if (snprintf(dstFile, sizeof(dstFile), "%s_new.pgm", srcFile) >= (int)sizeof(dstFile)) {
        fprintf(stderr, "Erreur : chemin de sortie trop long\n");
        return EXIT_FAILURE;
    }

    printf("Lecture du fichier : %s\n", srcFile);
    ImagePGM input = read_pgm(srcFile);

    ImagePGM output = {
        .width = input.width,
        .height = input.height,
        .maxval = MAX_VALEUR,
        .data = alloc_image(input.height, input.width)
    };

    printf("Traitement de l'image (%dx%d)...\n", input.width, input.height);

    /* Demarrage de la mesure du temps */
    ClockStart;

    etalement_lineaire(input, output);

    /* Fin de la mesure du temps */
    ClockEnd;

    /* Affichage du temps d'execution */

    printf(ClockMesure_Sec);
    printf(ClockMesure_m_Sec);
    printf(ClockMesure_u_Sec);

    printf("Écriture du fichier résultat : %s\n", dstFile);
    write_pgm(dstFile, output);

    free_image(input);
    free_image(output);

    printf("Traitement terminé avec succès.\n");
    return EXIT_SUCCESS;
}
