/*==============================================================================*/
/* Programme : code_avx_int.c                                                   */
/* Auteur    : Daniel CHILLET                                                   */
/* Version   : 2025 10                                                          */
/* Objet     : Rehaussement de contraste                                        */
/*==============================================================================*/


#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int **data;
} ImagePGM;

/*==============================================================================*/
/* Allocation et libération d'image                                             */
/*==============================================================================*/
int **alloc_image(int height, int width) {
    int **img = malloc(height * sizeof(int *));
    CHECK_ALLOC(img);
    for (int i = 0; i < height; i++) {
        img[i] = calloc(width, sizeof(int));
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
ImagePGM read_pgm(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Erreur ouverture fichier source");
        exit(EXIT_FAILURE);
    }

    ImagePGM img;
    char line[MAX_CHAINE];

    // Lecture du type de fichier
    if (!fgets(line, sizeof(line), f) || strncmp(line, "P2", 2) != 0) {
        fprintf(stderr, "Erreur : format non supporté (P2 attendu)\n");
        exit(EXIT_FAILURE);
    }

    // Ignorer les commentaires
    do {
        if (!fgets(line, sizeof(line), f)) {
            fprintf(stderr, "Erreur lecture dimensions\n");
            exit(EXIT_FAILURE);
        }
    } while (line[0] == '#');

    // Lecture dimensions
    sscanf(line, "%d %d", &img.width, &img.height);
    fscanf(f, "%d", &img.maxval);

    img.data = alloc_image(img.height, img.width);

    for (int i = 0; i < img.height; i++)
        for (int j = 0; j < img.width; j++)
            fscanf(f, "%d", &img.data[i][j]);

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
            fprintf(f, "%3d ", img.data[i][j]);
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
void etalement_lineaire(ImagePGM in, ImagePGM out) {
    __m256 min_avx = _mm256_set1_ps(MAX_VALEUR);
    __m256 max_avx = _mm256_setzero_ps();
    int min = MAX_VALEUR, max = 0;
    const int fin_avx = in.width - in.width % 8;

    for (int i = 0; i < in.height; i++) {
        for (int j = 0; j < fin_avx; j += 8) {
            // calloc ne garantit pas un alignement à 32 octets.
            __m256i entiers = _mm256_loadu_si256((const __m256i *)&in.data[i][j]);
            __m256 pixels = _mm256_cvtepi32_ps(entiers);
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
            __m256i entiers = _mm256_loadu_si256((const __m256i *)&in.data[i][j]);
            __m256 pixels = _mm256_cvtepi32_ps(entiers);
            __m256 etales = _mm256_mul_ps(_mm256_sub_ps(pixels, minimum), facteurs);
            // Tronquer vers zéro, comme le cast (int) de la version séquentielle.
            _mm256_storeu_si256((__m256i *)&out.data[i][j], _mm256_cvttps_epi32(etales));
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

    char srcFile[MAX_CHAINE];
    char dstFile[MAX_CHAINE];

    snprintf(srcFile, sizeof(srcFile), "%s", argv[1]);
    snprintf(dstFile, sizeof(dstFile), "%s_new.pgm", argv[1]);

    printf("Lecture du fichier : %s\n", srcFile);
    ImagePGM input = read_pgm(srcFile);

    ImagePGM output = {
        .width = input.width,
        .height = input.height,
        .maxval = input.maxval,
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
