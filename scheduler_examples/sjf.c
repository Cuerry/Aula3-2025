#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"
#include "queue.h"

/**
 * Shortest Job First (SJF) Scheduling Algorithm (non-preemptive).
 */
void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Task finished
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free(*cpu_task);
            *cpu_task = NULL;
        }
    }

    if (*cpu_task == NULL) {
        // Procurar na fila de prontos a tarefa com menor tempo restante
        queue_elem_t *it = rq->head;
        queue_elem_t *min_elem = NULL;
        int min_time_left = -1;

        while (it != NULL) {
            int time_left = it->pcb->time_ms - it->pcb->ellapsed_time_ms;
            if (min_elem == NULL || time_left < min_time_left) {
                min_elem = it;
                min_time_left = time_left;
            }
            it = it->next;
        }

        if (min_elem) {
            // Remove da fila e mete no CPU
            queue_elem_t *removed = remove_queue_elem(rq, min_elem);
            if (removed) {
                *cpu_task = removed->pcb;
                free(removed); // liberta só o nó, não o PCB
            }
        }
    }
}