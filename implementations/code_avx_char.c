/*==============================================================================*/
/* Programme : code_avx_char.c                                                  */
/* Auteur    : Daniel CHILLET                                                   */
/* Version   : 2025 10                                                          */
/* Objet     : Rehaussement de contraste                                        */
/*==============================================================================*/


#include <immintrin.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/*==============================================================================*/
/* Macros utiles pour la mesure du temps                                        */
/*==============================================================================*/

#define InitClock    struct timespec start, stop
#define ClockStart   clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start)
#define ClockEnd   clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop)
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
static inline __m256 charger_pixels(const uint8_t *pixels) {
    __m128i zeros = _mm_setzero_si128();
    __m128i octets = _mm_loadl_epi64((const __m128i *)pixels);
    __m128i valeurs = _mm_unpacklo_epi8(octets, zeros);

    // Élargir les deux moitiés à 32 bits sans extension de signe.
    __m128 bas = _mm_cvtepi32_ps(_mm_unpacklo_epi16(valeurs, zeros));
    __m128 haut = _mm_cvtepi32_ps(_mm_unpackhi_epi16(valeurs, zeros));
    return _mm256_insertf128_ps(_mm256_castps128_ps256(bas), haut, 1);
}

void etalement_lineaire(ImagePGM in, ImagePGM out) {
    __m256 min_avx = _mm256_set1_ps(MAX_VALEUR);
    __m256 max_avx = _mm256_setzero_ps();
    int min = MAX_VALEUR, max = 0;
    const int fin_avx = in.width - in.width % 8;

    for (int i = 0; i < in.height; i++) {
        for (int j = 0; j < fin_avx; j += 8) {
            __m256 pixels = charger_pixels(&in.data[i][j]);
            min_avx = _mm256_min_ps(min_avx, pixels);
            max_avx = _mm256_max_ps(max_avx, pixels);
        }

        // La fin de chaque ligne peut contenir moins de huit pixels.
        for (int j = fin_avx; j < in.width; j++) {
            min = MIN(min, in.data[i][j]);
            max = MAX(max, in.data[i][j]);
        }
    }

    float mins[8], maxs[8];
    _mm256_storeu_ps(mins, min_avx);
    _mm256_storeu_ps(maxs, max_avx);
    for (int k = 0; k < 8; k++) {
        min = MIN(min, (int)mins[k]);
        max = MAX(max, (int)maxs[k]);
    }

    // Si l'image est uniforme (0 à 255), la sortie reste à zéro (calloc).
    if (max == min)
        return;

    float facteur = (float)MAX_VALEUR / (float)(max - min);
    __m256 minimum = _mm256_set1_ps((float)min);
    __m256 facteurs = _mm256_set1_ps(facteur);

    for (int i = 0; i < in.height; i++) {
        for (int j = 0; j < fin_avx; j += 8) {
            __m256 pixels = charger_pixels(&in.data[i][j]);
            __m256 etales = _mm256_mul_ps(_mm256_sub_ps(pixels, minimum), facteurs);
            // Tronquer vers zéro, comme le cast (int) de la version séquentielle.
            __m256i entiers = _mm256_cvttps_epi32(etales);
            // Les résultats sont dans [0, 255] et tiennent dans un entier 16 bits signé.
            __m128i pixels_16 = _mm_packs_epi32(_mm256_castsi256_si128(entiers),
                                              _mm256_extractf128_si256(entiers, 1));
            __m128i pixels_8 = _mm_packus_epi16(pixels_16, _mm_setzero_si128());
            _mm_storel_epi64((__m128i *)&out.data[i][j], pixels_8);
        }

        for (int j = fin_avx; j < in.width; j++)
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
