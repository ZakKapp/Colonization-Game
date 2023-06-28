//"I pledge that I have neither given nor received help from anyone other
//than the instructor/TA for all program components included here."
//Zak Kappenman
//Student ID: 43851109


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
int gameFinished = 0;

typedef struct threadInfo_ {
	int team;
	int boardRows;
	int boardCols;
	int* gameBoard;
	FILE* file;
} threadInfo;

// helper function for checking if command line arguments are valid numbers
int isValidNumber(char num[]) {
	for(int i = 0; num[i] != '\0' ; i++) {
		if(num[i] == '-')
			return 1;
		if(num[i] < '0' || num[i] > '9')
			return 1;
	}
	return 0;
}

// thread function for all of the soldiers
void * soldierThread(void * param) {
	threadInfo * myParam = (threadInfo *)param;
	while(gameFinished != 1) {
		int attack = myParam->team;
		int i = rand() % myParam->boardRows;
		int j = rand() % myParam->boardCols;
		int index = j + (i * myParam->boardCols);
		int teamFlag = 0;
		int enemyFlag = 0;
		int map[(myParam->boardRows * myParam->boardCols)];

		int bulletTime = (rand() % 3) + 1;

		sleep(bulletTime);
		if(gameFinished != 1) {

			// checking for soldier space
			if(myParam->gameBoard[index] != 3 && myParam->gameBoard[index] != 4) {

				// checking if missle hits an ally territory
				if(myParam->gameBoard[index] == attack) {
					pthread_mutex_lock(&myMutex);
					if(gameFinished != 1) {
						myParam->gameBoard[index] = 0;

						// updating map in binary file
						for(int i = 0; i < (myParam->boardRows * myParam->boardCols); i++) {
							map[i] = myParam->gameBoard[i];
						}
						myParam->file = fopen("board.bin", "wb");
						fwrite(&map, sizeof(map), 1, myParam->file);
						fclose(myParam->file);
					}
					pthread_mutex_unlock(&myMutex);
				}
				else {
					pthread_mutex_lock(&myMutex);
					if(gameFinished != 1) {
						myParam->gameBoard[index] = attack;

						// updating map in binary file
						for(int i = 0; i < (myParam->boardRows * myParam->boardCols); i++) {
							map[i] = myParam->gameBoard[i];
						}
						myParam->file = fopen("board.bin", "wb");
						fwrite(&map, sizeof(map), 1, myParam->file);
						fclose(myParam->file);
					}
					pthread_mutex_unlock(&myMutex);

					if(gameFinished != 1) {
						// check all around the index
						pthread_mutex_lock(&myMutex);
						for(int x = MAX(0, i-1); x <= MIN(i+1, myParam->boardRows); x++) {
							for(int y = MAX(0, j-1); y <= MIN(j+1, myParam->boardCols); y++) {
								int nIndex = y + (x * myParam->boardCols);
								if(gameFinished != 1) {
									if(myParam->gameBoard[nIndex] == attack || myParam->gameBoard[nIndex] == (attack+2))
										teamFlag++;
									else enemyFlag++;
								}
							}
						}
						pthread_mutex_unlock(&myMutex);
						if(gameFinished != 1) {
							// if majority of territory around is owned, capture the neighbors
							if(teamFlag > enemyFlag) {
								pthread_mutex_lock(&myMutex);
								for(int x = MAX(0, i-1); x <= MIN(i+1, myParam->boardRows); x++) {
									for(int y = MAX(0, j-1); y <= MIN(j+1, myParam->boardCols); y++) {
										int nIndex = y + (x * myParam->boardCols);
										if(gameFinished != 1) {
											if(myParam->gameBoard[nIndex] != 3 && myParam->gameBoard[nIndex] != 4) {
												if(gameFinished != 1) {
													myParam->gameBoard[nIndex] = attack;
												}
											}
										}
									}
								} // end neighbor assignments

								// updating map in binary file
								for(int i = 0; i < (myParam->boardRows * myParam->boardCols); i++) {
									map[i] = myParam->gameBoard[i];
								}
								myParam->file = fopen("board.bin", "wb");
								fwrite(&map, sizeof(map), 1, myParam->file);
								fclose(myParam->file);
								pthread_mutex_unlock(&myMutex);
							}
						}
					} // end game end catch
				} // end else
			} // end soldier space check
		} // end game end check
	} // end while
	return (void*)0;
} // end soldierThread

// thread function for the supervisor to tell all the other threads to stop when the game finishes
void * supervisorThread(void * param){
	threadInfo * myParam = (threadInfo *)param;
	int flag;

	while(gameFinished != 1) {
		flag = 0;

		for(int i =0; i < (myParam->boardRows * myParam->boardCols); i++) {
			if(myParam->gameBoard[i] == 0)
				flag++;
		}
		if(flag == 0)
			gameFinished = 1;
	}
	return (void*)0;
} // end supervisorThread

int main(int argc, char * argv[]) {

	// check if input format is correct
	if(argc != 5) {
		printf("ERROR: Program called incorrectly\n");
                printf("Correct Usage: executable numberOfT1Players numberOfT2Players numberOfRows numberOfColumns\n");
                return -1;
	}

	// check if input contains valid numbers
	for(int i = 1; i < argc; i++) {
		if(isValidNumber(argv[i]) != 0) {
			printf("ERROR: Invalid Input for Number of Players and/or Board Size\n");
			printf("Please make sure inputs are positive, non-zero integers\n");
			return -1;
		}
	}

	// displaying the legend
	printf("\n-------------------------\n");
	printf("|Team 1 Soldier is a 3  |\n");
	printf("|Team 1 Territory is a 1|\n");
	printf("|Team 2 Soldier is a 4  |\n");
	printf("|Team 2 Territory is a 2|\n");
	printf("-------------------------\n\n");

	// initializing variables
	int t1P = atoi(argv[1]);
	int t2P = atoi(argv[2]);
	int rows = atoi(argv[3]);
	int cols = atoi(argv[4]);
	FILE* file;
	pthread_t AThreadArray[t1P];
	pthread_t BThreadArray[t2P];
	pthread_t supervisor;
	printf("There are %d players on Team 1\n", t1P);
	printf("There are %d players on Team 2\n\n", t2P);

	srand(time(0));

	int length = rows * cols;
	int board[length];

	// initializing the board
	for(int i = 0; i < length; i++) {
		board[i] = 0;
	}
	// placing the soldiers on the board
	for(int i = 0; i < (t1P + t2P); i++) {
		int randI = rand() % rows;
		int randJ = rand() % cols;
		int ind = randJ + (randI * cols);
		int taken = 0;
		if(i < t1P) {
			if(board[ind] != 0) {
				taken = 1;
				while(taken == 1) {
					randI = rand() % rows;
					randJ = rand() % cols;
					ind = randJ + (randI * cols);
					if(board[ind] == 0)
						taken = 0;
				}
				board[ind] = 3;
				printf("Team 1 soldier started at [%d, %d]\n", randI, randJ);
			}
			else {
				board[ind] = 3;
				printf("Team 1 soldier started at [%d, %d]\n", randI, randJ);
			}
		}
		else {
			if(board[ind] != 0) {
				taken = 1;
				while(taken == 1) {
					randI = rand() % rows;
					randJ = rand() % cols;
					ind = randJ + (randI * cols);
					if(board[ind] == 0)
						taken = 0;
				}
				board[ind] = 4;
				printf("Team 2 soldier started at [%d, %d]\n", randI, randJ);
			}
			else {
				board[ind] = 4;
				printf("Team 2 soldier started at [%d, %d]\n", randI, randJ);
			}
		}
	}
	printf("\n");

	// preparing the binary file
	file = fopen("board.bin", "wb");
	if(file == NULL) {
		printf("ERROR: Could not open board.bin\n");
		return -2;
	}

	fwrite(&board, sizeof(board), 1, file);
	fclose(file);

	// creating team 1s object
	threadInfo * a = malloc(sizeof(threadInfo));
	a->gameBoard = board;
	a->file = file;
	a->boardRows = rows;
	a->boardCols = cols;
	a->team = 1;

	// creating team 2s object
	threadInfo * b = malloc(sizeof(threadInfo));
	b->gameBoard = board;
	b->file = file;
	b->boardRows = rows;
	b->boardCols = cols;
	b->team = 2;

	// creating the threads
	pthread_create(&supervisor, NULL, supervisorThread, (void*)a);

	for(int i = 0; i < t1P; i++) {
		pthread_create(&AThreadArray[i], NULL, soldierThread, (void *)a);
	}
	for(int i = 0; i < t2P; i++) {
		pthread_create(&BThreadArray[i], NULL, soldierThread, (void *)b);
	}

	// battle report of the board every 2 seconds
	while(gameFinished != 1) {
		sleep(2);
		for(int i = 0; i < length; i++) {
			printf("%d  ", board[i]);
			if((i + 1) % cols == 0) {
				printf("\n");
			}
		}
		printf("\n");
	}

	// ensuring all soldiers have stopped firing
	sleep(2);

	// checking who won
	int t1Flag = 0;
	int t2Flag = 0;
	for(int i = 0; i < (rows * cols); i++) {
		if(board[i] == 1 || board[i] == 3)
			t1Flag++;
		else t2Flag++;
	}

	// declaring the victor
	if(t1Flag > t2Flag)
		printf("Team 1 Wins With %d Territories\n", t1Flag);
	if(t1Flag < t2Flag)
		printf("Team 2 Wins With %d Territories\n", t2Flag);
	if(t1Flag == t2Flag)
		printf("Draw! Nobody Wins\n");

	// displaying the final board
	printf("\nEnd Game Board:\n");
	int finalBoard[length];
	file = fopen("board.bin", "rb");
	fread(&finalBoard, sizeof(finalBoard), 1, file);
	fclose(file);
	for(int i = 0; i < length; i++) {
		printf("%d  ", finalBoard[i]);
		if((i+1) % cols == 0)
			printf("\n");
	}
	printf("\n");

	free(a);
	free(b);
} // end main

