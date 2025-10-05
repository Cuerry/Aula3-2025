#include "rr.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"
#include "queue.h"

#define TIME_SLICE_MS 500

void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // Verifica se a tarefa terminou
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
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
        } else if (current_time_ms - (*cpu_task)->slice_start_ms >= TIME_SLICE_MS) {
            // Time slice terminou mas a tarefa ainda não acabou
            (*cpu_task)->slice_start_ms = 0; // opcional: limpas para próxima vez
            enqueue_pcb(rq, *cpu_task);      // devolve ao fim da fila
            *cpu_task = NULL;
        }
    }

    // Se o CPU está livre, mete próxima tarefa (reset ao slice_start_ms)
    if (*cpu_task == NULL) {
        pcb_t *next_task = dequeue_pcb(rq);
        if (next_task) {
            next_task->slice_start_ms = current_time_ms;
            *cpu_task = next_task;
        }
    }
}