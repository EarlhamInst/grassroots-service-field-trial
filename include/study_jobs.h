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
 * experimental_area_jobs.h
 *
 *  Created on: 1 Oct 2018
 *      Author: billy
 */

#ifndef SERVICES_DFW_FIELD_TRIAL_SERVICE_INCLUDE_EXPERIMENTAL_AREA_JOBS_H_
#define SERVICES_DFW_FIELD_TRIAL_SERVICE_INCLUDE_EXPERIMENTAL_AREA_JOBS_H_


#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"

#include "study.h"
#include "string_parameter.h"

#include "treatment_factor.h"


#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_STUDY_JOB_CONSTANTS
	#define STUDY_JOB_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define STUDY_JOB_STRUCT_VAL(x,y)	= { x, y}
#else
	#define STUDY_JOB_PREFIX extern
	#define STUDY_JOB_STRUCT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */

/*
 * Study parameters
 */
STUDY_JOB_PREFIX NamedParameterType STUDY_ID STUDY_JOB_STRUCT_VAL("ST Id", PT_STRING);

STUDY_JOB_PREFIX NamedParameterType STUDY_NAME STUDY_JOB_STRUCT_VAL("ST Name", PT_STRING);

STUDY_JOB_PREFIX NamedParameterType STUDY_LINK STUDY_JOB_STRUCT_VAL("ST Link", PT_STRING);

STUDY_JOB_PREFIX NamedParameterType STUDY_ASPECT STUDY_JOB_STRUCT_VAL("ST Field Aspect", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_SLOPE STUDY_JOB_STRUCT_VAL("ST Slope", PT_STRING);

//STUDY_JOB_PREFIX NamedParameterType STUDY_GET_ALL_PLOTS STUDY_JOB_STRUCT_VAL("Get all Plots for Study", PT_BOOLEAN);

STUDY_JOB_PREFIX NamedParameterType STUDY_DETAIL_LEVEL STUDY_JOB_STRUCT_VAL("The level of data to get for matching Studies", PT_STRING);

STUDY_JOB_PREFIX NamedParameterType STUDY_ADD_STUDY STUDY_JOB_STRUCT_VAL("Add Study", PT_BOOLEAN);

STUDY_JOB_PREFIX NamedParameterType STUDY_THIS_CROP STUDY_JOB_STRUCT_VAL("This Crop", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_PREVIOUS_CROP STUDY_JOB_STRUCT_VAL("Previous Crop", PT_STRING);


STUDY_JOB_PREFIX NamedParameterType STUDY_FIELD_TRIALS_LIST STUDY_JOB_STRUCT_VAL("Field Trials", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_LOCATIONS_LIST STUDY_JOB_STRUCT_VAL("Locations", PT_STRING);

STUDY_JOB_PREFIX NamedParameterType STUDY_DESCRIPTION STUDY_JOB_STRUCT_VAL("ST Description", PT_LARGE_STRING);


STUDY_JOB_PREFIX NamedParameterType STUDY_DESIGN STUDY_JOB_STRUCT_VAL("ST Design", PT_LARGE_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_GROWING_CONDITIONS STUDY_JOB_STRUCT_VAL("ST Growing Conditions", PT_LARGE_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_PHENOTYPE_GATHERING_NOTES STUDY_JOB_STRUCT_VAL("ST Phenotype Gathering Notes", PT_LARGE_STRING);


STUDY_JOB_PREFIX NamedParameterType STUDY_PLAN_CHANGES STUDY_JOB_STRUCT_VAL("ST Plan Changes", PT_LARGE_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_DATA_NOT_INCLUDED STUDY_JOB_STRUCT_VAL("ST Data not included", PT_LARGE_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_PHYSICAL_SAMPLES_COLLECTED STUDY_JOB_STRUCT_VAL("ST Samples collected", PT_LARGE_STRING);


STUDY_JOB_PREFIX NamedParameterType STUDY_CURATOR_NAME STUDY_JOB_STRUCT_VAL("ST Curator name", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_CURATOR_EMAIL STUDY_JOB_STRUCT_VAL("ST Curator email", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_CURATOR_ROLE STUDY_JOB_STRUCT_VAL("ST Curator role", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_CURATOR_AFFILATION STUDY_JOB_STRUCT_VAL("ST Curator affiliation", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_CURATOR_ORCID STUDY_JOB_STRUCT_VAL("ST Curator orcid", PT_STRING);

STUDY_JOB_PREFIX NamedParameterType STUDY_CONTACT_NAME STUDY_JOB_STRUCT_VAL("ST Contact name", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_CONTACT_EMAIL STUDY_JOB_STRUCT_VAL("ST Contact email", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_CONTACT_ROLE STUDY_JOB_STRUCT_VAL("ST Contact role", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_CONTACT_AFFILATION STUDY_JOB_STRUCT_VAL("ST Contact affiliation", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_CONTACT_ORCID STUDY_JOB_STRUCT_VAL("ST Contact orcid", PT_STRING);

STUDY_JOB_PREFIX NamedParameterType STUDY_SEARCH_STUDIES STUDY_JOB_STRUCT_VAL("ST Search Studies", PT_BOOLEAN);
STUDY_JOB_PREFIX NamedParameterType STUDY_SOWING_YEAR STUDY_JOB_STRUCT_VAL("ST Sowing Year", PT_UNSIGNED_INT);
STUDY_JOB_PREFIX NamedParameterType STUDY_HARVEST_YEAR STUDY_JOB_STRUCT_VAL("ST Harvest Year", PT_UNSIGNED_INT);


STUDY_JOB_PREFIX NamedParameterType STUDY_WEATHER_LINK STUDY_JOB_STRUCT_VAL("ST Weather", PT_STRING);

STUDY_JOB_PREFIX NamedParameterType STUDY_SHAPE_DATA STUDY_JOB_STRUCT_VAL("ST Shape Data", PT_JSON);

STUDY_JOB_PREFIX NamedParameterType STUDY_SHAPE_NOTES STUDY_JOB_STRUCT_VAL("ST Shape Data Notes", PT_LARGE_STRING);


STUDY_JOB_PREFIX NamedParameterType STUDY_NUM_PLOT_ROWS STUDY_JOB_STRUCT_VAL("ST Num Rows", PT_UNSIGNED_INT);
STUDY_JOB_PREFIX NamedParameterType STUDY_NUM_PLOT_COLS STUDY_JOB_STRUCT_VAL("ST Num Columns", PT_UNSIGNED_INT);
STUDY_JOB_PREFIX NamedParameterType STUDY_NUM_REPLICATES STUDY_JOB_STRUCT_VAL("ST Num Replicates", PT_UNSIGNED_INT);
STUDY_JOB_PREFIX NamedParameterType STUDY_PLOT_WIDTH STUDY_JOB_STRUCT_VAL("ST Plot Width", PT_UNSIGNED_REAL);
STUDY_JOB_PREFIX NamedParameterType STUDY_PLOT_LENGTH STUDY_JOB_STRUCT_VAL("ST Plot Length", PT_UNSIGNED_REAL);

STUDY_JOB_PREFIX NamedParameterType STUDY_PLOT_ROWS_PER_BLOCK STUDY_JOB_STRUCT_VAL("ST Rows Per Block", PT_UNSIGNED_INT);
STUDY_JOB_PREFIX NamedParameterType STUDY_PLOT_COLS_PER_BLOCK STUDY_JOB_STRUCT_VAL("ST Cols Per Block", PT_UNSIGNED_INT);
STUDY_JOB_PREFIX NamedParameterType STUDY_PLOT_HGAP STUDY_JOB_STRUCT_VAL("ST Plot Horizontal Gap", PT_UNSIGNED_REAL);
STUDY_JOB_PREFIX NamedParameterType STUDY_PLOT_VGAP STUDY_JOB_STRUCT_VAL("ST Plot Vertical Gap", PT_UNSIGNED_REAL);
STUDY_JOB_PREFIX NamedParameterType STUDY_PLOT_BLOCK_HGAP STUDY_JOB_STRUCT_VAL("ST Plot Block Horizontal Gap", PT_UNSIGNED_REAL);
STUDY_JOB_PREFIX NamedParameterType STUDY_PLOT_BLOCK_VGAP STUDY_JOB_STRUCT_VAL("ST Plot Block Vertical Gap", PT_UNSIGNED_REAL);

STUDY_JOB_PREFIX NamedParameterType STUDY_PHOTO STUDY_JOB_STRUCT_VAL("Photo", PT_STRING);
STUDY_JOB_PREFIX NamedParameterType STUDY_IMAGE_NOTES STUDY_JOB_STRUCT_VAL("ST Image Notes", PT_LARGE_STRING);


STUDY_JOB_PREFIX NamedParameterType STUDY_MEASURED_VARIABLES STUDY_JOB_STRUCT_VAL("ST Measured Variables", PT_STRING);


/* forward declarations */
struct TreatmentFactor;



#ifdef __cplusplus
extern "C"
{
#endif



DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddSubmissionStudyParams (ServiceData *data_p, ParameterSet *param_set_p, Study *active_study_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool RunForSubmissionStudyParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p, User *user_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetSubmissionStudyParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetSubmissionStudyParameterTypeForDefaultPlotNamedParameter (const char *param_name_s, ParameterType *pt_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddSearchStudyParams (ServiceData *data_p, ParameterSet *param_set_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool RunForSearchStudyParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetSearchStudyParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetAllStudiesAsJSON (const FieldTrialServiceData *data_p, bool full_data_flag);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetUpStudiesListParameter (const FieldTrialServiceData *data_p, Parameter *param_p, const Study *active_study_p, const bool empty_option_flag);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddStudyToServiceJob (ServiceJob *job_p, Study *study_p, const ViewFormat format, JSONProcessor *processor_p, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL char *GetStudyAsString (const Study *study_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL const KeyValuePair *GetAspect (const char *aspect_value_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetAllStudiesAsJSONInViewFormat (FieldTrialServiceData *data_p, const ViewFormat format);


DFW_FIELD_TRIAL_SERVICE_LOCAL Study *GetStudyFromResource (DataResource *resource_p, const NamedParameterType study_param_type, FieldTrialServiceData *dfw_data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FindAndAddStudyToServiceJob (const char *id_s, const ViewFormat format, ServiceJob *job_p, JSONProcessor *processor_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetStudyJSONForId (const char *id_s, const ViewFormat format, JSONProcessor *processor_p, char **study_name_ss, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetStudyDistinctPhenotypesAsJSON (bson_oid_t *study_id_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetStudyDistinctAccessionsAsJSON (bson_oid_t *study_id_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus RemovePlotsForStudyById (const char *id_s, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetAllStudyIds (Service *service_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL struct TreatmentFactor *GetOrCreateTreatmentFactorForStudy (Study *study_p, const bson_oid_t *treatment_id_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL struct TreatmentFactor *GetTreatmentFactorForStudy (Study *study_p, const bson_oid_t *treatment_id_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL struct TreatmentFactor *GetTreatmentFactorForStudyByUrl (const Study *study_p, const char *treatment_url_s, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetStudyAsFrictionlessDataResource (const Study *study_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SaveStudyAsFrictionlessData (Study *study_p, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddTreatmentFactorToStudy (TreatmentFactor *tf_p, Study *study_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetStudyDistinctPhenotypesAsFrictionlessDataJSON (bson_oid_t *study_id_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus CalculateStudyStatistics (Study *study_p, const FieldTrialServiceData *service_data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus GenerateStatisticsForAllStudies (ServiceJob *job_p,  FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus GenerateStatisticsForStudy (Study *study_p, ServiceJob *job_p,  FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool BackupStudyByIdString (const char *id_s, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool BackupStudy (Study *study_p, const char *id_s, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus DeleteStudyById (const char *id_s, ServiceJob *job_p, FieldTrialServiceData *data_p, const bool backup_flag);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus DeleteStudyFromLuceneIndexById (const char *id_s, uuid_t uuid, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus RemoveStudyPhenotypesFromStudyById (const char *id_s, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool RemoveCachedStudyById (const char * const id_s, FieldTrialServiceData *data_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVICES_DFW_FIELD_TRIAL_SERVICE_INCLUDE_EXPERIMENTAL_AREA_JOBS_H_ */
