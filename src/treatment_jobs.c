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
 * treatment_jobs.c
 *
 *  Created on: 6 Mar 2020
 *      Author: billy
 */


#include "treatment_jobs.h"
#include "treatment.h"
#include "dfw_util.h"

#include "json_parameter.h"
#include "string_parameter.h"
#include "parameter_group.h"

/**
 * The NamedParameterType for the target chromosome parameter.
 */
static NamedParameterType TR_NAME = { "TR Name", PT_STRING };
static NamedParameterType TR_DESCRIPTION = { "TR Description", PT_LARGE_STRING };
static NamedParameterType TR_ONOTOLOGY_URL = { "TR Web Address", PT_STRING };
static NamedParameterType TR_PARENT_NAME = { "TR Parent", PT_STRING };
static NamedParameterType TR_SYNONYMS = { "TR Parent", PT_LARGE_STRING };



/*
 fertilizer exposure


    Spacer chemical fertilizer exposure

        Spacer nitrogen fertilizer exposure

            ammonium sulfate exposure

            Spacer nitrate exposure

                ammonium nitrate exposure

                calcium nitrate exposure

                limited nitrate exposure

                potassium nitrate exposure

                sodium nitrate exposure

            NPK fertilizer exposure

            urea exposure

        Spacer phosphate fertilizer exposure

            NPK fertilizer exposure

            superphosphate exposure

        Spacer sulfate fertilizer exposure

            ammonium sulfate exposure

            ferrous sulfate exposure

            potassium sulfate exposure
 */


/*
 * STATIC DECLARATIONS
 */

static void *GetTreatmentCallback (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool AddTreatmentToServiceJobResult (ServiceJob *job_p, Treatment *treatment_p, json_t *treatment_json_p, const ViewFormat format, FieldTrialServiceData *data_p);


/*
 * API DEFINITIONS
 */

json_t *GetAllTreatmentsAsJSON (const FieldTrialServiceData *data_p, bson_t *opts_p)
{
	json_t *results_p = NULL;

	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_TREATMENT]))
				{
					bson_t *query_p = NULL;

					results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, opts_p);
				}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_TREATMENT])) */


			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}



	return results_p;
}



json_t *GetTreatmentIndexingData (Service *service_p)
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);
	json_t *src_treatments_p = GetAllTreatmentsAsJSON (data_p, NULL);

	if (src_treatments_p)
		{
			if (json_is_array (src_treatments_p))
				{
					size_t i;
					json_t *src_treatment_p;
					size_t num_added = 0;

					json_array_foreach (src_treatments_p, i, src_treatment_p)
						{
							if (AddDatatype (src_treatment_p, DFTD_TREATMENT))
								{

								}


						}		/* json_array_foreach (src_treatments_p, i, src_study_p) */

				}		/* if (json_is_array (src_treatments_p)) */

			return src_treatments_p;
		}		/* if (src_treatments_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No programs for \"%s\"", GetServiceName (service_p));
		}

	return NULL;
}


bool GetSubmissionTreatmentParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;


	if (strcmp (param_name_s, TR_NAME.npt_name_s) == 0)
		{
			*pt_p = TR_NAME.npt_type;
		}
	else if (strcmp (param_name_s, TR_DESCRIPTION.npt_name_s) == 0)
		{
			*pt_p = TR_DESCRIPTION.npt_type;
		}
	else if (strcmp (param_name_s, TR_ONOTOLOGY_URL.npt_name_s) == 0)
		{
			*pt_p = TR_ONOTOLOGY_URL.npt_type;
		}
	else if (strcmp (param_name_s, TR_PARENT_NAME.npt_name_s) == 0)
		{
			*pt_p = TR_PARENT_NAME.npt_type;
		}
	else if (strcmp (param_name_s, TR_SYNONYMS.npt_name_s) == 0)
		{
			*pt_p = TR_SYNONYMS.npt_type;
		}
	else
		{
			success_flag = false;
		}


	return success_flag;
}


Treatment *GetTreatmentByIdString (const char *treatment_id_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Treatment *treatment_p = GetDFWObjectByIdString (treatment_id_s, DFTD_TREATMENT, GetTreatmentCallback, format, data_p);

	return treatment_p;
}


Treatment *GetTreatmentById (const bson_oid_t *id_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Treatment *treatment_p = GetDFWObjectById (id_p, DFTD_TREATMENT, GetTreatmentCallback, format, data_p);

	return treatment_p;
}


Treatment *GetTreatmentByURL (const char *term_url_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Treatment *treatment_p = NULL;
	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_TREATMENT]))
				{
					bson_t *query_p = bson_new ();

					if (query_p)
						{
							if (BSON_APPEND_UTF8 (query_p, SCHEMA_TERM_URL_S, term_url_s))
								{
									json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, NULL);

									if (results_p)
										{
											if (json_is_array (results_p))
												{
													size_t num_results = json_array_size (results_p);

													if (num_results == 1)
														{
															json_t *res_p = json_array_get (results_p, 0);

															treatment_p = GetTreatmentFromJSON (res_p);

															if (!treatment_p)
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, res_p, "failed to get Treatment for url \"%s\"", term_url_s);
																}

														}		/* if (num_results == 1) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "" SIZET_FMT " results when searching for Treatment with term_url_s \"%s\"", num_results, term_url_s);
														}

												}		/* if (json_is_array (results_p) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "Results are not an array");
												}

											json_decref (results_p);
										}		/* if (results_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get results searching for Treatment with id \"%s\"", term_url_s);
										}

								}		/* if (BSON_APPEND_UTF8 (query_p, TR_TERM_S, term_url_s)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for Treatment with id \"%s\"", term_url_s);
								}

							bson_destroy (query_p);
						}		/* if (query_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for term \"%s\"", term_url_s);
						}

				}		/* if (SetMongoToolCollection (tool_p, data_p -> dftsd_collection_ss [collection_type])) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set collection to \"%s\"", data_p -> dftsd_collection_ss [DFTD_TREATMENT]);
				}


			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}





	return treatment_p;
}



bool AddSubmissionTreatmentParams (ServiceData *data_p, ParameterSet *param_set_p, DataResource *resource_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Treatments", false, data_p, param_set_p);

	if (group_p)
		{
			Parameter *param_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, TR_NAME.npt_type, TR_NAME.npt_name_s, "Treatment Factor", "Create a Treatment Factor for this Study", NULL, PL_ALL)) != NULL)
				{
					success_flag = true;
				}

		}		/* if (group_p) */

	return success_flag;
}



bool RunForSubmissionTreatmentParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const char *name_s = NULL;

	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, TR_NAME.npt_name_s, &name_s))
		{
			const char *description_s = NULL;

			if (GetCurrentStringParameterValueFromParameterSet (param_set_p, TR_DESCRIPTION.npt_name_s, &description_s))
				{

				}		/* if (GetCurrentParameterValueFromParameterSet (param_set_p, TR_DESCRIPTION.npt_name_s, &description_s)) */
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get param \"%s\"", TR_DESCRIPTION.npt_name_s);
				}

		}		/* if (GetCurrentParameterValueFromParameterSet (param_set_p, TR_NAME.npt_name_s, &name_s)) */
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get param \"%s\"", TR_NAME.npt_name_s);
		}

	return job_done_flag;
}



bool AddTreatmentToServiceJob (ServiceJob *job_p, Treatment *treatment_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	json_t *treatment_json_p = GetTreatmentAsJSON (treatment_p);

	if (treatment_json_p)
		{
			if (AddTreatmentToServiceJobResult (job_p, treatment_p, treatment_json_p, format, data_p))
				{
					success_flag = true;
				}

			json_decref (treatment_json_p);
		}		/* if (treatment_json_p) */

	return success_flag;
}



/*
 * STATIC DEFINITIONS
 */


static void *GetTreatmentCallback (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	return GetTreatmentFromJSON (json_p);
}




static bool AddTreatmentToServiceJobResult (ServiceJob *job_p, Treatment *treatment_p, json_t *treatment_json_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	json_t *dest_record_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, treatment_p -> tr_ontology_term_p -> st_name_s, treatment_json_p);

	if (dest_record_p)
		{
			AddImage (dest_record_p, DFTD_TREATMENT, data_p);

			if (AddResultToServiceJob (job_p, dest_record_p))
				{
					success_flag = true;
				}
			else
				{
					json_decref (dest_record_p);
				}

		}		/* if (dest_record_p) */

	return success_flag;
}
