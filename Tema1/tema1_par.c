// Author: APD team, except where source was noted

#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048

#define CLAMP(v, min, max) if(v < min) { v = min; } else if(v > max) { v = max; }

// structured used for thread function arg
typedef struct {
    pthread_barrier_t* barrier;
    ppm_image* image;
    ppm_image* initial_image;
    unsigned char** grid;
    ppm_image** contour_map;
    int step_x;
    int step_y;
    int thread_id;
    int num_threads;
    int p;
    int q;
} ThreadData;


// thread function that processes a section of the image
void* processImageSection(void* data) {
    ThreadData* threadData = (ThreadData*)data;

    uint8_t sample[3];
    // compute the section of the image to be processed based on the current thread id
    int img_start_i = (threadData->p * threadData->thread_id) / threadData->num_threads;
    int img_end_i = (threadData->p * (threadData->thread_id + 1)) / threadData->num_threads;
    img_end_i = (img_end_i < threadData->p) ? img_end_i : threadData->p;
    
    // compute the section of the contour map to be init based on the current thread id
    int cm_start = (CONTOUR_CONFIG_COUNT * threadData->thread_id) / threadData->num_threads;
    int cm_end = (CONTOUR_CONFIG_COUNT * (threadData->thread_id + 1)) / threadData->num_threads;
    cm_end = (cm_end < CONTOUR_CONFIG_COUNT) ? cm_end : CONTOUR_CONFIG_COUNT;

    // step 0. init the contour map
    for (int i = cm_start; i < cm_end; i ++) {
        char filename[FILENAME_MAX_SIZE];
        sprintf(filename, "../checker/contours/%d.ppm", i);
        threadData->contour_map[i] = read_ppm(filename);
    }

    // wait for all the threads to run step 0 on their section
    pthread_barrier_wait(threadData->barrier);


    // step 1. rescale the current section of the image
    if (!(threadData->initial_image->x <= RESCALE_X && threadData->initial_image->y <= RESCALE_Y)) {
        for (int i = img_start_i * threadData->step_x; i < img_end_i * threadData->step_x; i ++) {
            for (int j = 0; j < threadData->image->y; j ++) {
                float u = (float)i / (float)(threadData->image->x - 1);
                float v = (float)j / (float)(threadData->image->y - 1);
                sample_bicubic(threadData->initial_image, u, v, sample);

                threadData->image->data[i * threadData->image->y + j].red = sample[0];
                threadData->image->data[i * threadData->image->y + j].green = sample[1];
                threadData->image->data[i * threadData->image->y + j].blue = sample[2];
            }
        }
    }

    // wait for all the threads to run step 1 on their section
    pthread_barrier_wait(threadData->barrier);

    // step 2. sample the current section of the grid
    for (int i = img_start_i; i < img_end_i; i ++) {
        // last sample points have no neighbors below / to the right, so we use pixels on the
        // last row / column of the input image for them
        ppm_pixel curr_pixel = threadData->image->data[i * threadData->step_x * threadData->image->y + threadData->image->x - 1];
        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > SIGMA) {
            threadData->grid[i][threadData->q] = 0;
        } else {
            threadData->grid[i][threadData->q] = 1;
        }

        for (int j = 0; j < threadData->q; j ++) {
            ppm_pixel curr_pixel = threadData->image->data[i * threadData->step_x * threadData->image->y + j * threadData->step_y];
            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > SIGMA) {
                threadData->grid[i][j] = 0;
            } else {
                threadData->grid[i][j] = 1;
            }
        }
    }

    int img_start_j = (threadData->q * threadData->thread_id) / threadData->num_threads;
    int img_end_j = (threadData->q * (threadData->thread_id + 1)) / threadData->num_threads;
    img_end_j = (img_end_j < threadData->q) ? img_end_j : threadData->q; 

    for (int j = img_start_j; j < img_end_j; j ++) {
        // last sample points have no neighbors below / to the right, so we use pixels on the
        // last row / column of the input image for them
        ppm_pixel curr_pixel = threadData->image->data[(threadData->image->x - 1) * threadData->image->y + j * threadData->step_y];
        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > SIGMA) {
            threadData->grid[threadData->p][j] = 0;
        } else {
            threadData->grid[threadData->p][j] = 1;
        }
    }

    // wait for all the threads to run step 2 on their section
    pthread_barrier_wait(threadData->barrier);

    // step 3. march the squares
    for (int i = img_start_i; i < img_end_i; i ++) {
        for (int j = 0; j < threadData->q; j ++) {
            unsigned char k = 8 * threadData->grid[i][j] + 4 * threadData->grid[i][j + 1] + 2 * threadData->grid[i + 1][j + 1] + 1 * threadData->grid[i + 1][j];
            
            for (int xi = 0; xi < threadData->contour_map[k]->x; xi ++) {
                for (int yi = 0; yi < threadData->contour_map[k]->y; yi ++) {
                    int contour_pixel_index = threadData->contour_map[k]->x * xi + yi;
                    int image_pixel_index = (threadData->step_x * i + xi) * threadData->image->y +
                                             threadData->step_x * j + yi;
                    
                    threadData->image->data[image_pixel_index].red = threadData->contour_map[k]->data[contour_pixel_index].red;
                    threadData->image->data[image_pixel_index].green = threadData->contour_map[k]->data[contour_pixel_index].green;
                    threadData->image->data[image_pixel_index].blue = threadData->contour_map[k]->data[contour_pixel_index].blue;
                }
            }
        }
    }

    // wait for all the threads to run step 3 on their section
    pthread_barrier_wait(threadData->barrier);

    // step 5.1 free resources
    for (int i = img_start_i; i < img_end_i; i ++) {
        free(threadData->grid[i]);
    }

    for (int i = cm_start; i < cm_end; i ++) {
        free(threadData->contour_map[i]->data);
        free(threadData->contour_map[i]);
    }
    
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: ./tema1 <in_file> <out_file> <P>\n");
        return 1;
    }

    // read image
    ppm_image *image = read_ppm(argv[1]);
    int step_x = STEP;
    int step_y = STEP;

    // if image needs rescaling, alloc memory for the new (to be scaled) image
    ppm_image *new_image;

    if (!(image->x <= RESCALE_X && image->y <= RESCALE_Y)) {
        // we only rescale downwards
        new_image = (ppm_image *)malloc(sizeof(ppm_image));
        if (!new_image) {
            fprintf(stderr, "Unable to allocate memory\n");
            return 1;
        }

        new_image->x = RESCALE_X;
        new_image->y = RESCALE_Y;

        new_image->data = (ppm_pixel*)malloc(new_image->x * new_image->y * sizeof(ppm_pixel));
        if (!new_image) {
            fprintf(stderr, "Unable to allocate memory\n");
            return 1;
        }

    } else {
        new_image = image;
    }

    // alloc memory for contour map
    ppm_image **contour_map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
    if (!contour_map) {
        fprintf(stderr, "Unable to allocate memory for contour map\n");
        return 1;
    }

    // alloc memory for grid
    int p = new_image->x / step_x;
    int q = new_image->y / step_y;
    unsigned char **grid = (unsigned char **)malloc((p + 1) * sizeof(unsigned char*));

    if (!grid) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 1;
    }

    for (int i = 0; i <= p; i++) {
        grid[i] = (unsigned char *)malloc((q + 1) * sizeof(unsigned char));
        if (!grid[i]) {
            fprintf(stderr, "Unable to allocate memory\n");
            return 1;
        }
    }

    // threads & barrier creation
    int num_threads = atoi(argv[3]);
    pthread_t threads[num_threads];
    ThreadData threadData[num_threads];

    pthread_barrier_t barrier;

    if (pthread_barrier_init(&barrier, NULL, num_threads)) {
        fprintf(stderr, "Error init barrier\n");
        return 1;
    }

    // thread function call
    for (int i = 0; i < num_threads; i ++) {
        threadData[i].barrier = &barrier;
        threadData[i].thread_id = i;
        threadData[i].num_threads = num_threads;
        threadData[i].image = new_image;
        threadData[i].initial_image = image;
        threadData[i].grid = grid;
        threadData[i].contour_map = contour_map;
        threadData[i].step_x = step_x;
        threadData[i].step_y = step_y;
        threadData[i].p = p;
        threadData[i].q = q;
        if (pthread_create(&threads[i], NULL, processImageSection, &threadData[i])) {
            fprintf(stderr, "Error creating thread id = %d\n", i);
            return 1;
        }
    }
    
    for (int i = 0; i < num_threads; i ++) {
        if (pthread_join(threads[i], NULL)) {
            fprintf(stderr, "Error joining thread id = %d\n", i);
            return 1;
        }
    }
    
    // step 4. write output
    write_ppm(new_image, argv[2]);

    // step 5.2 free resources
    free(contour_map);
    free(grid);
    free(new_image->data);
    free(new_image);

    return 0;
}
