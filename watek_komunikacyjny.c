#include "main.h"
#include "watek_komunikacyjny.h"

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
}

int lastReceived = -1;

void markAsReceived(int data) {
    lastReceived = data;
}

int hasBeenReceived(int data) {
    return lastReceived == data;
}

const int CHANCE_OF_DEATH = 20;

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


void schedule(packet_t pkt, int* scheduled) {
    scheduled[pkt.src] = pkt.data;
}

int getScheduledData(packet_t pkt, int* scheduled) {
    return scheduled[pkt.src];
}

int isScheduled(packet_t pkt, int* scheduled) {
    return scheduled[pkt.src] != -1;
}

void unschedule(packet_t pkt, int* scheduled) {
    scheduled[pkt.src] = -1;
}

void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pkt;
    int* scheduled = malloc(sizeof(int)*(size - 1));

    for (int i = 0; i < size; i++) {
      scheduled[i] = -1;
    }


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
                markAsReceived(pkt.data);
                bestEffortBroadcastWithDying(pkt.data, ACTIVE_BROADCAST);
                break;

            case ACTIVE_BROADCAST:
                if (!hasBeenReceived(pkt.data)) {
                  debug("Dostalem %d aktywnie", pkt.data);
                  markAsReceived(pkt.data);
                  bestEffortBroadcastWithDying(pkt.data, ACTIVE_BROADCAST);
                }
                else {
                  debug("Dostalem %d po raz kolejny, ignoruje", pkt.data);
                }
                break;

            case BROADCAST_PASSIVELY:
                debug("Inicjuje rozglaszanie pasywne wartości %d", pkt.data);
                markAsReceived(pkt.data);
                bestEffortBroadcastWithDying(pkt.data, PASSIVE_BROADCAST);
                break;

            case PASSIVE_BROADCAST:
                if (!hasBeenReceived(pkt.data)) {
                  debug("Rozgloszenie pasywne wartosci %d od %d, kolejkuje.", pkt.data, pkt.src);
                  schedule(pkt, scheduled);
                  markAsReceived(pkt.data);
                }
                else {
                  debug("Dostalem %d ponownie, ignoruje", pkt.data);
                }
                break;

            case DEATH:
                if (isScheduled(pkt, scheduled)) {
                  debug("Rozglaszam pasywnie wiadomosc %d od niepoprawnego %d.", pkt.data, pkt.src);
                  int scheduledData = getScheduledData(pkt, scheduled);
                  unschedule(pkt, scheduled);
                  bestEffortBroadcastWithDying(scheduledData, PASSIVE_BROADCAST);
                }
                break;
        }
    }
}
