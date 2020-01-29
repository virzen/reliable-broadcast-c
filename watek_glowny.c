#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
    srandom(rank);
    while (stan != InFinish) {
        int perc = random() % 100; 

        if (perc < STATE_CHANGE_PROB && stan == InRun) {
            debug("Zmieniam stan na wysyłanie");

            changeState(InSend);

            packet_t *pkt = malloc(sizeof(packet_t));
            pkt->data = perc;

            sendPacket(pkt, (rank+1) % size, APPMSG);

            changeState(InRun);

            debug("Skończyłem wysyłać");
        }

        sleep(SEC_IN_STATE);
    }
}
