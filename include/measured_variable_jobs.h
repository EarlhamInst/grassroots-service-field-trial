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
 * phenotype_jobs.h
 *
 *  Created on: 26 Oct 2018
 *      Author: billy
 */

#ifndef SERVICES_DFW_FIELD_TRIAL_SERVICE_INCLUDE_TREATMENT_JOBS_H_
#define SERVICES_DFW_FIELD_TRIAL_SERVICE_INCLUDE_TREATMENT_JOBS_H_


#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"

#include "measured_variable.h"



#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_MEASURED_VARIABLE_CONSTANTS
	#define MEASURED_VARIABLE_JOB_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define MEASURED_VARIABLE_JOB_STRUCT_VAL(x,y)	= { x, y}
#else
	#define MEASURED_VARIABLE_JOB_PREFIX extern
	#define MEASURED_VARIABLE_JOB_STRUCT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */



/*
 * Study parameters
 */
MEASURED_VARIABLE_JOB_PREFIX NamedParameterType MV_TRAIT_ID MEASURED_VARIABLE_JOB_STRUCT_VAL("TR Trait Id", PT_STRING);




#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddSubmissionMeasuredVariableParams (ServiceData *data_p, ParameterSet *param_set_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool RunForSubmittedSpreadsheet (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetSubmissionMeasuredVariableParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL MeasuredVariable *GetMeasuredVariableByVariableName (const char *name_s, MEM_FLAG *mv_mem_p, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetAllMeasuredVariablesAsJSON (const FieldTrialServiceData *data_p, bson_t *opts_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL char *GetMeasuredVariableAsString (const MeasuredVariable *treatment_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddMeasuredVariableToServiceJob (ServiceJob *job_p, MeasuredVariable *treatment_p, const ViewFormat format, FieldTrialServiceData *data_p);


/**
 * @return 1 if the MeasuredVariable already exists, 0 if it doesn't and -1 if the same combination
 * of trait, measurement and unit are in use for another variable
 */
DFW_FIELD_TRIAL_SERVICE_LOCAL int CheckMeasuredVariable (MeasuredVariable *treatment_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetMeasuredVariableIndexingData (Service *service_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetAllMeasuredVariablesAsJSON (const FieldTrialServiceData *data_p, bson_t *opts_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL char *GetMeasuredVariablesNameKey (void);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeMeasuredVariablesNameKey (char *key_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus RunForCropOntologyAPIImport (ParameterSet *param_set_p, ServiceJob *job_p, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetAllMeasuredVariableIds (Service *service_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus GetAllStudiesContainingMeasuredVariable (const MeasuredVariable * const mv_p, ServiceJob *job_p, const ViewFormat format, FieldTrialServiceData *data_p);


#ifdef __cplusplus
}
#endif



#endif /* SERVICES_DFW_FIELD_TRIAL_SERVICE_INCLUDE_TREATMENT_JOBS_H_ */
