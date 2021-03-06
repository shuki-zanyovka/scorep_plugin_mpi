/**
* Copyright (C) Huawei Technologies Co., Ltd. 2020.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#include <scorep_plugin_mpi.h>
#include <iostream>
#include <sstream>
#include <utils.h>
#include <stdlib.h>
#include <stdio.h>


/* Global argc and argv*/
int global_argc = 0;
char** global_argv = NULL;

/*
   putting the constructor in the .init_array section.
   Functions in the .init_array (unlike .init) are called with the same arguments
   main will be called with: argc, argv and env.
*/
static int
printargs(int argc, char** argv, char** env) {
  DEBUG_PRINT("In printargs:\n");
  global_argc = argc;
  global_argv = argv;

  return 0;
}

/* Put the function into the init_array */
__attribute__((section(".init_array"))) static void *ctr = (void *)&printargs;

scorep_plugin_mpi::scorep_plugin_mpi()
{
    DEBUG_PRINT("Loading Metric Plugin: MPI Sampling\n");

    m_mpi_t_initialized = 0;
}


scorep_plugin_mpi::~scorep_plugin_mpi()
{
}


std::vector<MetricProperty>
scorep_plugin_mpi::get_metric_properties(const std::string& metric_name)
{
    const mpi_t_counters *pvars;
    unsigned long long int hex_dummy;
    int assigned_event = 0;
    std::vector<MetricProperty> metric_properties;
    uint32_t i;
    uint32_t pvars_max_index;

    DEBUG_PRINT("scorep_plugin_mpi::get_metric_properties() called with: %s\n", metric_name);

    auto [event, dummy] = parse_metric(metric_name, &hex_dummy);

    DEBUG_PRINT("Event=%s, dummy=%lu, hex_dummy=%lx\n", event, dummy, hex_dummy);

    /* MPI_T? */
    if (event == "MPI_T") {
        /* Enumerate PVARs */

        int ret;
        int is_initialized;

        /* Don't re-initialize MPI if already iniitalized */
        ret = PMPI_Initialized(&is_initialized);
        DEBUG_PRINT("MPI_Init(): Checking if initialized: %d\n", is_initialized);
        if (!is_initialized) {
           DEBUG_PRINT("get_metric_properties: global_argc=%d, global_argv=%p",
               global_argc, global_argv);
           PMPI_Init(&global_argc, &global_argv);
        }
        mpi_t_sampling_object.MPI_T_pvars_enumerate();

#if 0 /* Cannot enumerate since MPI_Init() has not executed yet by Score-P framework */
        //mpi_t_sampling_object.MPI_T_pvars_enumerate();
#else
        /* Get static initialization of counters list */
        mpi_t_sampling_object.pvars_enumeration_get(&pvars, &m_num_pvars);
#endif
        DEBUG_PRINT("MPI_T m_num_pvars = %zu\n", m_num_pvars);

        /* Allocate metrics last values */
        pvars_max_index = 0;
        for (i = 0; i < m_num_pvars; i++) {
            if (pvars[i].counter_index > pvars_max_index) {
                pvars_max_index = pvars[i].counter_index;
            }
        }
        /* Add 1 since the last index also requires an element */
        pvars_max_index = pvars_max_index + 1;

        for (int i = 0; i < m_num_pvars; i++) {
            metric_properties.push_back(
                MetricProperty(metric_name + "_" + pvars[i].counter_name, "", "").absolute_point().value_uint().decimal());
        }
    }

    /* Debug print */
    if (assigned_event) {
        DEBUG_PRINT("%s = %lu\n", event, (uintptr_t)hex_dummy);
    }

    return metric_properties;
}


int32_t
scorep_plugin_mpi::add_metric(const std::string& metric)
{
    const mpi_t_counters *pvars;
    size_t n_pvars;
    int32_t id = 0;
    unsigned long long int hex_dummy;

    auto [event, period] = parse_metric(metric, &hex_dummy);

    DEBUG_PRINT("add_metric() called with: %s = %lu\n", event, (uintptr_t)hex_dummy);

    /* MPI_T? */
    if (event == "MPI_T") {
        static int32_t allocated_index = 0;


        /* Get static initialization of counters list */
        mpi_t_sampling_object.pvars_enumeration_get(&pvars, &n_pvars);
        if (allocated_index < n_pvars) {
            id = pvars[allocated_index].counter_index;
            allocated_index++;
        }
    }

    return id;
}


void
scorep_plugin_mpi::start()
{
    DEBUG_PRINT("scorep_plugin_mpi::start()\n");
}


void
scorep_plugin_mpi::stop()
{
    DEBUG_PRINT("scorep_plugin_mpi::stop()\n");
}

SCOREP_METRIC_PLUGIN_CLASS(scorep_plugin_mpi, "scorep_plugin_mpi")
