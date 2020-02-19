#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
    srandom(rank);
    while (stan != InFinish) {
        int perc = random() % 100;

        if (perc < STATE_CHANGE_PROB && stan == InRun) {
            debug("Wysylanie...");

            changeState(InSend);

            packet_t *pkt = malloc(sizeof(packet_t));
            pkt->data = perc;

            int receiver = (rank+1) % size;

            sendPacket(pkt, receiver, APPMSG);

            debug("Wysłano %d do %d.", pkt->data, receiver);

            changeState(InRun);
        }

        sleep(SEC_IN_STATE);
    }
}
