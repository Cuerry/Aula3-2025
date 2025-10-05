#include "mlfq.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"
#include "queue.h"

#define MLFQ_LEVELS 3
#define TIME_SLICE_MS 50  // quantum por nível inferior

// Estrutura das filas de prioridade
static queue_t queues[MLFQ_LEVELS];
static int mlfq_initialized = 0;

// Inicializa manualmente as filas (já que não existe queue_init)
static void mlfq_init(void) {
    if (!mlfq_initialized) {
        for (int i = 0; i < MLFQ_LEVELS; i++) {
            queues[i].head = NULL;
            queues[i].tail = NULL;
        }
        mlfq_initialized = 1;
    }
}

// Função auxiliar para verificar se a fila está vazia
static int is_queue_empty(queue_t q) {
    return (q.head == NULL);
}

void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    mlfq_init();

    // Mover todas as tarefas novas da fila "rq" para o topo (nível 0)
    while (rq->head != NULL) {
        pcb_t *task = dequeue_pcb(rq);
        if (task) {
            task->priority = 0;  // começar sempre no topo
            enqueue_pcb(&queues[0], task);
        }
    }

    // Atualiza a tarefa no CPU
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
// Se terminou
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
            cpu_task = NULL;
        }
        // Se esgotou o quantum → desce de nível
        else if ((*cpu_task)->ellapsed_time_ms % TIME_SLICE_MS == 0) {
            int next_level = (*cpu_task)->priority + 1;
            if (next_level >= MLFQ_LEVELS)
                next_level = MLFQ_LEVELS - 1;
            (*cpu_task)->priority = next_level;
            enqueue_pcb(&queues[next_level], *cpu_task);
            cpu_task = NULL;
        }
    }

    // Se o CPU está livre → escolher a próxima tarefa
    if (cpu_task == NULL) {
        for (int lvl = 0; lvl < MLFQ_LEVELS; lvl++) {
            if (!is_queue_empty(queues[lvl])) {
                *cpu_task = dequeue_pcb(&queues[lvl]);
                break;
            }
        }
    }
}