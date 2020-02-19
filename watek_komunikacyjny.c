#include "main.h"
#include "watek_komunikacyjny.h"

void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;

    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (stan != InFinish) {
        debug("Oczekiwanie na wiadomosc...");
        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG) {
            case FINISH:
                changeState(InFinish);
                break;

            case APPMSG:
                debug("Otrzymano wiadomość %d od %d.", pakiet.data, pakiet.src);
                break;

            case INMONITOR:
                changeState(InMonitor);
                debug("Oczekiwanie na polecenia monitora...");
                break;

            case INRUN:
                changeState(InRun);
                debug("Autonomiczne podejmowanie decyzji rozpoczete.");
                break;
        }
    }
}
