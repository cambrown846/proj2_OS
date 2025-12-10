#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

/* Pick a random menu item */
MenuItem PickRandomMenuItem() {
    int index = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[index];
}

/* Open the restaurant */
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL *bcb = malloc(sizeof(BENSCHILLIBOWL));

    bcb->orders = NULL;
    bcb->max_size = max_size;
    bcb->current_size = 0;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    printf("Restaurant is open!\n");

    srand(time(NULL)); // seed once

    return bcb;
}

/* Close the restaurant */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    assert(bcb->orders_handled == bcb->expected_num_orders);

    pthread_mutex_unlock(&bcb->mutex);

    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    printf("Restaurant is closed!\n");

    free(bcb);
}

/* Add order to back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {

    pthread_mutex_lock(&bcb->mutex);

    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    order->order_number = bcb->next_order_number++;
    order->next = NULL;

    AddOrderToBack(&bcb->orders, order);
    bcb->current_size++;

    pthread_cond_signal(&bcb->can_get_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return order->order_number;
}

Order *GetOrder(BENSCHILLIBOWL* bcb) {

    pthread_mutex_lock(&bcb->mutex);

    while (IsEmpty(bcb)) {

        if (bcb->orders_handled == bcb->expected_num_orders) {
            pthread_cond_broadcast(&bcb->can_get_orders);
            pthread_mutex_unlock(&bcb->mutex);
            return NULL;
        }
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    Order *front = bcb->orders;
    bcb->orders = front->next;

    bcb->current_size--;
    bcb->orders_handled++;

    pthread_cond_signal(&bcb->can_add_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return front;
}

bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == bcb->max_size;
}

void AddOrderToBack(Order **orders, Order *order) {
    if (*orders == NULL) {
        *orders = order;
    } else {
        Order *temp = *orders;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = order;
    }
}
