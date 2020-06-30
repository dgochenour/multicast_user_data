/*
* (c) Copyright, Real-Time Innovations, 2020.  All rights reserved.
* RTI grants Licensee a license to use, modify, compile, and create derivative
* works of the software solely for use with RTI Connext DDS. Licensee may
* redistribute copies of the software provided that all such copies are subject
* to this license. The software is provided "as is", with no warranty of any
* type, including any warranty for fitness for any purpose. RTI is under no
* obligation to maintain or support the software. RTI shall not be liable for
* any incidental or consequential damages arising out of the use or inability
* to use the software.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rti_me_c.h"
#include "wh_sm/wh_sm_history.h"
#include "rh_sm/rh_sm_history.h"

#include "example.h"
#include "exampleSupport.h"
#include "exampleApplication.h"

void on_publication_matched(
        void *listener_data,
        DDS_DataWriter * writer,
        const struct DDS_PublicationMatchedStatus *status) {

    DDS_Topic* the_topic;
    char the_topic_name[64];
    
    the_topic = DDS_DataWriter_get_topic(writer);
    strcpy(the_topic_name, DDS_TopicDescription_get_name(DDS_Topic_as_topicdescription(the_topic)));	
    if (status->current_count_change > 0) {
        printf("Matched a DataReader on Topic: '%s'\n", the_topic_name);
    } else if (status->current_count_change < 0) {
        printf("Unmatched a DataReader on Topic: '%s'\n", the_topic_name);
    }
}

int
publisher_main_w_args(DDS_Long domain_id, char *udp_intf, char *peer,
DDS_Long sleep_time, DDS_Long count)
{
    DDS_Publisher *publisher;
    DDS_DataWriter *datawriter;
    my_typeDataWriter *hw_datawriter;
    struct DDS_DataWriterQos dw_qos = DDS_DataWriterQos_INITIALIZER;
    DDS_ReturnCode_t retcode;
    my_type *sample = NULL;
    struct Application *application = NULL;
    DDS_Long i;
    struct DDS_DataWriterListener dw_listener =
    DDS_DataWriterListener_INITIALIZER;

    sample = my_type_create();
    if (sample == NULL) {
        printf("failed my_type_create\n");
        return 0;
    }

    application = Application_create(
            "publisher", 
            "subscriber", 
            domain_id,
            udp_intf, 
            peer, 
            sleep_time, 
            count);
    if (application == NULL) {
        printf("failed Application create\n");
        goto done;
    }

    publisher = DDS_DomainParticipant_create_publisher(
            application->participant,
            &DDS_PUBLISHER_QOS_DEFAULT,
            NULL,
            DDS_STATUS_MASK_NONE);
    if (publisher == NULL) {
        printf("publisher == NULL\n");
        goto done;
    }

    #ifdef USE_RELIABLE_QOS
    dw_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    #else
    dw_qos.reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;
    #endif
    dw_qos.resource_limits.max_samples_per_instance = 32;
    dw_qos.resource_limits.max_instances = 2;
    dw_qos.resource_limits.max_samples = dw_qos.resource_limits.max_instances *
    dw_qos.resource_limits.max_samples_per_instance;
    dw_qos.history.depth = 32;
    dw_qos.protocol.rtps_reliable_writer.heartbeat_period.sec = 0;
    dw_qos.protocol.rtps_reliable_writer.heartbeat_period.nanosec = 250000000;

    /* INTEROPERABILITY NOTE:
    Non-Connext Micro DDS writers and readers will have default LivelinessQoS
    kind set to DDS_AUTOMATIC_LIVELINESS_QOS.

    Because Connext Micro currently only supports
    DDS_MANUAL_BY_TOPIC_LIVELINESS_QOS for finite lease_duration,
    a non-Connext Micro writer must set its liveliness kind to
    DDS_MANUAL_BY_TOPIC_LIVELINESS_QOS as well in order for
    Requested-Offered (RxO) semantics to be compatible.
    */

    dw_listener.on_publication_matched = on_publication_matched;
    datawriter = DDS_Publisher_create_datawriter(
            publisher, 
            application->topic, 
            &dw_qos,
            &dw_listener,
            DDS_PUBLICATION_MATCHED_STATUS);
    if (datawriter == NULL) {
        printf("datawriter == NULL\n");
        goto done;
    }

    retcode = Application_enable(application);
    if (retcode != DDS_RETCODE_OK) {
        printf("failed to enable application\n");
        goto done;
    }

    hw_datawriter = my_typeDataWriter_narrow(datawriter);

    for (i = 0; (application->count > 0 && i < application->count) ||
    (application->count == 0); ++i) {

        /* TODO set sample attributes here */

        retcode = my_typeDataWriter_write(hw_datawriter, sample, &DDS_HANDLE_NIL);
        if (retcode != DDS_RETCODE_OK) {
            printf("Failed to write sample\n");
        } else {
            printf("Written sample %d\n",(i+1));
        } 

        OSAPI_Thread_sleep(application->sleep_time);
    }

    done:
    Application_delete(application);
    DDS_DataWriterQos_finalize(&dw_qos);
    if (sample != NULL) {
        my_type_delete(sample);
    }
    return 0;
}


int main(int argc, char **argv) {

    DDS_Long i = 0;
    DDS_Long domain_id = 0;
    char *peer = NULL;
    char *udp_intf = NULL;
    DDS_Long sleep_time = 1000;
    DDS_Long count = 0;

    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-domain"))
        {
            ++i;
            if (i == argc)
            {
                printf("-domain <domain_id>\n");
                return -1;
            }
            domain_id = strtol(argv[i], NULL, 0);
        }
        else if (!strcmp(argv[i], "-udp_intf"))
        {
            ++i;
            if (i == argc)
            {
                printf("-udp_intf <interface>\n");
                return -1;
            }
            udp_intf = argv[i];
        }
        else if (!strcmp(argv[i], "-peer"))
        {
            ++i;
            if (i == argc)
            {
                printf("-peer <address>\n");
                return -1;
            }
            peer = argv[i];
        }
        else if (!strcmp(argv[i], "-sleep"))
        {
            ++i;
            if (i == argc)
            {
                printf("-sleep_time <sleep_time>\n");
                return -1;
            }
            sleep_time = strtol(argv[i], NULL, 0);
        }
        else if (!strcmp(argv[i], "-count"))
        {
            ++i;
            if (i == argc)
            {
                printf("-count <count>\n");
                return -1;
            }
            count = strtol(argv[i], NULL, 0);
        }
        else if (!strcmp(argv[i], "-h"))
        {
            Application_help(argv[0]);
            return 0;
        }
        else
        {
            printf("unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    return publisher_main_w_args(domain_id, udp_intf, peer, sleep_time, count);
}