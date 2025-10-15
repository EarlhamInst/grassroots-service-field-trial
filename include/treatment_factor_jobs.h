/*
** Copyright 2014-2020 The Earlham Institute
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
 * treatment_factor_jobs.h
 *
 *  Created on: 7 Dec 2020
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_TREATMENT_FACTOR_JOBS_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_TREATMENT_FACTOR_JOBS_H_


#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"

#include "treatment_factor.h"

#include "string_parameter.h"


#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_TREATMENT_FACTOR_JOB_CONSTANTS
	#define TREATMENT_FACTOR_JOB_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define TREATMENT_FACTOR_JOB_STRUCT_VAL(x,y)	= { x, y}
	#define TREATMENT_FACTOR_JOB_VAL(x)	= { x }
#else
	#define TREATMENT_FACTOR_JOB_PREFIX extern
	#define TREATMENT_FACTOR_JOB_STRUCT_VAL(x,y)
	#define TREATMENT_FACTOR_JOB_VAL(x)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */


TREATMENT_FACTOR_JOB_PREFIX NamedParameterType TFJ_STUDY_ID TREATMENT_FACTOR_JOB_STRUCT_VAL ("TF Study ID", PT_STRING);
TREATMENT_FACTOR_JOB_PREFIX NamedParameterType TFJ_TREATMENT_ID TREATMENT_FACTOR_JOB_STRUCT_VAL ("TF Treatment ID", PT_STRING);
TREATMENT_FACTOR_JOB_PREFIX NamedParameterType TFJ_TREATMENT_NAME TREATMENT_FACTOR_JOB_STRUCT_VAL ("TF Name", PT_STRING);
TREATMENT_FACTOR_JOB_PREFIX NamedParameterType TFJ_VALUES TREATMENT_FACTOR_JOB_STRUCT_VAL ("TF Levels", PT_JSON_TABLE);



TREATMENT_FACTOR_JOB_PREFIX const char * const TFJ_LABEL_TITLE_S TREATMENT_FACTOR_JOB_VAL ("Label");
TREATMENT_FACTOR_JOB_PREFIX const char * const TFJ_VALUE_TITLE_S TREATMENT_FACTOR_JOB_VAL ("Value");


/* forward declarations */
struct Study;


#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddSubmissionTreatmentFactorParams (ServiceData *data_p, ParameterSet *param_set_p, DataResource *resource_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetUpTreatmentFactorsListParameter (const FieldTrialServiceData *data_p, Parameter *param_p, const struct Study *active_study_p, const TreatmentFactor *active_tf_p, const bool empty_option_flag);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetSubmissionTreatmentFactorParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool RunForSubmissionTreatmentFactorParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Parameter *GetTreatmentFactorTableParameter (ParameterSet *param_set_p, ParameterGroup *group_p, json_t *tf_json_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool IsTreatmentFactorParameter (const char * const param_name_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddTreatmentFactorByPartsToStudy (const char *treatment_url_s, const json_t *factors_json_p, struct Study *study_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetTreatmentFactorAsFrictionlessData (const TreatmentFactor *treatment_factor_p);


#ifdef __cplusplus
}
#endif



#endif /* SERVICES_FIELD_TRIALS_INCLUDE_TREATMENT_FACTOR_JOBS_H_ */
