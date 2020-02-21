#include <stdbool.h>
#include "main.h"
#include "watek_komunikacyjny.h"


const int CHANCE_OF_DEATH = 10;

packet_t* createPacket(data) {
  packet_t *pkt = malloc(sizeof(packet_t));

  pkt->data = data;

  return pkt;
}

void bestEffortBroadcast(int data, int tag) {
  packet_t* pkt = createPacket(data);

  for (int receiver = 0; receiver < size; receiver++) {
    if (receiver != rank) {
      sendPacket(pkt, receiver, tag);
    }
  }

  free(pkt);
}

void die() {
  bestEffortBroadcast(0, DEATH);
  debug("Umieram...");
  changeState(InFinish);
}

packet_t* createBroadcastPacket(int data, int origin) {
  packet_t *pkt = malloc(sizeof(packet_t));

  pkt->data = data;
  pkt->origin = origin;

  return pkt;
}

void bestEffortBroadcast2(packet_t* pkt, int tag) {
  for (int receiver = 0; receiver < size; receiver++) {
    if (receiver != rank) {
      sendPacket(pkt, receiver, tag);
    }
  }
}

void bestEffortBroadcast2WithDying(packet_t* pkt, int tag) {
  for (int receiver = 0; receiver < size; receiver++) {
    if (receiver != rank) {
      if ((random() % 100) < CHANCE_OF_DEATH) {
        die();
        break;
      }

      sendPacket(pkt, receiver, tag);
    }
  }
}


int lastReceived = -1;

void markAsReceived(int data) {
    lastReceived = data;
}

int hasBeenReceived(int data) {
    return lastReceived == data;
}

void bestEffortBroadcastWithDying(int data, int tag) {
  packet_t* pkt = createPacket(data);

  for (int receiver = 0; receiver < size; receiver++) {
    if (receiver != rank) {

      if ((random() % 100) < CHANCE_OF_DEATH) {
        die();
        break;
      }

      sendPacket(pkt, receiver, tag);
    }
  }

  free(pkt);
}


bool* init_correct() {
    int i;
    bool* correct = malloc(sizeof(bool) * size);

    for (i = 0; i < size; i++) {
      correct[i] = true;
    }

    return correct;
}

bool is_correct(int process, bool* correct) {
  return correct[process] == true;
}

void mark_incorrect(int process, bool* correct) {
  correct[process] = 0;
}

const int DELIVERED_SIZE = 256;

int* init_delivered() {
  int i;
  int* delivered = malloc(sizeof(int) * DELIVERED_SIZE);

  for (i = 0; i < DELIVERED_SIZE; i++) {
    delivered[i] = -1;
  }

  return delivered;
}

bool is_delivered(int data, int* delivered) {
  int i;
  for (i = 0; i < DELIVERED_SIZE; i++) {
    if (delivered[i] == data) {
      return true;
    }
  }

  return false;
}

void add_to_delivered(int data, int* delivered, int* next_index) {
  delivered[*next_index % DELIVERED_SIZE] = data;
  *next_index = (*next_index + 1) % DELIVERED_SIZE;
}

int* init_from() {
  int i;
  int* from = (int*) malloc(size * sizeof(int));

  for (i = 0; i < size; i++) {
      from[i] = -1;
  }

  return from;
}

void save_received(packet_t* pkt, int* from) {
  from[pkt->origin] = pkt->data;
}


void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pkt;
    packet_t* broadcastPacket;
    bool* correct = init_correct();
    int* delivered_passive = init_delivered();
    int next_passive_index = 0;
    int* delivered_active = init_delivered();
    int next_active_index = 0;
    int* from = init_from();

    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (stan != InFinish) {
        int perc = random() * 100;
        MPI_Recv(&pkt, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG) {
            case FINISH:
                changeState(InFinish);
                break;

            case APPMSG:
                debug("Otrzymano %d od %d.", pkt.data, pkt.src);
                break;

            case INMONITOR:
                changeState(InMonitor);
                debug("Oczekiwanie na polecenia monitora...");
                break;

            case INRUN:
                changeState(InRun);
                debug("Autonomiczne podejmowanie decyzji rozpoczete.");
                break;

            case BROADCAST_ACTIVELY:
                debug("Inicjuje rozglaszanie aktywne wartości %d", pkt.data);

                add_to_delivered(pkt.data, delivered_active, &next_active_index);
                broadcastPacket = createBroadcastPacket(pkt.data, rank);
                bestEffortBroadcast2(broadcastPacket, ACTIVE_BROADCAST);
                break;

            case ACTIVE_BROADCAST:
                if (!is_delivered(pkt.data, delivered_active)) {
                  debug("Dostalem %d aktywnie", pkt.data);
                  add_to_delivered(pkt.data, delivered_active, &next_active_index);
                  bestEffortBroadcast2(&pkt, ACTIVE_BROADCAST);
                }
                else {
                  debug("Dostalem %d po raz kolejny, ignoruje", pkt.data);
                }
                break;

            case BROADCAST_PASSIVELY:
                debug("Inicjuje rozglaszanie pasywne wartości %d", pkt.data);
                broadcastPacket = createBroadcastPacket(pkt.data, rank);
                bestEffortBroadcast2(broadcastPacket, PASSIVE_BROADCAST);
                break;

            case PASSIVE_BROADCAST:
                if (!is_delivered(pkt.data, delivered_passive)) {
                  debug("Dostalem rozgloszenie pasywne %d od %d nadane przez %d.", pkt.data, pkt.src, pkt.origin);
                  add_to_delivered(pkt.data, delivered_passive, &next_passive_index);
                  save_received(&pkt, from);

                  if (!is_correct(pkt.src, correct)) {
                    debug("Nadawca %d ulegl awarii, rozglaszam.", pkt.src);
                    bestEffortBroadcast2(&pkt, PASSIVE_BROADCAST);
                  }
                }
                else {
                  // debug("Dostalem rozgloszenie pasywne %d ponownie, ignoruje.", pkt.data);
                }
                break;

            case DEATH:
                mark_incorrect(pkt.src, correct);

                debug("Proces %d umarl.", pkt.src);
                debug("%d", from[pkt.src]);

                int data = from[pkt.src];
                if (data != -1) {
                  debug("Rozglaszam pasywnie wiadomosc %d od procesu %d uleglego awarii.", data, pkt.src);
                  broadcastPacket = createBroadcastPacket(data, pkt.src);
                  bestEffortBroadcast2(broadcastPacket, PASSIVE_BROADCAST);
                }

                break;

            case DIE:
                die();
                break;
        }
    }

    free(broadcastPacket);
    free(correct);
    free(delivered_passive);
    free(from);
}
