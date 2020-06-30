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
#include "examplePlugin.h"
#include "exampleApplication.h"


void my_on_data_available(void *listener_data, DDS_DataReader * reader) {

    my_typeDataReader *hw_reader = my_typeDataReader_narrow(reader);
    DDS_ReturnCode_t retcode;
    struct DDS_SampleInfo *sample_info = NULL;
    my_type *sample = NULL;

    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    struct my_typeSeq sample_seq = DDS_SEQUENCE_INITIALIZER;
    const DDS_Long TAKE_MAX_SAMPLES = 32;
    DDS_Long i;

    retcode = my_typeDataReader_take(
            hw_reader, 
            &sample_seq, 
            &info_seq, 
            TAKE_MAX_SAMPLES, 
            DDS_ANY_SAMPLE_STATE, 
            DDS_ANY_VIEW_STATE, 
            DDS_ANY_INSTANCE_STATE);
    if (retcode != DDS_RETCODE_OK) {
        printf("failed to take data, retcode(%d)\n", retcode);
        goto done;
    }

    /* Print each valid sample taken */
    for (i = 0; i < my_typeSeq_get_length(&sample_seq); ++i) {
        sample_info = DDS_SampleInfoSeq_get_reference(&info_seq, i);

        if (sample_info->valid_data) {
            sample = my_typeSeq_get_reference(&sample_seq, i);
            printf("\nValid sample received\n");
            /* TODO read sample attributes here */ 
        } else {
            printf("\nSample received\n\tINVALID DATA\n");
        }
    }

    my_typeDataReader_return_loan(hw_reader, &sample_seq, &info_seq);

    done:
    my_typeSeq_finalize(&sample_seq);
    DDS_SampleInfoSeq_finalize(&info_seq);
}

void my_on_subscription_matched(
        void *listener_data,
        DDS_DataReader * reader,
        const struct DDS_SubscriptionMatchedStatus *status) {

    DDS_TopicDescription* the_topic;
    char the_topic_name[64];
    
    the_topic = DDS_DataReader_get_topicdescription(reader);
    strcpy(the_topic_name, DDS_TopicDescription_get_name(the_topic));	
    if (status->current_count_change > 0) {
        printf("Matched a DataWriter on Topic: '%s'\n", the_topic_name);
    } else if (status->current_count_change < 0) {
        printf("Unmatched a DataWriter on Topic: '%s'\n", the_topic_name);
    }
}

int subscriber_main_w_args(
        DDS_Long domain_id, 
        char *udp_intf, 
        char *peer,
        DDS_Long sleep_time, 
        DDS_Long count) {

    DDS_Subscriber *subscriber;
    DDS_DataReader *datareader;
    struct DDS_DataReaderQos dr_qos = DDS_DataReaderQos_INITIALIZER;
    DDS_ReturnCode_t retcode;
    struct DDS_DataReaderListener dr_listener = 
            DDS_DataReaderListener_INITIALIZER;
    struct Application *application;

    application = Application_create(
            "subscriber", 
            "publisher", 
            domain_id,
            udp_intf, 
            peer, 
            sleep_time, 
            count);
    if (application == NULL) {
        return 0;
    }

    subscriber = DDS_DomainParticipant_create_subscriber(
            application->participant,
            &DDS_SUBSCRIBER_QOS_DEFAULT,
            NULL, 
            DDS_STATUS_MASK_NONE);
    if (subscriber == NULL) {
        printf("subscriber == NULL\n");
        goto done;
    }

    dr_listener.on_data_available = my_on_data_available;
    dr_listener.on_subscription_matched = my_on_subscription_matched;

    /*
     * LAB: Enable multicast communication for user data
     * 
     * In the DataReader's QoS, the transport.enabled_transports parameter is a 
     * sequence that overrides the default transports configured in the 
     * DomainParticipant. When we configure this sequence to a length of 1 and 
     * assign a multicast address to that location, the DataReader will 
     * use that address as its locator. 
     */ 
    if (!REDA_StringSeq_set_maximum(&dr_qos.transport.enabled_transports,1)) {
        printf("failed to set enabled_transports maximum\n");
        goto done;
    }
    if (!REDA_StringSeq_set_length(&dr_qos.transport.enabled_transports,1)) {
        printf("failed to set enabled_transports length\n");
        goto done;
    }
    *REDA_StringSeq_get_reference(&dr_qos.transport.enabled_transports,0) = 
            DDS_String_dup("239.255.10.10");

    dr_qos.resource_limits.max_instances = 1;
    dr_qos.resource_limits.max_samples_per_instance = 32;
    dr_qos.resource_limits.max_samples = dr_qos.resource_limits.max_instances *
            dr_qos.resource_limits.max_samples_per_instance;
    /* if there are more remote writers, you need to increase these limits */
    dr_qos.reader_resource_limits.max_remote_writers = 10;
    dr_qos.reader_resource_limits.max_remote_writers_per_instance = 10;
    dr_qos.history.depth = 32;
    #ifdef USE_RELIABLE_QOS
    dr_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    #else
    dr_qos.reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;
    #endif

    datareader = DDS_Subscriber_create_datareader(
            subscriber,
            DDS_Topic_as_topicdescription(application->topic), 
            &dr_qos,
            &dr_listener,
            DDS_DATA_AVAILABLE_STATUS | DDS_SUBSCRIPTION_MATCHED_STATUS);
    if (datareader == NULL) {
        printf("datareader == NULL\n");
        goto done;
    }

    retcode = Application_enable(application);
    if (retcode != DDS_RETCODE_OK) {
        printf("failed to enable application\n");
        goto done;
    }

    if (application->count != 0) {
        printf("Running for %d seconds, press Ctrl-C to exit\n",
        application->count);
        OSAPI_Thread_sleep(application->count * 1000);
    } else {
        int sleep_loop_count =  (24 * 60 * 60) / 2000;
        int sleep_loop_left = (24 * 60 * 60) % 2000;

        printf("Running for 24 hours, press Ctrl-C to exit\n");

        while (sleep_loop_count) {
            OSAPI_Thread_sleep(2000  * 1000); /* sleep is in ms */
            --sleep_loop_count;
        }
        OSAPI_Thread_sleep(sleep_loop_left * 1000);
    }

    done:
    Application_delete(application);
    DDS_DataReaderQos_finalize(&dr_qos);
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

    return subscriber_main_w_args(domain_id, udp_intf, peer, sleep_time, count);
}