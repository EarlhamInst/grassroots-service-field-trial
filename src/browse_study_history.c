/*
 * browse_study_history.c
 *
 *  Created on: 5 Oct 2023
 *      Author: billy
 */


/*
** Copyright 2014-2023 The Earlham Institute
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
 * browse_study_history.c
 *
 *  Created on: 24 Aug 2023
 *      Author: billy
 */


#include "browse_study_history.h"

#include "dfw_util.h"

#include "audit.h"



#include "study_jobs.h"
#include "person_jobs.h"
#include "string_parameter.h"
#include "string_array_parameter.h"
#include "time_parameter.h"

/*
 * Static declarations
 */



static const char *GetBrowseStudyHistoryServiceName (const Service *service_p);

static const char *GetBrowseStudyHistoryServiceDescription (const Service *service_p);

static const char *GetBrowseStudyHistoryServiceAlias (const Service *service_p);

static const char *GetBrowseStudyHistoryServiceInformationUri (const Service *service_p);

static ParameterSet *GetBrowseStudyHistoryServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetBrowseStudyHistoryServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseBrowseStudyHistoryServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunBrowseStudyHistoryService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ParameterSet *IsResourceForBrowseStudyHistoryService (Service *service_p, DataResource *resource_p, Handler *handler_p);

static bool CloseBrowseStudyHistoryService (Service *service_p);

static ServiceMetadata *GetBrowseStudyHistoryServiceMetadata (Service *service_p);


static Parameter *CreateSubmitStudyParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag);


static bool AddBrowseStudyHistoryParams (ServiceData *data_p, ParameterSet *param_set_p, Study *active_study_p, const char *original_id_s);


static Study *GetVersionedStudyFromResource (DataResource *resource_p, const NamedParameterType study_param_type, const char **original_id_ss, FieldTrialServiceData *ft_data_p);


static bool SetUpVersionsParameter (const FieldTrialServiceData *data_p, StringParameter *param_p, const char * const id_s,  const char * const timestamp_s, const FieldTrialDatatype dt);


static bool AddStudyVersionsList (Study *active_study_p, const char *id_s, ParameterSet *param_set_p, ParameterGroup *group_p, const bool read_only_flag, FieldTrialServiceData *dfw_data_p);



/*
 * API definitions
 */


Service *GetBrowseStudyHistoryService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
														 GetBrowseStudyHistoryServiceName,
														 GetBrowseStudyHistoryServiceDescription,
														 GetBrowseStudyHistoryServiceAlias,
														 GetBrowseStudyHistoryServiceInformationUri,
														 RunBrowseStudyHistoryService,
														 NULL,
														 GetBrowseStudyHistoryServiceParameters,
														 GetBrowseStudyHistoryServiceParameterTypesForNamedParameters,
														 ReleaseBrowseStudyHistoryServiceParameters,
														 CloseBrowseStudyHistoryService,
														 NULL,
														 false,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetBrowseStudyHistoryServiceMetadata,
														 NULL,
														 grassroots_p))
						{

							if (ConfigureFieldTrialService (data_p, grassroots_p))
								{
									service_p -> se_custom_parameter_decoder_fn = CreateSubmitStudyParameterFromJSON;

									return service_p;
								}

						}		/* if (InitialiseService (.... */

					FreeFieldTrialServiceData (data_p);
				}

			FreeMemory (service_p);
		}		/* if (service_p) */

	return NULL;
}


static const char *GetBrowseStudyHistoryServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Browse Study Revisions";
}


static const char *GetBrowseStudyHistoryServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "Browse all of the revisions of a given Study.";
}


static const char *GetBrowseStudyHistoryServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "browse_study_history";
}


static const char *GetBrowseStudyHistoryServiceInformationUri (const Service *service_p)
{
	const char *url_s = GetServiceInformationPage (service_p);

	if (!url_s)
		{
			url_s = "https://grassroots.tools/docs/user/services/field_study/browse_study_history.md";
		}

	return url_s;
}


static bool GetBrowseStudyHistoryServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = false;

	const NamedParameterType params [] =
		{
			FT_TIMESTAMP,
			NULL
		};


	if (DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params))
		{
			success_flag = true;
		}
	else
		{
			success_flag = GetSubmissionStudyParameterTypeForNamedParameter (param_name_s, pt_p);
		}

	return success_flag;
}



static ParameterSet *GetBrowseStudyHistoryServiceParameters (Service *service_p, DataResource *resource_p, User * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p = AllocateParameterSet ("Study History Browser service parameters", "The parameters used for the Browse Field Trial versions service");

	if (params_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			FieldTrialServiceData *fts_data_p = (FieldTrialServiceData *) data_p;
			const char *original_id_s = NULL;
			Study *active_study_p = GetVersionedStudyFromResource (resource_p, STUDY_ID, &original_id_s, fts_data_p);

			if (AddBrowseStudyHistoryParams (data_p, params_p, active_study_p, original_id_s))
				{
					return params_p;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddBrowseStudyHistoryParams failed");
				}

			FreeParameterSet (params_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetBrowseStudyHistoryServiceName (service_p));
		}

	return NULL;
}







static Study *GetVersionedStudyFromResource (DataResource *resource_p, const NamedParameterType study_param_type, const char **original_id_ss, FieldTrialServiceData *ft_data_p)
{
	Study *study_p = NULL;

	/*GetFieldTrialFromResourceGetFieldTrialFromResource
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
							const char *study_id_s = GetNamedParameterDefaultValueFromJSON (study_param_type.npt_name_s, params_json_p);
							const char *version_timestamp_s = GetNamedParameterDefaultValueFromJSON (FT_TIMESTAMP.npt_name_s, params_json_p);

							/*
							 * Do we have an existing study id?
							 */
							if (study_id_s)
								{
									*original_id_ss = study_id_s;

									if ((!IsStringEmpty (version_timestamp_s)) && (strcmp (version_timestamp_s, FT_DEFAULT_TIMESTAMP_S) != 0))
										{
										//	study_p = GetVersionedStudy (study_id_s, version_timestamp_s, VF_CLIENT_MINIMAL, ft_data_p);
										}

									/*
									 * The request may be requesting a new field study but sending the timestamp from the
									 * previous one so if we failed to get the versioned field study get the current one
									 */
									if (!study_p)
										{
											study_p = GetStudyByIdString (study_id_s, VF_CLIENT_MINIMAL, ft_data_p);
										}

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



static bool AddStudyVersionsList (Study *active_study_p, const char *id_s, ParameterSet *param_set_p, ParameterGroup *group_p, const bool read_only_flag, FieldTrialServiceData *dfw_data_p)
{
	bool success_flag = false;
	ServiceData *data_p = (ServiceData *) dfw_data_p;
	Parameter *param_p = NULL;
	const char *timestamp_s = NULL;

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FT_TIMESTAMP.npt_type, FT_TIMESTAMP.npt_name_s, "Version", "View Field Trial revisions", timestamp_s, PL_ALL)) != NULL)
		{
			param_p -> pa_read_only_flag = read_only_flag;

			if ((active_study_p != NULL) && (active_study_p -> st_metadata_p != NULL))
				{
					timestamp_s = active_study_p -> st_metadata_p -> me_timestamp_s;
				}


			if (SetUpVersionsParameter (dfw_data_p, (StringParameter *) param_p, id_s, timestamp_s, DFTD_STUDY))
				{
					/*
					 * We want to update all of the values in the form
					 * when a user selects a study from the list so
					 * we need to make the parameter automatically
					 * refresh the values. So we set the
					 * pa_refresh_service_flag to true.
					 */
					param_p -> pa_refresh_service_flag = true;

					success_flag = true;
				}
		}

	return success_flag;
}




static bool SetUpVersionsParameter (const FieldTrialServiceData *data_p, StringParameter *param_p, const char * const id_s,  const char * const timestamp_s, const FieldTrialDatatype dt)
{
	bool success_flag = false;

	json_t *results_p = GetAllJSONVersionsOfObject (id_s, dt, data_p);


	if (results_p)
		{
			if (json_is_array (results_p))
				{
					const size_t num_results = json_array_size (results_p);

					success_flag = true;

					if (num_results > 0)
						{
							if (success_flag)
								{
									bool value_set_flag = false;
									size_t i = 0;
									const char *param_value_s = GetStringParameterDefaultValue (param_p);

									while ((i < num_results) && success_flag)
										{
											json_t *entry_p = json_array_get (results_p, i);
											Study *study_p = GetStudyFromJSON (entry_p, VF_CLIENT_MINIMAL, data_p);

											if (study_p)
												{
													const char *value_s = FT_DEFAULT_TIMESTAMP_S;

													if ((study_p -> st_metadata_p) && (study_p -> st_metadata_p -> me_timestamp_s))
														{
															value_s = study_p -> st_metadata_p -> me_timestamp_s;

															if (param_value_s && (strcmp (param_value_s, value_s) == 0))
																{
																	value_set_flag = true;
																}
														}

													if (!CreateAndAddStringParameterOption (& (param_p -> sp_base_param), value_s, value_s))
														{
															success_flag = false;
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param option for \"%s\"", value_s);
														}

													FreeStudy (study_p);
												}		/* if (study_p) */
											else
												{
													success_flag = false;
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get Study");
												}

											if (success_flag)
												{
													++ i;
												}

										}		/* while ((i < num_results) && success_flag) */

									/*
									 * If the parameter's value isn't on the list, reset it
									 */
									if ((param_value_s != NULL) && (value_set_flag == false))
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "param value \"%s\" not on list of existing studys", param_value_s);
										}

								}		/* if (success_flag) */


						}		/* if (num_results > 0) */
					else
						{
							/* nothing to add */
							success_flag = true;
						}

				}		/* if (json_is_array (results_p)) */

			json_decref (results_p);
		}		/* if (results_p) */


	if (success_flag)
		{
			success_flag = SetStringParameterDefaultValue (param_p, timestamp_s);
			success_flag = SetStringParameterCurrentValue (param_p, timestamp_s);
		}

	return success_flag;
}



static bool AddBrowseStudyHistoryParams (ServiceData *data_p, ParameterSet *param_set_p, Study *active_study_p, const char *original_id_s)
{
	FieldTrialServiceData *ft_data_p = (FieldTrialServiceData *) data_p;
	bool success_flag = false;
	LinkedList *existing_people_p = NULL;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Main", false, & (ft_data_p -> dftsd_base_data), param_set_p);
	const bool read_only_flag = true;
	json_t *studies_p = GetAllStudiesAsJSON (ft_data_p, false);

	if (studies_p)
		{
			/*
			 * If we don't have an active study, use the first one in the json results array
			 */
			if (!active_study_p)
				{
					if (json_is_array (studies_p))
						{
							if (json_array_size (studies_p) > 0)
								{
									json_t *study_json_p = json_array_get (studies_p, 0);

									active_study_p = GetStudyFromJSON (study_json_p, VF_CLIENT_MINIMAL, ft_data_p);

									if (!active_study_p)
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "GetFieldTrialFromJSON () failed");
										}
								}
						}
				}

			if (active_study_p)
				{
					char *id_s = NULL;
					const char *id_to_use_s = original_id_s;

					if (!id_to_use_s)
						{
							id_s = GetBSONOidAsString (active_study_p -> st_id_p);
							id_to_use_s = id_s;
						}

					if (id_to_use_s)
						{
							/*
							if (AddStudiesListFromJSON (id_to_use_s, studies_p, param_set_p, group_p, false, false, ft_data_p))
								{
									char *programme_id_s = NULL;
									const char *name_s = NULL;
									const char *team_s = NULL;

									if (PopulaterActiveStudiesValues (active_study_p, &id_s, &programme_id_s, &name_s, &team_s, &existing_people_p, param_set_p, ft_data_p))
										{
											if (AddStudyVersionsList (active_study_p, id_to_use_s, param_set_p, group_p, false, ft_data_p))
												{

												}
										}

									if (AddStudyEditor (name_s, team_s, programme_id_s, existing_people_p, param_set_p, group_p, read_only_flag, ft_data_p))
										{
											success_flag = true;
										}
								}

							if (id_s)
								{
									FreeBSONOidString (id_s);
								}
							*/

						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get id from \"%s\"", active_study_p -> st_name_s);
						}



				}

			json_decref (studies_p);
		}


	return success_flag;
}





static void ReleaseBrowseStudyHistoryServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool CloseBrowseStudyHistoryService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));

	return success_flag;
}



static ServiceJobSet *RunBrowseStudyHistoryService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Browse Field Trial history");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

	//		if (!RunForBrowseStudyHistoryParams (data_p, param_set_p, job_p))
	//			{

	//			}		/* if (!RunForBrowseStudyHistoryParams (data_p, param_set_p, job_p)) */


			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static ServiceMetadata *GetBrowseStudyHistoryServiceMetadata (Service *service_p)
{
	const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "topic_0625";
	SchemaTerm *category_p = AllocateSchemaTerm (term_url_s, "Genotype and phenotype",
		"The study of genetic constitution of a living entity, such as an individual, and organism, a cell and so on, "
		"typically with respect to a particular observable phenotypic traits, or resources concerning such traits, which "
		"might be an aspect of biochemistry, physiology, morphology, anatomy, development and so on.");

	if (category_p)
		{
			SchemaTerm *subcategory_p;

			term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "operation_0304";
			subcategory_p = AllocateSchemaTerm (term_url_s, "Query and retrieval", "Search or query a data resource and retrieve entries and / or annotation.");

			if (subcategory_p)
				{
					ServiceMetadata *metadata_p = AllocateServiceMetadata (category_p, subcategory_p);

					if (metadata_p)
						{
							SchemaTerm *input_p;

							term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "data_0968";
							input_p = AllocateSchemaTerm (term_url_s, "Keyword",
								"Boolean operators (AND, OR and NOT) and wildcard characters may be allowed. Keyword(s) or phrase(s) used (typically) for text-searching purposes.");

							if (input_p)
								{
									if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
										{
											SchemaTerm *output_p;

											/* Place */
											term_url_s = CONTEXT_PREFIX_SCHEMA_ORG_S "Place";
											output_p = AllocateSchemaTerm (term_url_s, "Place", "Entities that have a somewhat fixed, physical extension.");

											if (output_p)
												{
													if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
														{
															/* Date */
															term_url_s = CONTEXT_PREFIX_SCHEMA_ORG_S "Date";
															output_p = AllocateSchemaTerm (term_url_s, "Date", "A date value in ISO 8601 date format.");

															if (output_p)
																{
																	if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																		{
																			/* Pathogen */
																			term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000643";
																			output_p = AllocateSchemaTerm (term_url_s, "pathogen", "A biological agent that causes disease or illness to its host.");

																			if (output_p)
																				{
																					if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																						{
																							/* Phenotype */
																							term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000651";
																							output_p = AllocateSchemaTerm (term_url_s, "phenotype", "The observable form taken by some character (or group of characters) "
																								"in an individual or an organism, excluding pathology and disease. The detectable outward manifestations of a specific genotype.");

																							if (output_p)
																								{
																									if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																										{
																											/* Genotype */
																											term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000513";
																											output_p = AllocateSchemaTerm (term_url_s, "genotype", "Information, making the distinction between the actual physical material "
																												"(e.g. a cell) and the information about the genetic content (genotype).");

																											if (output_p)
																												{
																													if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																														{
																															return metadata_p;
																														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																													else
																														{
																															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																															FreeSchemaTerm (output_p);
																														}

																												}		/* if (output_p) */
																											else
																												{
																													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																												}
																										}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																									else
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																											FreeSchemaTerm (output_p);
																										}

																								}		/* if (output_p) */
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																								}

																						}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																							FreeSchemaTerm (output_p);
																						}

																				}		/* if (output_p) */
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																				}

																		}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																			FreeSchemaTerm (output_p);
																		}

																}		/* if (output_p) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																}


														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
															FreeSchemaTerm (output_p);
														}

												}		/* if (output_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
												}

										}		/* if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add input term %s to service metadata", term_url_s);
											FreeSchemaTerm (input_p);
										}

								}		/* if (input_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate input term %s for service metadata", term_url_s);
								}

							FreeServiceMetadata (metadata_p);
						}		/* if (metadata_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate service metadata");
						}

				}		/* if (subcategory_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate sub-category term %s for service metadata", term_url_s);
				}

		}		/* if (category_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate category term %s for service metadata", term_url_s);
		}

	return NULL;
}




static ParameterSet *IsResourceForBrowseStudyHistoryService (Service * UNUSED_PARAM (service_p), DataResource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}


static Parameter *CreateSubmitStudyParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag)
{
	Parameter *param_p = NULL;
	const char *name_s = GetJSONString (param_json_p, PARAM_NAME_S);
	bool done_flag = false;
	ParameterType pt = PT_NUM_TYPES;

	if (GetPersonParameterTypeForNamedParameter (name_s, &pt))
		{
			if (pt != PT_NUM_TYPES)
				{
					json_t *current_value_p = json_object_get (param_json_p, PARAM_CURRENT_VALUE_S);

					if (current_value_p)
						{
							if (json_is_array (current_value_p))
								{
									switch (pt)
										{
											case PT_STRING:
												{
													StringArrayParameter *string_array_param_p = AllocateStringArrayParameterFromJSON (param_json_p, service_p, concise_flag, NULL);

													if (string_array_param_p)
														{
															param_p = & (string_array_param_p -> sap_base_param);
														}
												}
											break;

											default:
												PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, param_json_p, "Unknown ParameterType %u", pt);
												break;
										}		/* switch (pt) */

								}		/* if (json_is_array (current_value_p)) */
							else
								{
									switch (pt)
										{
											case PT_STRING:
												{
													StringParameter *string_param_p  = AllocateStringParameterFromJSON (param_json_p, service_p, concise_flag, &pt);

													if (string_param_p)
														{
															param_p = & (string_param_p -> sp_base_param);
														}
												}
											break;

											default:
												PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, param_json_p, "Unknown ParameterType %u", pt);
												break;
										}		/* switch (pt) */

								}		/* if (json_is_array (current_value_p)) */

						}		/* if (current_value_p) */

				}		/* if (pt != PT_NUM_TYPES) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, param_json_p, "Unknown ParameterType %u", pt);
				}

		}		/* if (GetPersonParameterTypeForNamedParameter (name_s, &pt)) */
	else
		{
		}

	if (!param_p)
		{
			param_p = CreateParameterFromJSON (param_json_p, service_p, concise_flag);
		}

	return param_p;
}
