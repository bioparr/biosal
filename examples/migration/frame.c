
#include "frame.h"

#include <stdio.h>
#include <stdlib.h>

struct bsal_script frame_script = {
    .name = FRAME_SCRIPT,
    .init = frame_init,
    .destroy = frame_destroy,
    .receive = frame_receive,
    .size = sizeof(struct frame)
};

void frame_init(struct bsal_actor *actor)
{
    struct frame *concrete_actor;

    concrete_actor = (struct frame *)bsal_actor_concrete_actor(actor);
    concrete_actor->value = rand() % 12345;

    bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_PACK_ENABLE);
}

void frame_destroy(struct bsal_actor *actor)
{
    struct frame *concrete_actor;

    concrete_actor = (struct frame *)bsal_actor_concrete_actor(actor);
    concrete_actor->value = -1;
}

void frame_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct frame *concrete_actor;
    int other;
    struct bsal_vector initial_actors;
    struct bsal_vector *acquaintance_vector;
    int source;

    source = bsal_message_source(message);
    concrete_actor = (struct frame *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    name = bsal_actor_name(actor);
    buffer = bsal_message_buffer(message);
    acquaintance_vector = bsal_actor_acquaintance_vector(actor);

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_init(&initial_actors, sizeof(int));
        bsal_vector_unpack(&initial_actors, buffer);
        bsal_vector_push_back_vector(acquaintance_vector, &initial_actors);
        bsal_vector_destroy(&initial_actors);

        other = bsal_actor_spawn(actor, bsal_actor_script(actor));
        bsal_vector_push_back_int(acquaintance_vector, other);

        bsal_actor_send_empty(actor, other, BSAL_ACTOR_PING);

        printf("actor %d sends BSAL_ACTOR_PING to new actor %d\n",
                        name, other);

    } else if (tag == BSAL_ACTOR_PING) {

        /* new acquaintance
         */
        bsal_vector_push_back(acquaintance_vector, &source);

        printf("actor %d (value %d) receives BSAL_ACTOR_PING from actor %d\n",
                        name, concrete_actor->value, source);

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_PING_REPLY);

    } else if (tag == BSAL_ACTOR_PING_REPLY) {

        printf("actor %d receives BSAL_ACTOR_PING_REPLY from actor %d\n",
                        name, source);

        printf("actor %d asks actor %d to migrate using actor %d as spawner\n",
                        name, source, name);

        printf("Acquaintances of actor %d: ", name);
        bsal_vector_print_int(acquaintance_vector);
        printf("\n");

        bsal_actor_send_reply_int(actor, BSAL_ACTOR_MIGRATE, name);

    } else if (tag == BSAL_ACTOR_MIGRATE_REPLY) {

        bsal_message_unpack_int(message, 0, &other);
        printf("actor %d received migrated actor %d\n", name, other);
        printf("Acquaintances of actor %d: ", name);
        bsal_vector_print_int(acquaintance_vector);
        printf("\n");

        bsal_actor_send_empty(actor, other, BSAL_ACTOR_ASK_TO_STOP);
        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_ASK_TO_STOP);

    } else if (tag == BSAL_ACTOR_PACK) {

        bsal_actor_send_reply_int(actor, BSAL_ACTOR_PACK_REPLY, concrete_actor->value);

    } else if (tag == BSAL_ACTOR_UNPACK) {

        bsal_message_unpack_int(message, 0, &concrete_actor->value);
        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_UNPACK_REPLY);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("actor %d received BSAL_ACTOR_ASK_TO_STOP, value: %d ",
                        name, concrete_actor->value);
        printf("acquaintance vector: ");
        bsal_vector_print_int(acquaintance_vector);
        printf("\n");

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
