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

void broadcastPassive(int data) {
  bestEffortBroadcast(data, PASSIVE_BROADCAST);
}

void broadcastActive(int data) {
  bestEffortBroadcast(data, ACTIVE_BROADCAST);
}

void die() {
  bestEffortBroadcast(0, DEATH);
  debug("Umieram...");
}

void sendPassiveAndDie(int data) {
  int randomReceiver = random() * size;

  packet_t* pkt = createPacket(data);

  sendPacket(pkt, randomReceiver, PASSIVE_BROADCAST);

  die(size, rank);
}

void sendActiveAndDie(int data) {
  int randomReceiver = random() * size;

  packet_t* pkt = createPacket(data);

  sendPacket(pkt, randomReceiver, ACTIVE_BROADCAST);

  die(size, rank);
}


int lastReceivedActive = -1;

void markAsReceived(int data) {
    lastReceivedActive = data;
}

int hasBeenReceived(int data) {
    return lastReceivedActive == data;
}

const int CHANCE_OF_DEATH = 50;

void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pkt;

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

            case BROADCAST_PASSIVELY:
                debug("Teraz rozpoczalbym pasywne rozglaszanie...")
                break;

            case BROADCAST_ACTIVELY:
                debug("Inicjuje rozglaszanie aktywne %d", pkt.data);
                bestEffortBroadcast(pkt.data, ACTIVE_BROADCAST);
                markAsReceived(pkt.data);
                if (perc < CHANCE_OF_DEATH) {
                  die();
                }
                break;

            case PASSIVE_BROADCAST:
                debug("rozgloszenie PASYWNE (%d) od %d.", pkt.data, pkt.src);
                break;

            case ACTIVE_BROADCAST:
                if (!hasBeenReceived(pkt.data)) {
                  debug("Dostalem %d aktywnie", pkt.data);
                  bestEffortBroadcast(pkt.data, ACTIVE_BROADCAST);
                  markAsReceived(pkt.data);
                  if (perc < CHANCE_OF_DEATH) {
                    die();
                  }
                }
                else {
                  debug("Dostalem %d po raz kolejny, ignoruje", pkt.data);
                }
                break;

            case DEATH:
                debug("%d umarl.", pkt.src);
                break;
        }
    }
}
