#include "WriteOutput.h"
#include "helper.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

int draw_bridge_num;
int *N_travel_times, *N_max_times;

int ferries_num;
int *F_travel_times, *F_max_times, *F_capacities;

int crossroad_num;
int *C_travel_times, *C_max_times;

int cars_num;
int *Car_travel_times, *Car_path_lengths;

int ***paths; //type, number, from, to

int *narrow_bridge_line0;
int *narrow_bridge_line1;
int narrow_bridge_line_size0;
int narrow_bridge_line_size1;
unsigned narrow_bridge_line_start0;
unsigned narrow_bridge_line_start1;

int *ferry_line0;
int *ferry_line1;
int ferry_line_size0;
int ferry_line_size1;
unsigned ferry_line_start0;
unsigned ferry_line_start1;

int *crossroad_line0;
int *crossroad_line1;
int *crossroad_line2;
int *crossroad_line3;
int crossroad_line_size0;
int crossroad_line_size1;
int crossroad_line_size2;
int crossroad_line_size3;
unsigned crossroad_line_start0;
unsigned crossroad_line_start1;
unsigned crossroad_line_start2;
unsigned crossroad_line_start3;

int active_cars;
int narrow_bridge_direction = 0, ferry_direction = 0, crossroad_direction = 0;
int narrow_bridge_pass = 0, ferry_pass = 0, crossroad_pass = 0;

pthread_t *pt_cars;

sem_t check_cars_sem, writing_sem, narrow_bridge_line_sem, ferry_line_sem, crossroad_line_sem, narrow_bridge_direction_sem, ferry_direction_sem, crossroad_direction_sem, *car_pass_sem;
sem_t narrow_bridge_pass_sem, ferry_pass_sem, crossroad_pass_sem;

void read_input();
void check_output();
void free_vars();
void pass_narrow_bridge(int carID, int rcID, int from, int to);
void pass_ferry(int carID, int rcID, int from, int to);
void pass_crossroad(int carID, int rcID, int from, int to);
void *car_thread(void *i);

pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;

int mywait(int timeInMs)
{
    struct timespec timeToWait;
    struct timeval now;
    int rt;

    gettimeofday(&now,NULL);

    timeToWait.tv_nsec = now.tv_usec * 1000 + timeInMs * 1000000;
    timeToWait.tv_sec = now.tv_sec + timeToWait.tv_nsec / 1000000000;
    timeToWait.tv_nsec %= 1000000000;

    pthread_mutex_lock(&fakeMutex);
    rt = pthread_cond_timedwait(&fakeCond, &fakeMutex, &timeToWait);
    pthread_mutex_unlock(&fakeMutex);

    return 1;
}


void pass_narrow_bridge(int carID, int rcID, int from, int to){
    int turn = 0;
    int wait_pass_delay = 0;
    int notify0 = 0, notify1 = 0;

    struct timespec timeToWait;
    struct timeval now;
    while(1){
        sem_wait(&narrow_bridge_line_sem);
        if(narrow_bridge_pass == 0 && ((from && narrow_bridge_line_size0 == 0 && narrow_bridge_line1[narrow_bridge_line_start1] == carID) || (!from && narrow_bridge_line_size1 == 0 && narrow_bridge_line0[narrow_bridge_line_start0] == carID))){
            narrow_bridge_direction = from;
            turn = 1;
        }else if(narrow_bridge_direction == from && ((from && narrow_bridge_line1[narrow_bridge_line_start1] == carID) || (!from && narrow_bridge_line0[narrow_bridge_line_start0]))){
            turn = 1;
        }
        if(turn){
            narrow_bridge_pass++;
            sem_post(&narrow_bridge_line_sem);
            WriteOutput(carID, 'N', rcID, START_PASSING);
            sleep_milli(PASS_DELAY);
            sem_wait(&narrow_bridge_line_sem);
            if(from){
                narrow_bridge_line_size1--;
                narrow_bridge_line_start1 = (narrow_bridge_line_start1 + 1) % cars_num;
                if(narrow_bridge_line_size1 > 0){
                    sem_post(&car_pass_sem[narrow_bridge_line1[narrow_bridge_line_start1]]);
                }else if(narrow_bridge_line_size0 > 0){
                    notify0 = 1;
                }
            }else{
                narrow_bridge_line0--;
                narrow_bridge_line_start0 = (narrow_bridge_line_start0 + 1) % cars_num;
                if(narrow_bridge_line_size0 > 0){
                    sem_post(&car_pass_sem[narrow_bridge_line0[narrow_bridge_line_start0]]);
                }else if(narrow_bridge_line1 > 0){
                    notify1 = 1;
                }
            }
            sem_post(&narrow_bridge_line_sem);
            sleep_milli(N_travel_times[rcID] - PASS_DELAY);
            WriteOutput(carID, 'N', rcID, FINISH_PASSING);
            sem_wait(&narrow_bridge_line_sem);
            if(notify1){
                notify1 = 0;
                sem_post(&car_pass_sem[narrow_bridge_line1[narrow_bridge_line_start1]]);
            }
            if(notify0){
                notify0 = 0;
                sem_post(&car_pass_sem[narrow_bridge_line0[narrow_bridge_line_start0]]);
            }
            sem_post(&narrow_bridge_line_sem);
            break;
        }else if(narrow_bridge_direction != from && ((from && narrow_bridge_line1[narrow_bridge_line_start1] == carID) || (!from && narrow_bridge_line0[narrow_bridge_line_start0] == carID))){
            sem_post(&narrow_bridge_line_sem);
            gettimeofday(&now,NULL);
            //printf("carID: %d that is waiting for max time on other side\n", carID);
            timeToWait.tv_nsec = now.tv_usec * 1000 + N_max_times[rcID] * 1000000;
            timeToWait.tv_sec = now.tv_sec + timeToWait.tv_nsec / 1000000000;
            timeToWait.tv_nsec %= 1000000000;

            sem_timedwait(&car_pass_sem[carID], &timeToWait);

            turn = 1;
            wait_pass_delay = 0;
            narrow_bridge_direction = from;
        }else{
            sem_post(&narrow_bridge_line_sem);
        }
        turn = 1;
        sem_wait(&car_pass_sem[carID]);
    }
}

void pass_ferry(int carID, int rcID, int from, int to){

}

void pass_crossroad(int carID, int rcID, int from, int to){

}


void *car_thread(void *index){
    unsigned i = *(unsigned*)(&index);
    char connecter_type;
    for(int j=0 ; j<Car_path_lengths[i] ; ++j){
        switch(paths[i][j][0]){
            case 0:
                connecter_type = 'N';
                break;
            case 1:
                connecter_type = 'F';
                break;
            default:
                connecter_type = 'C';
        }
        WriteOutput(i, connecter_type, paths[i][j][1], TRAVEL);
        sleep_milli(Car_travel_times[i]);
        switch(connecter_type){
            case 'N':
                sem_wait(&narrow_bridge_line_sem);
                WriteOutput(i, connecter_type, paths[i][j][1], ARRIVE);
                if(paths[i][j][2]){
                    narrow_bridge_line1[(narrow_bridge_line_start1 + narrow_bridge_line_size1) % cars_num] = i;
                    narrow_bridge_line_size1++;
                }else{
                    narrow_bridge_line0[(narrow_bridge_line_start0 + narrow_bridge_line_size0) % cars_num] = i;
                    narrow_bridge_line_size0++;
                }
                /*printf("carID: %d\n", i);
                printf("line1 size: %d\n", narrow_bridge_line_size1);
                for(int k=0 ; k<narrow_bridge_line_size1 ; ++k){
                    printf("%d ", narrow_bridge_line1[k]);
                }
                printf("\n");
                printf("line0 size: %d\n", narrow_bridge_line_size0);
                for(int k=0 ; k<narrow_bridge_line_size0 ; ++k){
                    printf("%d ", narrow_bridge_line0[k]);
                }
                printf("\n");*/
                sem_post(&narrow_bridge_line_sem); 
                pass_narrow_bridge(i, paths[i][j][1], paths[i][j][2], paths[i][j][3]);
                break;
            case 'F':
                sem_wait(&ferry_line_sem);
                if(paths[i][j][2]){
                    ferry_line1[(ferry_line_start1 + ferry_line_size1) % cars_num] = i;
                    ferry_line_size1++;
                }else{
                    ferry_line0[(ferry_line_start0 + ferry_line_size0) % cars_num] = i;
                    ferry_line_size0++;
                }
                sem_post(&ferry_line_sem);
                pass_ferry(i, paths[i][j][1], paths[i][j][2], paths[i][j][3]);
                break;
            default:
                sem_wait(&crossroad_line_sem);
                if(paths[i][j][2] == 0){
                    crossroad_line0[(crossroad_line_start0 + crossroad_line_size0) % cars_num] = i;
                    crossroad_line_size0++;
                }else if(paths[i][j][2] == 1){
                    crossroad_line1[(crossroad_line_start1 + crossroad_line_size1) % cars_num] = i;
                    crossroad_line_size1++;
                }else if(paths[i][j][2] == 2){
                    crossroad_line2[(crossroad_line_start2 + crossroad_line_size2) % cars_num] = i;
                    crossroad_line_size2++;
                }else{
                    crossroad_line3[(crossroad_line_start3 + crossroad_line_size3) % cars_num] = i;
                    crossroad_line_size3++;
                }
                sem_post(&crossroad_line_sem);
                pass_crossroad(i, paths[i][j][1], paths[i][j][2], paths[i][j][3]);
        }
    }
    sem_wait(&check_cars_sem);
    --active_cars;
    sem_post(&check_cars_sem);
}

int main(){
    read_input();
    //check_output();
    unsigned long i;
    sem_init(&writing_sem, 0, 1);
    sem_init(&narrow_bridge_line_sem, 0, 1);
    sem_init(&ferry_line_sem, 0, 1);
    sem_init(&crossroad_line_sem, 0, 1);
    sem_init(&check_cars_sem, 0, 1);
    sem_init(&narrow_bridge_direction_sem, 0, 1);
    sem_init(&ferry_direction_sem, 0, 1);
    sem_init(&crossroad_direction_sem, 0, 1);
    sem_init(&narrow_bridge_pass_sem, 0, 1);
    sem_init(&ferry_pass_sem, 0, 1);
    sem_init(&crossroad_pass_sem, 0, 1);
    for(i=0 ; i<cars_num ; ++i){
        sem_init(&car_pass_sem[i], 0, 0);
    }
    
    //create threads
    InitWriteOutput();
    for(i=0 ; i<cars_num; ++i){
        pthread_create(&pt_cars[i], NULL, &car_thread, (void*)i);
    }

    //wait for threads
    for(i=0 ; i<cars_num; ++i){
        pthread_join(pt_cars[i], NULL);
    }

    sem_destroy(&writing_sem);
    sem_destroy(&narrow_bridge_line_sem);
    sem_destroy(&ferry_line_sem);
    sem_destroy(&crossroad_line_sem);
    sem_destroy(&check_cars_sem);
    sem_destroy(&narrow_bridge_direction_sem);
    sem_destroy(&ferry_direction_sem);
    sem_destroy(&crossroad_direction_sem);
    sem_destroy(&narrow_bridge_pass_sem);
    sem_destroy(&ferry_pass_sem);
    sem_destroy(&crossroad_pass_sem);
    for(i=0 ; i<cars_num ; ++i){
        sem_destroy(&car_pass_sem[i]);
    }
    free_vars();
    return 0;
}

void read_input(){
    scanf("%d", &draw_bridge_num);
    if(draw_bridge_num > 0){
        N_travel_times = (int*)malloc(draw_bridge_num * sizeof(int));
        N_max_times = (int*)malloc(draw_bridge_num * sizeof(int));
    }
    for(int i=0 ; i<draw_bridge_num ; ++i){
        scanf("%d", &N_travel_times[i]);
        scanf("%d", &N_max_times[i]);
    }

    scanf("%d", &ferries_num);
    if(ferries_num > 0){
        F_travel_times = (int*)malloc(ferries_num * sizeof(int));
        F_max_times = (int*)malloc(ferries_num * sizeof(int));
        F_capacities = (int*)malloc(ferries_num * sizeof(int));
    }
    for(int i=0 ; i<ferries_num ; ++i){
        scanf("%d", &F_travel_times[i]);
        scanf("%d", &F_max_times[i]);
        scanf("%d", &F_capacities[i]);
    }

    scanf("%d", &crossroad_num);
    if(crossroad_num > 0){
        C_travel_times = (int*)malloc(crossroad_num * sizeof(int));
        C_max_times = (int*)malloc(crossroad_num * sizeof(int));
    }
    for(int i=0 ; i<crossroad_num ; ++i){
        scanf("%d", &C_travel_times[i]);
        scanf("%d", &C_max_times[i]);
    }

    scanf("%d", &cars_num);
    if(cars_num > 0){
        active_cars = cars_num;
        Car_travel_times = (int*)malloc(cars_num * sizeof(int));
        Car_path_lengths = (int*)malloc(cars_num * sizeof(int));
        pt_cars = (pthread_t*)malloc(cars_num * sizeof(pthread_t));
        paths = (int***)malloc(cars_num * sizeof(int**));
        car_pass_sem = (sem_t*)malloc(cars_num * sizeof(sem_t));
        narrow_bridge_line0 = (int*)malloc(cars_num * sizeof(int));
        narrow_bridge_line1 = (int*)malloc(cars_num * sizeof(int));
        narrow_bridge_line_size0 = 0;
        narrow_bridge_line_size1 = 0;
        narrow_bridge_line_start0 = 0;
        narrow_bridge_line_start1 = 0;
        ferry_line0 = (int*)malloc(cars_num * sizeof(int));
        ferry_line1 = (int*)malloc(cars_num * sizeof(int));
        ferry_line_size0 = 0;
        ferry_line_size1 = 0;
        ferry_line_start0 = 0;
        ferry_line_start1 = 0;
        crossroad_line0 = (int*)malloc(cars_num * sizeof(int));
        crossroad_line1 = (int*)malloc(cars_num * sizeof(int));
        crossroad_line2 = (int*)malloc(cars_num * sizeof(int));
        crossroad_line3 = (int*)malloc(cars_num * sizeof(int));
        crossroad_line_size0 = 0;
        crossroad_line_size1 = 0;
        crossroad_line_size2 = 0;
        crossroad_line_size3 = 0;
        crossroad_line_start0 = 0;
        crossroad_line_start1 = 0;
        crossroad_line_start2 = 0;
        crossroad_line_start3 = 0;
    }
    for(int i=0 ; i<cars_num ; ++i){
        scanf("%d", &Car_travel_times[i]);
        scanf("%d", &Car_path_lengths[i]);
        if(Car_path_lengths[i] > 0){
            paths[i] = (int**)malloc(Car_path_lengths[i] * sizeof(int*));
        }
        char temp;
        char *temp1;
        for(int j=0 ; j<Car_path_lengths[i] ; ++j){
            paths[i][j] = (int*)malloc(sizeof(int) * 4);
            while(temp = getchar()){
                if(temp == 'N' || temp == 'F' || temp == 'C'){
                    break;
                }
            }
            switch(temp){
                case 'N':
                    paths[i][j][0] = 0;
                    break;
                case 'F':
                    paths[i][j][0] = 1;
                    break;
                default:
                    paths[i][j][0] = 2;
            }
            temp = getchar();
            temp1 = &temp;
            paths[i][j][1] = atoi(temp1);
            scanf("%d", &paths[i][j][2]);
            scanf("%d", &paths[i][j][3]);
        }
    }
    return;
}

void check_output(){
    printf("%d\n", draw_bridge_num);
    for(int i=0 ; i<draw_bridge_num ; ++i){
        printf("%d %d\n", N_travel_times[i], N_max_times[i]);
    }

    printf("%d\n", ferries_num);
    for(int i=0 ; i<ferries_num ; ++i){
        printf("%d %d %d\n", F_travel_times[i], F_max_times[i], F_capacities[i]);
    }

    printf("%d\n", crossroad_num);
    for(int i=0 ; i<crossroad_num ; ++i){
        printf("%d %d\n", C_travel_times[i], C_max_times[i]);
    }

    printf("%d\n", cars_num);
    for(int i=0 ; i<cars_num ; ++i){
        printf("%d %d\n", Car_travel_times[i], Car_path_lengths[i]);
        for(int j=0 ; j<Car_path_lengths[i] ; ++j){
            switch(paths[i][j][0]){
                case 0:
                    printf("N");
                    break;
                case 1:
                    printf("F");
                    break;
                default:
                    printf("C");
            }
            printf("%d %d %d ", paths[i][j][1], paths[i][j][2], paths[i][j][3]);
        }
        printf("\n");
    }
    return;
}

void free_vars(){

    if(draw_bridge_num > 0){
        free(N_travel_times);
        free(N_max_times);
    }
    if(ferries_num > 0){
        free(F_travel_times);
        free(F_max_times);
        free(F_capacities);
    }
    if(crossroad_num > 0){
        free(C_travel_times);
        free(C_max_times);
    }
    if(cars_num > 0){
        free(Car_travel_times);
        for(int i=0 ; i<cars_num ; ++i){
            for(int j=0 ; j<Car_path_lengths[i] ; ++j){
                free(paths[i][j]);
            }
            free(paths[i]);
        }
        free(paths);
        free(Car_path_lengths);
        free(pt_cars);
        free(narrow_bridge_line0);
        free(narrow_bridge_line1);
        free(ferry_line0);
        free(ferry_line1);
        free(crossroad_line0);
        free(crossroad_line1);
        free(crossroad_line2);
        free(crossroad_line3);
    }
    return;
}