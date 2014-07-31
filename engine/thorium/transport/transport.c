
#include "transport.h"

#include "active_request.h"

#include <engine/thorium/message.h>
#include <engine/thorium/node.h>

#include <core/system/debugger.h>

void bsal_transport_init(struct bsal_transport *transport, struct bsal_node *node,
                int *argc, char ***argv)
{
        /*
    printf("DEBUG Initiating transport\n");
    */
    /* Select the transport layer
     */
    bsal_transport_select(transport);

    /*
     * Assign functions
     */
    bsal_transport_set(transport);

    transport->node = node;
    bsal_ring_queue_init(&transport->active_requests, sizeof(struct bsal_active_request));

    transport->rank = -1;
    transport->size = -1;

    transport->transport_init(transport, argc, argv);

    BSAL_DEBUGGER_ASSERT(transport->rank >= 0);
    BSAL_DEBUGGER_ASSERT(transport->size >= 1);
    BSAL_DEBUGGER_ASSERT(transport->node != NULL);

    printf("%s TRANSPORT Rank: %d RankCount: %d Implementation: %s\n",
                    BSAL_NODE_THORIUM_PREFIX,
                transport->rank, transport->size,
                bsal_transport_get_name(transport));
}

void bsal_transport_destroy(struct bsal_transport *transport)
{
    struct bsal_active_request active_request;

    transport->transport_destroy(transport);

    while (bsal_ring_queue_dequeue(&transport->active_requests, &active_request)) {
        bsal_active_request_destroy(&active_request);
    }

    bsal_ring_queue_destroy(&transport->active_requests);

    transport->node = NULL;
    transport->rank = -1;
    transport->size = -1;
    transport->implementation = BSAL_TRANSPORT_IMPLEMENTATION_MOCK;

    bsal_transport_set(transport);
}

int bsal_transport_send(struct bsal_transport *transport, struct bsal_message *message)
{
    return transport->transport_send(transport, message);
}

int bsal_transport_receive(struct bsal_transport *transport, struct bsal_message *message)
{
    return transport->transport_receive(transport, message);
}

void bsal_transport_resolve(struct bsal_transport *transport, struct bsal_message *message)
{
    int actor;
    int node_name;
    struct bsal_node *node;

    node = transport->node;

    actor = bsal_message_source(message);
    node_name = bsal_node_actor_node(node, actor);
    bsal_message_set_source_node(message, node_name);

    actor = bsal_message_destination(message);
    node_name = bsal_node_actor_node(node, actor);
    bsal_message_set_destination_node(message, node_name);
}

int bsal_transport_get_provided(struct bsal_transport *transport)
{
    return transport->provided;
}

int bsal_transport_get_rank(struct bsal_transport *transport)
{
    return transport->rank;
}

int bsal_transport_get_size(struct bsal_transport *transport)
{
    return transport->size;
}

int bsal_transport_test_requests(struct bsal_transport *transport, struct bsal_active_request *active_request)
{
    if (bsal_ring_queue_dequeue(&transport->active_requests, active_request)) {

        if (bsal_active_request_test(active_request)) {

            return 1;

        /* Just put it back in the FIFO for later */
        } else {
            bsal_ring_queue_enqueue(&transport->active_requests, active_request);

            return 0;
        }
    }

    return 0;
}

int bsal_transport_dequeue_active_request(struct bsal_transport *transport, struct bsal_active_request *active_request)
{
    return bsal_ring_queue_dequeue(&transport->active_requests, active_request);
}

int bsal_transport_get_implementation(struct bsal_transport *transport)
{
    return transport->implementation;
}

void *bsal_transport_get_concrete_transport(struct bsal_transport *transport)
{
    return transport->concrete_transport;
}

void bsal_transport_set(struct bsal_transport *transport)
{
    if (transport->implementation == BSAL_TRANSPORT_PAMI_IDENTIFIER) {
        bsal_transport_configure_pami(transport);

    } else if (transport->implementation == BSAL_TRANSPORT_MPI_IDENTIFIER) {

        bsal_transport_configure_mpi(transport);

    } else if (transport->implementation == BSAL_TRANSPORT_IMPLEMENTATION_MOCK) {

        bsal_transport_configure_mock(transport);

    } else {
        bsal_transport_configure_mock(transport);
    }
}

void bsal_transport_configure_pami(struct bsal_transport *transport)
{
    transport->concrete_transport = &transport->pami_transport;

    transport->transport_init = bsal_pami_transport_init;
    transport->transport_destroy = bsal_pami_transport_destroy;
    transport->transport_send = bsal_pami_transport_send;
    transport->transport_receive = bsal_pami_transport_receive;
    transport->transport_get_identifier = bsal_pami_transport_get_identifier;
    transport->transport_get_name = bsal_pami_transport_get_name;
}

void bsal_transport_configure_mpi(struct bsal_transport *transport)
{
    transport->concrete_transport = &transport->mpi_transport;

    transport->transport_init = bsal_mpi_transport_init;
    transport->transport_destroy = bsal_mpi_transport_destroy;
    transport->transport_send = bsal_mpi_transport_send;
    transport->transport_receive = bsal_mpi_transport_receive;
    transport->transport_get_identifier = bsal_mpi_transport_get_identifier;
    transport->transport_get_name = bsal_mpi_transport_get_name;
}

void bsal_transport_configure_mock(struct bsal_transport *transport)
{
    transport->concrete_transport = NULL;

    transport->transport_init = NULL;
    transport->transport_destroy = NULL;
    transport->transport_send = NULL;
    transport->transport_receive = NULL;
    transport->transport_get_identifier = NULL;
    transport->transport_get_name = NULL;
}

void bsal_transport_prepare_received_message(struct bsal_transport *transport, struct bsal_message *message,
                int source, int tag, int count, void *buffer)
{
    int metadata_size;
    int destination;

    destination = transport->rank;
    metadata_size = bsal_message_metadata_size(message);
    count -= metadata_size;

    /* Initially assign the MPI source rank and MPI destination
     * rank for the actor source and actor destination, respectively.
     * Then, read the metadata and resolve the MPI rank from
     * that. The resolved MPI ranks should be the same in all cases
     */
    bsal_message_init(message, tag, count, buffer);
    bsal_message_set_source(message, source);
    bsal_message_set_destination(message, destination);
    bsal_message_read_metadata(message);
    bsal_transport_resolve(transport, message);
}

int bsal_transport_get_active_request_count(struct bsal_transport *transport)
{
    return bsal_ring_queue_size(&transport->active_requests);
}

int bsal_transport_get_identifier(struct bsal_transport *transport)
{
    return transport->transport_get_identifier(transport);
}

const char *bsal_transport_get_name(struct bsal_transport *transport)
{
    return transport->transport_get_name(transport);
}

void bsal_transport_select(struct bsal_transport *transport)
{
    transport->implementation = BSAL_TRANSPORT_MOCK_IDENTIFIER;

#if defined(BSAL_TRANSPORT_USE_PAMI)
    transport->implementation = BSAL_TRANSPORT_PAMI_IDENTIFIER;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    transport->implementation = BSAL_TRANSPORT_MPI_IDENTIFIER;
#endif

    if (transport->implementation == BSAL_TRANSPORT_MOCK_IDENTIFIER) {
        printf("Error: no transport implementation is available.\n");
        exit(1);
    }

    /*
    printf("DEBUG Transport is %d\n",
                    transport->implementation);
                    */

}