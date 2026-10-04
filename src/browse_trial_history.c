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
 * browse_trial_history.c
 *
 *  Created on: 24 Aug 2023
 *      Author: billy
 */


#include "browse_trial_history.h"

#include "dfw_util.h"

#include "audit.h"



#include "field_trial_jobs.h"
#include "person_jobs.h"
#include "string_parameter.h"
#include "string_array_parameter.h"
#include "time_parameter.h"

/*
 * Static declarations
 */



static const char *GetBrowseTrialHistoryServiceName (const Service *service_p);

static const char *GetBrowseTrialHistoryServiceDescription (const Service *service_p);

static const char *GetBrowseTrialHistoryServiceAlias (const Service *service_p);

static const char *GetBrowseTrialHistoryServiceInformationUri (const Service *service_p);

static ParameterSet *GetBrowseTrialHistoryServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetBrowseTrialHistoryServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseBrowseTrialHistoryServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunBrowseTrialHistoryService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ParameterSet *IsResourceForBrowseTrialHistoryService (Service *service_p, DataResource *resource_p, Handler *handler_p);

static bool CloseBrowseTrialHistoryService (Service *service_p);

static ServiceMetadata *GetBrowseTrialHistoryServiceMetadata (Service *service_p);


static Parameter *CreateSubmitTrialParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag);


static bool AddBrowseTrialHistoryParams (ServiceData *data_p, ParameterSet *param_set_p, FieldTrial *active_trial_p, const char *original_id_s);


static FieldTrial *GetVersionedFieldTrialFromResource (DataResource *resource_p, const NamedParameterType trial_param_type, const char **original_id_ss, FieldTrialServiceData *ft_data_p);


static bool SetUpVersionsParameter (const FieldTrialServiceData *data_p, StringParameter *param_p, const char * const id_s,  const char * const timestamp_s, const FieldTrialDatatype dt);

static bool AddTrialVersionsList (FieldTrial *active_trial_p, const char *id_s, ParameterSet *param_set_p, ParameterGroup *group_p, const bool read_only_flag, FieldTrialServiceData *dfw_data_p);


/*
 * API definitions
 */


Service *GetBrowseTrialHistoryService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
														 GetBrowseTrialHistoryServiceName,
														 GetBrowseTrialHistoryServiceDescription,
														 GetBrowseTrialHistoryServiceAlias,
														 GetBrowseTrialHistoryServiceInformationUri,
														 RunBrowseTrialHistoryService,
														 NULL,
														 GetBrowseTrialHistoryServiceParameters,
														 GetBrowseTrialHistoryServiceParameterTypesForNamedParameters,
														 ReleaseBrowseTrialHistoryServiceParameters,
														 CloseBrowseTrialHistoryService,
														 NULL,
														 false,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetBrowseTrialHistoryServiceMetadata,
														 GetFieldTrialIndexingData,
														 grassroots_p))
						{

							if (ConfigureFieldTrialService (data_p, grassroots_p))
								{
									service_p -> se_custom_parameter_decoder_fn = CreateSubmitTrialParameterFromJSON;

									return service_p;
								}

						}		/* if (InitialiseService (.... */

					FreeFieldTrialServiceData (data_p);
				}

			FreeMemory (service_p);
		}		/* if (service_p) */

	return NULL;
}


static const char *GetBrowseTrialHistoryServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Browse Field Trial Revisions";
}


static const char *GetBrowseTrialHistoryServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "Browse all of the revisions of a given Field Trial. Following the same nomenclature as <a href='https://brapi.docs.apiary.io/'>BrAPI</a>,"
			" a Field Trial contains multiple Studies. This is equivalent to an Investigation in <a href='https://www.miappe.org/'>MIAPPE</a>.";
}


static const char *GetBrowseTrialHistoryServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "browse_trial_history";
}


static const char *GetBrowseTrialHistoryServiceInformationUri (const Service *service_p)
{
	const char *url_s = GetServiceInformationPage (service_p);

	if (!url_s)
		{
			url_s = "https://grassroots.tools/docs/user/services/field_trial/browse_trial_history.md";
		}

	return url_s;
}


static bool GetBrowseTrialHistoryServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
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
			success_flag = GetSubmissionFieldTrialParameterTypeForNamedParameter (param_name_s, pt_p);
		}

	return success_flag;
}



static ParameterSet *GetBrowseTrialHistoryServiceParameters (Service *service_p, DataResource *resource_p, User * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p = AllocateParameterSet ("FieldTrial History Browser service parameters", "The parameters used for the Browse Field Trial versions service");

	if (params_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			FieldTrialServiceData *fts_data_p = (FieldTrialServiceData *) data_p;
			const char *original_id_s = NULL;
			FieldTrial *active_trial_p = GetVersionedFieldTrialFromResource (resource_p, FIELD_TRIAL_ID, &original_id_s, fts_data_p);

			if (AddBrowseTrialHistoryParams (data_p, params_p, active_trial_p, original_id_s))
				{
					return params_p;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddBrowseTrialHistoryParams failed");
				}

			FreeParameterSet (params_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetBrowseTrialHistoryServiceName (service_p));
		}

	return NULL;
}



static FieldTrial *GetVersionedFieldTrialFromResource (DataResource *resource_p, const NamedParameterType trial_param_type, const char **original_id_ss, FieldTrialServiceData *ft_data_p)
{
	void *(*get_versioned_obj_fn) (const char *id_s, const char *timestamp_s, const ViewFormat vf, FieldTrialServiceData *ft_data_p) =
			(void * (*) (const char *, const char *, const ViewFormat, FieldTrialServiceData *)) GetVersionedFieldTrial;

	void *(*get_obj_by_id_fn) (const char *id_s, const ViewFormat vf, FieldTrialServiceData *ft_data_p) =
			(void *(*) (const char *id_s, const ViewFormat vf, FieldTrialServiceData *ft_data_p)) GetFieldTrialByIdString;


	FieldTrial *trial_p = (FieldTrial *) GetVersionedObjectFromResource (resource_p, trial_param_type, original_id_ss, ft_data_p,
																																			 get_versioned_obj_fn, get_obj_by_id_fn);

	return trial_p;
}



static bool AddTrialVersionsList (FieldTrial *active_trial_p, const char *id_s, ParameterSet *param_set_p, ParameterGroup *group_p, const bool read_only_flag, FieldTrialServiceData *dfw_data_p)
{
	bool success_flag = false;
	ServiceData *data_p = (ServiceData *) dfw_data_p;
	Parameter *param_p = NULL;
	const char *timestamp_s = NULL;

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FT_TIMESTAMP.npt_type, FT_TIMESTAMP.npt_name_s, "Version", "View Field Trial revisions", timestamp_s, PL_ALL)) != NULL)
		{
			param_p -> pa_read_only_flag = read_only_flag;


			if (SetUpVersionsParameter (dfw_data_p, (StringParameter *) param_p, id_s, active_trial_p  ? active_trial_p -> ft_timestamp_s : NULL, DFTD_FIELD_TRIAL))
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
											FieldTrial *trial_p = GetFieldTrialFromJSON (entry_p, VF_CLIENT_MINIMAL, data_p);

											if (trial_p)
												{
													const char *value_s = FT_DEFAULT_TIMESTAMP_S;

													if (trial_p -> ft_timestamp_s)
														{
															value_s = trial_p -> ft_timestamp_s;

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

													FreeFieldTrial (trial_p);
												}		/* if (trial_p) */
											else
												{
													success_flag = false;
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get FieldTrial");
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
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "param value \"%s\" not on list of existing trials", param_value_s);
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



static bool AddBrowseTrialHistoryParams (ServiceData *data_p, ParameterSet *param_set_p, FieldTrial *active_trial_p, const char *original_id_s)
{
	FieldTrialServiceData *ft_data_p = (FieldTrialServiceData *) data_p;
	bool success_flag = false;
	LinkedList *existing_people_p = NULL;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Main", false, & (ft_data_p -> dftsd_base_data), param_set_p);
	const bool read_only_flag = true;
	json_t *trials_p = GetAllFieldTrialsAsJSON (ft_data_p, false);

	if (trials_p)
		{
			/*
			 * If we don't have an active trial, use the first one in the json results array
			 */
			if (!active_trial_p)
				{
					if (json_is_array (trials_p))
						{
							if (json_array_size (trials_p) > 0)
								{
									json_t *trial_json_p = json_array_get (trials_p, 0);

									active_trial_p = GetFieldTrialFromJSON (trial_json_p, VF_CLIENT_MINIMAL, ft_data_p);

									if (!active_trial_p)
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_json_p, "GetFieldTrialFromJSON () failed");
										}
								}
						}
				}

			if (active_trial_p)
				{
					char *id_s = NULL;
					const char *id_to_use_s = original_id_s;

					if (!id_to_use_s)
						{
							id_s = GetBSONOidAsString (active_trial_p -> ft_id_p);
							id_to_use_s = id_s;
						}

					if (id_to_use_s)
						{
							if (AddTrialsListFromJSON (id_to_use_s, trials_p, param_set_p, group_p, false, NULL, ft_data_p))
								{
									char *programme_id_s = NULL;
									const char *name_s = NULL;
									const char *team_s = NULL;

									if (PopulaterActiveTrialValues (active_trial_p, &id_s, &programme_id_s, &name_s, &team_s, &existing_people_p, param_set_p, ft_data_p))
										{
											if (AddTrialVersionsList (active_trial_p, id_to_use_s, param_set_p, group_p, false, ft_data_p))
												{

												}
										}

									if (AddTrialEditor (name_s, team_s, programme_id_s, existing_people_p, param_set_p, group_p, read_only_flag, ft_data_p))
										{
											success_flag = true;
										}
								}

							if (id_s)
								{
									FreeBSONOidString (id_s);
								}

						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get id from \"%s\"", active_trial_p -> ft_name_s);
						}



				}

			json_decref (trials_p);
		}


	return success_flag;
}





static void ReleaseBrowseTrialHistoryServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool CloseBrowseTrialHistoryService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));

	return success_flag;
}



static ServiceJobSet *RunBrowseTrialHistoryService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Browse Field Trial history");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

	//		if (!RunForBrowseTrialHistoryParams (data_p, param_set_p, job_p))
	//			{

	//			}		/* if (!RunForBrowseTrialHistoryParams (data_p, param_set_p, job_p)) */


			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}



bool AddBrowseFieldTrialParams (ServiceData *data_p, ParameterSet *param_set_p, DataResource *resource_p)
{
	bool success_flag = false;

	return success_flag;
}



static ServiceMetadata *GetBrowseTrialHistoryServiceMetadata (Service *service_p)
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




static ParameterSet *IsResourceForBrowseTrialHistoryService (Service * UNUSED_PARAM (service_p), DataResource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}


static Parameter *CreateSubmitTrialParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag)
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
