// Single-Threaded Monster World
// Thomas Padron-McCarthy (thomas.padron-mccarthy@oru.se)
// Sat May  4 17:05:40 CEST 2019

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

struct Monster {
  int number;
  int fights;
  int fights_won;
  int fights_lost;
  // pthread_t thread;
  pthread_mutex_t mutex;
};



// In a multi-threaded version,
// have a thread and a lock for each monster.
// They can be stored in this struct.

// Creates an array of nr_monster monsters
struct Monster *make_monsters(int nr_monsters) {
    struct Monster *all_monsters = malloc(nr_monsters * sizeof(struct Monster));
    if (all_monsters == NULL) {
        fprintf(stderr, "Error: Couldn't allocate space for %d monsters."
                " Error %d = %s\n",
                nr_monsters, errno, strerror(errno));
        exit(EXIT_FAILURE);

    }
    for (int monster = 0; monster < nr_monsters; ++monster) {
        all_monsters[monster].number = monster;
        all_monsters[monster].fights = 0;
        all_monsters[monster].fights_won = 0;
        all_monsters[monster].fights_lost = 0;
	pthread_mutex_init(&all_monsters[monster].mutex, NULL);
    }
    return all_monsters;
} // make_monsters

int global_nr_monsters;
int global_nr_rounds;
struct Monster *global_all_monsters;

// Simulates one round of one monster fighting
void one_monster_fights(struct Monster all_monsters[], int nr_monsters, int this_monster) {
    assert(nr_monsters >= 2); // Otherwise the next loop will hang!
    int opponent;
    
    do {
        opponent = rand() % nr_monsters;
    } while (this_monster == opponent);

    // In a multi-threaded version,
    // we don't want to have two threads
    // updating the same monster's data at the same time.
    // Here, we are updating monsters number "this_monster" and "opponent".

    if(this_monster > opponent) {
      pthread_mutex_lock(&all_monsters[this_monster].mutex);
      pthread_mutex_lock(&all_monsters[opponent].mutex);
    } else {
      pthread_mutex_lock(&all_monsters[opponent].mutex);
      pthread_mutex_lock(&all_monsters[this_monster].mutex);
    }
    
    //Lock monsters
     
   

     all_monsters[this_monster].fights++;
    all_monsters[opponent].fights++;

    if (rand() % 2 == 0) {
        // Won the fight!
        all_monsters[this_monster].fights_won++;
        all_monsters[opponent].fights_lost++;
    }
    else {
        // Lost the fight.
        all_monsters[this_monster].fights_lost++;
        all_monsters[opponent].fights_won++;
    }
    pthread_mutex_unlock(&all_monsters[this_monster].mutex);
    pthread_mutex_unlock(&all_monsters[opponent].mutex);
} // one_monster_fights

// Simulates one round of all the monsters fighting.
// Each monster will attack a random other monster,
// so there will be 2*nr_monsters fights in each round.
//
// In a multi-threaded version,
// each thread should instead do "one_monster_fights"
// in a loop for "nr_rounds" times.
//
void all_monsters_fight(struct Monster all_monsters[], int nr_monsters) {
    for (int monster = 0; monster < nr_monsters; ++monster)
        one_monster_fights(all_monsters, nr_monsters, monster);
} // all_monsters_fight

// Shows some statistics, after the simulation has finished
void show_fights(struct Monster *all_monsters, int nr_monsters) {
    int best_fighter = 0;
    int worst_fighter = 0;
    int total_fights = 0;
    int total_fights_won = 0;
    int total_fights_lost = 0;
    for (int this_monster = 0; this_monster < nr_monsters; ++this_monster) {
        if (all_monsters[this_monster].fights_won > all_monsters[best_fighter].fights_won)
            best_fighter = this_monster;
        if (all_monsters[this_monster].fights_won < all_monsters[worst_fighter].fights_won)
            worst_fighter = this_monster;
        total_fights += all_monsters[this_monster].fights;
        total_fights_won += all_monsters[this_monster].fights_won;
        total_fights_lost += all_monsters[this_monster].fights_lost;
    }
    printf("Total fights: %d, total won: %d, total lost: %d\n", total_fights / 2, total_fights_won, total_fights_lost);
    if (total_fights_won != total_fights_lost || total_fights_won + total_fights_lost != total_fights)
        printf("    *** That seems strange! The numbers don't add up. ***\n");
    printf("There are %d monsters, with an average of %.2f fights per monster.\n",
           nr_monsters, (double)total_fights / nr_monsters);
    printf("Best fighter: Monster %d with %d won fight(s)\n",
           all_monsters[best_fighter].number, all_monsters[best_fighter].fights_won);
    printf("Worst fighter: Monster %d with %d won fight(s)\n",
           all_monsters[worst_fighter].number, all_monsters[worst_fighter].fights_won);
}

void usage(void) {
    fprintf(stderr, "Usage: monster-world NR-MONSTERS NR-ROUNDS\n");
    exit(EXIT_FAILURE);
}

void* thread_function( void *ptr ) {
  printf("Tråd startar: antal %d, detta nummer %d\n",  global_nr_monsters, ((struct Monster*)ptr)->number);
  fflush(stdout);
  for(int round = 0; round < global_nr_rounds; ++round) {
    one_monster_fights(global_all_monsters, global_nr_monsters, ((struct Monster*)ptr)->number);
  }

  
  return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3)
        usage();
    int nr_monsters;
    if (sscanf(argv[1], "%d", &nr_monsters) != 1 || nr_monsters < 2)
        usage();
    global_nr_monsters = nr_monsters;
    int nr_rounds;
    if (sscanf(argv[2], "%d", &nr_rounds) != 1 || nr_rounds < 1)
        usage();
    global_nr_rounds = nr_rounds;

    srand(time(NULL));

    printf("Creating %d monsters...\n", nr_monsters);
    struct Monster *all_monsters = make_monsters(nr_monsters);
    global_all_monsters = all_monsters;

    pthread_t thread[nr_monsters];

    printf("Simulating %d rounds with %d monsters...\n",
           nr_rounds, nr_monsters);

    // all_monsters[monster]  ((struct Monster*)ptr)->number
    
    for (int monster = 0; monster < nr_monsters; ++monster) {
      // Create thread for each monster with "one_monster_fight()" as argumen
      // assert(pthread_create(&(thread[monster]), NULL, print_message, (void *) message) == 0);
      printf("Startar tråd %d...\n", monster);
      fflush(stdout);
      pthread_create(&(thread[monster]), NULL, thread_function, (void *)&all_monsters[monster]);
    }

    for (int monster = 0; monster < nr_monsters; ++monster) {
      pthread_join(thread[monster], NULL);
    }
    

    printf("Done!\n");
    show_fights(all_monsters, nr_monsters);

    return EXIT_SUCCESS;
} // main
