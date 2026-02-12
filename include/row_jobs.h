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
 * row_jobs.h
 *
 *  Created on: 28 Oct 2018
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_ROW_JOBS_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_ROW_JOBS_H_

#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"

#include "standard_row.h"

#include "plot.h"
#include "treatment_factor_value.h"
#include "observation_metadata.h"




#ifdef __cplusplus
extern "C"
{
#endif



DFW_FIELD_TRIAL_SERVICE_LOCAL Row *GetRowByRackIndex (const int32 row, Plot *plot_p, const bool expand_fields_flag, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Row *GetRowByStudyIndex (const int32 by_study_index, Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL LinkedList *GetAllRowsContainingMaterial (Material *material_p, const FieldTrialServiceData *data_p);



DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus AddObservationValueToStandardRow (StandardRow *row_p, const uint32 row_index, const char *key_s, const json_t *value_p, ServiceJob *job_p, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus AddTreatmentFactorValuesToStandardRow (StandardRow *row_p, json_t *plot_json_p, Study *study_p, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus AddStatsValuesToBaseRow (Row *row_p, json_t *stas_json_p, Study *study_p, ServiceJob *job_p, const uint32 row_index, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddTreatmentFactorValueToRowByParts (StandardRow *row_p, TreatmentFactor *tf_p, const char *value_s);

DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetRowAsFrictionlessData (const Row *row_p, const Study * const study_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddRowFrictionlessDataDetails (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL Observation *GetMatchingObservation (const StandardRow *row_p, const MeasuredVariable *variable_p, const ObservationMetadata *metadata_p, const bool include_metadata_flag);


DFW_FIELD_TRIAL_SERVICE_LOCAL ObservationNode *GetMatchingObservationNode (const StandardRow *row_p, const MeasuredVariable *variable_p, const ObservationMetadata *metadata_p, const bool include_metadata_flag);


DFW_FIELD_TRIAL_SERVICE_LOCAL void RemoveObservationNode (const StandardRow *row_p, ObservationNode *node_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus AddSingleTreatmentFactorValueToStandardRow  (StandardRow *row_p, const char *key_s, const char *value_s, Study *study_p, ServiceJob *job_p, const uint32 row_index, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetDiscardValueFromSubmissionJSON (const json_t *row_json_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetBlankValueFromSubmissionJSON (const json_t *row_json_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Row *GetRowByIdString (const char *row_id_s, const ViewFormat format, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL char *GetRowsNameKey (void);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeRowsNameKey (char *key_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus AddObservationValueToStandardRowByParts (ServiceJob *job_p, StandardRow *row_p, MeasuredVariable *measured_variable_p, ObservationMetadata *obs_metadata_p,
											const char *key_s, const json_t *raw_value_p, const json_t *corrected_value_p, const char *notes_s, bool *free_measured_variable_flag_p,
											void (*on_error_callback_fn) (ServiceJob *job_p, const char * const observation_field_s, const void *value_p, void *user_data_p), void *user_data_p);

#ifdef __cplusplus
}
#endif



#endif /* SERVICES_FIELD_TRIALS_INCLUDE_ROW_JOBS_H_ */
