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
 * study_jobs.c
 *
 *  Created on: 1 Oct 2018
 *      Author: billy
 */

#define ALLOCATE_STUDY_JOB_CONSTANTS (1)
#include "study_jobs.h"

#include "study.h"
#include "location.h"
#include "string_utils.h"
#include "crop_jobs.h"
#include "plot_jobs.h"
#include "row_jobs.h"
#include "location_jobs.h"
#include "field_trial_jobs.h"
#include "programme_jobs.h"
#include "treatment_jobs.h"
#include "treatment_factor_jobs.h"
#include "dfw_util.h"
#include "key_value_pair.h"
#include "time_util.h"
#include "frictionless_data_util.h"


#include "plot.h"
#include "row.h"
#include "observation.h"
#include "numeric_observation.h"
#include "treatment_factor.h"

#include "string_parameter.h"
#include "string_array_parameter.h"
#include "double_parameter.h"
#include "time_parameter.h"
#include "boolean_parameter.h"
#include "signed_int_parameter.h"
#include "unsigned_int_parameter.h"
#include "json_parameter.h"
#include "mongodb_tool.h"
#include "lucene_tool.h"
#include "statistics.h"
#include "phenotype_statistics.h"
#include "person_jobs.h"
#include "permissions_editor.h"

typedef struct
{
	Study *spd_study_p;
	StatisticsTool *spd_stats_tool_p;
} StudyProcessData;


/*
 * Study parameters
 */

static NamedParameterType S_SEARCH_TRIAL_ID = { "Get all studies for this Field Trial", PT_STRING };


static NamedParameterType S_SEARCH_STUDY_HAS_PLOTS = { "Study must have plot data", PT_BOOLEAN };


static NamedParameterType S_SEARCH_STUDIES_ACCESSIONS STUDY_JOB_STRUCT_VAL("ST Search Study Accessions", PT_STRING);
static NamedParameterType S_SEARCH_STUDIES_PHENOTYPES STUDY_JOB_STRUCT_VAL("ST Search Study Phenotypes", PT_STRING);





#define S_NUM_DIRECTIONS (9)

static const uint32 S_UNKNOWN_DIRECTION_INDEX = S_NUM_DIRECTIONS - 1;

static const KeyValuePair S_DIRECTIONS_P [] =
		{
				{ "North", "http://purl.obolibrary.org/obo/NCIT_C45849" },
				{ "North-East", "http://purl.obolibrary.org/obo/NCIT_C45853" },
				{ "East", "http://purl.obolibrary.org/obo/NCIT_C45851" },
				{ "South-East", "http://purl.obolibrary.org/obo/NCIT_C45855" },
				{ "South", "http://purl.obolibrary.org/obo/NCIT_C45850" },
				{ "South-West", "http://purl.obolibrary.org/obo/NCIT_C45856" },
				{ "West", "http://purl.obolibrary.org/obo/NCIT_C45852" },
				{ "North-West", "http://purl.obolibrary.org/obo/NCIT_C45854" },
				{ ST_UNKNOWN_DIRECTION_S, ST_UNKNOWN_DIRECTION_S},
		};


static const char * const S_EMPTY_LIST_OPTION_S = "<empty>";

static const char * const S_UNKNOWN_CROP_OPTION_S = "Unknown";

static const char * const S_DETAIL_LEVEL_IDS_S = "Names and Ids only";

static const char * const S_DETAIL_LEVEL_METADATA_S = "Metadata";

static const char * const S_DETAIL_LEVEL_FULL_S = "Full";

/*
 * STATIC DECLARATIONS
 */

static bool AddStudy (ServiceJob *job_p, ParameterSet *param_set_p, FieldTrialServiceData *data_p, User *user_p);


static bool GetStudyForGivenId (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p, ViewFormat format, JSONProcessor *processor_p);


static bool AddStudyLocationCriteria (bson_t *query_p, ParameterSet *param_set_p);


static bool AddStudyDateCriteria (bson_t *query_p, ParameterSet *param_set_p);



static Parameter *GetAndAddAspectParameter (const char *aspect_s, FieldTrialServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p);




static bool GetValidCrop (const char *crop_s, Crop **crop_pp, const FieldTrialServiceData *data_p);

static Study *GetStudyFromJSONResource (const json_t *resource_data_p, ServiceData *data_p);


static bool SetUpDefaultsFromExistingStudy (const Study * const study_p, char **id_ss, char **this_crop_ss, char **previous_crop_ss, char **trial_ss, char **location_ss);



static bool AddDefaultPlotsParameters (ServiceData *data_p, ParameterSet *params_p, const bool read_only_flag, const Study *study_p);


static bool AddLayoutParams (ParameterSet *params_p, const Study *study_p, const bool read_only_flag, FieldTrialServiceData *dfw_data_p);


static bool AddTermToJSON (const SchemaTerm *term_p, json_t *phenotypes_p);


static bool AddAccession (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p);


static bool AddPhenotype (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p);

static bool AddFullPhenotype (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p);

static bool AddFullPhenotypeAsJSON (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p);


static json_t *GetDistinctValuesAsJSON (bson_oid_t *study_id_p, const char *key_s, bool (*add_value_fn) (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p), const FieldTrialServiceData *data_p);


static bool AddTreatmentFactorParameters (ParameterSet *params_p, const Study *study_p, const bool read_only_flag, FieldTrialServiceData *data_p);


static bool AddTreatmentFactorsToStudy (Study *study_p, Parameter *treatment_names_p, Parameter *treatment_levels_p, const size_t num_treatments, ServiceJob *job_p, const FieldTrialServiceData *data_p);


static void ReportTreatmentFactorError (Study *study_p, const char *name_s, const json_t *levels_p, ServiceJob *job_p);


static bool GetPersonFromParameters (Person **person_pp, ParameterSet *param_set_p, const char *name_param_s, const char *email_param_s,
																		 const char *role_param_s, const char *affilation_param_s, const char *orcid_param_s);


static bool AddPhenotypeAsFrictionlessData (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p);


static bool AddPersonAsFrictionlessData (const Person * const person_p, json_t *json_p, const char * const name_key_s, const char * const email_key_s);


static bool AddCropAsFrictionlessData (const Crop * const crop_p, json_t *json_p, const char * const key_s);


static bool AddTreatmentFactorsAsFrictionlessData (json_t *json_p, LinkedList *treatments_p, const char * const key_s);


static bool AddGeneralSubmissionStudyParams (Study *active_study_p, const char *id_s, const char *trial_s, const char *location_s, ParameterSet *params_p, ParameterGroup *group_p, const bool read_only_flag, ServiceData *data_p);


static OperationStatus ProcessDistinctValues (bson_oid_t *study_id_p, const char *key_s, bool (*process_value_fn) (const char *oid_s, void *user_data_p, const FieldTrialServiceData *service_data_p), void *user_data_p, const FieldTrialServiceData *service_data_p);

static bool ProcessStudyPhenotype (const char *phenotype_oid_s, void *user_data_p, const FieldTrialServiceData *service_data_p);

static bool AddCuratorSubmissionParams (const Person *curator_p, ParameterSet *params_p, const bool read_only_flag, ServiceData *data_p);

static bool AddContactSubmissionParams (const Person *curator_p, ParameterSet *params_p, const bool read_only_flag, ServiceData *data_p);


static bool ProcessPersonForStudy (Person *person_p, void *user_data_p);


static bool AddMeasuredVariableParameters (ParameterSet *params_p, const Study *study_p, const bool read_only_flag, FieldTrialServiceData *data_p);


static OperationStatus ProcessMeasuredVariables (ServiceJob *job_p, ParameterSet *param_set_p, Study *study_p, FieldTrialServiceData *ft_service_data_p);

static bool AddFieldTrialSubmissionParameter (Study *active_study_p, ParameterSet *params_p, ParameterGroup *group_p, FieldTrialServiceData *dfw_data_p);

static Parameter *ProcessTreatmentNames (ParameterSet *param_set_p, size_t *num_names_p);

static Parameter *ProcessTreatmentLevels (ParameterSet *param_set_p, size_t *num_names_p);

static bool UpdateStudyStringValue (char **variable_ss, const ParameterSet * const param_set_p, const NamedParameterType * const param_p, ServiceJob *job_p, const char * const study_id_s);

static bool UpdateStudyUnsignedIntValue (uint32 **variable_pp, const ParameterSet * const param_set_p, const NamedParameterType * const param_p, ServiceJob *job_p, const char * const study_id_s);

static bool UpdateStudyDoubleValue (double64 **variable_pp, const ParameterSet * const param_set_p, const NamedParameterType * const param_p, ServiceJob *job_p, const char * const study_id_s);


static bool UpdateStudyJSONValue (json_t **variable_pp, const ParameterSet * const param_set_p, const NamedParameterType * const param_p, ServiceJob *job_p, const char * const study_id_s);


static bool ProcessPersonForStudy (Person *person_p, void *user_data_p);


#ifdef ENABLE_MARTI
static OperationStatus SearchMarti (const double64 *latitude_p, const double64 *longitude_p, const struct tm *date_p, FieldTrialServiceData *ft_service_data_p);
#endif


static bool RunForAdvancedSearchStudyParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


static bool RunForWizardSearchStudyParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


/*
 * API DEFINITIONS
 */


bool AddSubmissionStudyParams (ServiceData *data_p, ParameterSet *params_p, Study *active_study_p, const bool read_only_flag)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Study", false, data_p, params_p);
	FieldTrialServiceData *dfw_data_p = (FieldTrialServiceData *) data_p;

	bool defaults_flag = false;
	char *id_s = NULL;
	char *this_crop_s = NULL;
	char *previous_crop_s = NULL;
	char *trial_s = NULL;
	char *location_s = NULL;

	if (active_study_p)
		{
			if (SetUpDefaultsFromExistingStudy (active_study_p, &id_s, &this_crop_s, &previous_crop_s, &trial_s, &location_s))
				{
					defaults_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpDefaultsFromExistingStudy () failed for \"%s\"", active_study_p -> st_name_s);
				}
		}
	else
		{
			id_s = (char *) S_EMPTY_LIST_OPTION_S;
			defaults_flag = true;
		}

	if (defaults_flag)
		{
			FieldTrialServiceData *dfw_data_p = (FieldTrialServiceData *) data_p;
			Parameter *param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_ID.npt_type, STUDY_ID.npt_name_s, "Load Study", "Edit an existing study", id_s, PL_ALL_AND_WIZARD);

			if (param_p)
				{
					if (SetUpStudiesListParameter (dfw_data_p, param_p, NULL, true))
						{
							/*
							 * We want to update all of the values in the form
							 * when a user selects a study from the list so
							 * we need to make the parameter automatically
							 * refresh the values. So we set the
							 * pa_refresh_service_flag to true.
							 */
							param_p -> pa_refresh_service_flag = true;

							if (AddStudyEditor (active_study_p, id_s, params_p, group_p, trial_s,
																	location_s, this_crop_s, previous_crop_s, read_only_flag, data_p))
								{
									success_flag = true;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddStudyEditor () failed for \"%s\"", active_study_p ? active_study_p -> st_name_s : "NULL");
								}


						}		/* if (SetUpStudiesListParameter (dfw_data_p, param_p, NULL, true)) */

				}		/* if (param_p) */






		}		/* if (defaults_flag) */




	if (this_crop_s)
		{
			FreeCopiedString (this_crop_s);
		}

	if (previous_crop_s)
		{
			FreeCopiedString (previous_crop_s);
		}

	if (trial_s)
		{
			FreeCopiedString (trial_s);
		}

	if (location_s)
		{
			FreeCopiedString (location_s);
		}

	return success_flag;
}


bool AddStudyEditor (Study *study_p, const char *id_s, ParameterSet *params_p, ParameterGroup *group_p, const char * const trial_s,
										 const char * const location_s, const char * const this_crop_s,  const char * const previous_crop_s, const bool read_only_flag, ServiceData *data_p)
{
	Parameter *param_p = NULL;
	FieldTrialServiceData *dfw_data_p = (FieldTrialServiceData *) data_p;
	bool success_flag = false;



	if (AddGeneralSubmissionStudyParams (study_p, id_s, trial_s, location_s, params_p, group_p, read_only_flag, data_p))
		{
			if (AddLayoutParams (params_p, study_p, read_only_flag, dfw_data_p))
				{
					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_THIS_CROP.npt_type, STUDY_THIS_CROP.npt_name_s, "Crop", "The crop variety for this study", this_crop_s, PL_ALL)) != NULL)
						{
							const bool new_study_flag = study_p ? false : true;

							param_p -> pa_read_only_flag = read_only_flag;

							if (SetUpCropsListParameter (dfw_data_p, param_p, study_p ? study_p -> st_current_crop_p : NULL, S_UNKNOWN_CROP_OPTION_S, new_study_flag))
								{
									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_PREVIOUS_CROP.npt_type, STUDY_PREVIOUS_CROP.npt_name_s, "Previous Crop", "The previous crop variety planted in this field", previous_crop_s, PL_ALL)) != NULL)
										{
											param_p -> pa_read_only_flag = read_only_flag;

											if (SetUpCropsListParameter (dfw_data_p, param_p, study_p ? study_p -> st_previous_crop_p : NULL, S_UNKNOWN_CROP_OPTION_S, new_study_flag))
												{
													if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_LINK.npt_type, STUDY_LINK.npt_name_s, "Link", "The url for any downloads relating to this Study", study_p ? study_p -> st_data_url_s : NULL, PL_ALL)) != NULL)
														{
															param_p -> pa_read_only_flag = read_only_flag;

															if (AddDefaultPlotsParameters (data_p, params_p, read_only_flag, study_p))
																{
																	if ((param_p = EasyCreateAndAddJSONParameterToParameterSet (data_p, params_p, group_p, STUDY_SHAPE_DATA.npt_type, STUDY_SHAPE_DATA.npt_name_s, "Plots GPS", "The GeoJSON for the vertices of the plots layout", study_p ? study_p -> st_shape_p : NULL, PL_ALL)) != NULL)
																		{
																			param_p -> pa_read_only_flag = read_only_flag;

																			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_SHAPE_NOTES.npt_type, STUDY_SHAPE_NOTES.npt_name_s, "Plots GPS Notes", "Any notes relating to the GPS data of the plots", study_p ? study_p -> st_shape_notes_s : NULL, PL_ALL)) != NULL)
																				{
																					param_p -> pa_read_only_flag = read_only_flag;

																					if (AddMultiplePeopleParameters (params_p, "Contributors", study_p ? study_p -> st_contributors_p : NULL, read_only_flag, dfw_data_p))
																						{
																							success_flag = true;
																						}
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddMultiplePeopleParameters () failed for \"%s\"", study_p ? study_p -> st_name_s : "NULL");
																						}
																				}
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpCropsListParameter failed for \"%s\"", STUDY_SHAPE_NOTES.npt_name_s);
																				}

																		}
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpCropsListParameter failed for \"%s\"", STUDY_SHAPE_DATA.npt_name_s);
																		}

																	if (AddTreatmentFactorParameters (params_p, study_p, read_only_flag, dfw_data_p))
																		{
																			if (AddMeasuredVariableParameters (params_p, study_p, read_only_flag, dfw_data_p))
																				{
																					success_flag = true;
																				}
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddMeasuredVariableParameters () failed");
																				}
																		}
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddTreatmentFactorParameters () failed");
																		}


																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddDefaultPlotsParameters () failed");
																}


														}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (data_p, param_set_p, group_p, STUDY_LINK.npt_type, STUDY_LINK.npt_name_s, "Link", "The url for any downloads relating to this Study", def, PL_ALL)) != NULL) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_LINK.npt_name_s);
														}

												}		/* if (SetUpCropsListParameter (dfw_data_p, param_p)) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpCropsListParameter failed for \"%s\"", STUDY_PREVIOUS_CROP.npt_name_s);
												}

										}		/* if (param_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PREVIOUS_CROP.npt_name_s);
										}
								}		/* if (SetUpCropsListParameter (dfw_data_p, param_p)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpCropsListParameter failed for \"%s\"", STUDY_THIS_CROP.npt_name_s);
								}

						}		/* if (param_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_THIS_CROP.npt_name_s);
						}

				}		/* if (AddLayoutParams (params_p, active_study_p, dfw_data_p)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddLayoutParams failed");
				}

		}		/* if (AddGeneralSubmissionStudyParams (active_study_p, params_p, group_p, data_p)) */


	return success_flag;
}


json_t *GetOldStudyIndexingData (Service *service_p)
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);
	json_t *src_studies_p = GetAllStudiesAsJSON (data_p, true);


	if (src_studies_p)
		{
			if (json_is_array (src_studies_p))
				{
					FieldTrialServiceData *dfw_data_p = (FieldTrialServiceData *) (service_p -> se_data_p);
					size_t i;
					json_t *src_study_p;

					json_array_foreach (src_studies_p, i, src_study_p)
					{
						bson_oid_t id;

						if (AddDatatype (src_study_p, DFTD_STUDY))
							{
								if (GetMongoIdFromJSON (src_study_p, &id))
									{
										Crop *crop_p = NULL;

										/*
										 * Add the phenotypes
										 */
										json_t *values_p = GetStudyDistinctPhenotypesAsJSON (&id, dfw_data_p);

										if (values_p)
											{
												if (json_object_set_new (src_study_p, "phenotypes", values_p) != 0)
													{
														PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, src_study_p, "Failed to add phenotypes");
														json_decref (values_p);
													}
											}

										/*
										 * Add the accessions
										 */
										values_p = GetStudyDistinctAccessionsAsJSON (&id, dfw_data_p);

										if (values_p)
											{
												if (json_object_set_new (src_study_p, "accessions", values_p) != 0)
													{
														PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, src_study_p, "Failed to add accessions");
														json_decref (values_p);
													}
											}


										/*
										 * Add the parent field trial
										 */
										if (GetNamedIdFromJSON (src_study_p, ST_PARENT_FIELD_TRIAL_S, &id))
											{
												FieldTrial *trial_p = GetFieldTrialById (&id, VF_STORAGE, data_p);

												json_object_del (src_study_p, ST_PARENT_FIELD_TRIAL_S);

												if (trial_p)
													{
														json_t *trial_json_p = GetFieldTrialAsJSON (trial_p, VF_CLIENT_MINIMAL, data_p);

														if (trial_json_p)
															{
																json_object_del (trial_json_p, MONGO_ID_S);

																if (json_object_set_new (src_study_p, ST_PARENT_FIELD_TRIAL_S, trial_json_p) == 0)
																	{

																	}
																else
																	{
																		json_decref (trial_json_p);
																	}
															}

														FreeFieldTrial (trial_p);
													}		/* if (trial_p) */

											}

										/*
										 * Add the location
										 */
										if (GetNamedIdFromJSON (src_study_p, ST_LOCATION_ID_S, &id))
											{
												Location *location_p = GetLocationById (&id, VF_STORAGE, data_p);

												json_object_del (src_study_p, ST_LOCATION_ID_S);

												if (location_p)
													{
														json_t *location_json_p = GetLocationAsJSON (location_p);

														if (location_json_p)
															{
																json_object_del (location_json_p, MONGO_ID_S);

																if (json_object_set_new (src_study_p, ST_LOCATION_S, location_json_p) == 0)
																	{

																	}
																else
																	{
																		json_decref (location_json_p);
																	}

															}

														FreeLocation (location_p);
													}		/* if (location_p) */

											}		/* */


										/*
										 * Add the current crop
										 */
										crop_p = GetStoredCropValue (src_study_p, ST_CURRENT_CROP_S, data_p);
										if (crop_p)
											{
												SetJSONString (src_study_p, ST_CURRENT_CROP_S, crop_p -> cr_name_s);
												FreeCrop (crop_p);
											}

										/*
										 * Add the previous crop
										 */
										crop_p = GetStoredCropValue (src_study_p, ST_PREVIOUS_CROP_S, data_p);
										if (crop_p)
											{
												SetJSONString (src_study_p, ST_PREVIOUS_CROP_S, crop_p -> cr_name_s);
												FreeCrop (crop_p);
											}

										/*
										 * Add the treatments
										 */
										values_p = json_object_get (src_study_p, ST_TREATMENTS_S);

										if (values_p)
											{
												if (json_is_array (values_p))
													{
														json_t *treatment_factor_p;
														size_t j;

														json_array_foreach (values_p, j, treatment_factor_p)
														{
															if (GetNamedIdFromJSON (treatment_factor_p, TF_TREATMENT_S, &id))
																{
																	Treatment *treatment_p = GetTreatmentById (&id, VF_STORAGE, data_p);

																	if (treatment_p)
																		{
																			if (AddTreatmentToJSON (treatment_p, treatment_factor_p))
																				{

																				}


																			FreeTreatment (treatment_p);
																		}		/* if (treatment_p) */

																}		/* if (GetNamedIdFromJSON (src_study_p, TFJ_TREATMENT_ID, &id)) */

														}		/* json_array_foreach (values_p, j, treatment_factor_p) */

													}		/* if (json_is array (values_p)) */

											}		/* if (values_p) */

									}		/* if (GetMongoIdFromJSON (entry_p, &id)) */

							}		/* if (AddDatatype (src_study_p, DFTD_STUDY)) */

					}		/* json_array_foreach (src_studies_p, i, src_study_p) */

				}		/* if (json_is_array (src_studies_p)) */

			return src_studies_p;
		}		/* if (src_studies_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No studies for \"%s\"", GetServiceName (service_p));
		}

	return NULL;
}


OperationStatus ImportStudy ()
{
	return OS_IDLE;
}



bool RemoveCachedStudyById (const char * const id_s, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	char *filename_s = GetFullCacheFilename (id_s, data_p);

	if (filename_s)
		{
			if (RemoveFile (filename_s))
				{
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to remove file \"%s\"", filename_s);
				}

			FreeCopiedString (filename_s);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetFullCacheFilename () failed for \"%s\"", id_s);
		}

	return success_flag;
}


static bool AddWizardParams (ParameterSet *params_p, Study *active_study_p, ServiceData *data_p)
{

	/*
	 * Study Name
	 * Trial
	 * Location
	 * Plot Rows
	 * Plot cols
	 *
	 * create the study and plots with their ids, bson oids, row and column
	 */
	bool success_flag = false;
	Parameter *param_p = NULL;
	const FieldTrialServiceData *ft_data_p = (const FieldTrialServiceData  *) data_p;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Study", false, data_p, params_p);

	bool defaults_flag = false;

	char *id_s = NULL;
	char *this_crop_s = NULL;
	char *previous_crop_s = NULL;
	char *trial_s = NULL;
	char *location_s = NULL;

	if (active_study_p)
		{
			if (SetUpDefaultsFromExistingStudy (active_study_p, &id_s, &this_crop_s, &previous_crop_s, &trial_s, &location_s))
				{
					defaults_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpDefaultsFromExistingStudy () failed for \"%s\"", active_study_p -> st_name_s);
				}
		}
	else
		{
			id_s = (char *) S_EMPTY_LIST_OPTION_S;
			defaults_flag = true;
		}

	if (defaults_flag)
		{
			const bool read_only_flag = false;

			if (AddGeneralSubmissionStudyParams (active_study_p, id_s, trial_s, location_s, params_p, group_p, read_only_flag, data_p))
				{
					if (AddLayoutParams (params_p, active_study_p, read_only_flag, ft_data_p))
						{
							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_THIS_CROP.npt_type, STUDY_THIS_CROP.npt_name_s, "Crop", "The crop variety for this study", this_crop_s, PL_ALL)) != NULL)
								{
									const bool new_study_flag = active_study_p ? false : true;

									if (SetUpCropsListParameter (ft_data_p, param_p, active_study_p ? active_study_p -> st_current_crop_p : NULL, S_UNKNOWN_CROP_OPTION_S, new_study_flag))
										{
											if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_PREVIOUS_CROP.npt_type, STUDY_PREVIOUS_CROP.npt_name_s, "Previous Crop", "The previous crop variety planted in this field", previous_crop_s, PL_ALL)) != NULL)
												{
													if (SetUpCropsListParameter (ft_data_p, param_p, active_study_p ? active_study_p -> st_previous_crop_p : NULL, S_UNKNOWN_CROP_OPTION_S, new_study_flag))
														{
															if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_LINK.npt_type, STUDY_LINK.npt_name_s, "Link", "The url for any downloads relating to this Study", active_study_p ? active_study_p -> st_data_url_s : NULL, PL_ALL)) != NULL)
																{
																	if (AddDefaultPlotsParameters (data_p, params_p, read_only_flag, active_study_p))
																		{
																			if ((param_p = EasyCreateAndAddJSONParameterToParameterSet (data_p, params_p, group_p, STUDY_SHAPE_DATA.npt_type, STUDY_SHAPE_DATA.npt_name_s, "Plots GPS", "The GeoJSON for the vertices of the plots layout", active_study_p ? active_study_p -> st_shape_p : NULL, PL_ALL)) != NULL)
																				{
																					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_SHAPE_NOTES.npt_type, STUDY_SHAPE_NOTES.npt_name_s, "Plots GPS Notes", "Any notes relating to the GPS data of the plots", active_study_p ? active_study_p -> st_shape_notes_s : NULL, PL_ALL)) != NULL)
																						{
																							if (AddMultiplePeopleParameters (params_p, "Contributors", active_study_p ? active_study_p -> st_contributors_p : NULL, read_only_flag, ft_data_p))
																								{
																									success_flag = true;
																								}
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddMultiplePeopleParameters () failed for \"%s\"", active_study_p ? active_study_p -> st_name_s : "NULL");
																								}
																						}
																					else
																						{


																						}
																				}
																		}
																}
														}
												}
										}
								}
						}
				}

		}

	return false;
}



static bool AddLayoutParams (ParameterSet *params_p, const Study *study_p, const bool read_only_flag, FieldTrialServiceData *dfw_data_p)
{
	ServiceData *data_p = & (dfw_data_p -> dftsd_base_data);
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Layout", false, data_p, params_p);
	Parameter *param_p;

	if ((param_p = GetAndAddAspectParameter (study_p ? study_p -> st_aspect_s : NULL, dfw_data_p, params_p, group_p)) != NULL)
		{
			param_p -> pa_read_only_flag = read_only_flag;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_SLOPE.npt_type, STUDY_SLOPE.npt_name_s, "Slope", "The slope of the Study", study_p ? study_p -> st_slope_s : NULL, PL_ALL)) != NULL)
				{
					param_p -> pa_read_only_flag = read_only_flag;

					if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (data_p, params_p, group_p, STUDY_PLOT_HGAP.npt_type, STUDY_PLOT_HGAP.npt_name_s, "Horizontal plot gap", "The distance (in metres) between plots in a row.", study_p ? study_p -> st_plot_horizontal_gap_p : NULL, PL_ALL)) != NULL)
						{
							param_p -> pa_read_only_flag = read_only_flag;

							if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (data_p, params_p, group_p, STUDY_PLOT_VGAP.npt_type, STUDY_PLOT_VGAP.npt_name_s, "Vertical plot gap", "The distance (in metres) between plots in a column.", study_p ? study_p -> st_plot_vertical_gap_p : NULL, PL_ALL)) != NULL)
								{
									param_p -> pa_read_only_flag = read_only_flag;

									if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, params_p, group_p, STUDY_PLOT_ROWS_PER_BLOCK.npt_name_s, "Plot rows per block", "The number of rows of plots in a block", study_p ? study_p -> st_plots_rows_per_block_p : NULL, PL_ALL)) != NULL)
										{
											param_p -> pa_read_only_flag = read_only_flag;

											if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, params_p, group_p, STUDY_PLOT_COLS_PER_BLOCK.npt_name_s, "Plot columns per block", "The number of columns of plots in a block", study_p ? study_p -> st_plots_columns_per_block_p : NULL, PL_ALL)) != NULL)
												{
													param_p -> pa_read_only_flag = read_only_flag;

													if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (data_p, params_p, group_p, STUDY_PLOT_BLOCK_HGAP.npt_type, STUDY_PLOT_BLOCK_HGAP.npt_name_s, "Horizontal plot block gap", "The distance (in metres) between blocks of plots in a row.", study_p ? study_p -> st_plot_block_horizontal_gap_p : NULL, PL_ALL)) != NULL)
														{
															param_p -> pa_read_only_flag = read_only_flag;

															if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (data_p, params_p, group_p, STUDY_PLOT_BLOCK_VGAP.npt_type, STUDY_PLOT_BLOCK_VGAP.npt_name_s, "Vertical plot block gap", "The distance (in metres) between blocks of plots in a column.", study_p ? study_p -> st_plot_block_vertical_gap_p : NULL, PL_ALL)) != NULL)
																{
																	param_p -> pa_read_only_flag = read_only_flag;

																	return true;
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PLOT_BLOCK_VGAP.npt_name_s);
																}
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PLOT_BLOCK_HGAP.npt_name_s);
														}
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PLOT_VGAP.npt_name_s);
												}
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PLOT_ROWS_PER_BLOCK.npt_name_s);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PLOT_ROWS_PER_BLOCK.npt_name_s);
								}

						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PLOT_ROWS_PER_BLOCK.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_SLOPE.npt_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetAndAddAspectParameter failed with aspect \"%s\"", study_p ? study_p -> st_aspect_s : "NULL");
		}

	return false;
}


bool RunForSubmissionStudyParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p, User *user_p)
{
	bool success_flag = AddStudy (job_p, param_set_p, data_p, user_p);

	return success_flag;
}


bool GetSubmissionStudyParameterTypeForDefaultPlotNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
			{
					STUDY_NUM_PLOT_ROWS ,
					STUDY_NUM_PLOT_COLS,
					STUDY_NUM_REPLICATES,
					STUDY_PLOT_WIDTH,
					STUDY_PLOT_LENGTH,
					STUDY_SOWING_YEAR,
					STUDY_HARVEST_YEAR,
					NULL
			};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}

bool GetSubmissionStudyParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = false;

	const NamedParameterType params [] =
			{
					STUDY_ID,
					STUDY_NAME,
					STUDY_LINK,
					STUDY_FIELD_TRIALS_LIST,
					STUDY_LOCATIONS_LIST,
					STUDY_ASPECT,
					STUDY_SLOPE,
					STUDY_THIS_CROP,
					STUDY_PREVIOUS_CROP,
					STUDY_DESCRIPTION,
					STUDY_DESIGN,
					STUDY_GROWING_CONDITIONS,
					STUDY_PHENOTYPE_GATHERING_NOTES,
					STUDY_WEATHER_LINK,
					STUDY_SHAPE_DATA,
					STUDY_PLOT_ROWS_PER_BLOCK,
					STUDY_PLOT_COLS_PER_BLOCK,
					STUDY_PLOT_HGAP,
					STUDY_PLOT_VGAP,
					STUDY_PLOT_BLOCK_HGAP,
					STUDY_PLOT_BLOCK_VGAP,
					STUDY_CURATOR_NAME,
					STUDY_CURATOR_EMAIL,
					STUDY_CURATOR_ROLE,
					STUDY_CURATOR_AFFILATION,
					STUDY_CURATOR_ORCID,
					STUDY_CONTACT_NAME,
					STUDY_CONTACT_EMAIL,
					STUDY_CONTACT_ROLE,
					STUDY_CONTACT_AFFILATION,
					STUDY_CONTACT_ORCID,
					STUDY_SOWING_YEAR,
					STUDY_HARVEST_YEAR,
					STUDY_PLOT_BLOCK_VGAP,
					STUDY_PLAN_CHANGES,
					STUDY_DATA_NOT_INCLUDED,
					STUDY_PHYSICAL_SAMPLES_COLLECTED,
					STUDY_PHOTO,
					STUDY_IMAGE_NOTES,
					STUDY_SHAPE_NOTES,
					STUDY_MEASURED_VARIABLES,
					NULL
			};

	if (DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params))
		{
			success_flag = true;
		}
	else if (GetSubmissionStudyParameterTypeForDefaultPlotNamedParameter (param_name_s, pt_p))
		{
			success_flag = true;
		}
	else if (GetPersonParameterTypeForNamedParameter (param_name_s, pt_p))
		{
			success_flag = true;
		}
	else
		{
			success_flag = GetPermissionsEditorParameterTypeForNamedParameter (param_name_s, pt_p);
		}

	return success_flag;
}


/*
Parameters

    commonCropName
    Common name for the crop associated with this study
    String

    programDbId
    Program filter to only return studies associated with given program id.
    String

    locationDbId
    Filter by location
    String

    seasonDbId
    Filter by season or year
    String

    trialDbId
    Filter by trial
    String

    studyDbId
    Filter by study DbId
    String

		active
    Filter active status true/false.
    String

    sortBy
    Name of the field to sort by.
    String

    sortOrder
    Sort order direction. Ascending/Descending.
    String

    page
    Which result page is requested. The page indexing starts at 0 (the first page is 'page'= 0). Default is 0.
    String

    pageSize
    The size of the pages to be returned. Default is 1000.
    String

 */




json_t *GetStudyDistinctAccessionsAsJSON (bson_oid_t *study_id_p, const FieldTrialServiceData *data_p)
{
	json_t *accessions_p = NULL;
	char *key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", SR_MATERIAL_ID_S, NULL);

	if (key_s)
		{
			accessions_p = GetDistinctValuesAsJSON (study_id_p, key_s, AddAccession, data_p);
			FreeCopiedString (key_s);
		}

	return accessions_p;
}


bool GetSearchStudyParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
			{
					STUDY_SEARCH_STUDIES,
					STUDY_ID,
					//STUDY_GET_ALL_PLOTS,
					STUDY_DETAIL_LEVEL,
					STUDY_LOCATIONS_LIST,
					STUDY_HARVEST_YEAR,
					STUDY_SOWING_YEAR,
					S_SEARCH_TRIAL_ID,
					S_SEARCH_STUDIES_ACCESSIONS,
					S_SEARCH_STUDIES_PHENOTYPES,
					NULL
			};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}




bool AddSearchStudyParams (ServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	const char * const group_name_s = "Studies";
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet (group_name_s, false, data_p, param_set_p);

	if (group_p)
		{
			bool search_flag = false;

			if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, param_set_p, group_p, STUDY_SEARCH_STUDIES.npt_name_s, "Search Studies", "Get the matching Studies", &search_flag, PL_ADVANCED)) != NULL)
				{
					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, STUDY_ID.npt_type, STUDY_ID.npt_name_s, "Id", "The id of the Study", NULL, PL_ADVANCED)) != NULL)
						{
							if (AddStudyLevelDetailParameter (param_set_p, group_p, data_p))
								{
									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_SEARCH_TRIAL_ID.npt_type, S_SEARCH_TRIAL_ID.npt_name_s, "Parent Field Trial", "Get all Studies for a given Field Trial", NULL, PL_ADVANCED)) != NULL)
										{
											if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, STUDY_LOCATIONS_LIST.npt_type, STUDY_LOCATIONS_LIST.npt_name_s, "Locations", "The available locations", NULL, PL_ADVANCED)) != NULL)
												{
													if (SetUpLocationsListParameter ((FieldTrialServiceData *) data_p, (StringParameter *) param_p, NULL, GetUnsetLocationValue ()))
														{
															uint32 year = 2017;

															if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, group_p, STUDY_SOWING_YEAR.npt_name_s, "Sowing year", "Year that the Study was/will be sown", &year, PL_ADVANCED)) != NULL)
																{
																	if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, group_p, STUDY_HARVEST_YEAR.npt_name_s, "Harvest year", "Year that the Study was/will be harvested", &year, PL_ADVANCED)) != NULL)
																		{
																			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_SEARCH_STUDIES_ACCESSIONS.npt_type, S_SEARCH_STUDIES_ACCESSIONS.npt_name_s, "Accessions", "Search for Studies containing these accessions", NULL, PL_WIZARD)) != NULL)
																				{
																					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_SEARCH_STUDIES_PHENOTYPES.npt_type, S_SEARCH_STUDIES_PHENOTYPES.npt_name_s, "Phenotypes", "Search for Studies containing these phenotypes", NULL, PL_WIZARD)) != NULL)
																						{
																							success_flag = true;
																						}
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_SEARCH_STUDIES_PHENOTYPES.npt_name_s);
																						}
																				}
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_SEARCH_STUDIES_ACCESSIONS.npt_name_s);
																				}
																		}
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_HARVEST_YEAR.npt_name_s);
																		}
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_SOWING_YEAR.npt_name_s);
																}

														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpLocationsListParameter failed");
														}

												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_LOCATIONS_LIST.npt_name_s);
												}

										}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (data_p, param_set_p, group_p, S_SEARCH_TRIAL_ID_S.npt_type, S_SEARCH_TRIAL_ID_S.npt_name_s, "Parent Field Trial", "Get all Studies for a given Field Trial", def, PL_ADVANCED)) != NULL) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_SEARCH_TRIAL_ID.npt_name_s);
										}

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddStudyLevelDetailParameter () failed");
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_ID.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_SEARCH_STUDIES.npt_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CreateAndAddParameterGroupToParameterSet failed for %s", group_name_s);
		}

	return success_flag;
}




static bool RunForAdvancedSearchStudyParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const bool *search_flag_p = NULL;
	ViewFormat format = VF_CLIENT_MINIMAL;

	if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, STUDY_SEARCH_STUDIES.npt_name_s, &search_flag_p))
		{
			if ((search_flag_p != NULL) && (*search_flag_p == true))
				{
					const char *id_s = NULL;

					GetStudyLevelDetailParameterValue (param_set_p, &format);

					/*
					 * Are we searching for all studies within a trial?
					 */
					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_SEARCH_TRIAL_ID.npt_name_s, &id_s))
						{
							if (!IsStringEmpty (id_s))
								{

									/* Is it a single id or a list? */
									if (DoesStringContainWhitespace (id_s))
										{

										}

									/*
									 * We're building up a query for the given parameters
									 */
									bson_t *query_p = bson_new ();

									if (query_p)
										{
											bool built_query_success_flag = true;

											bson_oid_t *id_p = GetBSONOidFromString (id_s);

											if (id_p)
												{
													if (!BSON_APPEND_OID (query_p, ST_PARENT_FIELD_TRIAL_S, id_p))
														{
															FreeBSONOid (id_p);
															built_query_success_flag = false;
														}
												}
											else
												{
												}

											if (!built_query_success_flag)
												{
													const char *prefix_s = "Failed to build Field Trial query";
													char *error_s = ConcatenateVarargsStrings (prefix_s, " for ", id_s, NULL);

													if (error_s)
														{
															AddParameterErrorMessageToServiceJob  (job_p, S_SEARCH_TRIAL_ID.npt_name_s, S_SEARCH_TRIAL_ID.npt_type, error_s);
															FreeCopiedString (error_s);
														}
													else
														{
															AddParameterErrorMessageToServiceJob  (job_p, S_SEARCH_TRIAL_ID.npt_name_s, S_SEARCH_TRIAL_ID.npt_type, prefix_s);
														}
												}
										}		/* if (query_p) */

									/*
									 * Search with our given criteria
									 */
									if (GetMatchingStudies (query_p, data_p, job_p, format))
										{

										}

									job_done_flag = true;
								}
						}

					if (!job_done_flag)
						{
							if (GetStudyForGivenId (data_p, param_set_p, job_p, format, NULL))
								{
									job_done_flag = true;
								}		/* if (GetStudyForGivenId (data_p, param_set_p, job_p)) */
							else
								{
									/*
									 * We're building up a query for the given parameters
									 */
									bson_t *query_p = bson_new ();

									if (query_p)
										{
											if (AddStudyLocationCriteria (query_p, param_set_p))
												{
													if (AddStudyDateCriteria (query_p, param_set_p))
														{
															/*
															 * Search with our given criteria
															 */
															if (GetMatchingStudies (query_p, data_p, job_p, format))
																{
																	job_done_flag = true;
																}

														}		/* if (AddStudyLocationCriteria (query_p, param_set_p)) */

												}		/* if (AddStudyLocationCriteria (query_p, param_set_p)) */

											bson_destroy (query_p);
										}		/* if (query_p) */


								}		/* if (GetStudyForGivenId (data_p, param_set_p, job_p)) else ... */

						}
				}
		}


	return job_done_flag;

}


bool RunForSearchStudyParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool run_flag = false;

	switch (param_set_p -> ps_current_level)
		{
			case PL_ADVANCED:
				run_flag = RunForAdvancedSearchStudyParams (data_p, param_set_p, job_p);
				break;

			case PL_WIZARD:
				run_flag = RunForWizardSearchStudyParams (data_p, param_set_p, job_p);
				break;

			case PL_SIMPLE:
			default:
				break;
		}

	return run_flag;
}



static bool RunForWizardSearchStudyParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const bool *search_flag_p = NULL;

	const char *accession_s = NULL;
	const char *phenotype_s = NULL;

	GetCurrentStringParameterValueFromParameterSet (param_set_p, S_SEARCH_STUDIES_ACCESSIONS.npt_name_s, &accession_s);
	GetCurrentStringParameterValueFromParameterSet (param_set_p, S_SEARCH_STUDIES_PHENOTYPES.npt_name_s, &phenotype_s);


	if ((!IsStringEmpty (accession_s)) || (!IsStringEmpty (phenotype_s)))
		{
			ViewFormat format = VF_CLIENT_MINIMAL;

			GetStudyLevelDetailParameterValue (param_set_p, &format);


			/*
			 * We're building up a query for the given parameters
			 */
			bson_t *query_p = bson_new ();

			if (query_p)
				{
					bool built_query_success_flag = true;

					if (!IsStringEmpty (accession_s))
						{
							if (!BSON_APPEND_UTF8 (query_p, ST_ACCESSIONS_S, accession_s))
								{
									built_query_success_flag = false;
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\": \"%s\" to query", ST_ACCESSIONS_S, accession_s);
								}
						}		/* if (!IsStringEmpty (accession_s)) */


					if (built_query_success_flag)
						{
							if (!IsStringEmpty (phenotype_s))
								{
									if (!BSON_APPEND_UTF8 (query_p, ST_PHENOTYPES_S, phenotype_s))
										{
											built_query_success_flag = false;
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\": \"%s\" to query", ST_PHENOTYPES_S, phenotype_s);
										}
								}		/* if (!IsStringEmpty (accession_s)) */


							if (built_query_success_flag)
								{
									/*
									 * Search with our given criteria
									 */
									if (GetMatchingStudies (query_p, data_p, job_p, format))
										{

										}

								}		/* if (built_query_success_flag) */

						}		/* if (built_query_success_flag) */

				}		/* if (query_p) */

			job_done_flag = true;
		}		/* if ((!IsStringEmpty (accession_s)) || (!IsStringEmpty (accession_s))) */

	return job_done_flag;
}


/*
 * STATIC DEFINITIONS
 */



static bool SetUpDefaultsFromExistingStudy (const Study * const study_p, char **id_ss, char **this_crop_ss, char **previous_crop_ss, char **trial_ss, char **location_ss)
{
	bool success_flag = false;
	char *study_id_s = GetBSONOidAsString (study_p -> st_id_p);

	if (study_id_s)
		{
			if (study_p -> st_location_p)
				{
					char *location_id_s = GetBSONOidAsString (study_p -> st_location_p -> lo_id_p);

					if (location_id_s)
						{
							if (study_p -> st_parent_p)
								{
									char *field_trial_id_s = GetBSONOidAsString (study_p -> st_parent_p -> ft_id_p);

									if (field_trial_id_s)
										{
											bool got_crops_flag = true;
											char *current_crop_id_s = NULL;
											char *previous_crop_id_s = NULL;

											if (study_p -> st_current_crop_p)
												{
													current_crop_id_s = GetBSONOidAsString (study_p -> st_current_crop_p -> cr_id_p);

													if (!current_crop_id_s)
														{
															got_crops_flag = false;
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get current crop id as string for study \"%s\"", study_p -> st_name_s);
														}
												}

											if (got_crops_flag)
												{
													if (study_p -> st_previous_crop_p)
														{
															previous_crop_id_s = GetBSONOidAsString (study_p -> st_previous_crop_p -> cr_id_p);

															if (!previous_crop_id_s)
																{
																	got_crops_flag = false;
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get previous crop id as string for study \"%s\"", study_p -> st_name_s);
																}
														}
												}


											if (got_crops_flag)
												{
													*id_ss = study_id_s;
													/*
													 *name_ss = study_p -> st_name_s;
													 *soil_ss = study_p -> st_soil_type_s;
													 *link_ss = study_p -> st_data_url_s;
													 *slope_ss = study_p -> st_slope_s;
													 *aspect_ss = study_p -> st_aspect_s;
													 */
													*this_crop_ss = current_crop_id_s;
													*previous_crop_ss = previous_crop_id_s;
													*trial_ss = field_trial_id_s;
													*location_ss = location_id_s;
													/*
													 *description_ss = study_p -> st_description_s;
													 *design_ss = study_p -> st_design_s;
													 *growing_conditons_ss = study_p -> st_growing_conditions_s;
													 *phenotype_gathering_notes_ss = study_p -> st_phenotype_gathering_notes_s;
													 *sowing_time_pp = study_p -> st_sowing_date_p;
													 *harvest_time_pp = study_p -> st_harvest_date_p;
													 *ph_min_pp = study_p -> st_min_ph_p;
													 *ph_max_pp = study_p -> st_max_ph_p;
													 *weather_ss = study_p -> st_weather_link_s;
													 *shape_pp = study_p -> st_shape_p;
													 *plot_horizontal_gap_pp = study_p -> st_plot_horizontal_gap_p;
													 *plot_vertical_gap_pp = study_p -> st_plot_vertical_gap_p;
													 *plots_rows_per_block_pp = study_p -> st_plots_rows_per_block_p;
													 *plots_cols_per_block_pp = study_p -> st_plots_columns_per_block_p;
													 *plot_block_horizontal_gap_pp = study_p -> st_plot_block_horizontal_gap_p;
													 *plot_block_vertical_gap_pp = study_p -> st_plot_block_vertical_gap_p;
													 */

													return true;

												}		/* if (got_crops_flag) */

											FreeBSONOidString (field_trial_id_s);
										}		/* if (field_trial_id_s) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get field trial id as string for study \"%s\"", study_p -> st_name_s);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No parent field trial for study \"%s\"", study_p -> st_name_s);
								}

							FreeBSONOidString (location_id_s);
						}		/* if (location_id_s) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get location id as string for study \"%s\"", study_p -> st_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No location for study \"%s\"", study_p -> st_name_s);
				}

			FreeBSONOidString (study_id_s);
		}		/* if (study_id_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy study id");
		}

	return success_flag;
}


static bool UpdateStudyStringValue (char **variable_ss, const ParameterSet * const param_set_p, const NamedParameterType * const param_p, ServiceJob *job_p, const char * const study_id_s)
{
	bool success_flag = true;
	const char *value_s = NULL;

	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, param_p -> npt_name_s, &value_s))
		{
			if (!ReplaceStringValue (variable_ss, value_s))
				{
					char *error_s = ConcatenateVarargsStrings ("Failed to set \"", param_p -> npt_name_s, "\" to \"", value_s, "\"", NULL);

					if (error_s)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "%s for Study \"%s\"", error_s, study_id_s);
							AddParameterErrorMessageToServiceJob (job_p, param_p -> npt_name_s, param_p -> npt_type, error_s);
							FreeCopiedString (error_s);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "%s for Study \"%s\"", error_s, study_id_s);
							AddParameterErrorMessageToServiceJob (job_p, param_p -> npt_name_s, param_p -> npt_type, error_s);
						}

					success_flag = false;
				}
		}

	return success_flag;
}


static bool UpdateStudyUnsignedIntValue (uint32 **variable_pp, const ParameterSet * const param_set_p, const NamedParameterType * const param_p, ServiceJob *job_p, const char * const study_id_s)
{
	bool success_flag = true;
	const uint32 *value_p = NULL;

	if (GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, param_p -> npt_name_s, &value_p))
		{
			*variable_pp = value_p;
		}

	return success_flag;
}


static bool UpdateStudyDoubleValue (double64 **variable_pp, const ParameterSet * const param_set_p, const NamedParameterType * const param_p, ServiceJob *job_p, const char * const study_id_s)
{
	bool success_flag = true;
	const double64 *value_p = NULL;

	if (GetCurrentDoubleParameterValueFromParameterSet (param_set_p, param_p -> npt_name_s, &value_p))
		{
			*variable_pp = value_p;
		}

	return success_flag;
}


static bool UpdateStudyJSONValue (json_t **variable_pp, const ParameterSet * const param_set_p, const NamedParameterType * const param_p, ServiceJob *job_p, const char * const study_id_s)
{
	bool success_flag = true;
	const json_t *value_p = NULL;

	if (GetCurrentJSONParameterValueFromParameterSet (param_set_p, param_p -> npt_name_s, &value_p))
		{
			if (*variable_pp)
				{
					json_decref (*variable_pp);
				}

			*variable_pp = (json_t *) value_p;
		}

	return success_flag;
}



static bool AddStudy (ServiceJob *job_p, ParameterSet *param_set_p, FieldTrialServiceData *data_p, User *user_p)
{
	OperationStatus status = OS_FAILED;
	const char *study_id_s = NULL;
	const char *name_s = NULL;
	bson_oid_t *study_id_p = NULL;
	Study *study_p = NULL;
	Location *location_p = NULL;
	FieldTrial *trial_p = NULL;
	bool study_freed_flag = false;
	bool min_req_flag = false;

	/* Are we editing an existing study? */
	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_ID.npt_name_s, &study_id_s))
		{
			if (strcmp (S_EMPTY_LIST_OPTION_S, study_id_s) != 0)
				{
					study_p = GetStudyByIdString (study_id_s, VF_CLIENT_FULL, data_p);

					if (study_p)
						{
							min_req_flag = true;
						}		/* if (active_study_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find existing study with id \"%s\"", study_id_s);
							AddParameterErrorMessageToServiceJob (job_p, STUDY_ID.npt_name_s, STUDY_ID.npt_type, "No existing study");
						}

				}		/* if (strcmp (S_EMPTY_LIST_OPTION_S, study_id_s) != 0) */

		}		/* if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_ID.npt_name_s, &study_id_s)) */


	if (!min_req_flag)
		{
			if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_NAME.npt_name_s, &name_s))
				{
					const char *parent_field_trial_id_s = NULL;

					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_FIELD_TRIALS_LIST.npt_name_s, &parent_field_trial_id_s))
						{
							if (parent_field_trial_id_s)
								{
									trial_p = GetUniqueFieldTrialBySearchString (parent_field_trial_id_s, VF_STORAGE, data_p);

									if (trial_p)
										{
											const char *location_s = NULL;

											if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_LOCATIONS_LIST.npt_name_s, &location_s))
												{
													if (location_s)
														{
															location_p = GetUniqueLocationBySearchString (location_s, VF_STORAGE, data_p);

															if (location_p)
																{
																	min_req_flag = true;
																}		/* if (location_p) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find Location named \"%s\"", location_s);
																}

														}		/* if (location_id_s) */



												}		/* if (GetCurrentParameterValueFromParameterSet (param_set_p, STUDY_LOCATIONS_LIST.npt_name_s, &name_value)) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get study parameter %s", STUDY_LOCATIONS_LIST.npt_name_s);
												}


										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find Field Trial named \"%s\"", parent_field_trial_id_s);
										}

								}		/* if (parent_field_trial_id_s) */


						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get study parameter %s", STUDY_FIELD_TRIALS_LIST.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get study parameter %s", STUDY_NAME.npt_name_s);
				}

		}		/* if (!min_req_flag) */


	if (min_req_flag)
		{
			size_t num_treatment_names = 0;
			size_t num_treatment_levels = 0;
			Parameter *treatment_names_p = ProcessTreatmentNames (param_set_p, &num_treatment_names);
			Parameter *treatment_levels_p = ProcessTreatmentLevels (param_set_p, &num_treatment_levels);

			if (num_treatment_levels == num_treatment_names)
				{
					const char *notes_s = NULL;

					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_DESCRIPTION.npt_name_s, &notes_s))
						{
							Crop *current_crop_p = NULL;
							const char *crop_s = S_UNKNOWN_CROP_OPTION_S;

							GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_THIS_CROP.npt_name_s, &crop_s);

							if ((strcmp (crop_s, S_UNKNOWN_CROP_OPTION_S) == 0) || (GetValidCrop (crop_s, &current_crop_p, data_p)))
								{
									Crop *previous_crop_p = NULL;

									crop_s = S_UNKNOWN_CROP_OPTION_S;

									GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_PREVIOUS_CROP.npt_name_s, &crop_s);

									if ((strcmp (crop_s, S_UNKNOWN_CROP_OPTION_S) == 0) || (GetValidCrop (crop_s, &previous_crop_p, data_p)))
										{
											Person *curator_p = NULL;

											if (GetPersonFromParameters (&curator_p, param_set_p, STUDY_CURATOR_NAME.npt_name_s, STUDY_CURATOR_EMAIL.npt_name_s,
																									 STUDY_CURATOR_ROLE.npt_name_s, STUDY_CURATOR_AFFILATION.npt_name_s,
																									 STUDY_CURATOR_ORCID.npt_name_s))
												{
													Person *contact_p = NULL;

													if (GetPersonFromParameters (&contact_p, param_set_p, STUDY_CONTACT_NAME.npt_name_s, STUDY_CONTACT_EMAIL.npt_name_s,
																											 STUDY_CONTACT_ROLE.npt_name_s, STUDY_CONTACT_AFFILATION.npt_name_s,
																											 STUDY_CONTACT_ORCID.npt_name_s))
														{
															const char *aspect_s = NULL;
															const char *slope_s = NULL;
															const char *data_link_s = NULL;
															const char *design_s = NULL;
															const char *growing_conditions_s = NULL;
															const char *phenotype_notes_s = NULL;
															const char *plan_changes_s = NULL;
															const char *samples_collected_s = NULL;
															const char *data_not_included_s = NULL;
															const uint32 *num_rows_p = NULL;
															const uint32 *num_cols_p = NULL;
															const uint32 *num_replicates_p = NULL;
															const double64 *plot_width_p = NULL;
															const double64 *plot_length_p = NULL;
															const char *weather_s = NULL;
															const json_t *shape_p = NULL;

															const double64 *plot_horizontal_gap_p = NULL;
															const double64 *plot_vertical_gap_p = NULL;
															const uint32 *plots_rows_per_block_p = NULL;
															const uint32 *plots_columns_per_block_p = NULL;
															const double64 *plot_block_horizontal_gap_p = NULL;
															const double64 *plot_block_vertical_gap_p = NULL;
															const uint32 *sowing_year_p = NULL;
															const uint32 *harvest_year_p = NULL;

															const char *photo_s = NULL;
															const char *image_collection_notes_s = NULL;
															const char *gps_notes_s = NULL;

															PermissionsGroup *perms_group_p = NULL;
															Metadata *metadata_p = NULL;

															if (GetInitialisedPermissionsGroupAndMetadata (&perms_group_p, &metadata_p, param_set_p, job_p, user_p, & (data_p -> dftsd_base_data)))
																{
																	if (study_p)
																		{
																			size_t num_successes = 0;
																			size_t num_errors = 0;

																			UpdateStudyStringValue (& (study_p -> st_name_s), param_set_p, &STUDY_NAME, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_description_s), param_set_p, &STUDY_DESCRIPTION, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_aspect_s), param_set_p, &STUDY_ASPECT, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_data_url_s), param_set_p, &STUDY_LINK, job_p, study_id_s) ? ++ num_successes : ++ num_errors;

																			UpdateStudyStringValue (& (study_p -> st_plan_changes_s), param_set_p, &STUDY_PLAN_CHANGES, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_physical_samples_collected_s), param_set_p, &STUDY_PHYSICAL_SAMPLES_COLLECTED, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_data_not_included_s), param_set_p, &STUDY_DATA_NOT_INCLUDED, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_design_s), param_set_p, &STUDY_DESIGN, job_p, study_id_s) ? ++ num_successes : ++ num_errors;

																			UpdateStudyStringValue (& (study_p -> st_growing_conditions_s), param_set_p, &STUDY_GROWING_CONDITIONS, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_phenotype_gathering_notes_s), param_set_p, &STUDY_PHENOTYPE_GATHERING_NOTES, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_weather_link_s), param_set_p, &STUDY_WEATHER_LINK, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_shape_notes_s), param_set_p, &STUDY_SHAPE_NOTES, job_p, study_id_s) ? ++ num_successes : ++ num_errors;


																			UpdateStudyStringValue (& (study_p -> st_photo_url_s), param_set_p, &STUDY_PHOTO, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyStringValue (& (study_p -> st_image_collection_notes_s), param_set_p, &STUDY_IMAGE_NOTES, job_p, study_id_s) ? ++ num_successes : ++ num_errors;


																			UpdateStudyUnsignedIntValue (& (study_p -> st_predicted_sowing_year_p), param_set_p, &STUDY_SOWING_YEAR, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyUnsignedIntValue (& (study_p -> st_predicted_harvest_year_p), param_set_p, &STUDY_HARVEST_YEAR, job_p, study_id_s) ? ++ num_successes : ++ num_errors;

																			UpdateStudyUnsignedIntValue (& (study_p -> st_predicted_sowing_year_p), param_set_p, &STUDY_NUM_PLOT_ROWS, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyUnsignedIntValue (& (study_p -> st_predicted_harvest_year_p), param_set_p, &STUDY_NUM_PLOT_COLS, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyUnsignedIntValue (& (study_p -> st_predicted_sowing_year_p), param_set_p, &STUDY_NUM_REPLICATES, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyUnsignedIntValue (& (study_p -> st_predicted_harvest_year_p), param_set_p, &STUDY_PLOT_ROWS_PER_BLOCK, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyUnsignedIntValue (& (study_p -> st_predicted_sowing_year_p), param_set_p, &STUDY_PLOT_COLS_PER_BLOCK, job_p, study_id_s) ? ++ num_successes : ++ num_errors;


																			UpdateStudyDoubleValue (& (study_p -> st_default_plot_width_p), param_set_p, &STUDY_PLOT_WIDTH, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyDoubleValue (& (study_p -> st_default_plot_length_p), param_set_p, &STUDY_PLOT_LENGTH, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyDoubleValue (& (study_p -> st_plot_horizontal_gap_p), param_set_p, &STUDY_PLOT_HGAP, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyDoubleValue (& (study_p -> st_plot_vertical_gap_p), param_set_p, &STUDY_PLOT_VGAP, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyDoubleValue (& (study_p -> st_plot_block_horizontal_gap_p), param_set_p, &STUDY_PLOT_BLOCK_HGAP, job_p, study_id_s) ? ++ num_successes : ++ num_errors;
																			UpdateStudyDoubleValue (& (study_p -> st_plot_block_vertical_gap_p), param_set_p, &STUDY_PLOT_BLOCK_VGAP, job_p, study_id_s) ? ++ num_successes : ++ num_errors;


																			UpdateStudyJSONValue (& (study_p -> st_shape_p), param_set_p, &STUDY_SHAPE_DATA, job_p, study_id_s) ? ++ num_successes : ++ num_errors;

																			if (num_errors == 0)
																				{
																					status = SaveStudy (study_p, job_p, data_p, NULL);

																					if (status == OS_FAILED)
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to save Study named \"%s\"", name_s);
																						}

																				}
																			else
																				{
																					if (num_successes > 0)
																						{
																							status = OS_PARTIALLY_SUCCEEDED;
																						}
																					else
																						{
																							status = OS_FAILED;
																						}

																					FreeStudy (study_p);
																					study_p = NULL;
																				}


																		}
																	else
																		{
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_ASPECT.npt_name_s, &aspect_s);
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_SLOPE.npt_name_s, &slope_s);
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_LINK.npt_name_s, &data_link_s);

																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_PLAN_CHANGES.npt_name_s, &plan_changes_s);
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_PHYSICAL_SAMPLES_COLLECTED.npt_name_s, &samples_collected_s);
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_DATA_NOT_INCLUDED.npt_name_s, &data_not_included_s);

																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_DESIGN.npt_name_s, &design_s);
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_GROWING_CONDITIONS.npt_name_s, &growing_conditions_s);
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_PHENOTYPE_GATHERING_NOTES.npt_name_s, &phenotype_notes_s);


																			GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, STUDY_SOWING_YEAR.npt_name_s, &sowing_year_p);
																			GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, STUDY_HARVEST_YEAR.npt_name_s, &harvest_year_p);


																			GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, STUDY_NUM_PLOT_ROWS.npt_name_s, &num_rows_p);
																			GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, STUDY_NUM_PLOT_COLS.npt_name_s, &num_cols_p);
																			GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, STUDY_NUM_REPLICATES.npt_name_s, &num_replicates_p);
																			GetCurrentDoubleParameterValueFromParameterSet (param_set_p, STUDY_PLOT_WIDTH.npt_name_s, &plot_width_p);
																			GetCurrentDoubleParameterValueFromParameterSet (param_set_p, STUDY_PLOT_LENGTH.npt_name_s, &plot_length_p);


																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_WEATHER_LINK.npt_name_s, &weather_s);

																			GetCurrentJSONParameterValueFromParameterSet (param_set_p, STUDY_SHAPE_DATA.npt_name_s, &shape_p);
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_SHAPE_NOTES.npt_name_s, &gps_notes_s);

																			GetCurrentDoubleParameterValueFromParameterSet (param_set_p, STUDY_PLOT_HGAP.npt_name_s, &plot_horizontal_gap_p);
																			GetCurrentDoubleParameterValueFromParameterSet (param_set_p, STUDY_PLOT_VGAP.npt_name_s, &plot_vertical_gap_p);
																			GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, STUDY_PLOT_ROWS_PER_BLOCK.npt_name_s, &plots_rows_per_block_p);
																			GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, STUDY_PLOT_COLS_PER_BLOCK.npt_name_s, &plots_columns_per_block_p);
																			GetCurrentDoubleParameterValueFromParameterSet (param_set_p, STUDY_PLOT_BLOCK_HGAP.npt_name_s, &plot_block_horizontal_gap_p);
																			GetCurrentDoubleParameterValueFromParameterSet (param_set_p, STUDY_PLOT_BLOCK_VGAP.npt_name_s, &plot_block_vertical_gap_p);

																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_PHOTO.npt_name_s, &photo_s);
																			GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_IMAGE_NOTES.npt_name_s, &image_collection_notes_s);

																			study_p = AllocateStudy (study_id_p, metadata_p, name_s, data_link_s, aspect_s,
																															 slope_s, location_p, trial_p, MF_SHALLOW_COPY, current_crop_p, previous_crop_p,
																															 notes_s, design_s,
																															 growing_conditions_s, phenotype_notes_s,
																															 num_rows_p, num_cols_p, num_replicates_p, plot_width_p, plot_length_p,
																															 weather_s, shape_p,
																															 plot_horizontal_gap_p, plot_vertical_gap_p, plots_rows_per_block_p, plots_columns_per_block_p,
																															 plot_block_horizontal_gap_p, plot_block_vertical_gap_p,
																															 curator_p, contact_p,
																															 sowing_year_p, harvest_year_p,
																															 plan_changes_s, samples_collected_s, data_not_included_s,
																															 photo_s, image_collection_notes_s,
																															 gps_notes_s,
																															 data_p);
																		}


																	/*
																	 * Now we have the Study, lets process its remaining metadata
																	 */
																	if (study_p)
																		{
																			OperationStatus perms_status = RunForPermissionEditor (param_set_p, metadata_p -> me_permissions_p, job_p, user_p, & (data_p -> dftsd_base_data));

																			if ((perms_status == OS_SUCCEEDED) || (perms_status == OS_PARTIALLY_SUCCEEDED) || (perms_status == OS_IDLE))
																				{
																					if (AddTreatmentFactorsToStudy (study_p, treatment_names_p, treatment_levels_p, num_treatment_levels, job_p, data_p))
																						{
																							status = ProcessPeople (job_p, param_set_p, ProcessPersonForStudy, study_p, data_p);

																							if (status == OS_SUCCEEDED)
																								{
																									status = ProcessMeasuredVariables (job_p, param_set_p, study_p, data_p);

																									if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED))
																										{
																											OperationStatus s = SaveStudy (study_p, job_p, data_p, NULL);

																											if ((s == OS_SUCCEEDED) || (s == OS_PARTIALLY_SUCCEEDED))
																												{
																													if (param_set_p -> ps_current_level == PL_WIZARD)
																														{
																															/*
																															 * If the study doesn't currently have plots and rows
																															 * and columns have been specified, we generate them here.
																															 */

																															if (num_rows_p && num_cols_p)
																																{
																																	const uint32 num_rows = *num_rows_p;
																																	const uint32 num_cols = *num_cols_p;

																																	if ((num_rows > 0) && (num_cols > 0))
																																		{
																																			/*
																																			 * Check that the study doesn't already have plots
																																			 */
																																			int64 num_existing_plots = GetNumberOfPlotsInStudy (study_p, data_p);

																																			if (num_existing_plots == 0)
																																				{
																																					status = GenerateAndAddSkeletonPlotsToStudy (study_p, num_rows, num_cols, job_p, data_p);
																																				}
																																			else
																																				{
																																					char *error_s = ConcatenateVarargsStrings ("Study ", study_p -> st_name_s, " already has plots", NULL);

																																					if (error_s)
																																						{
																																							AddGeneralErrorMessageToServiceJob (job_p, error_s);
																																							FreeCopiedString (error_s);
																																						}
																																					else
																																						{
																																							AddGeneralErrorMessageToServiceJob (job_p, "Study already has plots");
																																						}

																																					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Study \"%s\" already has plots", study_p -> st_name_s);
																																				}

																																		}		/* if ((num_rows > 0) && (num_cols > 0)) */

																																}		/* if (num_rows_p && num_cols_p) */

																														}		/* if (param_set_p -> ps_current_level == PL_WIZARD) */

																												}		/* if ((s == OS_SUCCEEDED) || (s == OS_PARTIALLY_SUCCEEDED)) */

																											if (s == OS_FAILED)
																												{
																													status = s;
																													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to save Study named \"%s\"", name_s);
																												}

																										}

																								}

																						}		/* if (AddTreatmentFactorsToStudy (study_p, treatment_names_p, treatment_levels_p, num_treatment_levels, job_p, data_p)) */
																					else
																						{
																							status = OS_FAILED_TO_START;
																						}

																				}		/*  if ((perms_status == OS_SUCCEEDED) || (perms_status == OS_PARTIALLY_SUCCEEDED) || (perms_status == OS_IDLE)) */



																			FreeStudy (study_p);
																			study_freed_flag = true;
																		}		/* if (study_p) */





																	if (!study_freed_flag)
																		{
																			if (contact_p)
																				{
																					FreePerson (contact_p);
																				}
																		}

																}		/* if (GetPersonFromParameters (&contact_p, param_set_p, STUDY_CONTACT_NAME.npt_name_s, STUDY_CONTACT_EMAIL.npt_name_s)) */

														}		/* if (GetInitialisedPermissionsGroupAndMetadata (&perms_group_p, &metadata_p, param_set_p, job_p, user_p, & (data_p -> dftsd_base_data))) */


													if (!study_freed_flag)
														{
															if (curator_p)
																{
																	FreePerson (curator_p);
																}
														}

												}		/* if (GetPersonFromParameters (&curator_p, param_set_p, STUDY_CURATOR_NAME.npt_name_s, STUDY_CURATOR_EMAIL.npt_name_s)) */

											if (!study_freed_flag)
												{
													if (previous_crop_p)
														{
															FreeCrop (previous_crop_p);
														}
												}
										}		/* if (GetValidCrop (previous_crop_value.st_string_value_s, &previous_crop_p)) */


									if (!study_freed_flag)
										{
											if (current_crop_p)
												{
													FreeCrop (current_crop_p);
												}
										}
								}		/* if (GetValidCrop (current_crop_value.st_string_value_s, &current_crop_p)) */


						}		/* if (GetCurrentParameterValueFromParameterSet (param_set_p, STUDY_NOTES.npt_name_s, &notes_value)) */
					else
						{

						}

				}		/* if (num_treatment_levels == num_treatment_names) */
			else
				{
					const char * const error_prefix_s = "Need equal number of treatment names and sets of factors";
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "%s: " UINT32_FMT " names and " UINT32_FMT " levels", error_prefix_s, num_treatment_names, num_treatment_levels);

					AddParameterErrorMessageToServiceJob (job_p, TFJ_TREATMENT_NAME.npt_name_s, TFJ_TREATMENT_NAME.npt_type, error_prefix_s);
				}

		}		/* if (min_req_flag) */
	else
		{

		}

	if (!study_freed_flag)
		{
			if (location_p)
				{
					FreeLocation (location_p);
				}

			if (trial_p)
				{
					FreeFieldTrial (trial_p);
				}
		}




	SetServiceJobStatus (job_p, status);

	return ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED));
}



bool AddStudyLevelDetailParameter (ParameterSet *param_set_p, ParameterGroup *group_p, ServiceData * data_p)
{
	bool success_flag = false;

	Parameter *param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, STUDY_DETAIL_LEVEL.npt_type, STUDY_DETAIL_LEVEL.npt_name_s, "Study Detail Level", "The level of detail to return for matching Studies", NULL, PL_ADVANCED);

	if (param_p)
		{
			if (CreateAndAddStringParameterOption (param_p, S_DETAIL_LEVEL_IDS_S, S_DETAIL_LEVEL_IDS_S))
				{
					if (CreateAndAddStringParameterOption (param_p, S_DETAIL_LEVEL_METADATA_S, S_DETAIL_LEVEL_METADATA_S))
						{
							if (CreateAndAddStringParameterOption (param_p, S_DETAIL_LEVEL_FULL_S, S_DETAIL_LEVEL_FULL_S))
								{
									success_flag = true;
								}

						}

				}

		}

	return success_flag;
}


bool GetStudyLevelDetailParameterValue (ParameterSet *param_set_p, ViewFormat *format_p)
{
	const char *level_s = NULL;
	bool success_flag = false;


	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_DETAIL_LEVEL.npt_name_s, &level_s))
		{
			if (!IsStringEmpty (level_s))
				{
					if (strcmp (level_s, S_DETAIL_LEVEL_FULL_S) == 0)
						{
							*format_p = VF_CLIENT_FULL;
							success_flag = true;
						}
					else if (strcmp (level_s, S_DETAIL_LEVEL_METADATA_S) == 0)
						{
							*format_p = VF_CLIENT_MINIMAL;
							success_flag = true;
						}
					if (strcmp (level_s, S_DETAIL_LEVEL_IDS_S) == 0)
						{
							*format_p = VF_REFERENCE;
							success_flag = true;
						}
				}
		}

	return success_flag;
}



static Parameter *ProcessTreatmentNames (ParameterSet *param_set_p, size_t *num_names_p)
{
	size_t num_treatment_names = 0;
	Parameter *treatment_names_p = GetParameterFromParameterSetByName (param_set_p, TFJ_TREATMENT_NAME.npt_name_s);

	if (treatment_names_p)
		{
			if (IsStringArrayParameter (treatment_names_p))
				{
					size_t i;
					StringArrayParameter *tf_names_p = (StringArrayParameter *) treatment_names_p;
					size_t num_null_treatment_names = 0;

					num_treatment_names = GetNumberOfStringArrayCurrentParameterValues (tf_names_p);

					for (i = 0; i < num_treatment_names; ++ i)
						{
							const char *value_s = GetStringArrayParameterCurrentValueAtIndex (tf_names_p, i);

							if (value_s == NULL)
								{
									++ num_null_treatment_names;
								}
						}

					if (num_null_treatment_names == num_treatment_names)
						{
							num_treatment_names = 0;
						}
				}
			else if (IsStringParameter (treatment_names_p))
				{
					StringParameter *tf_names_p = (StringParameter *) treatment_names_p;
					const char *value_s = GetStringParameterCurrentValue (tf_names_p);

					if (value_s != NULL)
						{
							num_treatment_names = 1;
						}
				}

		}

	*num_names_p = num_treatment_names;

	return treatment_names_p;
}


static Parameter *ProcessTreatmentLevels (ParameterSet *param_set_p, size_t *num_levels_p)
{
	size_t num_treatment_levels = 0;
	Parameter *treatment_levels_p = GetParameterFromParameterSetByName (param_set_p, TFJ_VALUES.npt_name_s);

	if (treatment_levels_p)
		{
			JSONParameter *tf_levels_p = (JSONParameter *) treatment_levels_p;
			const json_t *levels_json_p = GetJSONParameterCurrentValue (tf_levels_p);

			if (levels_json_p)
				{
					if (json_is_array (levels_json_p))
						{
							size_t i = 0;
							size_t num_null_treatment_levels = 0;
							num_treatment_levels = json_array_size (levels_json_p);

							for (i = 0; i < num_treatment_levels; ++ i)
								{
									const json_t *level_json_p = json_array_get (levels_json_p, i);

									if ((level_json_p == NULL) || (level_json_p == json_null ()))
										{
											++ num_null_treatment_levels;
										}
								}

							if (num_null_treatment_levels == num_treatment_levels)
								{
									num_treatment_levels = 0;
								}
						}
				}
		}

	*num_levels_p = num_treatment_levels;

	return treatment_levels_p;
}



bool AddStudyToServiceJob (ServiceJob *job_p, Study *study_p, const ViewFormat format, JSONProcessor *processor_p, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	json_t *study_json_p = GetStudyAsJSON (study_p, format, processor_p, data_p);

	if (study_json_p)
		{
			char *title_s = GetStudyAsString (study_p);

			if (title_s)
				{
					if (AddContext (study_json_p))
						{
							json_t *dest_record_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, title_s, study_json_p);

							if (dest_record_p)
								{
									AddImage (dest_record_p, DFTD_STUDY, data_p);

									if (AddResultToServiceJob (job_p, dest_record_p))
										{
											success_flag = true;
										}
									else
										{
											json_decref (dest_record_p);
										}

								}		/* if (dest_record_p) */

						}		/* if (AddContext (trial_json_p)) */

					FreeCopiedString (title_s);
				}		/* if (title_s) */

			json_decref (study_json_p);
		}		/* if (study_json_p) */

	return success_flag;
}


json_t *GetAllStudiesAsJSON (const FieldTrialServiceData *data_p, bool full_data_flag)
{
	json_t *results_p = NULL;

	if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_STUDY]))
		{
			bson_t *query_p = NULL;
			bson_t *opts_p = NULL;

			if (full_data_flag)
				{
					opts_p = BCON_NEW ("sort", "{", ST_NAME_S, BCON_INT32 (1), "}");
				}
			else
				{
					opts_p = BCON_NEW ("projection", "{", ST_NAME_S, BCON_BOOL (true), "}",
														 "sort", "{", ST_NAME_S, BCON_INT32 (1), "}");
				}

			results_p = GetAllMongoResultsAsJSON (data_p -> dftsd_mongo_p, query_p, opts_p);

			if (opts_p)
				{
					bson_destroy (opts_p);
				}
		}

	return results_p;
}


json_t *GetAllStudyIds (Service *service_p)
{
	json_t *id_results_p = NULL;
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_STUDY]))
		{
			bson_t query;
			const char *fields_ss [] = { MONGO_ID_S, NULL };

			bson_init (&query);

			if (FindMatchingMongoDocumentsByBSON (data_p -> dftsd_mongo_p, &query, fields_ss, NULL))
				{
					id_results_p = GetAllExistingMongoResultsAsJSON (data_p -> dftsd_mongo_p);

					if (!id_results_p)
						{

						}		/* if (id_results_p) */

				}		/* if (FindMatchingMongoDocumentsByBSON (data_p -> dftsd_mongo_p, NULL, fields_ss, NULL)) */

		}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_STUDY])) */

	return id_results_p;
}



json_t *GetStudyDistinctPhenotypesAsJSON (bson_oid_t *study_id_p, const FieldTrialServiceData *data_p)
{
	json_t *phenotypes_p = NULL;
	char *key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", SR_OBSERVATIONS_S, ".", OB_PHENOTYPE_ID_S, NULL);

	if (key_s)
		{
			phenotypes_p = GetDistinctValuesAsJSON (study_id_p, key_s, AddFullPhenotypeAsJSON /*  AddPhenotype */, data_p);

			FreeCopiedString (key_s);
		}		/* if (key_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConcatenateVarargsStrings () failed for \"%s\", \"%s\", \"%s\"", PL_ROWS_S, SR_OBSERVATIONS_S, OB_PHENOTYPE_ID_S);
		}

	return phenotypes_p;
}


json_t *GetStudyDistinctPhenotypesAsFrictionlessDataJSON (bson_oid_t *study_id_p, const FieldTrialServiceData *data_p)
{
	json_t *phenotypes_p = NULL;
	char *key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", SR_OBSERVATIONS_S, ".", OB_PHENOTYPE_ID_S, NULL);

	if (key_s)
		{
			phenotypes_p = GetDistinctValuesAsJSON (study_id_p, key_s, AddPhenotypeAsFrictionlessData, data_p);

			FreeCopiedString (key_s);
		}		/* if (key_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConcatenateVarargsStrings () failed for \"%s\", \"%s\", \"%s\"", PL_ROWS_S, SR_OBSERVATIONS_S, OB_PHENOTYPE_ID_S);
		}

	return phenotypes_p;
}



static bool AddPhenotypeAsFrictionlessData (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	bson_oid_t *phenotype_id_p = GetBSONOidFromString (oid_s);

	if (phenotype_id_p)
		{
			MeasuredVariable *variable_p = GetMeasuredVariableById (phenotype_id_p, data_p);

			if (variable_p)
				{
					const char *name_s = NULL;

					if (variable_p -> mv_variable_term_p)
						{
							name_s = variable_p -> mv_variable_term_p -> st_name_s;

							if (name_s)
								{
									if (variable_p -> mv_trait_term_p)
										{
											const char *title_s = variable_p -> mv_trait_term_p -> st_name_s;

											if (title_s)
												{
													const char *description_s = variable_p -> mv_trait_term_p -> st_description_s;
													json_t *phenotype_fd_p = json_object ();

													if (phenotype_fd_p)
														{
															if (SetJSONString (phenotype_fd_p, FD_TABLE_FIELD_NAME, name_s))
																{
																	if (SetJSONString (phenotype_fd_p, FD_TABLE_FIELD_TITLE, title_s))
																		{
																			if ((!description_s) || (SetJSONString (phenotype_fd_p, FD_TABLE_FIELD_DESCRIPTION, description_s)))
																				{
																					if (SetJSONString (phenotype_fd_p, FD_TABLE_FIELD_TYPE, "string"))
																						{
																							if (json_array_append_new (values_p, phenotype_fd_p) == 0)
																								{
																									success_flag = true;
																								}
																						}

																				}		/* if ((!description_s) || (SetJSONString (phenotype_fd_p, FD_TABLE_FIELD_DESCRIPTION, description_s))) */

																		}		/* if (SetJSONString (phenotype_fd_p, FD_TABLE_FIELD_TITLE, title_s)) */

																}		/* if (SetJSONString (phenotype_fd_p, FD_TABLE_FIELD_NAME, url_s)) */

															if (!success_flag)
																{
																	json_decref (phenotype_fd_p);
																}

														}		/* if (phenotype_fd_p) */

												}		/* if (title_s) */

										}		/* if (variable_p -> mv_trait_term_p) */

								}		/* if (name_s) */

						}		/* if (variable_p -> mv_variable_term_p */

					FreeMeasuredVariable (variable_p);
				}		/* if (variable_p) */

			FreeBSONOid (phenotype_id_p);
		}		/* if (phenotype_id_p) */

	return success_flag;
}		/* if (oid_s) */



OperationStatus GenerateStatisticsForAllStudies (ServiceJob *job_p,  FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	json_t *all_studies_p = GetAllStudiesAsJSON (data_p, true);

	if (all_studies_p)
		{
			size_t i;
			json_t *study_json_p;
			const size_t num_studies = json_array_size (all_studies_p);
			size_t num_successes = 0;

			char *stats_filename_s = MakeFilename (data_p -> dftsd_study_cache_path_s, "stats");

			if (stats_filename_s)
				{
					json_array_foreach (all_studies_p, i, study_json_p)
						{
							Study *study_p = GetStudyFromJSON (study_json_p, VF_STORAGE, data_p);

							if (study_p)
								{
									OperationStatus stats_status = GenerateStatisticsForStudy (study_p, job_p, data_p);

									FILE *stats_f = fopen (stats_filename_s, "a");
									if (stats_f)
										{
											char *id_s = GetBSONOidAsString (study_p->st_id_p);
											int64 num_plots = GetNumberOfPlotsInStudy (study_p, data_p);

											fprintf (stats_f, "\"%s\" %s %" PRId64 "\n", study_p->st_name_s, id_s ? id_s : "_", num_plots);

											if (id_s)
												{
													FreeBSONOidString (id_s);
												}

											fclose (stats_f);
										}


									if (stats_status == OS_SUCCEEDED)
										{
											++num_successes;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GenerateStatisticsForStudy () failed for \"%s\"", study_p->st_name_s);
										}

									FreeStudy (study_p);
								}		/* if (study_p) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "GetStudyFromJSON () failed");
								}

						}		/* json_array_foreach (all_studies_p, i, study_json_p) */


					FreeCopiedString (stats_filename_s);
				}


			if (num_successes == num_studies)
				{
					status = OS_SUCCEEDED;
				}
			else if (num_successes > 0)
				{
					status = OS_PARTIALLY_SUCCEEDED;
				}

			json_decref (all_studies_p);
		}



	MergeServiceJobStatus (job_p, status);

	return status;
}


OperationStatus GenerateStatisticsForStudy (Study *study_p, ServiceJob *job_p,  FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_IDLE;

	/* Does the study already have its plots? */
	if (study_p -> st_plots_p -> ll_size == 0)
		{
			if (!GetStudyPlots (study_p, VF_STORAGE, data_p))
				{
					status = OS_FAILED;
				}

		}		/*( if (study_p -> st_plots_p -> ll_size == 0) */

	if (status == OS_IDLE)
		{
			/* Does the study have any plots? */
			if (study_p -> st_plots_p -> ll_size > 0)
				{
					status = CalculateStudyStatistics (study_p, data_p);

					if (status == OS_SUCCEEDED)
						{
							OperationStatus old_status = job_p -> sj_status;

							status = SaveStudy (study_p, job_p, data_p, NULL);

							if (status != OS_SUCCEEDED)
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SaveStudy () for \"%s\" had status %d", study_p -> st_name_s, status);
								}


							MergeServiceJobStatus (job_p, old_status);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CalculateStudyStatistics () failed for \"%s\"", study_p -> st_name_s);
						}
				}
			else
				{
					/* no plots so nothing to do */
					status = OS_SUCCEEDED;
				}
		}

	return status;
}


bool SaveStudyAsFrictionlessData (Study *study_p, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	char *full_study_filename_s = GetFrictionlessDataFilename (study_p -> st_name_s, data_p);

	if (full_study_filename_s)
		{
			char *id_s = GetBSONOidAsString (study_p -> st_id_p);

			if (id_s)
				{
					json_t *data_package_p = GetDataPackage (study_p -> st_name_s, study_p -> st_description_s, id_s);

					if (data_package_p)
						{
							FieldTrial *trial_p = study_p -> st_parent_p;
							json_t *trial_fd_p = GetFieldTrialAsFrictionlessDataResource (trial_p, data_p);

							if (trial_fd_p)
								{
									/*
									 * GetDataPackage () creates the resources array so we know it exists
									 */
									json_t *resources_p = json_object_get (data_package_p, FD_RESOURCES_S);

									if (json_array_append_new (resources_p, trial_fd_p) == 0)
										{
											Programme *programme_p = trial_p -> ft_parent_p;

											if (programme_p)
												{
													json_t *programme_fd_p = GetProgrammeAsFrictionlessDataResource (programme_p, data_p);

													if (programme_fd_p)
														{
															if (json_array_append_new (resources_p, programme_fd_p) == 0)
																{
																	if (GetStudyPlots (study_p, VF_CLIENT_FULL, data_p))
																		{
																			json_t *study_fd_p = GetStudyAsFrictionlessDataResource (study_p, data_p);

																			if (study_fd_p)
																				{
																					if (json_array_append_new (resources_p, study_fd_p) == 0)
																						{
																							if (study_p -> st_plots_p -> ll_size > 0)
																								{
																									json_t *plots_fd_p = GetPlotsAsFDTabularPackage (study_p, data_p);

																									if (plots_fd_p)
																										{
																											if (json_array_append_new (resources_p, plots_fd_p) == 0)
																												{
																													success_flag = true;
																												}		/* if (json_array_append_new (resources_p, plots_fd_p) == 0) */
																											else
																												{
																													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plots_fd_p, "Failed to add plots to resources array");
																													json_decref (plots_fd_p);
																												}
																										}		/* if (plots_fd_p) */
																									else
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get plots as fd for %s", study_p -> st_name_s);
																										}

																								}		/* if (study_p -> st_plots_p -> ll_size > 0) */
																							else
																								{
																									success_flag = true;
																								}


																							if (json_dump_file (data_package_p, full_study_filename_s, JSON_INDENT (2)) == 0)
																								{
																									success_flag = true;
																								}
																							else
																								{
																									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, data_package_p, "json_dump_file () failed for study fd \"%s\" to file \"%s\"", study_p -> st_name_s, full_study_filename_s);
																								}


																						}		/* if (json_array_append_new (resources_p, study_fd_p) == 0) */
																					else
																						{
																							json_decref (study_fd_p);
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add study fd to \"%s\"", study_p -> st_name_s);
																						}

																				}		/* if (study_fd_p) */
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetStudyAsFrictionlessDataResource () failed for \"%s\"", study_p -> st_name_s);
																				}

																		}		/* if (GetStudyPlots (study_p, data_p)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetStudyPlots () failed for \"%s\"", study_p -> st_name_s);
																		}

																}		/* if (json_array_append_new (resources_p, programme_fd_p) == 0) */
															else
																{
																	json_decref (programme_fd_p);
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add programme fd to \"%s\"", study_p -> st_name_s);
																}

														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get programme fd for \"%s\"", study_p -> st_name_s);
														}
												}		/* if (programme_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Trial %s has no parent programme for \"%s\"", trial_p -> ft_name_s);
												}

										}
									else
										{
											json_decref (trial_fd_p);
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add trial fd to \"%s\"", study_p -> st_name_s);
										}

								}		/* if (trial_fd_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get trial fd for \"%s\"", study_p -> st_name_s);
								}

							json_decref (data_package_p);
						}		/* if (data_package_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create data package for \"%s\"", study_p -> st_name_s);
						}

					FreeBSONOidString (id_s);
				}		/* if (id_s) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get id as string for \"%s\"", study_p -> st_name_s);
				}

			FreeCopiedString (full_study_filename_s);
		}		/* if (full_study_filename_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get full study filename for \"%s\"", study_p -> st_name_s);
		}

	return success_flag;
}



static bool AddPersonAsFrictionlessData (const Person * const person_p, json_t *json_p, const char * const name_key_s, const char * const email_key_s)
{
	bool success_flag = false;

	if (person_p)
		{
			if (SetNonTrivialString (json_p, name_key_s, person_p -> pe_name_s, false))
				{
					if (SetNonTrivialString (json_p, email_key_s, person_p -> pe_email_s, false))
						{
							success_flag = true;
						}		/* if (SetNonTrivialString (json_p, email_key_s, person_p -> pe_email_s, false)) */
					else
						{

						}

				}		/* if (SetNonTrivialString (json_p, KEY_S, study_p -> st_curator_p -> pe_name_s, false)) */
			else
				{

				}

		}		/* if (study_p -> st_curator_p) */
	else
		{
			success_flag = true;
		}

	return success_flag;
}


static bool AddCropAsFrictionlessData (const Crop * const crop_p, json_t *json_p, const char * const key_s)
{
	bool success_flag = false;

	if (crop_p)
		{
			if (SetNonTrivialString (json_p, key_s, crop_p -> cr_name_s, false))
				{
					success_flag = true;
				}		/* if (SetNonTrivialString (json_p, email_key_s, person_p -> pe_email_s, false)) */
			else
				{

				}
		}		/* if (crop_p) */
	else
		{
			success_flag = true;
		}

	return success_flag;
}




static bool AddTreatmentFactorsAsFrictionlessData (json_t *json_p, LinkedList *treatments_p, const char * const key_s)
{
	bool success_flag = false;

	if (treatments_p)
		{
			json_t *treatments_fd_p = json_array ();

			if (treatments_fd_p)
				{
					if (json_object_set_new (json_p, key_s, treatments_fd_p) == 0)
						{
							TreatmentFactorNode *node_p = (TreatmentFactorNode *) (treatments_p -> ll_head_p);

							success_flag = true;

							while (node_p && success_flag)
								{
									TreatmentFactor *tf_p = node_p -> tfn_p;
									json_t *tf_fd_p = GetTreatmentFactorAsFrictionlessData (tf_p);

									if (tf_fd_p)
										{
											if (json_array_append_new (treatments_fd_p, tf_fd_p) == 0)
												{
													node_p = (TreatmentFactorNode *) (node_p -> tfn_node.ln_next_p);
												}
											else
												{
													success_flag = false;
												}
										}
									else
										{
											success_flag = false;
										}

								}		/* while (node_p) */

						}
					else
						{
							json_decref (treatments_fd_p);
						}
				}


		}
	else
		{
			success_flag = true;
		}

	return success_flag;
}



json_t *GetStudyAsFrictionlessDataResource (const Study *study_p, const FieldTrialServiceData *data_p)
{

	/*
	 * id
	 * Name
	 * Trial
	 * Location
	 * Curator Name
	 * Curator Email
	 * Contact name
	 * contact email
	 * sowing date
	 * harvest date
	 * description
	 * design
	 * growing conditions
	 * phenotype gathering notes
	 * weather
	 * crop
	 * previous crop
	 * Link
	 * Plots GPS
	 * Plots GPS notes
	 * Aspect
	 * Slope
	 * Horiz plot gap
	 * vert plot gap
	 * plot rows per block
	 * plot cols per block
	 * horiz plot block gap
	 * vert plot block gap
	 * num plot rows
	 * num plot cols
	 * num replicates
	 * default plot width
	 * default plot length
	 * treatment factors
	 */

	json_t *study_fd_p = json_object ();

	if (study_fd_p)
		{
			bool success_flag = false;

			const char * const SCHEMA_URL_S = "https://grassroots.tools/frictionless-data/schemas/field-trials/study-resource.json";

			if (SetJSONString (study_fd_p, FD_PROFILE_S, SCHEMA_URL_S))
				{
					char *id_s = GetBSONOidAsString (study_p -> st_id_p);

					if (id_s)
						{
							if (SetJSONString (study_fd_p, FD_ID_S, id_s))
								{
									if (SetJSONString (study_fd_p, FD_NAME_S, study_p -> st_name_s))
										{
											const char *KEY_S = "field_trial";

											if ((! (study_p -> st_parent_p)) || (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_parent_p -> ft_name_s, false)))
												{
													KEY_S = "location";

													if ((! (study_p -> st_location_p)) || (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_location_p -> lo_address_p -> ad_name_s, false)))
														{
															if (AddPersonAsFrictionlessData (study_p -> st_curator_p, study_fd_p, "curator_name", "curator_email"))
																{
																	if (AddPersonAsFrictionlessData (study_p -> st_contact_p, study_fd_p, "contact_name", "contact_email"))
																		{
																			if (AddPeopleAsFrictionlessData (study_p -> st_contributors_p, "contributors", study_fd_p, data_p))
																				{
																					if (SetNonTrivialString (study_fd_p, FD_DESCRIPTION_S, study_p -> st_description_s, false))
																						{
																							KEY_S = "design";

																							if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_design_s, false))
																								{
																									KEY_S = "growing_conditions";

																									if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_growing_conditions_s, false))
																										{
																											KEY_S = "phenotype_notes";

																											if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_phenotype_gathering_notes_s, false))
																												{
																													KEY_S = "weather";

																													if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_growing_conditions_s, false))
																														{
																															if (AddCropAsFrictionlessData (study_p -> st_current_crop_p, study_fd_p, "crop"))
																																{
																																	if (AddCropAsFrictionlessData (study_p -> st_previous_crop_p, study_fd_p, "previous_crop"))
																																		{
																																			KEY_S = "link";

																																			if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_data_url_s, false))
																																				{
																																					const json_t *shape_p = study_p -> st_shape_p;

																																					if ((!shape_p) || (shape_p == json_null ()) || (json_object_set (study_fd_p, "plots_gps", study_p -> st_shape_p) == 0))
																																						{
																																							KEY_S = "plots_gps_notes";

																																							if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_shape_notes_s, false))
																																								{
																																									KEY_S = "aspect";

																																									if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_aspect_s, false))
																																										{
																																											KEY_S = "slope";

																																											if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_slope_s, false))
																																												{
																																													if (AddTreatmentFactorsAsFrictionlessData (study_fd_p, study_p -> st_treatments_p, "treatments"))
																																														{
																																															if (SetNonTrivialUnsignedInt (study_fd_p, "sowing_year", study_p -> st_predicted_sowing_year_p, false))
																																																{
																																																	if (SetNonTrivialUnsignedInt (study_fd_p, "harvest_year", study_p -> st_predicted_harvest_year_p, false))
																																																		{
																																																			const char * const PEOPLE_S = "contributors";

																																																			if (AddPeopleAsFrictionlessData (study_p -> st_contributors_p, PEOPLE_S, study_fd_p, data_p))
																																																				{
																																																					success_flag = true;
																																																				}
																																																			else
																																																				{
																																																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\" for \"%s\"", PEOPLE_S, study_p -> st_name_s);
																																																				}
																																																		}
																																																}
																																														}

																																												}
																																										}
																																								}



																																						}		/* if ((! (study_p -> st_shape_p)) || (json_object_set (study_fd_p, "plots_gps", study_p -> st_shape_p) == 0)) */
																																					else
																																						{
																																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to add the plots gps");
																																						}
																																				}
																																		}

																																}



																														}		/* if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_weather_link_s, false)) */
																													else
																														{
																															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", KEY_S, study_p -> st_weather_link_s);
																														}


																												}		/* if (SetNonTrivialString (study_fd_p, key_S, study_p -> st_phenotype_gathering_notes_s, false)) */
																											else
																												{
																													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", KEY_S, study_p -> st_phenotype_gathering_notes_s);
																												}

																										}		/* if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_growing_conditions_s, false)) */
																									else
																										{
																											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", KEY_S, study_p -> st_growing_conditions_s);
																										}

																								}		/* if (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_design_, false)) */
																							else
																								{
																									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", KEY_S, study_p -> st_design_s);
																								}


																						}		/* if (SetNonTrivialString (study_fd_p, FD_DESCRIPTION_S, study_p -> st_description_s, false)) */
																					else
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", FD_DESCRIPTION_S, study_p -> st_description_s);
																						}



																				}		/* if (AddPeopleAsFrictionlessData (study_p -> st_contributors_p, "contributors", study_fd_p, data_p)) */



																		}		/* if (AddPersonAsFrictionlessData (study_p -> st_contact_p, study_fd_p, "contact_name", "contact_email")) */

																}		/* if (AddPersonAsFrictionlessData (study_p -> st_curator_p, study_fd_p, "curator_name", "curator_email")) */

														}		/* if ((! (study_p -> st_location_p)) || (SetNonTrivialString (study_fd_p, KEY_S, study_p -> st_location_p -> lo_address_p -> ad_name_s, false))) */

												}


										}		/* if (SetJSONString (study_fd_p, FD_NAME_S, study_p -> st_name_s)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", FD_NAME_S, study_p -> st_name_s);
										}



								}		/* if (SetJSONString (study_fd_p, FD_ID_S, id_s)) */

							FreeBSONOidString (id_s);
						}		/* if (id_s) */
					else
						{

						}

				}		/* if (SetJSONString (study_fd_p, FD_PROFILE_S, SCHEMA_URL_S)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", FD_PROFILE_S, SCHEMA_URL_S);
				}


			if (!success_flag)
				{
					json_decref (study_fd_p);
					study_fd_p = NULL;
				}
		}		/* if (study_fd_p) */

	return study_fd_p;
}





json_t *GetStudyAsFrictionlessDataPackage (const Study *study_p, const FieldTrialServiceData *data_p)
{
	json_t *study_fd_p = json_object ();

	if (study_fd_p)
		{
			bool success_flag = false;

			if (SetJSONString (study_fd_p, FD_PROFILE_S, FD_PROFILE_DATA_PACKAGE_S))
				{
					if (SetJSONString (study_fd_p, FD_NAME_S, study_p -> st_name_s))
						{
							if (SetNonTrivialString (study_fd_p, FD_DESCRIPTION_S, study_p -> st_description_s, false))
								{
									char *id_s = GetBSONOidAsString (study_p -> st_id_p);

									if (id_s)
										{
											if (SetJSONString (study_fd_p, FD_ID_S, id_s))
												{
													json_t *resources_p = json_array ();

													if (resources_p)
														{
															if (json_object_set_new (study_fd_p, FD_RESOURCES_S, resources_p) == 0)
																{
																	json_t *plots_fd_p = GetPlotsAsFDTabularPackage (study_p, data_p);

																	if (plots_fd_p)
																		{
																			if (json_array_append_new (resources_p, plots_fd_p) == 0)
																				{
																					success_flag = true;
																				}		/* if (json_array_append_new (resources_p, plots_fd_p) == 0) */
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plots_fd_p, "Failed to add plots to resources array");
																					json_decref (plots_fd_p);
																				}
																		}		/* if (plots_fd_p) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get plots as fd for %s", study_p -> st_name_s);
																		}

																}		/* if (json_object_set_new (study_fd_p, FD_RESOURCES_S, resources_p) == 0)*/
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to add reources array");
																	json_decref (resources_p);
																}

														}		/* if (resources_p) */


												}		/* if (SetJSONString (study_fd_p, FD_ID_S, id_s)) */

											FreeBSONOidString (id_s);
										}		/* if (id_s) */
									else
										{

										}

								}		/* if (SetNonTrivialString (study_fd_p, FD_DESCRIPTION_S, study_p -> st_description_s, false)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", FD_DESCRIPTION_S, study_p -> st_description_s);
								}

						}		/* if (SetJSONString (study_fd_p, FD_NAME_S, study_p -> st_name_s)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", FD_NAME_S, study_p -> st_name_s);
						}

				}		/* if (SetJSONString (study_fd_p, FD_PROFILE_S, FD_PROFILE_DATA_S)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_fd_p, "Failed to set \"%s\": \"%s\"", FD_PROFILE_S, FD_PROFILE_DATA_PACKAGE_S);
				}

			if (!success_flag)
				{
					json_decref (study_fd_p);
					study_fd_p = NULL;
				}

		}		/* if (study_fd_p) */


	return study_fd_p;
}


OperationStatus CalculateStudyStatistics (Study *study_p, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED;
	size_t num_plots = study_p -> st_plots_p -> ll_size;
	StatisticsTool *stats_tool_p = AllocateStatisticsTool (num_plots);

	if (stats_tool_p)
		{
			char *key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", SR_OBSERVATIONS_S, ".", OB_PHENOTYPE_ID_S, NULL);

			if (key_s)
				{
					StudyProcessData spd;

					spd.spd_study_p = study_p;
					spd.spd_stats_tool_p = stats_tool_p;

					status = ProcessDistinctValues (study_p -> st_id_p, key_s, ProcessStudyPhenotype, &spd, service_data_p);

					FreeCopiedString (key_s);
				}		/* if (key_s) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConcatenateVarargsStrings () failed for \"%s\", \"%s\", \"%s\"", PL_ROWS_S, SR_OBSERVATIONS_S, OB_PHENOTYPE_ID_S);
				}

			FreeStatisticsTool (stats_tool_p);
		}


	return status;
}




json_t *GetAllStudyMaterialIds (const Study * const study_p, const FieldTrialServiceData *data_p)
{
	json_t *results_p = NULL;
	bson_t *query_p = bson_new ();

	if (query_p)
		{
			if (BSON_APPEND_OID (query_p, PL_PARENT_STUDY_S, study_p -> st_id_p))
				{
					char *key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", SR_MATERIAL_ID_S, NULL);

					if (key_s)
						{
							results_p = DistinctMatchingMongoDocumentsByBSON (data_p -> dftsd_mongo_p, data_p -> dftsd_database_s, data_p -> dftsd_collection_ss [DFTD_PLOT], key_s, query_p);

							FreeCopiedString (key_s);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConcatenateVarargsStrings () failed for \"%s\", \".\", \"%s\" for study \"%s\"", PL_ROWS_S, SR_MATERIAL_ID_S, study_p -> st_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "BSON_APPEND_OID () failed for \"%s\"", study_p -> st_name_s);
				}

			bson_destroy (query_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "bson_new () failed for \"%s\"", study_p -> st_name_s);
		}

	return results_p;
}



static bool ProcessStudyPhenotype (const char *phenotype_oid_s, void *user_data_p, const FieldTrialServiceData *service_data_p)
{
	bool success_flag = false;
	MeasuredVariable *phenotype_p = GetMeasuredVariableByIdString (phenotype_oid_s, service_data_p);

	if (phenotype_p)
		{
			const ScaleClass *class_p = GetMeasuredVariableScaleClass (phenotype_p);
			const char *mv_s = GetMeasuredVariableName (phenotype_p);
			Statistics *stats_p = NULL;
			PhenotypeStatisticsNode *node_p = NULL;
			StudyProcessData *spd_p = (StudyProcessData *) user_data_p;
			Study *study_p = spd_p -> spd_study_p;

			if (class_p)
				{
					/*
					 * Is it a numerical phenotype?
					 */
					if (strcmp (class_p -> sc_name_s, SCALE_NUMERICAL.sc_name_s) == 0)
						{
							StatisticsTool *stats_tool_p = spd_p -> spd_stats_tool_p;
							PlotNode *plot_node_p = (PlotNode *) (study_p -> st_plots_p -> ll_head_p);

							ResetStatisticsTool (stats_tool_p);

							while (plot_node_p)
								{
									Plot *plot_p = plot_node_p -> pn_plot_p;

									/*
									 * Are there any rows?
									 */
									if (plot_p -> pl_rows_p -> ll_size > 0)
										{
											RowNode *row_node_p = (RowNode *) (plot_p -> pl_rows_p -> ll_head_p);

											while (row_node_p)
												{
													Row *row_p = row_node_p -> rn_row_p;

													/*
													 * Is it a standard row?
													 */
													if (row_p -> ro_type == RT_STANDARD)
														{
															StandardRow *standard_row_p = (StandardRow *) row_p;

															/*
															 * Are there any observations?
															 */
															if (standard_row_p -> sr_observations_p -> ll_size > 0)
																{
																	Observation *obs_p = GetMatchingObservation (standard_row_p, phenotype_p, NULL);

																	if (obs_p)
																		{
																			if (obs_p -> ob_type == OT_NUMERIC)
																				{
																					NumericObservation *num_obs_p = (NumericObservation *) obs_p;
																					double d;
																					bool value_flag = true;

																					if (num_obs_p -> no_corrected_value_p)
																						{
																							d = * (num_obs_p -> no_corrected_value_p);
																						}
																					else if (num_obs_p -> no_raw_value_p)
																						{
																							d = * (num_obs_p -> no_raw_value_p);
																						}
																					else
																						{
																							value_flag = false;
																						}

																					if (value_flag)
																						{
																							if (!AddStatisticsValue (stats_tool_p, d))
																								{
																									char *obs_id_s = GetBSONOidAsString (obs_p -> ob_id_p);
																									const char *phenotype_s = GetMeasuredVariableName (phenotype_p);

																									if (obs_id_s)
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %lf to \"%s\" for variable \"%s\"", d, obs_id_s, phenotype_s);
																											FreeBSONOidString (obs_id_s);
																										}
																									else
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %lf for variable \"%s\"", d, phenotype_s);
																										}
																								}
																						}

																				}
																		}
																}

														}		/* if (row_p -> ro_type == RT_STANDARD) */



													row_node_p = (RowNode *) (row_node_p -> rn_node.ln_next_p);
												}		/* while (row_node_p) */

										}		/* if (plot_p -> pl_rows_p -> ll_size > 0) */

									plot_node_p = (PlotNode *) (plot_node_p -> pn_node.ln_next_p);
								}		/* while (plot_node_p) */


							/*
							 * Do we have any stats?
							 */
							if (stats_tool_p -> st_current_index > 0)
								{
									CalculateStatistics (stats_tool_p);

									stats_p = & (stats_tool_p -> st_stats);
								}

						}		/* if (strcmp (class_p -> sc_name_s, SCALE_NUMERICAL -> sc_name_s) == 0) */

				}		/* if (class_p) */


			if (AddPhenotypeStatisticsToStudy (study_p, mv_s, stats_p))
				{
					success_flag = true;
				}


			FreeMeasuredVariable (phenotype_p);
		}		/* if (phenotype_p) */

	return success_flag;
}


static OperationStatus ProcessDistinctValues (bson_oid_t *study_id_p, const char *key_s, bool (*process_value_fn) (const char *oid_s, void *user_data_p, const FieldTrialServiceData *service_data_p), void *user_data_p, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED_TO_START;

	if (SetMongoToolCollection (service_data_p -> dftsd_mongo_p, service_data_p -> dftsd_collection_ss [DFTD_PLOT]))
		{
			bson_t *command_p = BCON_NEW ("distinct",
																		BCON_UTF8 (service_data_p -> dftsd_collection_ss [DFTD_PLOT]),
																		"key",
																		BCON_UTF8 (key_s),
																		"query",
																		"{",
																		PL_PARENT_STUDY_S,
																		BCON_OID (study_id_p),
																		"}");

			if (command_p)
				{
					bson_t *reply_p = NULL;

					if (RunMongoCommand (service_data_p -> dftsd_mongo_p, command_p, &reply_p))
						{
							if (reply_p)
								{
									json_t *results_p = ConvertBSONToJSON (reply_p, NULL);

									if (results_p)
										{
											json_t *oid_values_p = json_object_get (results_p, "values");

											if (oid_values_p)
												{
													if (json_is_array (oid_values_p))
														{
															json_t *oid_value_p;
															size_t i;
															const size_t size = json_array_size (oid_values_p);
															size_t num_successes = 0;

															/*
															 * Create a stats list node for each measured variable
															 */

															json_array_foreach (oid_values_p, i, oid_value_p)
															{
																const char *oid_s = GetJSONString (oid_value_p, "$oid");

																if (oid_s)
																	{
																		if (process_value_fn (oid_s, user_data_p, service_data_p))
																			{
																				++ num_successes;
																			}
																		else
																			{
																				PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to process data for\"%s\"", oid_s);
																			}
																	}		/* if (oid_s) */

															}		/* json_array_foreach (oids_p, i, oid_p) */


															if (num_successes == size)
																{
																	status = OS_SUCCEEDED;
																}
															else if (num_successes > 0)
																{
																	status = OS_PARTIALLY_SUCCEEDED;
																}
															else
																{
																	status = OS_FAILED;
																}

														}		/* if (json_is_array (oids_p)) */

												}		/* if (oids_p) */


											json_decref (results_p);
										}		/* if (results_p) */


									bson_destroy (reply_p);
								}		/* if (reply_p) */
							else
								{
									size_t length;
									char *json_s = ConvertBSONToJSON (command_p, &length);
									if (json_s)
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RunMongoCommand had empty reply for \"%s\"", json_s);
											bson_free (json_s);
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RunMongoCommand had empty reply");
										}

								}
						}		/* if (RunMongoCommand (data_p -> dftsd_mongo_p, command_p, &reply_p)) */
					else
						{
							size_t length;
							char *json_s = ConvertBSONToJSON (command_p, &length);

							if (json_s)
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RunMongoCommand failed for \"%s\"", json_s);
									bson_free (json_s);
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RunMongoCommand failed");
								}
						}

					bson_destroy (command_p);
				}		/* if (command_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create command");
				}

		}

	return status;
}



static json_t *GetDistinctValuesAsJSON (bson_oid_t *study_id_p, const char *key_s, bool (*add_value_fn) (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p), const FieldTrialServiceData *data_p)
{
	json_t *values_p = NULL;

	if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_PLOT]))
		{
			bson_t *command_p = BCON_NEW ("distinct",
																		BCON_UTF8 (data_p -> dftsd_collection_ss [DFTD_PLOT]),
																		"key",
																		BCON_UTF8 (key_s),
																		"query",
																		"{",
																		PL_PARENT_STUDY_S,
																		BCON_OID (study_id_p),
																		"}");

			if (command_p)
				{
					bson_t *reply_p = NULL;

					if (RunMongoCommand (data_p -> dftsd_mongo_p, command_p, &reply_p))
						{
							if (reply_p)
								{
									json_t *results_p = ConvertBSONToJSON (reply_p, NULL);

									if (results_p)
										{
											json_t *oid_values_p = json_object_get (results_p, "values");

											if (oid_values_p)
												{
													if (json_is_array (oid_values_p))
														{
															values_p = json_array ();

															if (values_p)
																{
																	json_t *oid_value_p;
																	size_t i;

																	json_array_foreach (oid_values_p, i, oid_value_p)
																	{
																		const char *oid_s = GetJSONString (oid_value_p, "$oid");

																		if (oid_s)
																			{
																				if (!add_value_fn (oid_s, values_p, data_p))
																					{
																						PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add data for\"%s\"", oid_s);
																					}
																			}		/* if (oid_s) */

																	}		/* json_array_foreach (oids_p, i, oid_p) */

																}		/* if (phenotypes_p) */


														}		/* if (json_is_array (oids_p)) */

												}		/* if (oids_p) */


											json_decref (results_p);
										}		/* if (results_p) */


									bson_destroy (reply_p);
								}		/* if (reply_p) */
							else
								{
									size_t length;
									char *json_s = ConvertBSONToJSON (command_p, &length);
									if (json_s)
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RunMongoCommand had empty reply for \"%s\"", json_s);
											bson_free (json_s);
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RunMongoCommand had empty reply");
										}

								}
						}		/* if (RunMongoCommand (data_p -> dftsd_mongo_p, command_p, &reply_p)) */
					else
						{
							size_t length;
							char *json_s = ConvertBSONToJSON (command_p, &length);

							if (json_s)
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RunMongoCommand failed for \"%s\"", json_s);
									bson_free (json_s);
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RunMongoCommand failed");
								}
						}

					bson_destroy (command_p);
				}		/* if (command_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create command");
				}

		}

	return values_p;
}



static bool AddAccession (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	bson_oid_t *material_id_p = GetBSONOidFromString (oid_s);

	if (material_id_p)
		{
			Material *material_p = GetMaterialById (material_id_p, data_p);

			if (material_p)
				{
					json_t *accession_p = json_string (material_p -> ma_accession_s);

					if (accession_p)
						{
							if (json_array_append_new (values_p, accession_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, accession_p, "Failed to add accession to array");
									json_decref (accession_p);
								}
						}

					FreeMaterial (material_p);
				}		/* if (material_p) */

			FreeBSONOid (material_id_p);
		}		/* if (material_id_p) */

	return success_flag;
}



static bool AddPhenotype (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	bson_oid_t *phenotype_id_p = GetBSONOidFromString (oid_s);

	if (phenotype_id_p)
		{
			MeasuredVariable *variable_p = GetMeasuredVariableById (phenotype_id_p, data_p);

			if (variable_p)
				{
					if (AddTermToJSON (variable_p -> mv_trait_term_p, values_p))
						{
							if (AddTermToJSON (variable_p -> mv_measurement_term_p, values_p))
								{
									success_flag = true;
								}
						}

					FreeMeasuredVariable (variable_p);
				}		/* if (variable_p) */

			FreeBSONOid (phenotype_id_p);
		}		/* if (phenotype_id_p) */

	return success_flag;
}		/* if (oid_s) */



static bool AddFullPhenotypeAsJSON (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	bson_oid_t *phenotype_id_p = GetBSONOidFromString (oid_s);

	if (phenotype_id_p)
		{
			MeasuredVariable *variable_p = GetMeasuredVariableById (phenotype_id_p, data_p);

			if (variable_p)
				{
					json_t * mv_json_p = GetMeasuredVariableAsJSON (variable_p, VF_CLIENT_FULL, data_p);

					if (mv_json_p)
						{
							if (json_array_append_new (values_p, mv_json_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									json_decref (mv_json_p);
								}
						}
					FreeMeasuredVariable (variable_p);
				}		/* if (variable_p) */

			FreeBSONOid (phenotype_id_p);
		}		/* if (phenotype_id_p) */

	return success_flag;
}		/* if (oid_s) */


static bool AddFullPhenotype (const char *oid_s, json_t *values_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	bson_oid_t *phenotype_id_p = GetBSONOidFromString (oid_s);

	if (phenotype_id_p)
		{
			MeasuredVariable *variable_p = GetMeasuredVariableById (phenotype_id_p, data_p);

			if (variable_p)
				{
					json_t *mv_json_p = GetMeasuredVariableAsJSON (variable_p, VF_CLIENT_FULL, data_p);

					if (mv_json_p)
						{
							if (json_array_append_new (values_p, mv_json_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									json_decref (mv_json_p);
								}

						}		/* if (mv_json_p) */

					FreeMeasuredVariable (variable_p);
				}		/* if (variable_p) */

			FreeBSONOid (phenotype_id_p);
		}		/* if (phenotype_id_p) */

	return success_flag;
}




static bool AddTermToJSON (const SchemaTerm *term_p, json_t *phenotypes_p)
{
	json_t *value_p = json_object ();

	if (value_p)
		{
			if (SetJSONString (value_p, INDEXING_NAME_S, term_p -> st_name_s))
				{
					if (SetJSONString (value_p, INDEXING_DESCRIPTION_S, term_p -> st_description_s))
						{
							if (json_array_append_new (phenotypes_p, value_p) == 0)
								{
									return true;
								}
						}

				}

			json_decref (value_p);
		}		/* if (value_p) */

	return false;
}


bool SetUpStudiesListParameter (const FieldTrialServiceData *data_p, Parameter *param_p, const Study *active_study_p, const bool empty_option_flag)
{
	bool success_flag = false;
	json_t *results_p = GetAllStudiesAsJSON (data_p, false);
	bool value_set_flag = false;

	if (results_p)
		{
			if (json_is_array (results_p))
				{
					const size_t num_results = json_array_size (results_p);

					success_flag = true;

					/*
					 * If there's an empty option, add it
					 */
					if (empty_option_flag)
						{
							success_flag = CreateAndAddStringParameterOption (param_p, S_EMPTY_LIST_OPTION_S, S_EMPTY_LIST_OPTION_S);
						}

					if (success_flag)
						{
							if (num_results > 0)
								{
									size_t i = 0;
									const char *param_value_s = GetStringParameterCurrentValue (param_p);

									bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

									if (id_p)
										{
											while ((i < num_results) && success_flag)
												{
													json_t *entry_p = json_array_get (results_p, i);

													if (GetMongoIdFromJSON (entry_p, id_p))
														{
															char *id_s = GetBSONOidAsString (id_p);

															if (id_s)
																{
																	const char *name_s = GetJSONString (entry_p, ST_NAME_S);

																	if (name_s)
																		{
																			if (param_value_s && (strcmp (param_value_s, id_s) == 0))
																				{
																					value_set_flag = true;
																				}

																			if (!CreateAndAddStringParameterOption (param_p, id_s, name_s))
																				{
																					success_flag = false;
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param option \"%s\": \"%s\"", id_s, name_s);
																				}

																		}		/* if (name_s) */
																	else
																		{
																			success_flag = false;
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get \"%s\"", ST_NAME_S);
																		}

																	FreeBSONOidString (id_s);
																}
															else
																{
																	success_flag = false;
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get Study BSON oid");
																}

														}		/* if (GetMongoIdFromJSON (entry_p, id_p)) */
													else
														{
															success_flag = false;
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "GetMongoIdFromJSON () failed");

														}

													if (success_flag)
														{
															++ i;
														}

												}		/* while ((i < num_results) && success_flag) */

											FreeBSONOid (id_p);
										}		/* if (id_p) */


									/*
									 * If the parameter's value isn't on the list, reset it
									 */
									if ((param_value_s != NULL) && (strcmp (param_value_s, S_EMPTY_LIST_OPTION_S) != 0) && (value_set_flag == false))
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "param value \"%s\" not on list of existing studies", param_value_s);
										}

								}		/* if (num_results > 0) */
							else
								{
									/* nothing to add */
									success_flag = true;
								}

						}		/* if (success_flag) */


				}		/* if (json_is_array (results_p)) */

			json_decref (results_p);
		}		/* if (results_p) */

	if (success_flag)
		{
			if (active_study_p)
				{
					char *id_s = GetBSONOidAsString (active_study_p -> st_id_p);

					if (id_s)
						{
							StringParameter *str_param_p = (StringParameter *) param_p;

							success_flag = SetStringParameterDefaultValue (str_param_p, id_s);
							success_flag = SetStringParameterCurrentValue (str_param_p, id_s);
							FreeBSONOidString (id_s);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get id string for active study \"%s\"", active_study_p -> st_name_s);
							success_flag = false;
						}
				}
		}

	return success_flag;
}


json_t *GetAllStudiesAsJSONInViewFormat (FieldTrialServiceData *data_p, const ViewFormat format)
{
	json_t *raw_results_p = GetAllStudiesAsJSON (data_p, true);
	json_t *formatted_results_p = NULL;

	if (raw_results_p)
		{
			const size_t num_results = json_array_size (raw_results_p);

			if (num_results > 0)
				{
					if ((format == VF_CLIENT_FULL) || (format == VF_CLIENT_MINIMAL))
						{
							formatted_results_p = json_array ();

							if (formatted_results_p)
								{
									size_t i = 0;
									bool success_flag = true;

									while ((i < num_results) && success_flag)
										{
											json_t *entry_p = json_array_get (raw_results_p, i);
											Study *study_p = GetStudyFromJSON (entry_p, format, data_p);

											if (study_p)
												{
													json_t *formatted_study_json_p = GetStudyAsJSON (study_p, format, NULL, data_p);

													if (formatted_study_json_p)
														{
															if (json_array_append_new (formatted_results_p, formatted_study_json_p) != 0)
																{
																	json_decref (formatted_study_json_p);
																	PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, formatted_results_p, "Failed to append formatted study for \"%s\"", study_p -> st_name_s);
																}
														}		/* if (formatted_study_json_p) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, formatted_results_p, "GetStudyAsJSON failed in format %d for study \"%s\"", format, study_p -> st_name_s);
														}

													FreeStudy (study_p);
												}		/* if (study_p) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, entry_p, "GetStudyFromJSON failed in format %d", format);
												}

											++ i;
										}		/* while ((i < num_results) && success_flag) */

								}		/* if (formatted_results_p) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, raw_results_p, "Failed to create formatted results array");
								}





						}		/* if ((format == VF_CLIENT_FULL) || (format == VF_CLIENT_MINIMAL)) */
					else if (format == VF_STORAGE)
						{
							formatted_results_p = raw_results_p;
						}		/* else if (format == VF_STORAGE) */

				}		/* if (num_results > 0) */
			else
				{
					/* return the empty array */
					formatted_results_p = raw_results_p;
				}

			if (raw_results_p != formatted_results_p)
				{
					json_decref (raw_results_p);
				}

		}		/* if (raw_results_p) */

	return formatted_results_p;
}


const KeyValuePair *GetAspect (const char *aspect_value_s)
{
	const KeyValuePair *aspect_p = S_DIRECTIONS_P;
	uint32 i;

	for (i = S_NUM_DIRECTIONS; i > 0; -- i, ++ aspect_p)
		{
			if ((Stricmp (aspect_p -> kvp_value_s, aspect_value_s) == 0) || (strcmp (aspect_p -> kvp_key_s, aspect_value_s) == 0))
				{
					return aspect_p;
				}
		}

	return NULL;
}


static bool GetStudyForGivenId (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p, ViewFormat format, JSONProcessor *processor_p)
{
	bool job_done_flag = false;
	const char *id_s = NULL;

	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_ID.npt_name_s, &id_s))
		{
			if (!IsStringEmpty (id_s))
				{
					FindAndAddStudyToServiceJob (id_s, format, job_p, processor_p, data_p);
					job_done_flag = true;
				}		/* if (!IsStringEmpty (id_s)) */

		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_ARST_ID.npt_name_s, &value, true)) */

	return job_done_flag;
}


json_t *GetStudyJSONForId (const char *id_s, const ViewFormat format, JSONProcessor *processor_p, char **study_name_ss, const FieldTrialServiceData *data_p)
{
	json_t *study_json_p = NULL;

	if (format == VF_CLIENT_FULL)
		{
			study_json_p = GetCachedStudy (id_s, data_p);

			if (study_json_p)
				{
					const char *name_s = GetJSONString (study_json_p, ST_NAME_S);

					if (name_s)
						{
							*study_name_ss = EasyCopyToNewString (name_s);
						}
				}
		}

	if (!study_json_p)
		{
			bson_oid_t *id_p = GetBSONOidFromString (id_s);

			if (id_p)
				{
					Study *study_p = GetStudyById (id_p, format, data_p);

					if (study_p)
						{
							study_json_p = GetStudyAsJSON (study_p, format, processor_p, data_p);

							if (study_json_p)
								{
									if (AddContext (study_json_p))
										{
											if (format == VF_CLIENT_FULL)
												{
													CacheStudy (id_s, study_json_p, data_p);
												}

											*study_name_ss = EasyCopyToNewString (study_p -> st_name_s);
										}		/* if (AddContext (trial_json_p)) */

								}		/* if (study_json_p) */

							FreeStudy (study_p);
						}		/* if (study_p) */


					FreeBSONOid (id_p);
				}		/* if (id_p) */

		}		/* if (!study_json_p) */

	return study_json_p;
}


void FindAndAddStudyToServiceJob (const char *id_s, const ViewFormat format, ServiceJob *job_p, JSONProcessor *processor_p, const FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	char *study_name_s = NULL;
	json_t *study_json_p = GetStudyJSONForId (id_s, format, processor_p, &study_name_s, data_p);

	/*
	bson_oid_t *study_id_p = GetBSONOidFromString (id_s);

	if (study_id_p)
		{
			GetStudyDistinctPhenotypesAsJSON (study_id_p, data_p);
		}
	 */

	if (study_json_p)
		{
			json_t *dest_record_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, study_name_s, study_json_p);

			if (dest_record_p)
				{
					if (AddResultToServiceJob (job_p, dest_record_p))
						{
							status = OS_SUCCEEDED;
						}
					else
						{
							json_decref (dest_record_p);
						}

				}		/* if (dest_record_p) */

			json_decref (study_json_p);
		}		/* if (study_json_p) */

	if (study_name_s)
		{
			FreeCopiedString (study_name_s);
		}

	SetServiceJobStatus (job_p, status);
}


bool GetMatchingStudies (bson_t *query_p, FieldTrialServiceData *data_p, ServiceJob *job_p, ViewFormat format)
{
	bool job_done_flag = false;

	if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_STUDY]))
		{
			bson_t *opts_p =  BCON_NEW ( "sort", "{", ST_NAME_S, BCON_INT32 (1), "}");
			json_t *results_p = GetAllMongoResultsAsJSON (data_p -> dftsd_mongo_p, query_p, opts_p);

			if (results_p)
				{
					if (json_is_array (results_p))
						{
							OperationStatus status = OS_FAILED;
							size_t num_added = 0;
							size_t i = 0;
							const size_t num_results = json_array_size (results_p);

							job_done_flag = true;

							for (i = 0; i < num_results; ++ i)
								{
									json_t *study_json_p = NULL;
									json_t *entry_p = json_array_get (results_p, i);
									char *study_s = NULL;

									switch (format)
									{
										case VF_CLIENT_FULL:
										case VF_CLIENT_MINIMAL:
											{
												Study *study_p = GetStudyFromJSON (entry_p, format, data_p);

												if (study_p)
													{
														study_json_p = GetStudyAsJSON (study_p, format, NULL, data_p);

														if (study_json_p)
															{
																study_s = EasyCopyToNewString (study_p -> st_name_s);
															}

														FreeStudy (study_p);
													}
											}
											break;

										case VF_INDEXING:
											{
												const char *value_s = GetJSONString (entry_p, ST_NAME_S);

												if (value_s)
													{
														study_json_p = json_deep_copy (entry_p);

														if (study_json_p)
															{
																study_s = EasyCopyToNewString (value_s);
															}
													}

											}
											break;

										case VF_REFERENCE:
											{
												const char *name_s = GetJSONString (entry_p, ST_NAME_S);

												if (name_s)
													{
														bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

														if (id_p)
															{
																if (GetMongoIdFromJSON (entry_p, id_p))
																	{
																		study_json_p = json_object ();

																		if (study_json_p)
																			{
																				if (SetJSONString (study_json_p, ST_NAME_S, name_s))
																					{
																						if (!AddCompoundIdToJSON (study_json_p, id_p))
																							{
																								json_decref (study_json_p);
																								study_json_p = NULL;
																							}
																					}
																				else
																					{
																						json_decref (study_json_p);
																						study_json_p = NULL;
																					}
																			}
																	}

																FreeBSONOid (id_p);
															}
													}

											}
											break;

									}

									if (study_json_p)
										{
											bool added_flag = false;

											if (AddContext (study_json_p))
												{
													json_t *dest_record_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, study_s, study_json_p);

													if (dest_record_p)
														{
															if (AddResultToServiceJob (job_p, dest_record_p))
																{
																	++ num_added;
																	added_flag = true;
																}
															else
																{
																	json_decref (dest_record_p);
																}

														}		/* if (dest_record_p) */

												}		/* if (AddContext (trial_json_p)) */

											if (!added_flag)
												{
													json_decref (study_json_p);
												}

											if (study_s)
												{
													FreeCopiedString (study_s);
												}

										}		/* if (study_json_p) */

								}		/* if (num_results > 0) */

							if (num_added == num_results)
								{
									status = OS_SUCCEEDED;
								}
							else if (status > 0)
								{
									status = OS_PARTIALLY_SUCCEEDED;
								}

							SetServiceJobStatus (job_p, status);
						}		/* if (json_is_array (results_p)) */

				}		/* if (results_p) */

			if (opts_p)
				{
					bson_destroy (opts_p);
				}

		}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_EXPERIMENTAL_AREA]) */

	return job_done_flag;
}


static Parameter *GetAndAddAspectParameter (const char *aspect_s, FieldTrialServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	const char *def_s = (S_DIRECTIONS_P + S_UNKNOWN_DIRECTION_INDEX) -> kvp_value_s;
	Parameter *param_p = NULL;

	/*
	 * Is the given aspect on our list?
	 */
	if (aspect_s)
		{
			uint32 i = S_NUM_DIRECTIONS;
			const KeyValuePair *direction_p = S_DIRECTIONS_P;

			while (i > 0)
				{
					if (strcmp (direction_p -> kvp_value_s, aspect_s) == 0)
						{
							def_s = aspect_s;
							/* force exit from loop */
							i = 0;
						}
					else
						{
							-- i;
							++ direction_p;
						}
				}

			if (def_s != aspect_s)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Unknown aspect \"%s\"", aspect_s);
				}

		}


	param_p = EasyCreateAndAddStringParameterToParameterSet (& (data_p -> dftsd_base_data), param_set_p, group_p, STUDY_ASPECT.npt_type, STUDY_ASPECT.npt_name_s, "Aspect", "The direction that the study area was oriented to", def_s, PL_ALL);

	if (param_p)
		{
			uint32 i = S_NUM_DIRECTIONS;
			const KeyValuePair *direction_p = S_DIRECTIONS_P;
			bool success_flag = true;

			/*
			 * Set up the direction options
			 */
			while (success_flag & (i > 0))
				{
					if (CreateAndAddStringParameterOption (param_p, direction_p -> kvp_value_s, direction_p -> kvp_key_s))
						{
							-- i;
							++ direction_p;
						}
					else
						{
							success_flag = false;
						}
				}

			if (success_flag)
				{
					return param_p;
				}

			FreeParameter (param_p);
		}		/* if (param_p) */

	return NULL;
}



static bool AddStudyLocationCriteria (bson_t *query_p, ParameterSet *param_set_p)
{
	bool success_flag = true;
	const char *location_id_s = NULL;

	/*
	 * Are we looking for a specific location?
	 */
	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, STUDY_LOCATIONS_LIST.npt_name_s, &location_id_s))
		{
			if (location_id_s)
				{
					const char *unset_value_s = GetUnsetLocationValue ();

					if (strcmp (location_id_s, unset_value_s) != 0)
						{
							bson_oid_t *id_p = GetBSONOidFromString (location_id_s);

							success_flag  = false;

							if (id_p)
								{
									if (BSON_APPEND_OID (query_p, ST_LOCATION_ID_S, id_p))
										{
											success_flag = true;
										}

									FreeBSONOid (id_p);
								}		/* if (id_p) */

						}		/* if (strcmp (value.st_string_value_s, unset_value_s) != 0) */

				}		/* if (value.st_string_value_s) */

		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_LOCATIONS_LIST.npt_name_s, &value, true)) */

	return success_flag;
}




static bool AddStudyDateCriteria (bson_t *query_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	const uint32 *harvest_year_p = NULL;

	if (GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, STUDY_HARVEST_YEAR.npt_name_s, &harvest_year_p))
		{
			if (harvest_year_p)
				{
					bson_t *sowing_date_doc_p = bson_new ();

					if (sowing_date_doc_p)
						{
							const int32 i = * ((int32 *) harvest_year_p);

							if (BSON_APPEND_INT32 (sowing_date_doc_p, "$gte", i))
								{
									if (BSON_APPEND_DOCUMENT (query_p, ST_HARVEST_YEAR_S, sowing_date_doc_p))
										{
											success_flag = true;
										}
								}

							bson_destroy (sowing_date_doc_p);
						}


				}		/* if (value.st_time_p) */

		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_LOCATIONS_LIST.npt_name_s, &value, true)) */
	else
		{
			success_flag = true;
		}

	return success_flag;
}


char *GetStudyAsString (const Study *study_p)
{
	char *study_s = NULL;

	if (study_p -> st_parent_p)
		{
			char *trial_s = GetFieldTrialAsString (study_p -> st_parent_p);

			if (trial_s)
				{
					study_s = ConcatenateVarargsStrings (trial_s, ": ", study_p -> st_name_s, NULL);

					FreeCopiedString (trial_s);
				}
		}
	else
		{
			study_s = EasyCopyToNewString (study_p -> st_name_s);
		}

	return study_s;
}


static bool GetValidCrop (const char *crop_s, Crop **crop_pp, const FieldTrialServiceData *data_p)
{
	bool success_flag = true;

	if (crop_s)
		{
			*crop_pp = GetCropByIdString (crop_s, data_p);

			if (! (*crop_pp))
				{
					success_flag = false;
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create crop from \"%s\"", crop_s);
				}
		}

	return success_flag;
}



static Study *GetStudyFromJSONResource (const json_t *resource_data_p, ServiceData *data_p)
{
	const json_t *param_set_json_p = json_object_get (resource_data_p, PARAM_SET_KEY_S);

	if (param_set_json_p)
		{
			const json_t *params_json_p = json_object_get (resource_data_p, PARAM_SET_PARAMS_S);

			if (params_json_p)
				{
					if (json_is_array (params_json_p))
						{
							const size_t num_params = json_array_size (params_json_p);
							size_t i = 0;

							for (i = 0; i < num_params; ++ i)
								{
									const json_t *param_json_p = json_array_get (params_json_p, i);
									const char *param_name_s = GetJSONString (param_json_p, PARAM_NAME_S);

									if (param_name_s)
										{
											/*
											 * Is this the Study ID parameter?
											 */
											if (strcmp (param_name_s, STUDY_ID.npt_name_s) == 0)
												{
													const char *id_s = GetJSONString (param_json_p, PARAM_CURRENT_VALUE_S);

													if (id_s)
														{
															Study *study_p = GetStudyByIdString (id_s, VF_STORAGE, (FieldTrialServiceData *) data_p);

															if (study_p)
																{
																	return study_p;
																}		/* if (study_p) */
															else
																{

																}

														}		/* if (id_s) */

												}

										}		/* if (param_name_s) */

								}		/* for (i = 0; i < num_params; ++ i) */

						}		/* if (json_is_array (params_json_p)) */

				}		/* if (params_json_p) */

		}		/* if (param_set_json_p) */

	return NULL;
}


Study *GetStudyFromResource (DataResource *resource_p, const NamedParameterType study_param_type, FieldTrialServiceData *dfw_data_p)
{
	Study *study_p = NULL;

	/*
	 * Have we been set some parameter values to refresh from?
	 */
	if (resource_p && (resource_p -> re_data_p))
		{
			const json_t *param_set_json_p = json_object_get (resource_p -> re_data_p, PARAM_SET_KEY_S);

			if (param_set_json_p)
				{
					json_t *params_json_p = json_object_get (param_set_json_p, PARAM_SET_PARAMS_S);

					if (params_json_p)
						{
							const char *study_id_s = GetStringDefaultValueFromJSON (study_param_type.npt_name_s, params_json_p);

							/*
							 * Do we have an existing study id?
							 */
							if (study_id_s)
								{
									study_p = GetStudyByIdString (study_id_s, VF_CLIENT_FULL , dfw_data_p);

									if (!study_p)
										{
											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, params_json_p, "Failed to load Study with id \"%s\"", study_id_s);
										}

								}		/* if (study_id_s) */

						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, param_set_json_p, "Failed to get params with key \"%s\"", PARAM_SET_PARAMS_S);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, resource_p -> re_data_p, "Failed to get param set with key \"%s\"", PARAM_SET_KEY_S);
				}

		}		/* if (resource_p && (resource_p -> re_data_p)) */

	return study_p;
}


OperationStatus RemovePlotsForStudyById (const char *id_s, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	MongoTool *tool_p = data_p -> dftsd_mongo_p;

	OperationStatus phenotypes_status = RemoveStudyPhenotypesFromStudyById (id_s, data_p);

	if (phenotypes_status == OS_SUCCEEDED)
		{
			if (SetMongoToolCollection (tool_p, data_p -> dftsd_collection_ss [DFTD_PLOT]))
				{
					bson_oid_t *id_p = GetBSONOidFromString (id_s);

					if (id_p)
						{
							bson_t *query_p = bson_new ();

							if (query_p)
								{
									if (BSON_APPEND_OID (query_p, PL_PARENT_STUDY_S, id_p))
										{
											if (RemoveMongoDocumentsByBSON (tool_p, query_p, false))
												{
													status = OS_SUCCEEDED;
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RemoveMongoDocumentsByBSON () failed for \"%s\"", id_s);
												}
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "BSON_APPEND_OID () failed for \"%s\"", id_s);
										}

									bson_free (query_p);
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "bson_new () failed");
								}

							FreeBSONOid (id_p);
						}		/* if (id_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetBSONOidFromString () failed for \"%s\"", id_s);
						}

				}		/* if (SetMongoToolCollection (tool_p, data_p -> dftsd_collection_ss [DFTD_PLOT])) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set mongo collection to \"%s\"", data_p -> dftsd_collection_ss [DFTD_PLOT]);
				}

		}		/* if (phenotypes_status == OS_SUCCEEDED) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "RemoveStudyPhenotypesFromStudyById () failed for \"%s\"", id_s);
		}

	return status;
}


OperationStatus RemoveStudyPhenotypesFromStudyById (const char *id_s, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	MongoTool *tool_p = data_p -> dftsd_mongo_p;

	if (SetMongoToolCollection (tool_p, data_p -> dftsd_collection_ss [DFTD_STUDY]))
		{
			bson_oid_t *id_p = GetBSONOidFromString (id_s);

			if (id_p)
				{
					bson_t *query_p = bson_new ();

					if (query_p)
						{
							if (BSON_APPEND_OID (query_p, MONGO_ID_S, id_p))
								{
									bson_t *reply_p = NULL;
									const char *fields_ss [] = { ST_PHENOTYPES_S, NULL };

									if (RemoveMongoFields (tool_p, query_p, fields_ss, &reply_p))
										{
											status = OS_SUCCEEDED;
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "BSON_APPEND_OID () failed for \"%s\"", id_s);
								}

							bson_free (query_p);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "bson_new () failed");
						}

					FreeBSONOid (id_p);
				}		/* if (id_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetBSONOidFromString () failed for \"%s\"", id_s);
				}

		}		/* if (SetMongoToolCollection (tool_p, data_p -> dftsd_collection_ss [DFTD_PLOT])) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set mongo collection to \"%s\"", data_p -> dftsd_collection_ss [DFTD_PLOT]);
		}

	return status;
}


bool BackupStudyByIdString (const char *id_s, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	Study *study_p = GetStudyByIdString (id_s, VF_CLIENT_FULL, data_p);

	if (study_p)
		{
			if (BackupStudy (study_p, id_s, data_p))
				{
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "BackupStudy () failed for id \"%s\"", id_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetStudyByIdString () failed for id \"%s\"", id_s);
		}

	return success_flag;
}


bool BackupStudy (Study *study_p, const char *id_s, FieldTrialServiceData *data_p)
{
	bool saved_study_flag = false;

	if (data_p -> dftsd_wastebasket_path_s)
		{
			json_t *study_json_p = GetStudyAsJSON (study_p, VF_CLIENT_FULL, NULL, data_p);

			if (study_json_p)
				{
					char *filename_s = GetBackupFilename (id_s, data_p);

					if (filename_s)
						{
							int res = json_dump_file (study_json_p, filename_s, JSON_INDENT (2));

							if (res == 0)
								{
									saved_study_flag = true;
								}


							FreeCopiedString (filename_s);
						}

					json_decref (study_json_p);
				}

		}

	return saved_study_flag;
}


OperationStatus DeleteStudyById (const char *id_s, ServiceJob *job_p, FieldTrialServiceData *data_p, const bool backup_flag)
{
	OperationStatus status = OS_FAILED;
	MongoTool *tool_p = data_p -> dftsd_mongo_p;

	if (SetMongoToolCollection (tool_p, data_p -> dftsd_collection_ss [DFTD_STUDY]))
		{
			bool success_flag = false;

			if (backup_flag)
				{
					Study *study_p = GetStudyByIdString (id_s, VF_CLIENT_FULL, data_p);

					if (study_p)
						{
							json_t *study_json_p = GetStudyAsJSON (study_p, VF_CLIENT_FULL, NULL, data_p);

							if (study_json_p)
								{
									/*
									 * Back up study
									 */
									success_flag = BackupStudy (study_p, id_s, data_p);

									json_decref (study_json_p);
								}

							FreeStudy (study_p);
						}

				}		/* if (backup_flag) */
			else
				{
					success_flag = true;
				}

			if (success_flag)
				{
					bson_oid_t *id_p = GetBSONOidFromString (id_s);

					if (id_p)
						{
							bson_t *query_p = bson_new ();
							OperationStatus s;

							if (query_p)
								{
									if (BSON_APPEND_OID (query_p, MONGO_ID_S, id_p))
										{
											if (RemoveMongoDocumentsByBSON (tool_p, query_p, false))
												{
													status = RemovePlotsForStudyById (id_s, data_p);
												}
										}

									bson_free (query_p);
								}


							s = DeleteStudyFromLuceneIndexById (id_s,  job_p -> sj_id, data_p);

							FreeBSONOid (id_p);
						}		/* if (id_p) */

				}		/* if (success_flag) */



		}		/* if (SetMongoToolCollection (tool_p, data_p -> dftsd_collection_ss [DFTD_STUDY])) */

	return status;

}


OperationStatus DeleteStudyFromLuceneIndexById (const char *id_s, uuid_t uuid, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED_TO_START;
	GrassrootsServer *server_p = GetGrassrootsServerFromService (data_p -> dftsd_base_data.sd_service_p);

	if (server_p)
		{
			LuceneTool *lucene_p = AllocateLuceneTool (server_p, uuid);

			if (lucene_p)
				{
					char *query_s = ConcatenateVarargsStrings (LUCENE_ID_S, LT_EXACT_SEARCH_OP_S, id_s, NULL);

					if (query_s)
						{
							status = DeleteLucene (lucene_p, query_s, QM_PARSER);
							FreeCopiedString (query_s);
						}

					FreeLuceneTool (lucene_p);
				}

		}		/* if (server_p) */

	return status;
}


TreatmentFactor *GetTreatmentFactorForStudyByUrl (const Study *study_p, const char *treatment_url_s, const FieldTrialServiceData *data_p)
{
	TreatmentFactor *treatment_factor_p = NULL;
	TreatmentFactorNode *node_p = (TreatmentFactorNode *) (study_p -> st_treatments_p -> ll_head_p);

	while (node_p)
		{
			TreatmentFactor *tf_p = node_p -> tfn_p;
			const char *url_s = GetTreatmentFactorUrl (tf_p);

			if (strcmp (url_s, treatment_url_s) == 0)
				{
					treatment_factor_p = tf_p;

					/* force exit from loop */
					node_p = NULL;
				}
			else
				{
					node_p = (TreatmentFactorNode *) (node_p -> tfn_node.ln_next_p);
				}

		}		/* while (node_p) */

	return treatment_factor_p;
}



TreatmentFactor *GetTreatmentFactorForStudy (Study *study_p, const bson_oid_t *treatment_id_p, const FieldTrialServiceData *data_p)
{
	TreatmentFactor *tf_p = NULL;
	TreatmentFactorNode *node_p = (TreatmentFactorNode *) (study_p -> st_treatments_p -> ll_head_p);

	while (node_p)
		{
			TreatmentFactor *temp_p = node_p -> tfn_p;

			if (bson_oid_equal (treatment_id_p, temp_p -> tf_treatment_p -> tr_id_p))
				{
					tf_p = temp_p;

					/* force exit from loop */
					node_p = NULL;
				}
			else
				{
					node_p = (TreatmentFactorNode *) (node_p -> tfn_node.ln_next_p);
				}

		}		/* while (node_p) */

	return tf_p;
}


TreatmentFactor *GetOrCreateTreatmentFactorForStudy (Study *study_p, const bson_oid_t *treatment_id_p, const FieldTrialServiceData *data_p)
{
	TreatmentFactor *tf_p = GetTreatmentFactorForStudy (study_p, treatment_id_p, data_p);

	if (!tf_p)
		{
			Treatment *treatment_p = GetTreatmentById (treatment_id_p, VF_STORAGE, data_p);

			if (treatment_p)
				{
					tf_p = AllocateTreatmentFactor (treatment_p, study_p);

					if (tf_p)
						{
							if (!AddTreatmentFactorToStudy (tf_p, study_p))
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add new TreatmentFactor \"%s\" to Study \"%s\"",
															 tf_p -> tf_treatment_p -> tr_ontology_term_p -> st_name_s, study_p -> st_name_s);

									FreeTreatmentFactor (tf_p);
									tf_p = NULL;
								}
						}
					else
						{
							FreeTreatment (treatment_p);
						}		/* if (!tf_p) */

				}		/* if (treatment_p) */

		}		/* if (!tf_p) */

	return tf_p;
}



bool AddTreatmentFactorToStudy (TreatmentFactor *tf_p, Study *study_p)
{
	bool success_flag = false;

	if (tf_p)
		{
			TreatmentFactorNode *node_p = AllocateTreatmentFactorNode (tf_p);

			if (node_p)
				{
					LinkedListAddTail (study_p -> st_treatments_p, & (node_p -> tfn_node));
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add TreatmentFactor \"%s\" to Study \"%s\"",
											 tf_p -> tf_treatment_p -> tr_ontology_term_p -> st_name_s, study_p -> st_name_s);
				}
		}

	return success_flag;
}


static bool AddDefaultPlotsParameters (ServiceData *data_p, ParameterSet *params_p, const bool read_only_flag, const Study *study_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Default Plots data", false, data_p, params_p);
	Parameter *param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, params_p, group_p, STUDY_NUM_PLOT_ROWS.npt_name_s, "Number of plot rows", "The number of plot rows", study_p ? study_p -> st_num_rows_p : NULL, PL_ALL_AND_WIZARD);

	if (param_p)
		{
			param_p -> pa_read_only_flag = read_only_flag;

			if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, params_p, group_p, STUDY_NUM_PLOT_COLS.npt_name_s, "Number of plot columns", "The number of plot columns", study_p ? study_p -> st_num_columns_p : NULL, PL_ALL_AND_WIZARD)) != NULL)
				{
					param_p -> pa_read_only_flag = read_only_flag;

					if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, params_p, group_p, STUDY_NUM_REPLICATES.npt_name_s, "Number of replicates", "The number of replicates", study_p ? study_p -> st_num_replicates_p : NULL, PL_ALL)) != NULL)
						{
							param_p -> pa_read_only_flag = read_only_flag;

							if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (data_p, params_p, group_p, STUDY_PLOT_WIDTH.npt_type, STUDY_PLOT_WIDTH.npt_name_s, "Plot width", "The default width, in metres, of each plot", study_p ? study_p -> st_default_plot_width_p : NULL, PL_ALL)) != NULL)
								{
									param_p -> pa_read_only_flag = read_only_flag;

									if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (data_p, params_p, group_p, STUDY_PLOT_LENGTH.npt_type, STUDY_PLOT_LENGTH.npt_name_s, "Plot length", "The default length, in metres, of each plot", study_p ? study_p -> st_default_plot_length_p : NULL, PL_ALL)) != NULL)
										{
											param_p -> pa_read_only_flag = read_only_flag;

											return true;
										}		/* if (EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, params_p, group_p, STUDY_NUM_PLOT_ROWS.npt_name_s, "Plot width", "The default width, in metres, of each plot", NULL, PL_ALL)) */

								}		/* if (EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, params_p, group_p, STUDY_NUM_PLOT_ROWS.npt_name_s, "Plot width", "The default width, in metres, of each plot", NULL, PL_ALL)) */

						}		/* if (EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, params_p, group_p, STUDY_NUM_REPLICATES.npt_name_s, "Number of replicates", "The number of replicates", NULL, PL_ALL)) */

				}		/* if (EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, params_p, group_p, STUDY_NUM_PLOT_ROWS.npt_name_s, "Plot width", "The default width, in metres, of each plot", NULL, PL_ALL)) */

		}		/* if (EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, params_p, group_p, STUDY_NUM_PLOT_ROWS.npt_name_s, "Plot width", "The default width, in metres, of each plot", NULL, PL_ALL)) */


	return success_flag;
}


static bool AddCuratorSubmissionParams (const Person *curator_p, ParameterSet *params_p, const bool read_only_flag, ServiceData *data_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Curator", false, data_p, params_p);
	Parameter *param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CURATOR_NAME.npt_type, STUDY_CURATOR_NAME.npt_name_s, "Curator name", "The name of the data curator for this Study", curator_p ? curator_p -> pe_name_s : NULL, PL_ALL);

	if (param_p)
		{
			param_p -> pa_read_only_flag = read_only_flag;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CURATOR_EMAIL.npt_type, STUDY_CURATOR_EMAIL.npt_name_s, "Curator email", "The email of the data curator for this Study", curator_p ? curator_p -> pe_email_s : NULL, PL_ALL)) != NULL)
				{
					param_p -> pa_read_only_flag = read_only_flag;

					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CURATOR_ROLE.npt_type, STUDY_CURATOR_ROLE.npt_name_s, "Curator role", "The role of the data curator for this Study", curator_p ? curator_p -> pe_role_s : NULL, PL_ALL)) != NULL)
						{
							param_p -> pa_read_only_flag = read_only_flag;

							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CURATOR_AFFILATION.npt_type, STUDY_CURATOR_AFFILATION.npt_name_s, "Curator affiliation", "The affiliation of the data curator for this Study", curator_p ? curator_p -> pe_affiliation_s : NULL, PL_ALL)) != NULL)
								{
									param_p -> pa_read_only_flag = read_only_flag;

									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CURATOR_ORCID.npt_type, STUDY_CURATOR_ORCID.npt_name_s, "Curator OrCID", "The OrCID of the data curator for this Study", curator_p ? curator_p -> pe_role_s : NULL, PL_ALL)) != NULL)
										{
											success_flag = true;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CURATOR_ORCID.npt_name_s);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CURATOR_AFFILATION.npt_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CURATOR_ROLE.npt_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CURATOR_EMAIL.npt_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CURATOR_NAME.npt_name_s);
		}

	return success_flag;
}


static bool AddContactSubmissionParams (const Person *contact_p, ParameterSet *params_p, const bool read_only_flag, ServiceData *data_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Contact", false, data_p, params_p);
	Parameter *param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CONTACT_NAME.npt_type, STUDY_CONTACT_NAME.npt_name_s, "Contact name", "The name of the contact for this Study", contact_p ? contact_p -> pe_name_s : NULL, PL_ALL);

	if (param_p)
		{
			param_p -> pa_read_only_flag = read_only_flag;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CONTACT_EMAIL.npt_type, STUDY_CONTACT_EMAIL.npt_name_s, "Contact email", "The email of the contact for this Study", contact_p ? contact_p -> pe_email_s : NULL, PL_ALL)) != NULL)
				{
					param_p -> pa_read_only_flag = read_only_flag;

					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CONTACT_ROLE.npt_type, STUDY_CONTACT_ROLE.npt_name_s, "Contact role", "The role of the contact for this Study", contact_p ? contact_p -> pe_role_s : NULL, PL_ALL)) != NULL)
						{
							param_p -> pa_read_only_flag = read_only_flag;

							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CONTACT_AFFILATION.npt_type, STUDY_CONTACT_AFFILATION.npt_name_s, "Contact affiliation", "The affiliation of the contact for this Study", contact_p ? contact_p -> pe_affiliation_s : NULL, PL_ALL)) != NULL)
								{
									param_p -> pa_read_only_flag = read_only_flag;

									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_CONTACT_ORCID.npt_type, STUDY_CONTACT_ORCID.npt_name_s, "Contact OrCID", "The OrCID of the contact for this Study", contact_p ? contact_p -> pe_role_s : NULL, PL_ALL)) != NULL)
										{
											param_p -> pa_read_only_flag = read_only_flag;

											success_flag = true;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CONTACT_ORCID.npt_name_s);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CONTACT_AFFILATION.npt_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CONTACT_ROLE.npt_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CONTACT_EMAIL.npt_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_CONTACT_NAME.npt_name_s);
		}

	return success_flag;
}


static bool AddFieldTrialSubmissionParameter (Study *active_study_p, ParameterSet *params_p, ParameterGroup *group_p, FieldTrialServiceData *dfw_data_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	char *trial_id_s = NULL;

	if (active_study_p && (active_study_p -> st_parent_p))
		{
			trial_id_s = GetBSONOidAsString (active_study_p -> st_parent_p -> ft_id_p);
		}

	param_p = EasyCreateAndAddStringParameterToParameterSet (& (dfw_data_p -> dftsd_base_data), params_p, group_p, STUDY_FIELD_TRIALS_LIST.npt_type, STUDY_FIELD_TRIALS_LIST.npt_name_s, "Field trials", "The available field trials", trial_id_s, PL_ALL);


	if (param_p)
		{
			if (SetUpFieldTrialsListParameter (dfw_data_p, param_p, trial_id_s, false))
				{
					success_flag = true;
				}
		}

	if (trial_id_s)
		{
			FreeBSONOidString (trial_id_s);
		}


	return success_flag;
}


static bool AddGeneralSubmissionStudyParams (Study *active_study_p, const char *id_s, const char *trial_s, const char *location_s, ParameterSet *params_p, ParameterGroup *group_p, const bool read_only_flag, ServiceData *data_p)
{
	bool success_flag = false;
	FieldTrialServiceData *dfw_data_p = (FieldTrialServiceData *) data_p;
	Parameter *param_p = NULL;

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_NAME.npt_type, STUDY_NAME.npt_name_s, "Name", "The name of the Study", active_study_p ? active_study_p -> st_name_s : NULL, PL_ALL_AND_WIZARD)) != NULL)
		{
			param_p -> pa_required_flag = true;
			param_p -> pa_read_only_flag = read_only_flag;

			param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_FIELD_TRIALS_LIST.npt_type, STUDY_FIELD_TRIALS_LIST.npt_name_s, "Field trials", "The available field trials", trial_s, PL_ALL_AND_WIZARD);

			if (param_p)
				{
					char *trial_id_s = NULL;

					param_p -> pa_read_only_flag = read_only_flag;


					if (active_study_p && (active_study_p -> st_parent_p))
						{
							trial_id_s = GetBSONOidAsString (active_study_p -> st_parent_p -> ft_id_p);
						}

					if (SetUpFieldTrialsListParameter (dfw_data_p, param_p, trial_id_s, false))
						{
							if (trial_id_s)
								{
									FreeBSONOidString (trial_id_s);
								}

							if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, params_p, group_p, STUDY_SOWING_YEAR.npt_name_s, "Sowing year", "The year that the Study was started", active_study_p ? active_study_p -> st_predicted_sowing_year_p : NULL, PL_ALL)) != NULL)
								{
									param_p -> pa_read_only_flag = read_only_flag;

									if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, params_p, group_p, STUDY_HARVEST_YEAR.npt_name_s, "Harvest year", "The year that the Study was finished", active_study_p ? active_study_p -> st_predicted_harvest_year_p : NULL, PL_ALL)) != NULL)
										{
											param_p -> pa_read_only_flag = read_only_flag;

											if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_LOCATIONS_LIST.npt_type, STUDY_LOCATIONS_LIST.npt_name_s, "Locations", "The available locations", location_s, PL_ALL_AND_WIZARD)) != NULL)
												{
													param_p -> pa_read_only_flag = read_only_flag;

													if (SetUpLocationsListParameter (dfw_data_p, (StringParameter *) param_p, active_study_p ? active_study_p -> st_location_p : NULL, NULL))
														{
															Person *curator_p = NULL;
															Person *contact_p = NULL;

															if (active_study_p)
																{
																	curator_p = active_study_p -> st_curator_p;
																	contact_p = active_study_p -> st_contact_p;
																}

															if (AddCuratorSubmissionParams (curator_p, params_p, read_only_flag, data_p))
																{
																	if (AddContactSubmissionParams (contact_p, params_p, read_only_flag, data_p))
																		{
																			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_DESCRIPTION.npt_type, STUDY_DESCRIPTION.npt_name_s, "Description", "A description of the Study", active_study_p ? active_study_p -> st_description_s : NULL, PL_ALL_AND_WIZARD)) != NULL)
																				{
																					param_p -> pa_read_only_flag = read_only_flag;

																					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_DESIGN.npt_type, STUDY_DESIGN.npt_name_s, "Design", "Information about the Study design", active_study_p ? active_study_p -> st_design_s : NULL, PL_ALL)) != NULL)
																						{
																							param_p -> pa_read_only_flag = read_only_flag;

																							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_GROWING_CONDITIONS.npt_type, STUDY_GROWING_CONDITIONS.npt_name_s, "Growing conditions", "Information about the Growing conditions", active_study_p ? active_study_p -> st_growing_conditions_s : NULL, PL_ALL)) != NULL)
																								{
																									param_p -> pa_read_only_flag = read_only_flag;

																									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_PHENOTYPE_GATHERING_NOTES.npt_type, STUDY_PHENOTYPE_GATHERING_NOTES.npt_name_s, "Phenotype gathering notes", "Notes on how the Phenotype information was gathered", active_study_p ? active_study_p -> st_phenotype_gathering_notes_s : NULL, PL_ALL)) != NULL)
																										{
																											param_p -> pa_read_only_flag = read_only_flag;

																											if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_WEATHER_LINK.npt_type, STUDY_WEATHER_LINK.npt_name_s, "Weather", "Link out to the weather data for this study", active_study_p ? active_study_p -> st_weather_link_s : NULL, PL_ALL)) != NULL)
																												{
																													param_p -> pa_read_only_flag = read_only_flag;

																													if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_PLAN_CHANGES.npt_type, STUDY_PLAN_CHANGES.npt_name_s, "Field plan changes", "Changes to the Study field experiment plan.", active_study_p ? active_study_p -> st_plan_changes_s : NULL, PL_ALL)) != NULL)
																														{
																															param_p -> pa_read_only_flag = read_only_flag;

																															if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_DATA_NOT_INCLUDED.npt_type, STUDY_DATA_NOT_INCLUDED.npt_name_s, "Data not included", "Data collected but not currently stored within the Grassroots Field Trial system", active_study_p ? active_study_p -> st_data_not_included_s : NULL, PL_ALL)) != NULL)
																																{
																																	param_p -> pa_read_only_flag = read_only_flag;

																																	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_PHYSICAL_SAMPLES_COLLECTED.npt_type, STUDY_PHYSICAL_SAMPLES_COLLECTED.npt_name_s, "Physical samples collected", "Details about plant, soil or other samples collected",  active_study_p ? active_study_p -> st_physical_samples_collected_s : NULL, PL_ALL)) != NULL)
																																		{
																																			param_p -> pa_read_only_flag = read_only_flag;

																																			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_PHOTO.npt_type, STUDY_PHOTO.npt_name_s, "Photo", "The web address of a photo or image to represent the study", active_study_p ? active_study_p -> st_photo_url_s : NULL, PL_ALL_AND_WIZARD)) != NULL)
																																				{
																																					param_p -> pa_read_only_flag = read_only_flag;

																																					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_IMAGE_NOTES.npt_type, STUDY_IMAGE_NOTES.npt_name_s, "Image collection notes", "Details about any images collected",  active_study_p ? active_study_p -> st_image_collection_notes_s : NULL, PL_ALL)) != NULL)
																																						{
																																							param_p -> pa_read_only_flag = read_only_flag;

																																							success_flag = true;
																																						}
																																					else
																																						{
																																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_IMAGE_NOTES.npt_name_s);
																																						}
																																				}
																																			else
																																				{
																																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PHOTO.npt_name_s);
																																				}
																																		}
																																	else
																																		{
																																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PHYSICAL_SAMPLES_COLLECTED.npt_name_s);
																																		}
																																}
																															else
																																{
																																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_DATA_NOT_INCLUDED.npt_name_s);
																																}
																														}
																													else
																														{
																															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_PLAN_CHANGES.npt_name_s);
																														}

																												}		/* if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, STUDY_WEATHER_LINK.npt_type, STUDY_WEATHER_LINK.npt_name_s, "Weather", "Link out to the weather data for this study", weather_s, PL_ALL)) != NULL) */
																											else
																												{
																													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_WEATHER_LINK.npt_name_s);
																												}


																										}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (data_p, param_set_p, group_p, STUDY_PHENOTYPE_GATHERING_NOTES.npt_type, STUDY_PHENOTYPE_GATHERING_NOTES.npt_name_s, "Phenotype Gathering", "NOtes on hoe the Phenotype information was gathered", def, PL_ALL)) != NULL) */
																									else
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_GROWING_CONDITIONS.npt_name_s);
																										}

																								}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (data_p, param_set_p, group_p, STUDY_GROWING_CONDITIONS.npt_type, STUDY_GROWING_CONDITIONS.npt_name_s, "Growing Conditions", "Information about the Growing conditions", def, PL_ALL)) != NULL) */
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_GROWING_CONDITIONS.npt_name_s);
																								}

																						}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (data_p, param_set_p, group_p, STUDY_DESIGN.npt_type, STUDY_DESIGN.npt_name_s, "Design", "Information about the Study design", def, PL_ALL)) != NULL) */
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_DESIGN.npt_name_s);
																						}

																				}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (data_p, param_set_p, group_p, STUDY_NOTES.npt_type, STUDY_NOTES.npt_name_s, "Notes", "Any additional information about the study", def, PL_ALL)) != NULL) */
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_DESCRIPTION.npt_name_s);
																				}

																		}		/* if (AddContactSubmissionParams (contact_p, params_p, data_p)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddContactSubmissionParams () failed");
																		}

																}		/* if (AddCuratorSubmissionParams (curator_p, params_p, data_p)) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddCuratorSubmissionParams () failed");
																}


														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpLocationsListParameter failed");
														}

												}		/* if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_HARVEST_YEAR.npt_type, STUDY_HARVEST_YEAR.npt_name_s, "Harvest year", "The year that the Study was finished", active_study_p ? active_study_p -> st_predicted_harvest_year_p : NULL, PL_ALL)) != NULL) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_HARVEST_YEAR.npt_name_s);
												}

										}		/* if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, STUDY_SOWING_YEAR.npt_type, STUDY_SOWING_YEAR.npt_name_s, "Sowing year", "The year that the Study was started", active_study_p ? active_study_p -> st_predicted_sowing_year_p : NULL, PL_ALL)) != NULL) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_SOWING_YEAR.npt_name_s);
										}

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_FIELD_TRIALS_LIST.npt_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpFieldTrialsListParameter failed");
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_FIELD_TRIALS_LIST.npt_name_s);
				}


		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", STUDY_NAME.npt_name_s);
		}

	return success_flag;
}


static bool AddTreatmentFactorParameters (ParameterSet *params_p, const Study *study_p, const bool read_only_flag, FieldTrialServiceData *data_p)
{
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Treatment Factors", true, & (data_p -> dftsd_base_data), params_p);

	if (group_p)
		{
			Parameter *name_param_p = NULL;
			Parameter *values_param_p = NULL;
			const char * const tf_name_display_name_s = "Treatment name";
			const char * const tf_name_description_s = "The name of the treatment";
			size_t num_treatments = 0;
			json_t *tf_json_p = NULL;

			if (study_p)
				{
					num_treatments = study_p -> st_treatments_p -> ll_size;
				}

			if (num_treatments > 1)
				{
					const char **values_ss = (const char **) AllocMemoryArray (num_treatments, sizeof (const char *));

					if (values_ss)
						{
							json_t *factors_array_p = json_array ();

							if (factors_array_p)
								{
									bool success_flag = true;
									const char **value_ss = values_ss;
									TreatmentFactorNode *node_p = ((TreatmentFactorNode *) (study_p -> st_treatments_p -> ll_head_p));

									while (node_p && success_flag)
										{
											TreatmentFactor *tf_p = node_p -> tfn_p;
											const char *url_s = GetTreatmentFactorUrl (tf_p);
											json_t *factors_p = GetTreatmentFactorValuesAsJSON (tf_p, VF_CLIENT_FULL);

											if (factors_p)
												{
													if (json_array_append_new (factors_array_p, factors_p) == 0)
														{
															*value_ss = url_s;

															node_p = (TreatmentFactorNode *) (node_p -> tfn_node.ln_next_p);
															++ value_ss;
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, factors_p, "Failed to add treatment factor \"%s\" from study \"%s\" as JSON", url_s, study_p -> st_name_s);

															json_decref (factors_p);

															success_flag = false;
														}
												}		/* if (factors_p) */
											else
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get treatment factor \"%s\" from study \"%s\" as JSON", url_s, study_p -> st_name_s);
													success_flag = false;
												}

										}		/* while (node_p) */

									if (success_flag)
										{
											*value_ss = NULL;

											tf_json_p = factors_array_p;
											name_param_p = EasyCreateAndAddStringArrayParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, TFJ_TREATMENT_NAME.npt_name_s, tf_name_display_name_s, tf_name_description_s, values_ss, num_treatments, PL_ALL);

											if (name_param_p)
												{
													name_param_p -> pa_read_only_flag = read_only_flag;
												}
											else
												{
													success_flag = false;
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EasyCreateAndAddStringArrayParameterToParameterSet () failed");
												}
										}		/* if (success_flag) */

								}		/* if (factors_array_p) */


							FreeMemory (values_ss);
						}		/* if (values_ss) */
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to copy treatment factor name values for Study \"%s\"", study_p -> st_name_s);
						}



				}		/* if (num_treatments > 1) */
			else
				{
					const char *active_tf_url_s = NULL;

					if (num_treatments == 1)
						{
							TreatmentFactor *tf_p = ((TreatmentFactorNode *) (study_p -> st_treatments_p -> ll_head_p)) -> tfn_p;
							tf_json_p = GetTreatmentFactorAsJSON (tf_p, VF_CLIENT_FULL);

							active_tf_url_s = GetTreatmentFactorUrl (tf_p);
						}

					name_param_p = EasyCreateAndAddStringParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, TFJ_TREATMENT_NAME.npt_type, TFJ_TREATMENT_NAME.npt_name_s, tf_name_display_name_s, tf_name_description_s, active_tf_url_s, PL_ALL);

					if (name_param_p)
						{
							name_param_p -> pa_read_only_flag = read_only_flag;
						}
				}

			if (name_param_p)
				{
					if (AddRepeatableParameterGroupLabelParam (group_p, name_param_p))
						{
							if ((name_param_p = GetTreatmentFactorTableParameter (params_p, group_p, tf_json_p, data_p)) != NULL)
								{
									name_param_p -> pa_read_only_flag = read_only_flag;
									return true;
								}
						}
				}

			if (tf_json_p)
				{
					json_decref (tf_json_p);
				}
		}

	return false;
}


static bool AddTreatmentFactorsToStudy (Study *study_p, Parameter *treatment_names_p, Parameter *treatment_levels_p, const size_t num_treatments, ServiceJob *job_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = true;


	if (treatment_names_p && treatment_levels_p)
		{
			const json_t *levels_json_p = GetJSONParameterCurrentValue ((JSONParameter *) treatment_levels_p);

			if (IsStringParameter (treatment_names_p))
				{
					const char *name_s = GetStringParameterCurrentValue ((StringParameter *) treatment_names_p);
					const json_t *level_p = json_array_get (levels_json_p, 0);

					if (!AddTreatmentFactorByPartsToStudy (name_s, level_p, study_p, data_p))
						{

							ReportTreatmentFactorError (study_p, name_s, level_p, job_p);
							success_flag = false;
						}
				}
			else if (IsStringArrayParameter (treatment_names_p))
				{
					const char **values_ss = GetStringArrayParameterCurrentValues ((StringArrayParameter *) treatment_names_p);

					if (values_ss)
						{
							const char **value_ss = values_ss;
							size_t i = 0;

							while (success_flag && (*value_ss))
								{
									const json_t *level_p = json_array_get (levels_json_p, i);

									if (AddTreatmentFactorByPartsToStudy (*value_ss, level_p, study_p, data_p))
										{
											++ i;
											++ value_ss;
										}
									else
										{
											ReportTreatmentFactorError (study_p, *value_ss, level_p, job_p);
											success_flag = false;
										}
								}

						}
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Treatment Parameter \"%s\" is not a string or string array, its type is %lu, to study \"%s\"", treatment_names_p -> pa_type, treatment_names_p -> pa_name_s, study_p -> st_name_s);

					AddParameterErrorMessageToServiceJob (job_p, TFJ_TREATMENT_NAME.npt_name_s, TFJ_TREATMENT_NAME.npt_type, "Input value is not a string object or array");

					success_flag = false;
				}

		}		/* if (treatment_names_p && treatment_levels_p) */
	else
		{
			if (treatment_names_p)
				{
					success_flag = false;

					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Treatment names are set with NULL levels", treatment_names_p -> pa_type, treatment_names_p -> pa_name_s, study_p -> st_name_s);
					AddParameterErrorMessageToServiceJob (job_p, TFJ_TREATMENT_NAME.npt_name_s, TFJ_TREATMENT_NAME.npt_type, "Treatment names are set but no levels specified");
				}
			else if (treatment_levels_p)
				{
					success_flag = false;

					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Treatment levels are set with NULL names", treatment_levels_p -> pa_type, treatment_levels_p -> pa_name_s, study_p -> st_name_s);
					AddParameterErrorMessageToServiceJob (job_p, TFJ_TREATMENT_NAME.npt_name_s, TFJ_TREATMENT_NAME.npt_type, "Treatment levels are set but no names specified");
				}

		}

	return success_flag;
}


static void ReportTreatmentFactorError (Study *study_p, const char *name_s, const json_t *levels_p, ServiceJob *job_p)
{
	const char * const error_prefix_s = "Unknown Treatment name";
	char *error_s = ConcatenateVarargsStrings (error_prefix_s, ": ", name_s, NULL);
	PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, levels_p, "Failed to add treatment factor \"%s\" to study \"%s\"", name_s, study_p -> st_name_s);

	AddParameterErrorMessageToServiceJob (job_p, TFJ_TREATMENT_NAME.npt_name_s, TFJ_TREATMENT_NAME.npt_type, error_s ? error_s : error_prefix_s);

	if (error_s)
		{
			FreeCopiedString (error_s);
		}
}


static bool ProcessPersonForStudy (Person *person_p, void *user_data_p)
{
	bool success_flag = true;
	Study *study_p = (Study *) user_data_p;
	PersonNode *node_p = FindPersonNodeOnList (study_p -> st_contributors_p, person_p);

	if (!node_p)
		{
			success_flag = AddStudyContributor (study_p, person_p, MF_SHALLOW_COPY);
		}

	return success_flag;
}


static bool GetPersonFromParameters (Person **person_pp, ParameterSet *param_set_p, const char *name_param_s, const char *email_param_s,
																		 const char *role_param_s, const char *affilation_param_s, const char *orcid_param_s)
{
	bool success_flag = true;
	const char *name_s = NULL;
	const char *email_s = NULL;
	const char *role_s = NULL;
	const char *affilation_s = NULL;
	const char *orcid_s = NULL;

	GetCurrentStringParameterValueFromParameterSet (param_set_p, name_param_s, &name_s);
	GetCurrentStringParameterValueFromParameterSet (param_set_p, email_param_s, &email_s);
	GetCurrentStringParameterValueFromParameterSet (param_set_p, role_param_s, &role_s);
	GetCurrentStringParameterValueFromParameterSet (param_set_p, affilation_param_s, &affilation_s);
	GetCurrentStringParameterValueFromParameterSet (param_set_p, orcid_param_s, &orcid_s);

	if ((!IsStringEmpty (name_s)) || (!IsStringEmpty (email_s)))
		{
			Person *person_p = AllocatePerson (name_s, email_s, role_s, affilation_s, orcid_s);

			if (person_p)
				{
					*person_pp = person_p;
				}
			else
				{
					success_flag = false;
				}
		}

	return success_flag;
}


static bool AddMeasuredVariableParameters (ParameterSet *params_p, const Study *study_p, const bool read_only_flag, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Measured Variables", true, & (data_p -> dftsd_base_data), params_p);

	if (group_p)
		{
			Parameter *param_p = NULL;
			const char * const display_name_s = "Measured Variable name";
			const char * const description_s = "The name of the Measured Variable";
			size_t num_mvs = 0;

			if ((study_p != NULL) && (study_p -> st_phenotypes_p != NULL))
				{
					num_mvs = study_p -> st_phenotypes_p -> ll_size;
				}

			if (num_mvs > 1)
				{
					char **mvs_ss = (char **) AllocMemoryArray (num_mvs, sizeof (const char *));

					if (mvs_ss)
						{
							PhenotypeStatisticsNode *node_p = (PhenotypeStatisticsNode *) (study_p -> st_phenotypes_p -> ll_head_p);
							bool loop_success_flag = true;
							char **mv_ss = mvs_ss;

							while (loop_success_flag && node_p)
								{
									if (CopyAndAddStringValue (node_p -> psn_measured_variable_name_s, mv_ss))
										{
											node_p = (PhenotypeStatisticsNode *) (node_p -> psn_node.ln_next_p);
											++ mv_ss;
										}
									else
										{
											loop_success_flag = false;
										}
								}


							if (loop_success_flag)
								{
									param_p = EasyCreateAndAddStringArrayParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, STUDY_MEASURED_VARIABLES.npt_name_s, display_name_s, description_s, mvs_ss, num_mvs, PL_ALL_AND_WIZARD);

									if (param_p)
										{
											param_p -> pa_read_only_flag = read_only_flag;
										}
									else
										{
											loop_success_flag = false;
											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "EasyCreateAndAddStringArrayParameterToParameterSet () failed for Study \"%s\"", study_p -> st_name_s);
										}
								}

							FreeMemory (mvs_ss);
						}		/* if (mvs_ss) */
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to copy measured variables for Study \"%s\"", study_p -> st_name_s);
						}

				}		/* if (num_treatments > 1) */
			else
				{
					const char *mv_s = NULL;

					if (num_mvs == 1)
						{
							PhenotypeStatisticsNode *node_p = (PhenotypeStatisticsNode *) (study_p -> st_phenotypes_p -> ll_head_p);

							mv_s = node_p -> psn_measured_variable_name_s;
						}

					param_p = EasyCreateAndAddStringParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, STUDY_MEASURED_VARIABLES.npt_type, STUDY_MEASURED_VARIABLES.npt_name_s, display_name_s, description_s, mv_s, PL_ALL_AND_WIZARD);
				}

			if (param_p)
				{
					param_p -> pa_read_only_flag = read_only_flag;

					if (AddRepeatableParameterGroupLabelParam (group_p, param_p))
						{
							success_flag = true;
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "EasyCreateAndAddStringArrayParameterToParameterSet () failed for Study \"%s\"", study_p -> st_name_s);
				}

		}

	return success_flag;
}


static OperationStatus ProcessMeasuredVariables (ServiceJob *job_p, ParameterSet *param_set_p, Study *study_p, FieldTrialServiceData *ft_service_data_p)
{
	OperationStatus status = OS_FAILED;
	Parameter *mvs_param_p = GetParameterFromParameterSetByName (param_set_p, STUDY_MEASURED_VARIABLES.npt_name_s);
	size_t num_mvs = 0;
	size_t num_null_mvs = 0;
	const char *value_s = NULL;
	const char **mvs_ss = NULL;

	if (mvs_param_p)
		{
			if (IsStringArrayParameter (mvs_param_p))
				{
					StringArrayParameter *sap_p = (StringArrayParameter *) mvs_param_p;
					num_mvs = GetNumberOfStringArrayCurrentParameterValues (sap_p);
					mvs_ss = GetStringArrayParameterCurrentValues (sap_p);
				}
			else if (IsStringParameter (mvs_param_p))
				{
					StringParameter *sp_p = (StringParameter *) mvs_param_p;

					value_s = GetStringParameterCurrentValue (sp_p);

					if (value_s)
						{
							num_mvs = 1;
							*mvs_ss = value_s;
						}
				}
		}


	if (mvs_ss)
		{
			size_t i;
			const char **mv_ss = mvs_ss;
			size_t num_valid_mvs = 0;

			for (i = 0; i < num_mvs; ++ i, ++ mv_ss)
				{
					if (*mv_ss != NULL)
						{
							/* remove any leading or trailing whitespace */
							char *mv_s = CopyToNewString (*mv_ss, 0, true);

							if (mv_s)
								{
									/* Is this measured variable already in the study? ... */
									if (IsMeasuredVariableOnStudy (study_p, *mv_ss))
										{
											++ num_valid_mvs;
										}
									else
										{
											/* ... if not, then check that it's a valid name ... */
											MeasuredVariable *mv_p = GetMeasuredVariableByName (*mv_ss, ft_service_data_p);

											if (mv_p)
												{
													if (AddPhenotypeStatisticsToStudy (study_p, mv_s, NULL))
														{
															++ num_valid_mvs;
														}
													else
														{
															char *error_s = ConcatenateVarargsStrings ("Failed to add \"", mv_s, "\" to Study, please contact Grassroots support to investigate", NULL);

															if (error_s)
																{
																	AddParameterErrorMessageToServiceJob (job_p, STUDY_MEASURED_VARIABLES.npt_name_s, STUDY_MEASURED_VARIABLES.npt_type, error_s);
																	FreeCopiedString (error_s);
																}
															else
																{
																	AddParameterErrorMessageToServiceJob (job_p, STUDY_MEASURED_VARIABLES.npt_name_s, STUDY_MEASURED_VARIABLES.npt_type, "Failed to add one or more Measured Variables to Study, please contact Grassroots support to investigate");
																}

														}

													FreeMeasuredVariable (mv_p);
												}
											else
												{
													char *error_s = ConcatenateVarargsStrings ("\"", mv_s, "\" is not a valid Measured Variable name", NULL);

													if (error_s)
														{
															AddParameterErrorMessageToServiceJob (job_p, STUDY_MEASURED_VARIABLES.npt_name_s, STUDY_MEASURED_VARIABLES.npt_type, error_s);
															FreeCopiedString (error_s);
														}
													else
														{
															AddParameterErrorMessageToServiceJob (job_p, STUDY_MEASURED_VARIABLES.npt_name_s, STUDY_MEASURED_VARIABLES.npt_type, "Invalid Measured Variable name");
														}
												}
										}

									FreeCopiedString (mv_s);
								}
						}
					else
						{
							++ num_null_mvs;
						}

				}		/* for (i = 0; i < num_mvs; ++ i, ++ mv_ss) */

			if (num_mvs == num_valid_mvs + num_null_mvs)
				{
					status = OS_SUCCEEDED;
				}
			else if (num_valid_mvs > 0)
				{
					status = OS_PARTIALLY_SUCCEEDED;
				}


		}		/* if (mvs_ss) */
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get %s parameter", STUDY_MEASURED_VARIABLES.npt_name_s);
		}

	return status;
}


/*
  {
    "so:name": "MARTi search service",
    "start_service": true,
    "parameter_set": {
      "level": "simple",
      "parameters": [
        {
          "param": "Latitude",
          "current_value": 0.070000000000000007
        },
        {
          "param": "Longitude",
          "current_value": 52.778055999999999
        },
        {
          "param": "Start Date",
          "current_value": null
        },
        {
          "param": "Maximum Distance",
          "current_value": 10000
        }
      ]
    }
  }
]
 */


