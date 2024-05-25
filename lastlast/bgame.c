#include "logging.h"
#include "message.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include <sys/types.h>
#include <sys/wait.h>
#include "signal.h"

typedef struct Bomber{
	int is_dead;
	int pid;
	int marked;
	int winner;
	int x;
	int y;
}Bomber;

typedef struct Map{
	int x;
	int y;
	int **world;
}Map;

typedef struct Obstacles{
	int x;
	int y;
	int durability;
}Obs;

typedef struct Bomb{
	int pid;
	unsigned int radius;
	int x;
	int y;
	int fd;
	struct Bomb *next;
}Bomb;


Bomb *head = NULL;

Map map;

void create_bomb(int pid, unsigned int radius, int x, int y, int fd){
	Bomb *current = head;
	if(current == NULL){
		head = malloc(sizeof(Bomb));
		head->pid = pid;
		head->radius = radius;
		head->x = x;
		head->y = y;
		head->fd = fd;
		head->next = NULL;
		return;
	}
	while(current->next){
		current = current->next;
	}
	current->next = malloc(sizeof(Bomb));
	current = current->next;
	current->pid = pid;
	current->radius = radius;
	current->x = x;
	current->y = y;
	current->fd = fd;
	current->next = NULL;
}

int can_move(coordinate target_location, Bomber *bombers, int i){
	if(target_location.x < 0 || target_location.y < 0 || target_location.x >= map.x || target_location.y >= map.y){
		//printf("cant move. out of bounds\n");
		return 0;
	}
	if((target_location.x+1 == bombers[i].x || target_location.x-1 == bombers[i].x) && target_location.y == bombers[i].y){
		switch(map.world[target_location.x][target_location.y]){
			case 0:
				if(map.world[bombers[i].x][bombers[i].y] == 2){
					map.world[bombers[i].x][bombers[i].y] = 0;
				}else{
					map.world[bombers[i].x][bombers[i].y] = 4;
				}
				map.world[target_location.x][target_location.y] = 2;
				bombers[i].x = target_location.x;
				bombers[i].y = target_location.y;
				return 1;
			case 3:
				if(map.world[bombers[i].x][bombers[i].y] == 2){
					map.world[bombers[i].x][bombers[i].y] = 0;
				}else{
					map.world[bombers[i].x][bombers[i].y] = 4;
				}
				map.world[target_location.x][target_location.y] = 4;
				bombers[i].x = target_location.x;
				bombers[i].y = target_location.y;
				return 1;
		}
		//printf("cant move. obstacle or bomber in the way.\n");
		return 0;
	}
	if((target_location.y+1 == bombers[i].y || target_location.y-1 == bombers[i].y) && target_location.x == bombers[i].x){
		switch(map.world[target_location.x][target_location.y]){
			case 0:
				if(map.world[bombers[i].x][bombers[i].y] == 2){
					map.world[bombers[i].x][bombers[i].y] = 0;
				}else{
					map.world[bombers[i].x][bombers[i].y] = 4;
				}
				map.world[target_location.x][target_location.y] = 2;
				bombers[i].x = target_location.x;
				bombers[i].y = target_location.y;
				return 1;
			case 3:
				if(map.world[bombers[i].x][bombers[i].y] == 2){
					map.world[bombers[i].x][bombers[i].y] = 0;
				}else{
					map.world[bombers[i].x][bombers[i].y] = 4;
				}
				map.world[target_location.x][target_location.y] = 4;
				bombers[i].x = target_location.x;
				bombers[i].y = target_location.y;
				return 1;
		}
		//printf("cant move. obstacle or bomber in the way.\n");
		return 0;
	}
	//printf("cant move. more than 1 distance\n");
	return 0;
}

//world: 0:empty, 1:obstacle, 2:bomber, 3:bomb, 4:bomb+bomber

char *inputString(FILE* fp, size_t size){
//The size is extended by the input with the value of the provisional
    char *str;
    int ch;
    size_t len = 0;
    str = realloc(NULL, sizeof(*str)*size);//size is start size
    if(!str)return str;
    while(EOF!=(ch=fgetc(fp)) && ch != ' ' && ch!= '\n'){
        str[len++]=ch;
        if(len==size){
            str = realloc(str, sizeof(*str)*(size+=16));
            if(!str)return str;
        }
    }
    str[len++]='\0';

    return realloc(str, sizeof(*str)*len);
}

int main(){
	int height, width, obs_count, bmber_count, bmb_count, obj_count, active_bmber;
	int marked_number = 0;
	int **obs_info, **bmber_info, **bmber_info2;
	int dont_check[4];
	int i, j, k, l;
	char ***arguments;
	char **bomb_arguments;
	char buffer[256];
	int **fd;
	int tmp_fd[2];
	int status;
	int WIN = 0;
	int stop_checking = 0;
	obsd *obstacle_send;
	Bomb *bombs = head;
	Bomb *tmp = head;
	Obs *obstacles;
	Bomber *bombers;
	od objects[25];
	im *imessage;
	om *omessage;
	imp *input_print;
	omp *output_print;
	struct pollfd fds[1];
	pid_t pid;
	
	scanf("%d %d %d %d ", &height, &width, &obs_count, &bmber_count);
	
	map.x = height;
	map.y = width;
	map.world = malloc(sizeof(int*)*height);
	for(i=0 ; i<height ; ++i){
		map.world[i] = malloc(sizeof(int)*width);
		for(j=0 ; j<width ; ++j){
			map.world[i][j] = 0;
		}
	}
	
	obs_info = (int**)malloc(sizeof(int*)*obs_count);
	bmber_info = (int**)malloc(sizeof(int*)*bmber_count);
	bmber_info2 = (int**)malloc(sizeof(int*)*bmber_count);
	arguments = (char***)malloc(sizeof(char**)*bmber_count);
	fd = (int**)malloc(sizeof(int*)*bmber_count);
	input_print = (imp*)malloc(sizeof(imp));
	output_print = (omp*)malloc(sizeof(omp));
	bombers = (Bomber*)malloc(sizeof(Bomber)*bmber_count);
	obstacles = (Obs*)malloc(sizeof(Obs)*obs_count);
	bomb_arguments = (char**)malloc(sizeof(char*)*2);
	obstacle_send = (obsd*)malloc(sizeof(obsd));
	omessage = (om*)malloc(sizeof(om));
	imessage = (im*)malloc(sizeof(im));
	
	for(i=0 ; i<obs_count ; ++i){
		obs_info[i] = (int*)malloc(sizeof(int)*3);
		scanf("%d %d %d ", &obs_info[i][0], &obs_info[i][1], &obs_info[i][2]);
		obstacles[i].x = obs_info[i][0];
		obstacles[i].y = obs_info[i][1];
		obstacles[i].durability = obs_info[i][2];
		map.world[obs_info[i][0]][obs_info[i][1]] = 1;
	}
	for(i=0 ; i<bmber_count ; ++i){
		fd[i] = (int*)malloc(sizeof(int)*2);
		bmber_info[i] = (int*)malloc(sizeof(int)*3);
		scanf("%d %d %d ", &bmber_info[i][0], &bmber_info[i][1], &bmber_info[i][2]);
		bombers[i].x = bmber_info[i][0];
		bombers[i].y = bmber_info[i][1];
		map.world[bmber_info[i][0]][bmber_info[i][1]] = 2;
		bombers[i].is_dead = 0;
		bombers[i].marked = 0;
		bombers[i].winner = 0; 
		bmber_info2[i] = (int*)malloc(sizeof(int)*(bmber_info[i][2]-1));
		arguments[i] = (char**)malloc(sizeof(char*)*bmber_info[i][2]);
		arguments[i][0] = inputString(stdin, 10);
		
		for(j=0 ; j<bmber_info[i][2]-1 ; ++j){
			arguments[i][j+1] = inputString(stdin, 10);
			bmber_info2[i][j] = atoi(arguments[i][j+1]);
		}
	}
	for(i=0 ; i<bmber_count ; ++i){
		PIPE(fd[i]);
		pid = fork();
		if(pid == 0){
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(fd[i][0]);
		
			dup2(fd[i][1], 1);
			dup2(fd[i][1], 0);
			close(fd[i][1]);
			execvp(arguments[i][0], arguments[i]);
		}else{
			for(j=0 ; j<bmber_info[i][2] ; ++j){
				free(arguments[i][j]);
			}
			free(arguments[i]);
			bombers[i].pid = pid;
			close(fd[i][1]);
		}
	}
	bmb_count = 0;
	active_bmber = bmber_count;
	
	while(!WIN){
		bombs = head;
		for(i=0 ; i<bmb_count ; ++i){
			fds[0].fd = bombs->fd;
			fds[0].events = POLLIN;
			l = poll(fds, 1, 1);
			if(l > 0){
				bmb_count--;
				tmp = head;
				if(bombs == head){
					head = head->next;
				}else{
					while(tmp->next != bombs){
						tmp = tmp->next;
					}
					tmp->next = bombs->next;
				}
				read_data(fds[0].fd, imessage);
				input_print->pid = bombs->pid;
				input_print->m = imessage;
				print_output(input_print, NULL, NULL, NULL);
				if(map.world[bombs->x][bombs->y] == 4){
					for(j=0 ; j<bmber_count ; ++j){
						if(bombers[j].is_dead || bombers[j].marked){
							continue;
						}
						if(bombers[j].x == bombs->x && bombers[j].y == bombs->y){
							bombers[j].marked = 1;
							marked_number++;
							active_bmber--;
							break;
						}
					}
				}
				//printf("%d\n", active_bmber);
				dont_check[0] = 1;
				dont_check[1] = 1;
				dont_check[2] = 1;
				dont_check[3] = 1;
				for(j=1 ; j<=bombs->radius ; ++j){
					if(active_bmber == 1){
						for(k=0 ; k<bmber_count ; ++k){
							if(bombers[k].is_dead || bombers[k].marked){
								continue;
							}
							bombers[k].winner = 1;
							stop_checking = 1;
							break;
						}
					}
					if(stop_checking){
						break;
					}
					if(bombs->x-j >= 0){
						if(!dont_check[0]){
							switch(map.world[bombs->x-j][bombs->y]){
								case 1:
									for(k=0 ; k<obs_count ; ++k){
										if(obstacles[k].durability == 0){
											continue;
										}
										if(obstacles[k].x == bombs->x-j && obstacles[k].y == bombs->y){
											if(obstacles[k].durability != -1){
												obstacles[k].durability--;
											}
											obstacle_send->position.x = (unsigned int)obstacles[k].x;
											obstacle_send->position.y = (unsigned int)obstacles[k].y;
											obstacle_send->remaining_durability = obstacles[k].durability;
											if(obstacles[k].durability == 0){
												map.world[obstacles[k].x][obstacles[k].y] = 0;	
											}
											dont_check[0] = 1;
											print_output(NULL, NULL, obstacle_send, NULL);
											break;
										}
									}
									break;
								case 2:
								case 4:
									for(k=0 ; k<bmber_count ; ++k){
										if(bombers[k].is_dead || bombers[k].marked){
											continue;
										}
										if(bombers[k].x == bombs->x-j && bombers[k].y == bombs->y){
											bombers[k].marked = 1;
											marked_number++;
											active_bmber--;
											break;
										}
									}
									break;
							}
						}
					}
					if(active_bmber == 1){
						for(k=0 ; k<bmber_count ; ++k){
							if(bombers[k].is_dead || bombers[k].marked){
								continue;
							}
							bombers[k].winner = 1;
							stop_checking = 1;
							break;
						}
					}
					if(stop_checking){
						break;
					}
					if(bombs->x+j < map.x){
						if(!dont_check[1]){
							switch(map.world[bombs->x+j][bombs->y]){
								case 1:
									for(k=0 ; k<obs_count ; ++k){
										if(obstacles[k].durability == 0){
											continue;
										}
										if(obstacles[k].x == bombs->x+j && obstacles[k].y == bombs->y){
											if(obstacles[k].durability != -1){
												obstacles[k].durability--;
											}
											obstacle_send->position.x = (unsigned int)obstacles[k].x;
											obstacle_send->position.y = (unsigned int)obstacles[k].y;
											obstacle_send->remaining_durability = obstacles[k].durability;
											if(obstacles[k].durability == 0){
												map.world[obstacles[k].x][obstacles[k].y] = 0;
											}
											dont_check[1] = 1;
											print_output(NULL, NULL, obstacle_send, NULL);
											break;
										}
									}
									break;
								case 2:
								case 4:
									for(k=0 ; k<bmber_count ; ++k){
										if(bombers[k].is_dead || bombers[k].marked){
											continue;
										}
										if(bombers[k].x == bombs->x+j && bombers[k].y == bombs->y){
											bombers[k].marked = 1;
											marked_number++;
											active_bmber--;
											break;
										}
									}
									break;
							}
						}
					}
					if(active_bmber == 1){
						for(k=0 ; k<bmber_count ; ++k){
							if(bombers[k].is_dead || bombers[k].marked){
								continue;
							}
							bombers[k].winner = 1;
							stop_checking = 1;
							break;
						}
					}
					if(stop_checking){
						break;
					}
					if(bombs->y-j >= 0){
						if(!dont_check[2]){
							switch(map.world[bombs->x][bombs->y-j]){
								case 1:
									for(k=0 ; k<obs_count ; ++k){
										if(obstacles[k].durability == 0){
											continue;
										}
										if(obstacles[k].x == bombs->x && obstacles[k].y == bombs->y-j){
											if(obstacles[k].durability != -1){
												obstacles[k].durability--;
											}
											obstacle_send->position.x = (unsigned int)obstacles[k].x;
											obstacle_send->position.y = (unsigned int)obstacles[k].y;
											obstacle_send->remaining_durability = obstacles[k].durability;
											if(obstacles[k].durability == 0){
												map.world[obstacles[k].x][obstacles[k].y] = 0;
											}
											dont_check[2] = 1;
											print_output(NULL, NULL, obstacle_send, NULL);
											break;
										}
									}
									break;
								case 2:
								case 4:
									for(k=0 ; k<bmber_count ; ++k){
										if(bombers[k].is_dead || bombers[k].marked){
											continue;
										}
										if(bombers[k].x == bombs->x && bombers[k].y == bombs->y-j){
											bombers[k].marked = 1;
											marked_number++;
											active_bmber--;
											break;
										}
									}
									break;
							}
						}
					}
					
					if(active_bmber == 1){
						for(k=0 ; k<bmber_count ; ++k){
							if(bombers[k].is_dead || bombers[k].marked){
								continue;
							}
							bombers[k].winner = 1;
							stop_checking = 1;
							break;
						}
					}
					if(stop_checking){
						break;
					}
					if(bombs->y+j < map.y){
						if(!dont_check[3]){
							switch(map.world[bombs->x][bombs->y+j]){
								case 1:
									for(k=0 ; k<obs_count ; ++k){
										if(obstacles[k].durability == 0){
											continue;
										}
										if(obstacles[k].x == bombs->x && obstacles[k].y == bombs->y-j){
											if(obstacles[k].durability != -1){
												obstacles[k].durability--;
											}
											obstacle_send->position.x = (unsigned int)obstacles[k].x;
											obstacle_send->position.y = (unsigned int)obstacles[k].y;
											obstacle_send->remaining_durability = obstacles[k].durability;
											if(obstacles[k].durability == 0){
												map.world[obstacles[k].x][obstacles[k].y] = 0;
											}
											dont_check[3] = 1;
											print_output(NULL, NULL, obstacle_send, NULL);
											break;
										}
									}
									break;
								case 2:
								case 4:
									for(k=0 ; k<bmber_count ; ++k){
										if(bombers[k].is_dead || bombers[k].marked){
											continue;
										}
										if(bombers[k].x == bombs->x && bombers[k].y == bombs->y+j){
											bombers[k].marked = 1;
											marked_number++;
											active_bmber--;
											break;
										}
									}
									break;
							}
						}
					}
				}
				
				if(map.world[bombs->x][bombs->y] == 3){
					map.world[bombs->x][bombs->y] = 0;
				}else if(map.world[bombs->x][bombs->y] == 4){
					map.world[bombs->x][bombs->y] = 2;
				}
				close(fds[0].fd);
				waitpid(bombs->pid, &status, 0);
				tmp = bombs;
				bombs = bombs->next;
				free(tmp);
			}else{
				bombs = bombs->next;
			}
		}
		//printf("\n\nhere\n");
		for(i=0 ; i<bmber_count ; ++i){
			//printf("is_dead: %d\n", bombers[i].is_dead);
			if(bombers[i].is_dead){
				continue;
			}
			fds[0].fd = fd[i][0];
			fds[0].events = POLLIN;
			l = poll(fds, 1, 1);
			if(l > 0){
				if(bombers[i].marked){
					//printf("\n\n\n\nhere1\n");
					omessage->type = BOMBER_DIE;
					output_print->pid = bombers[i].pid;
					output_print->m = omessage;
					print_output(NULL, output_print, NULL, NULL);
					close(fds[0].fd);
					kill(bombers[i].pid, SIGKILL);
					waitpid(bombers[i].pid, &status, 0);
					marked_number--;
					bombers[i].is_dead = 1;
					if(map.world[bombers[i].x][bombers[i].y] == 2){
						map.world[bombers[i].x][bombers[i].y] == 0;
					}else if(map.world[bombers[i].x][bombers[i].y] == 4){
						map.world[bombers[i].x][bombers[i].y] == 3;
					}
					bombers[i].marked = 0;
				}
				//printf("%d\n", marked_number);
				if(bombers[i].winner && marked_number == 0){
					WIN = 1;
					omessage->type = BOMBER_WIN;
					output_print->pid = bombers[i].pid;
					output_print->m = omessage;
					print_output(NULL, output_print, NULL, NULL);
					break;
				}
				if(bombers[i].winner){
					continue;
				}
				obj_count = 0;
				read_data(fds[0].fd, imessage);
				input_print->pid = bombers[i].pid;
				input_print->m = imessage;
				print_output(input_print, NULL, NULL, NULL);
				switch(imessage->type){
					case BOMBER_START:
						omessage->type = BOMBER_LOCATION;
						omessage->data.new_position.x = (unsigned int)bombers[i].x;
						omessage->data.new_position.y = (unsigned int)bombers[i].y;
						send_message(fds[0].fd, omessage);
						output_print->pid = bombers[i].pid;
						output_print->m = omessage;
						print_output(NULL, output_print, NULL, NULL);
						break;
					case BOMBER_SEE:
						omessage->type = BOMBER_VISION;
						if(map.world[bombers[i].x][bombers[i].y] == 4){
							objects[obj_count].position.x = (unsigned int)bombers[i].x;
							objects[obj_count].position.y = (unsigned int)bombers[i].y;
							objects[obj_count].type = BOMB;
							obj_count++;
						}
						for(j=1 ; j<=3 ; ++j){
							if(map.y > bombers[i].y+j){
								switch(map.world[bombers[i].x][bombers[i].y+j]){
									case 1:
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y+j;
										objects[obj_count].type = OBSTACLE;
										obj_count++;
										break;
									case 2:
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y+j;
										objects[obj_count].type = BOMBER;
										obj_count++;
										break;
									case 3:
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y+j;
										objects[obj_count].type = BOMB;
										obj_count++;
										break;
									case 4:
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y+j;
										objects[obj_count].type = BOMB;
										obj_count++;
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y+j;
										objects[obj_count].type = BOMBER;
										obj_count++;
										break;
								}
							}
						}
						for(j=1 ; j<=3 ; ++j){
							if(bombers[i].y-j >= 0){
								switch(map.world[bombers[i].x][bombers[i].y-j]){
									case 1:
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y-j;
										objects[obj_count].type = OBSTACLE;
										obj_count++;
										break;
									case 2:
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y-j;
										objects[obj_count].type = BOMBER;
										obj_count++;
										break;
									case 3:
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y-j;
										objects[obj_count].type = BOMB;
										obj_count++;
										break;
									case 4:
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y-j;
										objects[obj_count].type = BOMB;
										obj_count++;
										objects[obj_count].position.x = (unsigned int)bombers[i].x;
										objects[obj_count].position.y = (unsigned int)bombers[i].y-j;
										objects[obj_count].type = BOMBER;
										obj_count++;
										break;
								}
							}
						}
						for(j=1 ; j<=3 ; ++j){
							if(map.x > bombers[i].x+j){
								switch(map.world[bombers[i].x+j][bombers[i].y]){
									case 1:
										objects[obj_count].position.x = (unsigned int)bombers[i].x+j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = OBSTACLE;
										obj_count++;
										break;
									case 2:
										objects[obj_count].position.x = (unsigned int)bombers[i].x+j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = BOMBER;
										obj_count++;
										break;
									case 3:
										objects[obj_count].position.x = (unsigned int)bombers[i].x+j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = BOMB;
										obj_count++;
										break;
									case 4:
										objects[obj_count].position.x = (unsigned int)bombers[i].x+j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = BOMB;
										obj_count++;
										objects[obj_count].position.x = (unsigned int)bombers[i].x+j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = BOMBER;
										obj_count++;
										break;
								}
							}
						}
						for(j=1 ; j<=3 ; ++j){
							if(bombers[i].x-j >= 0){
								switch(map.world[bombers[i].x-j][bombers[i].y]){
									case 1:
										objects[obj_count].position.x = (unsigned int)bombers[i].x-j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = OBSTACLE;
										obj_count++;
										break;
									case 2:
										objects[obj_count].position.x = (unsigned int)bombers[i].x-j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = BOMBER;
										obj_count++;
										break;
									case 3:
										objects[obj_count].position.x = (unsigned int)bombers[i].x-j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = BOMB;
										obj_count++;
										break;
									case 4:
										objects[obj_count].position.x = (unsigned int)bombers[i].x-j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = BOMB;
										obj_count++;
										objects[obj_count].position.x = (unsigned int)bombers[i].x-j;
										objects[obj_count].position.y = (unsigned int)bombers[i].y;
										objects[obj_count].type = BOMBER;
										obj_count++;
										break;
								}
							}
						}
						omessage->type = BOMBER_VISION;
						omessage->data.object_count = obj_count;
						send_message(fds[0].fd, omessage);
						send_object_data(fds[0].fd, obj_count, objects);
						output_print->pid = bombers[i].pid;
						output_print->m = omessage;
						print_output(NULL, output_print, NULL, objects);
						break;
						
					case BOMBER_MOVE:
						if(can_move(imessage->data.target_position, bombers, i)){
							omessage->type = BOMBER_LOCATION;
							omessage->data.new_position.x = imessage->data.target_position.x;
							omessage->data.new_position.y = imessage->data.target_position.y;
							send_message(fds[0].fd, omessage);
							output_print->pid = bombers[i].pid;
							output_print->m = omessage;
							print_output(NULL, output_print, NULL, NULL);
						}else{
							omessage->type = BOMBER_LOCATION;
							omessage->data.new_position.x = (unsigned int)bombers[i].x;
							omessage->data.new_position.y = (unsigned int)bombers[i].y;
							send_message(fds[0].fd, omessage);
							output_print->pid = bombers[i].pid;
							output_print->m = omessage;
							print_output(NULL, output_print, NULL, NULL);
						}
						break;
					
					case BOMBER_PLANT:
						if(map.world[bombers[i].x][bombers[i].y] == 4){
							omessage->type = BOMBER_PLANT_RESULT;
							omessage->data.planted = 0;
							send_message(fds[0].fd, omessage);
							output_print->pid = bombers[i].pid;
							output_print->m = omessage;
							print_output(NULL, output_print, NULL, NULL);
						}else if(map.world[bombers[i].x][bombers[i].y] == 2){
							map.world[bombers[i].x][bombers[i].y] = 4;
							omessage->type = BOMBER_PLANT_RESULT;
							omessage->data.planted = 1;
							bmb_count++;
							bomb_arguments[0] = "./bomb";
							snprintf(buffer, sizeof(buffer), "%ld", imessage->data.bomb_info.interval);
							bomb_arguments[1] = buffer;
							PIPE(tmp_fd);
							pid = fork();
							if(pid == 0){
							
								close(STDIN_FILENO);
								close(STDOUT_FILENO);
								close(tmp_fd[0]);
							
								dup2(tmp_fd[1], 1);
								dup2(tmp_fd[1], 0);
								close(tmp_fd[1]);
								
								
								execvp("./bomb", bomb_arguments);
							}else{
								//free(bomb_arguments[0]);
								//free(bomb_arguments[1]);
								close(tmp_fd[1]);
								create_bomb(pid, imessage->data.bomb_info.radius, bombers[i].x, bombers[i].y, tmp_fd[0]);
								send_message(fds[0].fd, omessage);
								output_print->pid = bombers[i].pid;
								output_print->m = omessage;
								print_output(NULL, output_print, NULL, NULL);
							}
						}
				}
			}
		}
		sleep(0.001);
	}
	//printf("%d\n", bmb_count);
	while(bmb_count > 0){
		bombs = head;
		for(i=0 ; i<bmb_count ; ++i){
			fds[0].fd = bombs->fd;
			fds[0].events = POLLIN;
			if(poll(fds, 1, 1) > 0){
				bmb_count--;
				tmp = head;
				if(bombs == head){
					head = head->next;
				}else{
					while(tmp->next != bombs){
						tmp = tmp->next;
					}
					tmp->next = bombs->next;
				}
				read_data(fds[0].fd, imessage);
				input_print->pid = bombs->pid;
				input_print->m = imessage;
				print_output(input_print, NULL, NULL, NULL);
				close(fds[0].fd);
				waitpid(bombs->pid, &status, 0);
				tmp = bombs;
				bombs = bombs->next;
				free(tmp);
			}else{
				bombs = bombs->next;
			}
		}
	}
}	
		
		
