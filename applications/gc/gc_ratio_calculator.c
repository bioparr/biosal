
#include "gc_ratio_calculator.h"

#include <stdio.h>

struct bsal_script gc_ratio_calculator_script = {
    .name = GC_RATIO_CALCULATOR_SCRIPT,
    .init = gc_ratio_calculator_init,
    .destroy = gc_ratio_calculator_destroy,
    .receive = gc_ratio_calculator_receive,
    .size = sizeof(struct gc_ratio_calculator),
    .description = "gc_ratio_calculator"
};

void gc_ratio_calculator_init(struct bsal_actor *actor)
{
    struct gc_ratio_calculator *concrete_actor;
    concrete_actor = bsal_actor_concrete_actor(actor);
    bsal_vector_init(&concrete_actor->spawners, sizeof(int));
    concrete_actor->completed = 0;
}

void gc_ratio_calculator_destroy(struct bsal_actor *actor)
{
    struct gc_ratio_calculator *concrete_actor;
    concrete_actor = bsal_actor_concrete_actor(actor);
    bsal_vector_destroy(&concrete_actor->spawners);
}

void gc_ratio_calculator_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    struct bsal_message new_message;
    int name;
    int tag;
    int source;
    int index;
    int size;
    int neighbor_index;
    int neighbor_name;
    int boss;
    void * buffer;

    struct gc_ratio_calculator *concrete_actor;
    struct bsal_vector *spawners;

    concrete_actor = bsal_actor_concrete_actor(actor);

    name = bsal_actor_get_name(actor);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    source = bsal_message_source(message);
    spawners = &concrete_actor->spawners;
    size = bsal_vector_size(spawners);

    if (tag == BSAL_ACTOR_START) {
        printf("received BSAL_ACTOR_START\n");
        
        bsal_vector_unpack(spawners, buffer);
        size = bsal_vector_size(spawners);
        index = bsal_vector_index_of(spawners, &name);
        neighbor_index = (index + 1) % size;
        neighbor_name = bsal_vector_helper_at_as_int(spawners, neighbor_index);

        printf("about to send to neighbor\n");
        bsal_actor_helper_send_empty(actor, neighbor_name, GC_HELLO);
        /* bsal_message_init(&new_message, GC_HELLO, 0, NULL); */
        /* bsal_actor_send(actor, neighbor_name, &new_message); */
        /* bsal_message_destroy(&new_message); */

    } else if (tag == GC_HELLO) {
        printf("received GC_HELLO\n");

        bsal_actor_helper_send_reply_empty(actor, GC_HELLO_REPLY);

    } else if (tag == GC_HELLO_REPLY) {
        printf("Actor %d is satisfied with a reply from the neighbor %d.\n", name, source);

        boss = bsal_vector_helper_at_as_int(spawners, 0);
        bsal_message_init(&new_message, BSAL_ACTOR_NOTIFY, 0, NULL);
        bsal_actor_send(actor, boss, &new_message);
        bsal_message_destroy(&new_message);

    } else if (tag == BSAL_ACTOR_NOTIFY) {
        printf("received NOTIFY\n");

        ++concrete_actor->completed;
        if (concrete_actor->completed == size) {
            bsal_actor_helper_send_range_empty(actor, spawners, BSAL_ACTOR_ASK_TO_STOP);
        }

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {
        printf("received ASK_TO_STOP\n");

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
    
}
