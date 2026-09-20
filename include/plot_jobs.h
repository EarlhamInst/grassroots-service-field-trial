/*
** Copyright 2014-2018 The Earlham Institute
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
/*
 * plot_jobs.h
 *
 *  Created on: 1 Oct 2018
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_PLOT_JOBS_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_PLOT_JOBS_H_


#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"

#include "study.h"
#include "plot.h"



#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_PLOT_JOB_CONSTANTS
	#define PLOT_JOB_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define PLOT_JOB_VAL(x)	= x
	#define PLOT_VARIABLE_JOB_STRUCT_VAL(x,y)	= { x, y }
#else
	#define PLOT_JOB_PREFIX extern
	#define PLOT_JOB_VAL(x)
	#define PLOT_VARIABLE_JOB_STRUCT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */

/*
 * Study parameters
 */
PLOT_JOB_PREFIX const char *PL_REPLICATE_TITLE_S PLOT_JOB_VAL("Replicate");

PLOT_JOB_PREFIX const char *PL_INDEX_TABLE_TITLE_S PLOT_JOB_VAL("Plot ID");

PLOT_JOB_PREFIX const char *PL_ACCESSION_TABLE_TITLE_S PLOT_JOB_VAL ("Accession");


PLOT_JOB_PREFIX const char *PL_PLOT_CATEGORY_TABLE_TITLE_S PLOT_JOB_VAL ("Plot Category");


PLOT_JOB_PREFIX const char *PL_STORE_CODE_TABLE_TITLE_S PLOT_JOB_VAL ("Store Code");


PLOT_JOB_PREFIX const char *PL_ROW_TITLE_S PLOT_JOB_VAL("Row");

PLOT_JOB_PREFIX const char *PL_COLUMN_TITLE_S PLOT_JOB_VAL ("Column");



PLOT_JOB_PREFIX const char * const PL_RACK_TITLE_S PLOT_JOB_VAL ("Rack");

PLOT_JOB_PREFIX const char * const PL_GENE_BANK_S PLOT_JOB_VAL ("Gene Bank");


PLOT_JOB_PREFIX NamedParameterType PL_PLOT_TABLE PLOT_VARIABLE_JOB_STRUCT_VAL ("PL Upload", PT_JSON_TABLE);


PLOT_JOB_PREFIX NamedParameterType PL_ID PLOT_VARIABLE_JOB_STRUCT_VAL("Plot ID", PT_STRING);


#ifdef __cplusplus
extern "C"
{
#endif



DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddSubmissionPlotParams (ServiceData *data_p, ParameterSet *param_set_p, DataResource *resource_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddSearchPlotParams (ServiceData *data_p, ParameterSet *param_set_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool RunForSubmissionPlotParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetSubmissionPlotParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetSearchPlotParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Plot *GetPlotByRowColumnRack (const uint32 row, const uint32 column, const uint32 rack, Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Plot *GetPlotById (bson_oid_t *id_p, Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p);


/**
 * Get the plots as a Frictionless Data Table
 *
 * @param study_p The Study to get the plots for.
 * @param data_p The Field Trial Service Config
 * @return The JSON for the plots or <code>NULL</code> upon error.
 */
DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetPlotsAsFDTabularPackage (const Study *study_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetStudyPlotHeaderAsFrictionlessData (const Study *study_p, const FieldTrialServiceData *service_data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetPlotsFrictionlessDataTableSchema (const Study *study_p, const FieldTrialServiceData *service_data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetPlotsCSVDialect (const char *null_sequence_s);

DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetPlotAsFrictionlessData (const Plot *plot_p, const Study * const study_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddPlotAsFrictionlessData (const Plot *plot_p, json_t *plots_array_p, const Study * const study_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool RunForSearchPlotParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool RunForSearchPlotParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus GenerateAndAddSkeletonPlotsToStudy (Study *study_p, const uint32 num_rows, const uint32 num_cols, ServiceJob *job_p, FieldTrialServiceData *data_p);


#ifdef __cplusplus
}
#endif



#endif /* SERVICES_FIELD_TRIALS_INCLUDE_PLOT_JOBS_H_ */
